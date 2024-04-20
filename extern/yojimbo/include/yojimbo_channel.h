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

#ifndef YOJIMBO_CHANNEL_H
#define YOJIMBO_CHANNEL_H

#include "yojimbo_config.h"
#include "yojimbo_message.h"

namespace yojimbo
{
    struct ChannelPacketData
    {
        uint32_t channelIndex : 16;
        uint32_t initialized : 1;
        uint32_t blockMessage : 1;
        uint32_t messageFailedToSerialize : 1;

        struct MessageData
        {
            int numMessages;
            Message ** messages;
        };

        struct BlockData
        {
            BlockMessage * message;
            uint8_t * fragmentData;
            uint64_t messageId : 16;
            uint64_t fragmentId : 16;
            uint64_t fragmentSize : 16;
            uint64_t numFragments : 16;
            int messageType;
        };

        union
        {
            MessageData message;
            BlockData block;
        };

        void Initialize();

        void Free( MessageFactory & messageFactory );

        template <typename Stream> bool Serialize( Stream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( ReadStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( WriteStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );

        bool SerializeInternal( MeasureStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels );
    };

    /**
        Channel counters provide insight into the number of times an action was performed by a channel.
        They are intended for use in a telemetry system, eg. reported to some backend logging system to track behavior in a production environment.
     */

    enum ChannelCounters
    {
        CHANNEL_COUNTER_MESSAGES_SENT,                          ///< Number of messages sent over this channel.
        CHANNEL_COUNTER_MESSAGES_RECEIVED,                      ///< Number of messages received over this channel.
        CHANNEL_COUNTER_NUM_COUNTERS                            ///< The number of channel counters.
    };

    /**
        Channel error level.
        If the channel gets into an error state, it sets an error state on the corresponding connection. See yojimbo::CONNECTION_ERROR_CHANNEL.
        This way if any channel on a client/server connection gets into a bad state, that client is automatically kicked from the server.
        @see Client
        @see Server
        @see Connection
     */

    enum ChannelErrorLevel
    {
        CHANNEL_ERROR_NONE = 0,                                 ///< No error. All is well.
        CHANNEL_ERROR_DESYNC,                                   ///< This channel has desynced. This means that the connection protocol has desynced and cannot recover. The client should be disconnected.
        CHANNEL_ERROR_SEND_QUEUE_FULL,                          ///< The user tried to send a message but the send queue was full. This will assert out in development, but in production it sets this error on the channel.
        CHANNEL_ERROR_BLOCKS_DISABLED,                          ///< The channel received a packet containing data for blocks, but this channel is configured to disable blocks. See ChannelConfig::disableBlocks.
        CHANNEL_ERROR_FAILED_TO_SERIALIZE,                      ///< Serialize read failed for a message sent to this channel. Check your message serialize functions, one of them is returning false on serialize read. This can also be caused by a desync in message read and write.
        CHANNEL_ERROR_OUT_OF_MEMORY,                            ///< The channel tried to allocate some memory but couldn't.
    };

    /// Helper function to convert a channel error to a user friendly string.

    inline const char * GetChannelErrorString( ChannelErrorLevel error )
    {
        switch ( error )
        {
            case CHANNEL_ERROR_NONE:                    return "none";
            case CHANNEL_ERROR_DESYNC:                  return "desync";
            case CHANNEL_ERROR_SEND_QUEUE_FULL:         return "send queue full";
            case CHANNEL_ERROR_OUT_OF_MEMORY:           return "out of memory";
            case CHANNEL_ERROR_BLOCKS_DISABLED:         return "blocks disabled";
            case CHANNEL_ERROR_FAILED_TO_SERIALIZE:     return "failed to serialize";
            default:
                yojimbo_assert( false );
                return "(unknown)";
        }
    }

    /// Common functionality shared across all channel types.

    class Channel
    {
    public:

        /**
            Channel constructor.
         */

        Channel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelIndex, double time );

        /**
            Channel destructor.
         */

        virtual ~Channel() {}

        /**
            Reset the channel.
         */

        virtual void Reset() = 0;

        /**
            Returns true if a message can be sent over this channel.
         */

        virtual bool CanSendMessage() const = 0;

        /**
            Are there any messages in the send queue?
            @returns True if there is at least one message in the send queue.
         */

         virtual bool HasMessagesToSend() const = 0;

        /**
            Queue a message to be sent across this channel.
            @param message The message to be sent.
         */

        virtual void SendMessage( Message * message, void *context) = 0;

        /**
            Pops the next message off the receive queue if one is available.
            @returns A pointer to the received message, NULL if there are no messages to receive. The caller owns the message object returned by this function and is responsible for releasing it via Message::Release.
         */

        virtual Message * ReceiveMessage() = 0;

        /**
            Advance channel time.
            Called by Connection::AdvanceTime for each channel configured on the connection.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Get channel packet data for this channel.
            @param packetData The channel packet data to be filled [out]
            @param packetSequence The sequence number of the packet being generated.
            @param availableBits The maximum number of bits of packet data the channel is allowed to write.
            @returns The number of bits of packet data written by the channel.
            @see ConnectionPacket
            @see Connection::GeneratePacket
         */

        virtual int GetPacketData( void *context, ChannelPacketData & packetData, uint16_t packetSequence, int availableBits) = 0;

        /**
            Process packet data included in a connection packet.
            @param packetData The channel packet data to process.
            @param packetSequence The sequence number of the connection packet that contains the channel packet data.
            @see ConnectionPacket
            @see Connection::ProcessPacket
         */

        virtual void ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence ) = 0;

        /**
            Process a connection packet ack.
            Depending on the channel type:
                1. Acks messages and block fragments so they stop being included in outgoing connection packets (reliable-ordered channel),
                2. Does nothing at all (unreliable-unordered).
            @param sequence The sequence number of the connection packet that was acked.
         */

        virtual void ProcessAck( uint16_t sequence ) = 0;

    public:

        /**
            Get the channel error level.
            @returns The channel error level.
         */

        ChannelErrorLevel GetErrorLevel() const;

        /**
            Gets the channel index.
            @returns The channel index in [0,numChannels-1].
         */

        int GetChannelIndex() const;

        /**
            Get a counter value.
            @param index The index of the counter to retrieve. See ChannelCounters.
            @returns The value of the counter.
            @see ResetCounters
         */

        uint64_t GetCounter( int index ) const;

        /**
            Resets all counter values to zero.
         */

        void ResetCounters();

    protected:

        /**
            Set the channel error level.
            All errors go through this function to make debug logging easier.
         */

        void SetErrorLevel( ChannelErrorLevel errorLevel );

    protected:

        const ChannelConfig m_config;                                                   ///< Channel configuration data.
        Allocator * m_allocator;                                                        ///< Allocator for allocations matching life cycle of this channel.
        int m_channelIndex;                                                             ///< The channel index in [0,numChannels-1].
        double m_time;                                                                  ///< The current time.
        ChannelErrorLevel m_errorLevel;                                                 ///< The channel error level.
        MessageFactory * m_messageFactory;                                              ///< Message factory for creating and destroying messages.
        uint64_t m_counters[CHANNEL_COUNTER_NUM_COUNTERS];                              ///< Counters for unit testing, stats etc.
    };
}

#endif // #ifndef YOJIMBO_CHANNEL_H
