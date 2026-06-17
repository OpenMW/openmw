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

#include "yojimbo_channel.h"

namespace yojimbo
{
    void ChannelPacketData::Initialize()
    {
        channelIndex = 0;
        blockMessage = 0;
        messageFailedToSerialize = 0;
        message.numMessages = 0;
        initialized = 1;
    }

    void ChannelPacketData::Free( MessageFactory & messageFactory )
    {
        yojimbo_assert( initialized );
        Allocator & allocator = messageFactory.GetAllocator();
        if ( !blockMessage )
        {
            if ( message.numMessages > 0 )
            {
                for ( int i = 0; i < message.numMessages; ++i )
                {
                    if ( message.messages[i] )
                    {
                        messageFactory.ReleaseMessage( message.messages[i] );
                    }
                }
                YOJIMBO_FREE( allocator, message.messages );
            }
        }
        else
        {
            if ( block.message )
            {
                messageFactory.ReleaseMessage( block.message );
                block.message = NULL;
            }
            YOJIMBO_FREE( allocator, block.fragmentData );
        }
        initialized = 0;
    }

    template <typename Stream> bool SerializeOrderedMessages( Stream & stream, 
                                                              MessageFactory & messageFactory, 
                                                              int & numMessages, 
                                                              Message ** & messages, 
                                                              int maxMessagesPerPacket )
    {
        const int maxMessageType = messageFactory.GetNumTypes() - 1;

        bool hasMessages = Stream::IsWriting && numMessages != 0;

        serialize_bool( stream, hasMessages );

        if ( hasMessages )
        {
            serialize_int( stream, numMessages, 1, maxMessagesPerPacket );

            int * messageTypes = (int*) alloca( sizeof( int ) * numMessages );

            uint16_t * messageIds = (uint16_t*) alloca( sizeof( uint16_t ) * numMessages );

            memset( messageTypes, 0, sizeof( int ) * numMessages );
            memset( messageIds, 0, sizeof( uint16_t ) * numMessages );

            if ( Stream::IsWriting )
            {
                yojimbo_assert( messages );

                for ( int i = 0; i < numMessages; ++i )
                {
                    yojimbo_assert( messages[i] );
                    messageTypes[i] = messages[i]->GetType();
                    messageIds[i] = messages[i]->GetId();
                }
            }
            else
            {
                Allocator & allocator = messageFactory.GetAllocator();

                messages = (Message**) YOJIMBO_ALLOCATE( allocator, sizeof( Message* ) * numMessages );

                for ( int i = 0; i < numMessages; ++i )
                {
                    messages[i] = NULL;
                }
            }

            serialize_bits( stream, messageIds[0], 16 );

            for ( int i = 1; i < numMessages; ++i )
                serialize_sequence_relative( stream, messageIds[i-1], messageIds[i] );

            for ( int i = 0; i < numMessages; ++i )
            {
                if ( maxMessageType > 0 )
                {
                    serialize_int( stream, messageTypes[i], 0, maxMessageType );
                }
                else
                {
                    messageTypes[i] = 0;
                }

                if ( Stream::IsReading )
                {
                    messages[i] = messageFactory.CreateMessage( messageTypes[i] );

                    if ( !messages[i] )
                    {
                        yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to create message of type %d (SerializeOrderedMessages)\n", messageTypes[i] );
                        return false;
                    }

                    messages[i]->SetId( messageIds[i] );
                }

                yojimbo_assert( messages[i] );

                if ( !messages[i]->SerializeInternal( stream ) )
                {
                    yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to serialize message of type %d (SerializeOrderedMessages)\n", messageTypes[i] );
                    return false;
                }
            }
        }

        return true;
    }

