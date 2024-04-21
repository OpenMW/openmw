/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2024, Mas Bandwidth LLC.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef YOJIMBO_NETWORK_SIMULATOR_H
#define YOJIMBO_NETWORK_SIMULATOR_H

#include "yojimbo_config.h"
#include "yojimbo_allocator.h"

namespace yojimbo
{
    /**
        Simulates packet loss, latency, jitter and duplicate packets.
        This is useful during development, so your game is tested and played under real world conditions, instead of ideal LAN conditions.
        This simulator works on packet send. This means that if you want 125ms of latency (round trip), you must to add 125/2 = 62.5ms of latency to each side.
     */

    class NetworkSimulator
    {
    public:

        /**
            Create a network simulator.
            Initial network conditions are set to:
                Latency: 0ms
                Jitter: 0ms
                Packet Loss: 0%
                Duplicates: 0%
            @param allocator The allocator to use.
            @param numPackets The maximum number of packets that can be stored in the simulator at any time.
            @param time The initial time value in seconds.
         */

        NetworkSimulator( Allocator & allocator, int numPackets, double time );

        /**
            Network simulator destructor.
            Any packet data still in the network simulator is destroyed.
         */

        ~NetworkSimulator();

        /**
            Set the latency in milliseconds.
            This latency is added on packet send. To simulate a round trip time of 100ms, add 50ms of latency to both sides of the connection.
            @param milliseconds The latency to add in milliseconds.
         */

        void SetLatency( float milliseconds );

        /**
            Set the packet jitter in milliseconds.
            Jitter is applied +/- this amount in milliseconds. To be truly effective, jitter must be applied together with some latency.
            @param milliseconds The amount of jitter to add in milliseconds (+/-).
         */

        void SetJitter( float milliseconds );

        /**
            Set the amount of packet loss to apply on send.
            @param percent The packet loss percentage. 0% = no packet loss. 100% = all packets are dropped.
         */

        void SetPacketLoss( float percent );

        /**
            Set percentage chance of packet duplicates.
            If the duplicate chance succeeds, a duplicate packet is added to the queue with a random delay of up to 1 second.
            @param percent The percentage chance of a packet duplicate being sent. 0% = no duplicate packets. 100% = all packets have a duplicate sent.
         */

        void SetDuplicates( float percent );

        /**
            Is the network simulator active?
            The network simulator is active when packet loss, latency, duplicates or jitter are non-zero values.
            This is used by the transport to know whether it should shunt packets through the simulator, or send them directly to the network. This is a minor optimization.
         */

        bool IsActive() const;

        /**
            Queue a packet to send.
            IMPORTANT: Ownership of the packet data pointer is *not* transferred to the network simulator. It makes a copy of the data instead.
            @param to The slot index the packet should be sent to.
            @param packetData The packet data.
            @param packetBytes The packet size (bytes).
         */

        void SendPacket( int to, uint8_t * packetData, int packetBytes );

        /**
            Receive packets sent to any address.
            IMPORTANT: You take ownership of the packet data you receive and are responsible for freeing it. See NetworkSimulator::GetAllocator.
            @param maxPackets The maximum number of packets to receive.
            @param packetData Array of packet data pointers to be filled [out].
            @param packetBytes Array of packet sizes to be filled [out].
            @param to Array of to indices to be filled [out].
            @returns The number of packets received.
         */

        int ReceivePackets( int maxPackets, uint8_t * packetData[], int packetBytes[], int to[] );

        /**
            Discard all packets in the network simulator.
            This is useful if the simulator needs to be reset and used for another purpose.
         */

        void DiscardPackets();

        /**
            Discard packets sent to a particular client index.
            This is called when a client disconnects from the server.
         */

        void DiscardClientPackets( int clientIndex );

        /**
            Advance network simulator time.
            You must pump this regularly otherwise the network simulator won't work.
            @param time The current time value. Please make sure you use double values for time so you retain sufficient precision as time increases.
         */

        void AdvanceTime( double time );

        /**
            Get the allocator to use to free packet data.
            @returns The allocator that packet data is allocated with.
         */

        Allocator & GetAllocator() { yojimbo_assert( m_allocator ); return *m_allocator; }

    protected:

        /**
            Helper function to update the active flag whenever network settings are changed.
            Active is set to true if any of the network conditions are non-zero. This allows you to quickly check if the network simulator is active and would actually do something.
         */

        void UpdateActive();

    private:

        Allocator * m_allocator;                        ///< The allocator passed in to the constructor. It's used to allocate and free packet data.
        float m_latency;                                ///< Latency in milliseconds
        float m_jitter;                                 ///< Jitter in milliseconds +/-
        float m_packetLoss;                             ///< Packet loss percentage.
        float m_duplicates;                             ///< Duplicate packet percentage
        bool m_active;                                  ///< True if network simulator is active, eg. if any of the network settings above are enabled.

        /// A packet buffered in the network simulator.

        struct PacketEntry
        {
            int to;                                     ///< To index this packet should be sent to (for server -> client packets).
            double deliveryTime;                        ///< Delivery time for this packet (seconds).
            uint8_t * packetData;                       ///< Packet data (owns this pointer).
            int packetBytes;                            ///< Size of packet in bytes.
        };

        double m_time;                                  ///< Current time from last call to advance time.
        int m_currentIndex;                             ///< Current index in the packet entry array. New packets are inserted here.
        int m_numPacketEntries;                         ///< Number of elements in the packet entry array.
        PacketEntry * m_packetEntries;                  ///< Pointer to dynamically allocated packet entries. This is where buffered packets are stored.
    };
}

#endif // #ifndef YOJIMBO_NETWORK_SIMULATOR_H
