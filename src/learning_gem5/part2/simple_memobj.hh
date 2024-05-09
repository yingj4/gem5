#ifndef __LEARNING_GEM5_SIMPLE_MEMOBJ_HH__
#define __LEARNING_GEM5_SIMPLE_MEMOBJ_HH__

#include "mem/port.hh"
#include "params/SimpleMemobj.hh"
#include "sim/sim_object.hh"

namespace gem5
{

class SimpleMemobj : public SimObject
{
  private:
    /**
     * Port on the CPU-side that receives requests.
     * Mostly just forwards requests to the owner.
     * Part of a vector of ports. One for each CPU port (e.g., data, inst)
     */
    class CPUSidePort : public ResponsePort    // This class is for the ports from the CPU
    {
    private:
        /// The object that owns this object (SimpleMemobj)
        SimpleMemobj* owner;

        /// True if the port needs to send a retry req.
        bool needRetry;

        /// If we tried to send a packet and it was blocked, store it here
        PacketPtr blockedPacket;

    public:
        CPUSidePort(const std::string& name, SimpleMemobj* owner) : ResponsePort(name), owner(owner), needRetry(false), blockedPacket(nullptr) { }

        /**
         * Send a packet across this port. This is called by the owner and
         * all of the flow control is hanled in this function.
         *
         * @param packet to send.
         */
        void sendPacket(PacketPtr pkt);

        AddrRangeList getAddrRanges() const override;

        /**
         * Send a retry to the peer port only if it is needed. This is called
         * from the SimpleMemobj whenever it is unblocked.
         */
        void trySendRetry();
    
    protected:
        Tick recvAtomic(PacketPtr pkt) override { panic("recvAtomic unimplemented!\n"); }
        void recvFunctional(PacketPtr pkt) override;
        bool recvTimingReq(PacketPtr pkt) override;
        void recvRespRetry() override;
    };

    class MemSidePort : public RequestPort   // This class is for the ports from the memory
    {
    private:
        SimpleMemobj* owner;
    
        /// If we tried to send a packet and it was blocked, store it here
        PacketPtr blockedPacket;
    public:
        MemSidePort(const std::string& name, SimpleMemobj* owner) : RequestPort(name), owner(owner), blockedPacket(nullptr) { }

        /**
         * Send a packet across this port. This is called by the owner and
         * all of the flow control is hanled in this function.
         *
         * @param packet to send.
         */
        void sendPacket(PacketPtr pkt);

    protected:
        bool recvTimingResp(PacketPtr pkt) override;
        void recvReqRetry() override;
        void recvRangeChange() override;
    };


    /**
     * Handle a packet functionally. Update the data on a write and get the
     * data on a read.
     *
     * @param packet to functionally handle
     */
    void handleFunctional(PacketPtr pkt);

    /**
     * Handle the request from the CPU side
     *
     * @param requesting packet
     * @return true if we can handle the request this cycle, false if the
     *         requestor needs to retry later
     */
    bool handleRequest(PacketPtr pkt);

    /**
     * Handle the respone from the memory side
     *
     * @param responding packet
     * @return true if we can handle the response this cycle, false if the
     *         responder needs to retry later
     */
    bool handleResponse(PacketPtr pkt);


    /**
     * Return the address ranges this memobj is responsible for. Just use the
     * same as the next upper level of the hierarchy.
     *
     * @return the address ranges this memobj is responsible for
     */
    AddrRangeList getAddrRanges() const;

    /**
     * Tell the CPU side to ask for our memory ranges.
     */
    void sendRangeChange();

    CPUSidePort instPort;
    CPUSidePort dataPort;

    MemSidePort memPort;

    /// True if this is currently blocked waiting for a response.
    bool blocked;

  public:
    /** constructor
    */
    SimpleMemobj(const SimpleMemobjParams& p);

    Port& getPort(const std::string& if_name, PortID idx = InvalidPortID) override;
};

}

#endif // __LEARNING_GEM5_SIMPLE_MEMOBJ_HH__