    template <typename Stream> bool SerializeUnorderedMessages( Stream & stream, 
                                                                MessageFactory & messageFactory, 
                                                                int & numMessages, 
                                                                Message ** & messages, 
                                                                int maxMessagesPerPacket, 
                                                                int maxBlockSize )
    {
        const int maxMessageType = messageFactory.GetNumTypes() - 1;

        bool hasMessages = Stream::IsWriting && numMessages != 0;

        serialize_bool( stream, hasMessages );

        if ( hasMessages )
        {
            serialize_int( stream, numMessages, 1, maxMessagesPerPacket );

            int * messageTypes = (int*) alloca( sizeof( int ) * numMessages );

            memset( messageTypes, 0, sizeof( int ) * numMessages );

            if ( Stream::IsWriting )
            {
                yojimbo_assert( messages );

                for ( int i = 0; i < numMessages; ++i )
                {
                    yojimbo_assert( messages[i] );
                    messageTypes[i] = messages[i]->GetType();
                }
            }
            else
            {
                Allocator & allocator = messageFactory.GetAllocator();

                messages = (Message**) YOJIMBO_ALLOCATE( allocator, sizeof( Message* ) * numMessages );

                for ( int i = 0; i < numMessages; ++i )
                    messages[i] = NULL;
            }

            for ( int i = 0; i < numMessages; ++i )
            {
                if ( maxMessageType > 0 )
                {
                    serialize_int( stream, messageTypes[i], 0, maxMessageType );
                }
                else
                {
                    messageTypes[i] = 0;
                }

                if ( Stream::IsReading )
                {
                    messages[i] = messageFactory.CreateMessage( messageTypes[i] );

                    if ( !messages[i] )
                    {
                        yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to create message type %d (SerializeUnorderedMessages)\n", messageTypes[i] );
                        return false;
                    }
                }

                yojimbo_assert( messages[i] );

                if ( !messages[i]->SerializeInternal( stream ) )
                {
                    yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to serialize message type %d (SerializeUnorderedMessages)\n", messageTypes[i] );
                    return false;
                }

                if ( messages[i]->IsBlockMessage() )
                {
                    BlockMessage * blockMessage = (BlockMessage*) messages[i];
                    if ( !SerializeMessageBlock( stream, messageFactory, blockMessage, maxBlockSize ) )
                    {
                        yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to serialize message block (SerializeUnorderedMessages)\n" );
                        return false;
                    }
                }
            }
        }

        return true;
    }

    template <typename Stream> bool SerializeBlockFragment( Stream & stream, 
                                                            MessageFactory & messageFactory, 
                                                            ChannelPacketData::BlockData & block, 
                                                            const ChannelConfig & channelConfig )
    {
        const int maxMessageType = messageFactory.GetNumTypes() - 1;

        if (Stream::IsReading)
        {
            block.message = NULL;
            block.fragmentData = NULL;
        }

        serialize_bits( stream, block.messageId, 16 );

        if ( channelConfig.GetMaxFragmentsPerBlock() > 1 )
        {
            serialize_int( stream, block.numFragments, 1, channelConfig.GetMaxFragmentsPerBlock() );
        }
        else
        {
            if ( Stream::IsReading )
                block.numFragments = 1;
        }

        if ( block.numFragments > 1 )
        {
            serialize_int( stream, block.fragmentId, 0, block.numFragments - 1 );
        }
        else
        {
            if ( Stream::IsReading )
                block.fragmentId = 0;
        }

        serialize_int( stream, block.fragmentSize, 1, channelConfig.blockFragmentSize );

        if ( Stream::IsReading )
        {
            block.fragmentData = (uint8_t*) YOJIMBO_ALLOCATE( messageFactory.GetAllocator(), block.fragmentSize );

            if ( !block.fragmentData )
            {
                yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to serialize block fragment (SerializeBlockFragment)\n" );
                return false;
            }
        }

        serialize_bytes( stream, block.fragmentData, block.fragmentSize );

        if ( block.fragmentId == 0 )
        {
            // block message

            if ( maxMessageType > 0 )
            {
                serialize_int( stream, block.messageType, 0, maxMessageType );
            }
            else
            {
                block.messageType = 0;
            }

            if ( Stream::IsReading )
            {
                Message * message = messageFactory.CreateMessage( block.messageType );

                if ( !message )
                {
                    yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to create block message type %d (SerializeBlockFragment)\n", block.messageType );
                    return false;
                }

                if ( !message->IsBlockMessage() )
                {
                    yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: received block fragment attached to non-block message (SerializeBlockFragment)\n" );
                    return false;
                }

                block.message = (BlockMessage*) message;
            }

            yojimbo_assert( block.message );

            if ( !block.message->SerializeInternal( stream ) )
            {
                yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to serialize block message of type %d (SerializeBlockFragment)\n", block.messageType );
                return false;
            }
        }

        return true;
    }

