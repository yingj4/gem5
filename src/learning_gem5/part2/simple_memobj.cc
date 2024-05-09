// #include "learning_gem5/part2/simple_memobj.hh"

// #include "debug/SimpleMemobj.hh"

// namespace gem5
// {

// SimpleMemobj::SimpleMemobj(SimpleMemobjParams& params) : SimObject(params), instPort(params.name + ".inst_port", this), dataPort(params.name + ".data_port", this), memPort(params.name + ".mem_port", this), blocked(false)
// {
// }

// Port& SimpleMemobj::getPort(const std::string& if_name, PortID idx)
// {
//     panic_if(idx != InvalidPortID, "This object does not support vector ports\n");

//     // This is the name from the Python SimObject declaration (SimpleMemobj.py)
//     if (if_name == "mem_side") {
//         return memPort;
//     } else if (if_name == "inst_port") {
//         return instPort;
//     } else if (if_name == "data_port") {
//         return dataPort;
//     } else {
//         return SimObject::getPort(if_name, idx);
//     }
// }

// AddrRangeList SimpleMemobj::CPUSidePort::getAddrRanges() const
// {
//     return owner->getAddrRanges();
// }

// void SimpleMemobj::CPUSidePort::recvFunctional(PacketPtr pkt)
// {
//     return owner->handleFunctional(pkt);
// }

// void SimpleMemobj::handleFunctional(PacketPtr pkt)
// {
//     return memPort.sendFunctional(pkt);
// }

// AddrRangeList SimpleMemobj::getAddrRanges() const
// {
//     DPRINTF(SimpleMemobj, "Sending new ranges\n");
//     return memPort.getAddrRanges();
// }

// void SimpleMemobj::MemSidePort::recvRangeChange()
// {
//     return owner->sendRangeChange();
// }

// void SimpleMemobj::sendRangeChange()
// {
//     instPort.sendRangeChange();
//     dataPort.sendRangeChange();
// }

// bool SimpleMemobj::CPUSidePort::recvTimingReq(PacketPtr pkt)
// {
//     if (!owner->handleRequest(pkt)) {
//         needRetry = true;
//         return false;
//     } else {
//         return true;
//     }
// }

// bool SimpleMemobj::handleRequest(PacketPtr pkt)
// {
//     if (blocked) {
//         return false;
//     }
//     DPRINTF(SimpleMemobj, "Got request for address %x\n", pkt->getAddr());
//     blocked = true;
//     memPort.sendPacket(pkt);
//     return true;
// }

// void SimpleMemobj::MemSidePort::sendPacket(PacketPtr pkt)
// {
//     panic_if(blockedPacket != nullptr, "Should never try to send if blocked!\n");
//     if (!sendTimingReq(pkt)) {
//         blockedPacket = pkt;
//     }
// }

// void SimpleMemobj::MemSidePort::recvReqRetry()
// {
//     assert(blockedPacket != nullptr);

//     PacketPtr pkt = blockedPacket;
//     blockedPacket = nullptr;

//     sendPacket(pkt);
// }

// bool SimpleMemobj::MemSidePort::recvTimingResp(PacketPtr pkt)
// {
//     return owner->handleResponse(pkt);
// }

// bool SimpleMemobj::handleResponse(PacketPtr pkt)
// {
//     assert(blocked);
//     DPRINTF(SimpleMemobj, "Got response from address %x\n", pkt->getAddr());

//     blocked = false;

//     // Simply forward to the memory port
//     if (pkt->req->isInstFetch()) {
//         instPort.sendPacket(pkt);
//     } else {
//         dataPort.sendPacket(pkt);
//     }

//     instPort.trySendRetry();
//     dataPort.trySendRetry();

//     return true;
// }

// void SimpleMemobj::CPUSidePort::sendPacket(PacketPtr pkt)
// {
//     panic_if(blockedPacket != nullptr, "Should never try to send if blocked!\n");
//     if (!sendTimingResp(pkt)) {
//         blockedPacket = pkt;
//     }
// }

// void SimpleMemobj::CPUSidePort::recvRespRetry()
// {
//     assert(blockedPacket != nullptr);

//     PacketPtr pkt = blockedPacket;
//     blockedPacket = nullptr;

