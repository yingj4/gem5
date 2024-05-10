#include "learning_gem5/part2/simple_cache.hh"

#include "base/compiler.hh"
#include "base/random.hh"
#include "debug/SimpleCache.hh"
#include "sim/system.hh"

namespace gem5
{

SimpleCache::SimpleCache(const SimpleCacheParams& params) : ClockedObject(params), latency(params.latency), blockSize(params.system->cacheLineSize()), capacity(params.size / blockSize), memPort(params.name + ".mem_port", this), blocked(false),
                                                            originalPacket(nullptr), waitingPortId(-1), stat(this)
{
    for (int i = 0; i < params.port_cpu_side_connection_count; ++i) {
        cpuPorts.emplace_back(name() + csprintf(".cpu_side[%d]", i), i, this);
    }
}

Port& SimpleCache::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "mem_side") {
        panic_if(idx != InvalidPortID, "Memory side of simple cache is not a vector port\n");
        return mem_port;
    } else if (if_name == "cpu_side" and idx < cpuPorts.size()) {
        return cpuPorts[idx];
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void SimpleCache::CPUSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr, "Should never try to send when blocked\n");

    DPRINTF(SimpleCache, "Sending %s to CPU\n", pkt->print());

    if (!sendTimingResp(pkt)) {
        DPRINTF(SimpleCache, "Failed!\n");
        blockedPacket = pkt;
    }
}

AddrRangeList SimpleCache::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

void SimpleCache::CPUSidePort::trySendRetry()
{
    if (needRetry && blockedPacket == nullptr) {
        needRetry = false;
        DPRINTF(SimpleCache, "Sending retry for request %d\n", id);
        sendRetryReq();
    }
}

void SimpleCache::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    return owner->handleFunctional(pkt);
}

bool SimpleCache::CPUSidePort::recvTimingReq(PacketPtr, pkt)
{
    DPRINTF(SimpleCache, "Got request %s\n", pkt->print());

    if (blockedPacket || needRetry) {
        DPRINTF(SimpleCache, "Request blocked\n");
        needRetry = true;
        return false;
    }

    if (!owner->handleRequest(pkt, id)) {
        DPRINTF(SimpleCache, "Request failed\n");
        needRetry = true;
        return false;
    } else {
        DPRINTF(SimpleCache, "Request succeeded\n");
        return true;
    }
}

void SimpleCache::CPUSidePort::recvRespRetry()
{
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    DPRINTF(SimpleCache, "Retrying response pkt %s\n", pkt->print());
    sendPacket(pkt);

    trySendRetry();
}

void SimpleCache::MemSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr, "Should never try to send when blocked\n");

    DPRINTF(SimpleCache, "Sending %s to the memory\n", pkt->print());

    if (!sendTimingReq(pkt)) {
        DPRINTF(SimpleCache, "Failed!\n");
        blockedPacket = pkt;
    }
}

bool SimpleCache::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleResponse(pkt);
}

void SimpleCache::MemSidePort::recvReqRetry()
{
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    DPRINTF(SimpleCache, "Retrying request pkt %s\n", pkt->print());
    sendPacket(pkt);
}

void SimpleCache::MemSidePort::recvRangeChange()
{
    return owner->sendRangeChange();
}

bool SimpleCache::handleRequest(PacketPtr pkt, int port_id)
{
    if (blocked) {
        return false;
    }
    
    DPRINTF(SimpleCache, "Got request from addr %x\n", pkt->getAddr());
    blocked = true;

    assert(waitingPortId == -1);
    waitingPortId = port_id;

    schedule(new EventFunctionWrapper([this, pkt]{ accessTiming(pkt); }, name + ".accessEvent", true)), clockEdge(latency);

    return true;
}

bool SimpleCache::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    
    DPRINTF(SimpleCache, "Got response for addr %x\n", pkt->getAddr());

    insert(pkt);

    stats.missLatency.sample(curTick() - missTime);

    if (originalPacket != nullptr) {
        DPRINTF(SimpleCache, "Copying data from a new packet to an old one\n");

        [[maybe_unused]] bool hit = accessFunctional(originalPacket);
        panic_if(!hit, "Should always hit after inserting!\n");
        originalPacket->makeResponse();
        delete pkt;
        pkt = originalPacket;
        originalPacket = nullptr;
    }

    sendResponse(pkt);

    return true;
}

void SimpleCache::sendResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(SimpleCache, "Sending response to addr %x\n", pkt->getAddr());

    int port = waitingPortId;

    blocked = false;
    waitingPortId = -1;

    cpuPorts[port].sendPacket(pkt);

    for (auto port : cpuPorts) {
        port.trySendRetry();
    }
}

