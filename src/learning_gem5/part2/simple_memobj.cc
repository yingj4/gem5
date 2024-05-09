#include "learning_gem5/part2/simple_memobj.hh"

#include "debug/SimpleMemobj.hh"

namespace gem5
{

SimpleMemobj::SimpleMemobj(const SimpleMemobjParams& params) : SimObject(params), instPort(params.name + ".inst_port", this), dataPort(params.name + ".data_port", this), memPort(params.name + ".mem_side", this), blocked(false)
{
}

Port& SimpleMemobj::getPort(const std::string& if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object does not support vector ports\n");

    // This is the name from the Python SimObject declaration (SimpleMemobj.py)
    if (if_name == "mem_side") {
        return memPort;
    } else if (if_name == "inst_port") {
        return instPort;
    } else if (if_name == "data_port") {
        return dataPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

AddrRangeList SimpleMemobj::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

void SimpleMemobj::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    return owner->handleFunctional(pkt);
}

void SimpleMemobj::handleFunctional(PacketPtr pkt)
{
    return memPort.sendFunctional(pkt);
}

AddrRangeList SimpleMemobj::getAddrRanges() const
{
    DPRINTF(SimpleMemobj, "Sending new ranges\n");
    return memPort.getAddrRanges();
}

void SimpleMemobj::MemSidePort::recvRangeChange()
{
    return owner->sendRangeChange();
}

void SimpleMemobj::sendRangeChange()
{
    instPort.sendRangeChange();
    dataPort.sendRangeChange();
}

bool SimpleMemobj::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    if (!owner->handleRequest(pkt)) {
        needRetry = true;
        return false;
    } else {
        return true;
    }
}

bool SimpleMemobj::handleRequest(PacketPtr pkt)
{
    if (blocked) {
        return false;
    }
    DPRINTF(SimpleMemobj, "Got request for address %x\n", pkt->getAddr());
    blocked = true;
    memPort.sendPacket(pkt);
    return true;
}

void SimpleMemobj::MemSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!\n");
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

void SimpleMemobj::MemSidePort::recvReqRetry()
{
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);
}

bool SimpleMemobj::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleResponse(pkt);
}

bool SimpleMemobj::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(SimpleMemobj, "Got response from address %x\n", pkt->getAddr());

    blocked = false;

    // Simply forward to the memory port
    if (pkt->req->isInstFetch()) {
        instPort.sendPacket(pkt);
    } else {
        dataPort.sendPacket(pkt);
    }

    instPort.trySendRetry();
    dataPort.trySendRetry();

    return true;
}

void SimpleMemobj::CPUSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!\n");
    if (!sendTimingResp(pkt)) {
        blockedPacket = pkt;
    }
}

void SimpleMemobj::CPUSidePort::recvRespRetry()
{
    assert(blockedPacket != nullptr);

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    sendPacket(pkt);
}

void SimpleMemobj::CPUSidePort::trySendRetry()
{
    if (needRetry && blockedPacket == nullptr) {
        needRetry = false;
        DPRINTF(SimpleMemobj, "Sending retry for request %d\n", id);
        sendRetryReq();
    }
}

// SimpleMemobj* SimpleMemobjParams::create()
// {
//     return new SimpleMemobj(this);
// }

}