//     sendPacket(pkt);
// }

// void SimpleMemobj::CPUSidePort::trySendRetry()
// {
//     if (needRetry && blockedPacket == nullptr) {
//         needRetry = false;
//         DPRINTF(SimpleMemobj, "Sending retry for request %d\n", id);
//         sendRetryReq();
//     }
// }

// // SimpleMemobj* SimpleMemobjParams::create()
// // {
// //     return new SimpleMemobj(this);
// // }

// }

#include "learning_gem5/part2/simple_memobj.hh"

#include "base/trace.hh"
#include "debug/SimpleMemobj.hh"

namespace gem5
{

SimpleMemobj::SimpleMemobj(const SimpleMemobjParams &params) :
    SimObject(params),
    instPort(params.name + ".inst_port", this),
    dataPort(params.name + ".data_port", this),
    memPort(params.name + ".mem_side", this),
    blocked(false)
{
}

Port &
SimpleMemobj::getPort(const std::string &if_name, PortID idx)
{
    panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    // This is the name from the Python SimObject declaration (SimpleMemobj.py)
    if (if_name == "mem_side") {
        return memPort;
    } else if (if_name == "inst_port") {
        return instPort;
    } else if (if_name == "data_port") {
        return dataPort;
    } else {
        // pass it along to our super class
        return SimObject::getPort(if_name, idx);
    }
}

void
SimpleMemobj::CPUSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.

    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingResp(pkt)) {
        blockedPacket = pkt;
    }
}

AddrRangeList
SimpleMemobj::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

void
SimpleMemobj::CPUSidePort::trySendRetry()
{
    if (needRetry && blockedPacket == nullptr) {
        // Only send a retry if the port is now completely free
        needRetry = false;
        DPRINTF(SimpleMemobj, "Sending retry req for %d\n", id);
        sendRetryReq();
    }
}

void
SimpleMemobj::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleFunctional(pkt);
}

bool
SimpleMemobj::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    // Just forward to the memobj.
    if (!owner->handleRequest(pkt)) {
        needRetry = true;
        return false;
    } else {
        return true;
    }
}

void
SimpleMemobj::CPUSidePort::recvRespRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}

void
SimpleMemobj::MemSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.

    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

bool
SimpleMemobj::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleResponse(pkt);
}

void
SimpleMemobj::MemSidePort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}

void
SimpleMemobj::MemSidePort::recvRangeChange()
{
    owner->sendRangeChange();
}

bool
SimpleMemobj::handleRequest(PacketPtr pkt)
{
    if (blocked) {
        // There is currently an outstanding request. Stall.
        return false;
    }

    DPRINTF(SimpleMemobj, "Got request for addr %#x\n", pkt->getAddr());

    // This memobj is now blocked waiting for the response to this packet.
    blocked = true;

    // Simply forward to the memory port
    memPort.sendPacket(pkt);

    return true;
}

bool
SimpleMemobj::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(SimpleMemobj, "Got response for addr %#x\n", pkt->getAddr());

    // The packet is now done. We're about to put it in the port, no need for
    // this object to continue to stall.
    // We need to free the resource before sending the packet in case the CPU
    // tries to send another request immediately (e.g., in the same callchain).
    blocked = false;

    // Simply forward to the memory port
    if (pkt->req->isInstFetch()) {
        instPort.sendPacket(pkt);
    } else {
        dataPort.sendPacket(pkt);
    }

    // For each of the cpu ports, if it needs to send a retry, it should do it
    // now since this memory object may be unblocked now.
    instPort.trySendRetry();
    dataPort.trySendRetry();

    return true;
}

void
SimpleMemobj::handleFunctional(PacketPtr pkt)
{
    // Just pass this on to the memory side to handle for now.
    memPort.sendFunctional(pkt);
}

AddrRangeList
SimpleMemobj::getAddrRanges() const
{
    DPRINTF(SimpleMemobj, "Sending new ranges\n");
    // Just use the same ranges as whatever is on the memory side.
    return memPort.getAddrRanges();
}

void
SimpleMemobj::sendRangeChange()
{
    instPort.sendRangeChange();
    dataPort.sendRangeChange();
}

} // namespace gem5
