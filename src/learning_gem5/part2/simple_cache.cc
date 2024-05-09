#include "learning_gem5/part2/simple_cache.hh"

#include "base/compiler.hh"
#include "base/random.hh"
#include "debug/SimpleCache.hh"
#include "sime/system.hh"

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

}