void SimpleCache::handleFunctional(PacketPtr pkt)
{
    if (accessFunctional(pkt)) {
        pkt->makeResponse();
    } else {
        memPort.sendFunctional(pkt);
    }
}

void SimpleCache::accessTiming(PacketPtr pkt)
{
    bool hit = accessFunctional(pkt);

    DPRINTF(SimpleCache, "%s for packet: %s\n", hit ? "Hit" : "Miss", pkt->print());

    if (hit) {
        ++stats.hits;
        DDUMP(SimpleCache, pkt->getConstPtr<uint8_t>(), pkt->getSize());
        pkt->makeResponse();
        sendResponse(pkt);
    } else {
        ++stats.misses;
        missTime = curTick();

        Addr addr = pkt->getAddr();
        Addr block_addr = pkt->getBlockAddr(blockSize);
        unsigned size = pkt->getSize();

        if (addr == block_addr && size == blockSize) {
            DPRINTF(SimpleCache, "Forwarding packet\n");
            memPort.sendPacket(pkt);
        } else {
            DPRINTF(SimpleCache, "Updating packet to block size\n");
            panic_if(addr - block_addr + size > blockSize, "Cannot handle accesses that span multiple cache blocks\n");

            assert(pkt->needsResponse());
            MemCmd cmd;
            if (pkt->isWrite() || pkt->isRead()) {
                cmd = MemCmd::ReadReq;
            } else {
                panic("Unknown packet type in upgrade size\n");
            }

            PacketPtr new_pkt = new Packet(pkt->req, cmd, blockSize);
            new_pkt->allocate();

            assert(new_pkt->getAddr() == new_pkt->getBlockAddr(blockSize));

            originalPacket = pkt;

            DPRINTF(SimpleCache, "Forwarding packet\n");
            memPort.sendPacket(new_pkt);
        }
    }
}

bool SimpleCache::accessFunctional(PacketPtr pkt)
{
    Addr block_addr = pkt->getBlockAddr(blockSize);
    audo it = cacheStore.find(block_addr);

    if (it != cacheStore.end()) {
        if (pkt->isWrite()) {
            pkt->writeDataToBlock(it->second, blockSize);
        } else if (pkt->isRead()) {
            pkt->setDataFromBlock(it->second, blockSize);
        } else {
            panic("Unknown packet type\n");
        }

        return true;
    }

    return false;
}

void SimpleCache::insert(PacketPtr pkt)
{
    assert(pkt->getAddr() == pkt->getBlockAddr(blockSize));
    assert(cacheStore.find(pkt->getAddr()) == cacheStore.end());
    assert(pkt->isResponse());

    if (cacheStore.size() >= capacity) {
        int bucket, bucket_size;
        // Select random thing to evict. This is a little convoluted since we
        // are using a std::unordered_map. See http://bit.ly/2hrnLP2
        do {
            bucket = random_mt.random(0, (int) cacheStore.bucket_count() - 1);
        } while ((bucket_size = cacheStore.bucket_size(bucket)) == 0);

        auto block = std::next(cacheStore.begin(bucket), random_mt.random(0, bucket_size - 1));

        DPRINTF(SimpleCache, "Removing data at addr %x\n", block->first);

        RequestPtr req = std::make_shared<Request>(block->first, blockSize, 0, 0);

        PacketPtr new_pkt = new Packet(req, MemCmd::WritebackDirty, blockSize);
        new_pkt->dataDynamic(block->second);

        DPRINTF(SimpleCache, "Writing packet %s back\n", pkt->print());
        memPort.sendPacket(new_pkt);
        cacheStore.erase(block->first);
    }

    DPRINTF(SimpleCache, "Inserting %s\n", pkt->print());
    DDUMP(SimpleCache, pkt->getConstPtr<uint8_t>(), blockSize);

    uint8_t* data = new uint8_t[blockSize];
    cacheStore[pkt->getAddr()] = data;
    pkt->writeDataToBlock(data, blockSize);
}

AddrRangeList SimpleCache::getAddrRanges() const
{
    DPRINTF(SimpleCache, "Sending new ranges\n");
    return memPort.getAddrRanges();
}

void SimpleCache::sendRangeChange() const
{
    for (auto& port : cpuPorts) {
        port.sendRangeChange();
    }
}

SimpleCache::SimpleCacheStats::SimpleCacheStats(statistics::Group* parent) : statistics::Group(parent), ADD_STAT(hits, statistics::units::Count()::get, "Number of hits\n"), ADD_STAT(misses, statistics::units::Count()::get, "Number of misses\n"),
                                                                                ADD_STAT(missLatency, statistics::units::Tick::get(), "Ticks for misses to a cache\n"),
                                                                                ADD_STAT(hitRatio, statistics::units::Ratio::get(), "The hit ratio in the cache\n", hits / (hits + misses))
{
    missLatency.init(16);
}

}
