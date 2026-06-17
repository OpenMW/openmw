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

#ifndef YOJIMBO_CONFIG_H
#define YOJIMBO_CONFIG_H

#include "yojimbo_constants.h"

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define YOJIMBO_MAJOR_VERSION 1
#define YOJIMBO_MINOR_VERSION 2
#define YOJIMBO_PATCH_VERSION 5

#if !defined(YOJIMBO_DEBUG) && !defined(YOJIMBO_RELEASE)
#if defined(NDEBUG)
#define YOJIMBO_RELEASE
#else
#define YOJIMBO_DEBUG
#endif
#elif defined(YOJIMBO_DEBUG) && defined(YOJIMBO_RELEASE)
#error Can only define one of debug & release
#endif

#ifndef YOJIMBO_DEFAULT_TIMEOUT
#define YOJIMBO_DEFAULT_TIMEOUT 10
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4127 )
#pragma warning( disable : 4244 )
#endif // #ifdef _MSC_VER

#define YOJIMBO_PLATFORM_WINDOWS                    1
#define YOJIMBO_PLATFORM_MAC                        2
#define YOJIMBO_PLATFORM_UNIX                       3

#if defined(_WIN32)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_MAC
#else
#define YOJIMBO_PLATFORM YOJIMBO_PLATFORM_UNIX
#endif

#ifdef YOJIMBO_DEBUG

#define YOJIMBO_DEBUG_MEMORY_LEAKS                  1
#define YOJIMBO_DEBUG_MESSAGE_LEAKS                 1
#define YOJIMBO_DEBUG_MESSAGE_BUDGET                1

#else // #ifdef YOJIMBO_DEBUG

#define YOJIMBO_DEBUG_MEMORY_LEAKS                  0
#define YOJIMBO_DEBUG_MESSAGE_LEAKS                 0
#define YOJIMBO_DEBUG_MESSAGE_BUDGET                0

#endif // #ifdef YOJIMBO_DEBUG

#define YOJIMBO_ENABLE_LOGGING                      1

namespace yojimbo
{
    using namespace serialize;

    /// Determines the reliability and ordering guarantees for a channel.

    enum ChannelType
    {
        CHANNEL_TYPE_RELIABLE_ORDERED,                              ///< Messages are received reliably and in the same order they were sent.
        CHANNEL_TYPE_UNRELIABLE_UNORDERED                           ///< Messages are sent unreliably. Messages may arrive out of order, or not at all.
    };

    /**
        Configuration properties for a message channel.

        Channels let you specify different reliability and ordering guarantees for messages sent across a connection.

        They may be configured as one of two types: reliable-ordered or unreliable-unordered.

        Reliable ordered channels guarantee that messages (see Message) are received reliably and in the same order they were sent.
        This channel type is designed for control messages and RPCs sent between the client and server.

        Unreliable unordered channels are like UDP. There is no guarantee that messages will arrive, and messages may arrive out of order.
        This channel type is designed for data that is time critical and should not be resent if dropped, like snapshots of world state sent rapidly
        from server to client, or cosmetic events such as effects and sounds.

        Both channel types support blocks of data attached to messages (see BlockMessage), but their treatment of blocks is quite different.

        Reliable ordered channels are designed for blocks that must be received reliably and in-order with the rest of the messages sent over the channel.
        Examples of these sort of blocks include the initial state of a level, or server configuration data sent down to a client on connect. These blocks
        are sent by splitting them into fragments and resending each fragment until the other side has received the entire block. This allows for sending
        blocks of data larger that maximum packet size quickly and reliably even under packet loss.

        Unreliable-unordered channels send blocks as-is without splitting them up into fragments. The idea is that transport level packet fragmentation
        should be used on top of the generated packet to split it up into into smaller packets that can be sent across typical Internet MTU (<1500 bytes).
        Because of this, you need to make sure that the maximum block size for an unreliable-unordered channel fits within the maximum packet size.

        Channels are typically configured as part of a ConnectionConfig, which is included inside the ClientServerConfig that is passed into the Client and Server constructors.
     */

    struct ChannelConfig
    {
        ChannelType type;                                           ///< Channel type: reliable-ordered or unreliable-unordered.
        bool disableBlocks;                                         ///< Disables blocks being sent across this channel.
        int sentPacketBufferSize;                                   ///< Number of packet entries in the sent packet sequence buffer. Please consider your packet send rate and make sure you have at least a few seconds worth of entries in this buffer.
        int messageSendQueueSize;                                   ///< Number of messages in the send queue for this channel.
        int messageReceiveQueueSize;                                ///< Number of messages in the receive queue for this channel.
        int maxMessagesPerPacket;                                   ///< Maximum number of messages to include in each packet. Will write up to this many messages, provided the messages fit into the channel packet budget and the number of bytes remaining in the packet.
        int packetBudget;                                           ///< Maximum amount of message data to write to the packet for this channel (bytes). Specifying -1 means the channel can use up to the rest of the bytes remaining in the packet.
        int maxBlockSize;                                           ///< The size of the largest block that can be sent across this channel (bytes).
        int blockFragmentSize;                                      ///< Blocks are split up into fragments of this size (bytes). Reliable-ordered channel only.
        float messageResendTime;                                    ///< Minimum delay between message resends (seconds). Avoids sending the same message too frequently. Reliable-ordered channel only.
        float blockFragmentResendTime;                              ///< Minimum delay between block fragment resends (seconds). Avoids sending the same fragment too frequently. Reliable-ordered channel only.