    template <typename Stream> bool ChannelPacketData::Serialize( Stream & stream, 
                                                                  MessageFactory & messageFactory, 
                                                                  const ChannelConfig * channelConfigs, 
                                                                  int numChannels )
    {
        yojimbo_assert( initialized );

#if YOJIMBO_DEBUG_MESSAGE_BUDGET
        int startBits = stream.GetBitsProcessed();
#endif // #if YOJIMBO_DEBUG_MESSAGE_BUDGET

        if ( numChannels > 1 )
            serialize_int( stream, channelIndex, 0, numChannels - 1 );
        else
            channelIndex = 0;

        const ChannelConfig & channelConfig = channelConfigs[channelIndex];

        serialize_bool( stream, blockMessage );

        if ( !blockMessage )
        {
            switch ( channelConfig.type )
            {
                case CHANNEL_TYPE_RELIABLE_ORDERED:
                {
                    if ( !SerializeOrderedMessages( stream, messageFactory, message.numMessages, message.messages, channelConfig.maxMessagesPerPacket ) )
                    {
                        messageFailedToSerialize = 1;
                        return true;
                    }
                }
                break;

                case CHANNEL_TYPE_UNRELIABLE_UNORDERED:
                {
                    if ( !SerializeUnorderedMessages( stream, 
                                                      messageFactory, 
                                                      message.numMessages, 
                                                      message.messages, 
                                                      channelConfig.maxMessagesPerPacket, 
                                                      channelConfig.maxBlockSize ) )
                    {
                        messageFailedToSerialize = 1;
                        return true;
                    }
                }
                break;
            }

#if YOJIMBO_DEBUG_MESSAGE_BUDGET
            if ( channelConfig.packetBudget > 0 )
            {
                yojimbo_assert( stream.GetBitsProcessed() - startBits <= channelConfig.packetBudget * 8 );
            }
#endif // #if YOJIMBO_DEBUG_MESSAGE_BUDGET
        }
        else
        {
            if ( channelConfig.disableBlocks )
                return false;

            if ( !SerializeBlockFragment( stream, messageFactory, block, channelConfig ) )
                return false;
        }

        return true;
    }

    bool ChannelPacketData::SerializeInternal( ReadStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels )
    {
        return Serialize( stream, messageFactory, channelConfigs, numChannels );
    }

    bool ChannelPacketData::SerializeInternal( WriteStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels )
    {
        return Serialize( stream, messageFactory, channelConfigs, numChannels );
    }

    bool ChannelPacketData::SerializeInternal( MeasureStream & stream, MessageFactory & messageFactory, const ChannelConfig * channelConfigs, int numChannels )
    {
        return Serialize( stream, messageFactory, channelConfigs, numChannels );
    }

    // ------------------------------------------------------------------------------------

    Channel::Channel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelIndex, double time ) : m_config( config )
    {
        yojimbo_assert( channelIndex >= 0 );
        yojimbo_assert( channelIndex < MaxChannels );
        m_channelIndex = channelIndex;
        m_allocator = &allocator;
        m_messageFactory = &messageFactory;
        m_errorLevel = CHANNEL_ERROR_NONE;
        m_time = time;
        ResetCounters();
    }

    uint64_t Channel::GetCounter( int index ) const
    {
        yojimbo_assert( index >= 0 );
        yojimbo_assert( index < CHANNEL_COUNTER_NUM_COUNTERS );
        return m_counters[index];
    }

    void Channel::ResetCounters()
    { 
        memset( m_counters, 0, sizeof( m_counters ) ); 
    }

    int Channel::GetChannelIndex() const 
    { 
        return m_channelIndex;
    }

    void Channel::SetErrorLevel( ChannelErrorLevel errorLevel )
    {
        if ( errorLevel != m_errorLevel && errorLevel != CHANNEL_ERROR_NONE )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "channel went into error state: %s\n", GetChannelErrorString( errorLevel ) );
        }
        m_errorLevel = errorLevel;
    }

    ChannelErrorLevel Channel::GetErrorLevel() const
    {
        return m_errorLevel;
    }
}