        ChannelConfig() : type ( CHANNEL_TYPE_RELIABLE_ORDERED )
        {
            disableBlocks = false;
            sentPacketBufferSize = 1024;
            messageSendQueueSize = 1024;
            messageReceiveQueueSize = 1024;
            maxMessagesPerPacket = 256;
            packetBudget = -1;
            maxBlockSize = 256 * 1024;
            blockFragmentSize = 1024;
            messageResendTime = 0.1f;
            blockFragmentResendTime = 0.25f;
        }

        int GetMaxFragmentsPerBlock() const
        {
            return maxBlockSize / blockFragmentSize;
        }
    };

    /**
        Configures connection properties and the set of channels for sending and receiving messages.
        Specifies the maximum packet size to generate, and the number of message channels, and the per-channel configuration data. See ChannelConfig for details.
        Typically configured as part of a ClientServerConfig which is passed into Client and Server constructors.
     */

    struct ConnectionConfig
    {
        int numChannels;                                        ///< Number of message channels in [1,MaxChannels]. Each message channel must have a corresponding configuration below.
        int maxPacketSize;                                      ///< The maximum size of packets generated to transmit messages between client and server (bytes).
        ChannelConfig channel[MaxChannels];                     ///< Per-channel configuration. See ChannelConfig for details.

        ConnectionConfig()
        {
            numChannels = 2;
            maxPacketSize = 8 * 1024;
            channel[0].type = CHANNEL_TYPE_RELIABLE_ORDERED;
        }
    };

    /**
        Configuration shared between client and server.
        Passed to Client and Server constructors to configure their behavior.
        Please make sure that the message configuration is identical between client and server.
     */

    struct ClientServerConfig : public ConnectionConfig
    {
        uint64_t protocolId;                                    ///< Clients can only connect to servers with the same protocol id. Use this for versioning.
        int timeout;                                            ///< Timeout value in seconds. Set to negative value to disable timeouts (for debugging only).
        int clientMemory;                                       ///< Memory allocated inside Client for packets, messages and stream allocations (bytes)
        int serverGlobalMemory;                                 ///< Memory allocated inside Server for global connection request and challenge response packets (bytes)
        int serverPerClientMemory;                              ///< Memory allocated inside Server for packets, messages and stream allocations per-client (bytes)
        bool networkSimulator;                                  ///< If true then a network simulator is created for simulating latency, jitter, packet loss and duplicates.
        int maxSimulatorPackets;                                ///< Maximum number of packets that can be stored in the network simulator. Additional packets are dropped.
        int fragmentPacketsAbove;                               ///< Packets above this size (bytes) are split apart into fragments and reassembled on the other side.
        int packetFragmentSize;                                 ///< Size of each packet fragment (bytes).
        int maxPacketFragments;                                 ///< Maximum number of fragments a packet can be split up into.
        int packetReassemblyBufferSize;                         ///< Number of packet entries in the fragmentation reassembly buffer.
        int ackedPacketsBufferSize;                             ///< Number of packet entries in the acked packet buffer. Consider your packet send rate and aim to have at least a few seconds worth of entries.
        int receivedPacketsBufferSize;                          ///< Number of packet entries in the received packet sequence buffer. Consider your packet send rate and aim to have at least a few seconds worth of entries.
        float rttSmoothingFactor;                               ///< Round-Trip Time (RTT) smoothing factor over time.

        ClientServerConfig()
        {
            protocolId = 0;
            timeout = YOJIMBO_DEFAULT_TIMEOUT;
            clientMemory = 10 * 1024 * 1024;
            serverGlobalMemory = 10 * 1024 * 1024;
            serverPerClientMemory = 10 * 1024 * 1024;
            networkSimulator = true;
            maxSimulatorPackets = 4 * 1024;
            fragmentPacketsAbove = 1024;
            packetFragmentSize = 1024;
            maxPacketFragments = (int) ceil( maxPacketSize / packetFragmentSize );
            packetReassemblyBufferSize = 64;
            ackedPacketsBufferSize = 256;
            receivedPacketsBufferSize = 256;
            rttSmoothingFactor = 0.0025f;
        }
    };
}

#endif // # YOJIMBO_CONFIG_H
