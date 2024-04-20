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

#include "yojimbo_reliable_ordered_channel.h"
#include "yojimbo_utils.h"

namespace yojimbo
{
    ReliableOrderedChannel::ReliableOrderedChannel( Allocator & allocator, MessageFactory & messageFactory, const ChannelConfig & config, int channelIndex, double time ) 
        : Channel( allocator, messageFactory, config, channelIndex, time )
    {
        yojimbo_assert( config.type == CHANNEL_TYPE_RELIABLE_ORDERED );

        yojimbo_assert( ( 65536 % config.sentPacketBufferSize ) == 0 );
        yojimbo_assert( ( 65536 % config.messageSendQueueSize ) == 0 );
        yojimbo_assert( ( 65536 % config.messageReceiveQueueSize ) == 0 );

        m_sentPackets = YOJIMBO_NEW( *m_allocator, SequenceBuffer<SentPacketEntry>, *m_allocator, m_config.sentPacketBufferSize );
        m_messageSendQueue = YOJIMBO_NEW( *m_allocator, SequenceBuffer<MessageSendQueueEntry>, *m_allocator, m_config.messageSendQueueSize );
        m_messageReceiveQueue = YOJIMBO_NEW( *m_allocator, SequenceBuffer<MessageReceiveQueueEntry>, *m_allocator, m_config.messageReceiveQueueSize );
        m_sentPacketMessageIds = (uint16_t*) YOJIMBO_ALLOCATE( *m_allocator, sizeof( uint16_t ) * m_config.maxMessagesPerPacket * m_config.sentPacketBufferSize );

        if ( !config.disableBlocks )
        {
            m_sendBlock = YOJIMBO_NEW( *m_allocator, SendBlockData, *m_allocator, m_config.GetMaxFragmentsPerBlock() ); 
            m_receiveBlock = YOJIMBO_NEW( *m_allocator, ReceiveBlockData, *m_allocator, m_config.maxBlockSize, m_config.GetMaxFragmentsPerBlock() );
        }
        else
        {
            m_sendBlock = NULL;
            m_receiveBlock = NULL;
        }

        Reset();
    }

    ReliableOrderedChannel::~ReliableOrderedChannel()
    {
        Reset();

        YOJIMBO_DELETE( *m_allocator, SendBlockData, m_sendBlock );
        YOJIMBO_DELETE( *m_allocator, ReceiveBlockData, m_receiveBlock );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<SentPacketEntry>, m_sentPackets );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<MessageSendQueueEntry>, m_messageSendQueue );
        YOJIMBO_DELETE( *m_allocator, SequenceBuffer<MessageReceiveQueueEntry>, m_messageReceiveQueue );
        
        YOJIMBO_FREE( *m_allocator, m_sentPacketMessageIds );

        m_sentPacketMessageIds = NULL;
    }

    void ReliableOrderedChannel::Reset()
    {
        SetErrorLevel( CHANNEL_ERROR_NONE );

        m_sendMessageId = 0;
        m_receiveMessageId = 0;
        m_oldestUnackedMessageId = 0;

        for ( int i = 0; i < m_messageSendQueue->GetSize(); ++i )
        {
            MessageSendQueueEntry * entry = m_messageSendQueue->GetAtIndex( i );
            if ( entry && entry->message )
                m_messageFactory->ReleaseMessage( entry->message );
        }

        for ( int i = 0; i < m_messageReceiveQueue->GetSize(); ++i )
        {
            MessageReceiveQueueEntry * entry = m_messageReceiveQueue->GetAtIndex( i );
            if ( entry && entry->message )
                m_messageFactory->ReleaseMessage( entry->message );
        }

        m_sentPackets->Reset();
        m_messageSendQueue->Reset();
        m_messageReceiveQueue->Reset();

        if ( m_sendBlock )
        {
            m_sendBlock->Reset();
        }

        if ( m_receiveBlock )
        {
            m_receiveBlock->Reset();
            if ( m_receiveBlock->blockMessage )
            {
                m_messageFactory->ReleaseMessage( m_receiveBlock->blockMessage );
                m_receiveBlock->blockMessage = NULL;
            }
        }

        ResetCounters();
    }

#undef SendMessage

    bool ReliableOrderedChannel::CanSendMessage() const
    {
        yojimbo_assert( m_messageSendQueue );
        return m_messageSendQueue->Available( m_sendMessageId );
    }

    void ReliableOrderedChannel::SendMessage( Message * message, void *context )
    {
        yojimbo_assert( message );
        
        yojimbo_assert( CanSendMessage() );

        if ( GetErrorLevel() != CHANNEL_ERROR_NONE )
        {
            m_messageFactory->ReleaseMessage( message );
            return;
        }

        if ( !CanSendMessage() )
        {
            // Increase your send queue size!
            SetErrorLevel( CHANNEL_ERROR_SEND_QUEUE_FULL );
            m_messageFactory->ReleaseMessage( message );
            return;
        }

        yojimbo_assert( !( message->IsBlockMessage() && m_config.disableBlocks ) );

        if ( message->IsBlockMessage() && m_config.disableBlocks )
        {
            // You tried to send a block message, but block messages are disabled for this channel!
            SetErrorLevel( CHANNEL_ERROR_BLOCKS_DISABLED );
            m_messageFactory->ReleaseMessage( message );
            return;
        }

        message->SetId( m_sendMessageId );

        MessageSendQueueEntry * entry = m_messageSendQueue->Insert( m_sendMessageId );

        yojimbo_assert( entry );

        entry->block = message->IsBlockMessage();
        entry->message = message;
        entry->measuredBits = 0;
        entry->timeLastSent = -1.0;

        if ( message->IsBlockMessage() )
        {
            yojimbo_assert( ((BlockMessage*)message)->GetBlockSize() > 0 );
            yojimbo_assert( ((BlockMessage*)message)->GetBlockSize() <= m_config.maxBlockSize );
        }

        MeasureStream measureStream;
		measureStream.SetContext( context );
        measureStream.SetAllocator( &m_messageFactory->GetAllocator() );
        message->SerializeInternal( measureStream );
        entry->measuredBits = measureStream.GetBitsProcessed();
        m_counters[CHANNEL_COUNTER_MESSAGES_SENT]++;
        m_sendMessageId++;
    }

    Message * ReliableOrderedChannel::ReceiveMessage()
    {
        if ( GetErrorLevel() != CHANNEL_ERROR_NONE )
            return NULL;

        MessageReceiveQueueEntry * entry = m_messageReceiveQueue->Find( m_receiveMessageId );
        if ( !entry )
            return NULL;

        Message * message = entry->message;
        yojimbo_assert( message );
        yojimbo_assert( message->GetId() == m_receiveMessageId );
        m_messageReceiveQueue->Remove( m_receiveMessageId );
        m_counters[CHANNEL_COUNTER_MESSAGES_RECEIVED]++;
        m_receiveMessageId++;

        return message;
    }

    void ReliableOrderedChannel::AdvanceTime( double time )
    {
        m_time = time;
    }
    
    int ReliableOrderedChannel::GetPacketData( void *context, ChannelPacketData & packetData, uint16_t packetSequence, int availableBits )
    {
        if ( !HasMessagesToSend() )
            return 0;

        if ( SendingBlockMessage() )
        {
            if (m_config.blockFragmentSize * 8 > availableBits)
                return 0;

            uint16_t messageId;
            uint16_t fragmentId;
            int fragmentBytes;
            int numFragments;
            int messageType;

            uint8_t * fragmentData = GetFragmentToSend( messageId, fragmentId, fragmentBytes, numFragments, messageType );

            if ( fragmentData )
            {
                const int fragmentBits = GetFragmentPacketData( packetData, messageId, fragmentId, fragmentData, fragmentBytes, numFragments, messageType );
                AddFragmentPacketEntry( messageId, fragmentId, packetSequence );
                return fragmentBits;
            }
        }
        else
        {
            int numMessageIds = 0;
            uint16_t * messageIds = (uint16_t*) alloca( m_config.maxMessagesPerPacket * sizeof( uint16_t ) );
            const int messageBits = GetMessagesToSend( messageIds, numMessageIds, availableBits, context );

            if ( numMessageIds > 0 )
            {
                GetMessagePacketData( packetData, messageIds, numMessageIds );
                AddMessagePacketEntry( messageIds, numMessageIds, packetSequence );
                return messageBits;
            }
        }

        return 0;
    }

    bool ReliableOrderedChannel::HasMessagesToSend() const
    {
        return m_oldestUnackedMessageId != m_sendMessageId;
    }

    int ReliableOrderedChannel::GetMessagesToSend( uint16_t * messageIds, int & numMessageIds, int availableBits, void *context )
    {
        yojimbo_assert( HasMessagesToSend() );

        numMessageIds = 0;

        if ( m_config.packetBudget > 0 )
            availableBits = yojimbo_min( m_config.packetBudget * 8, availableBits );

        const int giveUpBits = 4 * 8;
        const int messageTypeBits = bits_required( 0, m_messageFactory->GetNumTypes() - 1 );
        const int messageLimit = yojimbo_min( m_config.messageSendQueueSize, m_config.messageReceiveQueueSize );
        uint16_t previousMessageId = 0;
        int usedBits = ConservativeMessageHeaderBits;
        int giveUpCounter = 0;
#ifdef YOJIMBO_DEBUG
        const int maxBits = availableBits;
#endif // YOJIMBO_DEBUG

        for ( int i = 0; i < messageLimit; ++i )
        {
            if ( availableBits - usedBits < giveUpBits )
                break;

            if ( giveUpCounter > m_config.messageSendQueueSize )
                break;

            uint16_t messageId = m_oldestUnackedMessageId + i;
            MessageSendQueueEntry * entry = m_messageSendQueue->Find( messageId );
            if ( !entry )
                continue;

            if ( entry->block )
                break;

            // Increase your max packet size!
            yojimbo_assert( entry->measuredBits <= uint32_t(maxBits) );
            
            if ( entry->timeLastSent + m_config.messageResendTime <= m_time && availableBits >= (int) entry->measuredBits )
            {                
                int messageBits = entry->measuredBits + messageTypeBits;
                
                if ( numMessageIds == 0 )
                {
                    messageBits += 16;
                }
                else
                {
                    MeasureStream stream;
                    stream.SetContext( context );
                    stream.SetAllocator( &m_messageFactory->GetAllocator() );
                    serialize_sequence_relative_internal( stream, previousMessageId, messageId );
                    messageBits += stream.GetBitsProcessed();
                }

                if ( usedBits + messageBits > availableBits )
                {
                    giveUpCounter++;
                    continue;
                }

                usedBits += messageBits;
                messageIds[numMessageIds++] = messageId;
                previousMessageId = messageId;
                entry->timeLastSent = m_time;
            }

            if ( numMessageIds == m_config.maxMessagesPerPacket )
                break;
        }

        return usedBits;
    }

    void ReliableOrderedChannel::GetMessagePacketData( ChannelPacketData & packetData, const uint16_t * messageIds, int numMessageIds )
    {
        yojimbo_assert( messageIds );

        packetData.Initialize();
        packetData.channelIndex = GetChannelIndex();
        packetData.message.numMessages = numMessageIds;
        
        if ( numMessageIds == 0 )
            return;

        packetData.message.messages = (Message**) YOJIMBO_ALLOCATE( m_messageFactory->GetAllocator(), sizeof( Message* ) * numMessageIds );

        for ( int i = 0; i < numMessageIds; ++i )
        {
            MessageSendQueueEntry * entry = m_messageSendQueue->Find( messageIds[i] );
            yojimbo_assert( entry );
            yojimbo_assert( entry->message );
            yojimbo_assert( entry->message->GetRefCount() > 0 );
            packetData.message.messages[i] = entry->message;
            m_messageFactory->AcquireMessage( packetData.message.messages[i] );
        }
    }

    void ReliableOrderedChannel::AddMessagePacketEntry( const uint16_t * messageIds, int numMessageIds, uint16_t sequence )
    {
        SentPacketEntry * sentPacket = m_sentPackets->Insert( sequence, true );
        yojimbo_assert( sentPacket );
        if ( sentPacket )
        {
            sentPacket->acked = 0;
            sentPacket->block = 0;
            sentPacket->timeSent = m_time;
            sentPacket->messageIds = &m_sentPacketMessageIds[ ( sequence % m_config.sentPacketBufferSize ) * m_config.maxMessagesPerPacket ];
            sentPacket->numMessageIds = numMessageIds;            
            for ( int i = 0; i < numMessageIds; ++i )
            {
                sentPacket->messageIds[i] = messageIds[i];
            }
        }
    }

    void ReliableOrderedChannel::ProcessPacketMessages( int numMessages, Message ** messages )
    {
        const uint16_t minMessageId = m_receiveMessageId;
        const uint16_t maxMessageId = m_receiveMessageId + m_config.messageReceiveQueueSize - 1;

        for ( int i = 0; i < (int) numMessages; ++i )
        {
            Message * message = messages[i];

            yojimbo_assert( message );  

            const uint16_t messageId = message->GetId();

            if ( yojimbo_sequence_less_than( messageId, minMessageId ) )
                continue;

            if ( yojimbo_sequence_greater_than( messageId, maxMessageId ) )
            {
                // Did you forget to dequeue messages on the receiver?
                yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "sequence overflow: %d vs. [%d,%d]\n", messageId, minMessageId, maxMessageId );
                SetErrorLevel( CHANNEL_ERROR_DESYNC );
                return;
            }

            if ( m_messageReceiveQueue->Find( messageId ) )
                continue;

            yojimbo_assert( !m_messageReceiveQueue->GetAtIndex( m_messageReceiveQueue->GetIndex( messageId ) ) );

            MessageReceiveQueueEntry * entry = m_messageReceiveQueue->Insert( messageId );
            if ( !entry )
            {
                // For some reason we can't insert the message in the receive queue
                SetErrorLevel( CHANNEL_ERROR_DESYNC );
                return;
            }

            entry->message = message;

            m_messageFactory->AcquireMessage( message );
        }
    }

    void ReliableOrderedChannel::ProcessPacketData( const ChannelPacketData & packetData, uint16_t packetSequence )
    {
        if ( m_errorLevel != CHANNEL_ERROR_NONE )
            return;
        
        if ( packetData.messageFailedToSerialize )
        {
            // A message failed to serialize read for some reason, eg. mismatched read/write.
            SetErrorLevel( CHANNEL_ERROR_FAILED_TO_SERIALIZE );
            return;
        }

        (void)packetSequence;

        if ( packetData.blockMessage )
        {
            ProcessPacketFragment( packetData.block.messageType, 
                                   packetData.block.messageId, 
                                   packetData.block.numFragments, 
                                   packetData.block.fragmentId, 
                                   packetData.block.fragmentData, 
                                   packetData.block.fragmentSize, 
                                   packetData.block.message );
        }
        else
        {
            ProcessPacketMessages( packetData.message.numMessages, packetData.message.messages );
        }
    }

    void ReliableOrderedChannel::ProcessAck( uint16_t ack )
    {
        SentPacketEntry * sentPacketEntry = m_sentPackets->Find( ack );
        if ( !sentPacketEntry )
            return;

        yojimbo_assert( !sentPacketEntry->acked );

        for ( int i = 0; i < (int) sentPacketEntry->numMessageIds; ++i )
        {
            const uint16_t messageId = sentPacketEntry->messageIds[i];
            MessageSendQueueEntry * sendQueueEntry = m_messageSendQueue->Find( messageId );
            if ( sendQueueEntry )
            {
                yojimbo_assert( sendQueueEntry->message );
                yojimbo_assert( sendQueueEntry->message->GetId() == messageId );
                m_messageFactory->ReleaseMessage( sendQueueEntry->message );
                m_messageSendQueue->Remove( messageId );
                UpdateOldestUnackedMessageId();
            }
        }

        if ( !m_config.disableBlocks && sentPacketEntry->block && m_sendBlock->active && m_sendBlock->blockMessageId == sentPacketEntry->blockMessageId )
        {        
            const int messageId = sentPacketEntry->blockMessageId;
            const int fragmentId = sentPacketEntry->blockFragmentId;

            if ( !m_sendBlock->ackedFragment->GetBit( fragmentId ) )
            {
                m_sendBlock->ackedFragment->SetBit( fragmentId );
                m_sendBlock->numAckedFragments++;
                if ( m_sendBlock->numAckedFragments == m_sendBlock->numFragments )
                {
                    m_sendBlock->active = false;
                    MessageSendQueueEntry * sendQueueEntry = m_messageSendQueue->Find( messageId );
                    yojimbo_assert( sendQueueEntry );
                    m_messageFactory->ReleaseMessage( sendQueueEntry->message );
                    m_messageSendQueue->Remove( messageId );
                    UpdateOldestUnackedMessageId();
                }
            }
        }
    }

    void ReliableOrderedChannel::UpdateOldestUnackedMessageId()
    {
        const uint16_t stopMessageId = m_messageSendQueue->GetSequence();

        while ( true )
        {
            if ( m_oldestUnackedMessageId == stopMessageId || m_messageSendQueue->Find( m_oldestUnackedMessageId ) )
            {
                break;
            }
            ++m_oldestUnackedMessageId;
        }

        yojimbo_assert( !yojimbo_sequence_greater_than( m_oldestUnackedMessageId, stopMessageId ) );
    }

    bool ReliableOrderedChannel::SendingBlockMessage()
    {
        yojimbo_assert( HasMessagesToSend() );

        MessageSendQueueEntry * entry = m_messageSendQueue->Find( m_oldestUnackedMessageId );

        return entry ? entry->block : false;
    }

    uint8_t * ReliableOrderedChannel::GetFragmentToSend( uint16_t & messageId, uint16_t & fragmentId, int & fragmentBytes, int & numFragments, int & messageType )
    {
        MessageSendQueueEntry * entry = m_messageSendQueue->Find( m_oldestUnackedMessageId );

        yojimbo_assert( entry );
        yojimbo_assert( entry->block );

        BlockMessage * blockMessage = (BlockMessage*) entry->message;

        yojimbo_assert( blockMessage );

        messageId = blockMessage->GetId();

        const int blockSize = blockMessage->GetBlockSize();

        if ( !m_sendBlock->active )
        {
            // start sending this block

            m_sendBlock->active = true;
            m_sendBlock->blockSize = blockSize;
            m_sendBlock->blockMessageId = messageId;
            m_sendBlock->numFragments = (int) ceil( blockSize / float( m_config.blockFragmentSize ) );
            m_sendBlock->numAckedFragments = 0;

            const int MaxFragmentsPerBlock = m_config.GetMaxFragmentsPerBlock();

            yojimbo_assert( m_sendBlock->numFragments > 0 );
            yojimbo_assert( m_sendBlock->numFragments <= MaxFragmentsPerBlock );

            m_sendBlock->ackedFragment->Clear();

            for ( int i = 0; i < MaxFragmentsPerBlock; ++i )
                m_sendBlock->fragmentSendTime[i] = -1.0;
        }

        numFragments = m_sendBlock->numFragments;

        // find the next fragment to send (there may not be one)

        fragmentId = 0xFFFF;

        for ( int i = 0; i < m_sendBlock->numFragments; ++i )
        {
            if ( !m_sendBlock->ackedFragment->GetBit( i ) && m_sendBlock->fragmentSendTime[i] + m_config.blockFragmentResendTime < m_time )
            {
                fragmentId = uint16_t( i );
                break;
            }
        }

        if ( fragmentId == 0xFFFF )
            return NULL;

        // allocate and return a copy of the fragment data

        messageType = blockMessage->GetType();

        fragmentBytes = m_config.blockFragmentSize;
        
        const int fragmentRemainder = blockSize % m_config.blockFragmentSize;

        if ( fragmentRemainder && fragmentId == m_sendBlock->numFragments - 1 )
            fragmentBytes = fragmentRemainder;

        uint8_t * fragmentData = (uint8_t*) YOJIMBO_ALLOCATE( m_messageFactory->GetAllocator(), fragmentBytes );

        if ( fragmentData )
        {
            memcpy( fragmentData, blockMessage->GetBlockData() + fragmentId * m_config.blockFragmentSize, fragmentBytes );

            m_sendBlock->fragmentSendTime[fragmentId] = m_time;
        }

        return fragmentData;
    }

    int ReliableOrderedChannel::GetFragmentPacketData( ChannelPacketData & packetData, 
                                                       uint16_t messageId, 
                                                       uint16_t fragmentId, 
                                                       uint8_t * fragmentData, 
                                                       int fragmentSize, 
                                                       int numFragments, 
                                                       int messageType )
    {
        packetData.Initialize();

        packetData.channelIndex = GetChannelIndex();

        packetData.blockMessage = 1;

        packetData.block.fragmentData = fragmentData;
        packetData.block.messageId = messageId;
        packetData.block.fragmentId = fragmentId;
        packetData.block.fragmentSize = fragmentSize;
        packetData.block.numFragments = numFragments;
        packetData.block.messageType = messageType;

        const int messageTypeBits = bits_required( 0, m_messageFactory->GetNumTypes() - 1 );

        int fragmentBits = ConservativeFragmentHeaderBits + fragmentSize * 8;

        if ( fragmentId == 0 )
        {
            MessageSendQueueEntry * entry = m_messageSendQueue->Find( packetData.block.messageId );

            yojimbo_assert( entry );
            yojimbo_assert( entry->message );

            packetData.block.message = (BlockMessage*) entry->message;

            m_messageFactory->AcquireMessage( packetData.block.message );

            fragmentBits += entry->measuredBits + messageTypeBits;
        }
        else
        {
            packetData.block.message = NULL;
        }

        return fragmentBits;
    }

    void ReliableOrderedChannel::AddFragmentPacketEntry( uint16_t messageId, uint16_t fragmentId, uint16_t sequence )
    {
        SentPacketEntry * sentPacket = m_sentPackets->Insert( sequence, true );
        yojimbo_assert( sentPacket );
        if ( sentPacket )
        {
            sentPacket->numMessageIds = 0;
            sentPacket->messageIds = NULL;
            sentPacket->timeSent = m_time;
            sentPacket->acked = 0;
            sentPacket->block = 1;
            sentPacket->blockMessageId = messageId;
            sentPacket->blockFragmentId = fragmentId;
        }
    }

    void ReliableOrderedChannel::ProcessPacketFragment( int messageType, 
                                                        uint16_t messageId, 
                                                        int numFragments, 
                                                        uint16_t fragmentId, 
                                                        const uint8_t * fragmentData, 
                                                        int fragmentBytes, 
                                                        BlockMessage * blockMessage )
    {  
        yojimbo_assert( !m_config.disableBlocks );

        if ( fragmentData )
        {
            const uint16_t expectedMessageId = m_messageReceiveQueue->GetSequence();
            if ( messageId != expectedMessageId )
                return;

            // start receiving a new block

            if ( !m_receiveBlock->active )
            {
                yojimbo_assert( numFragments >= 0 );
                yojimbo_assert( numFragments <= m_config.GetMaxFragmentsPerBlock() );

                m_receiveBlock->active = true;
                m_receiveBlock->numFragments = numFragments;
                m_receiveBlock->numReceivedFragments = 0;
                m_receiveBlock->messageId = messageId;
                m_receiveBlock->blockSize = 0;
                m_receiveBlock->receivedFragment->Clear();
            }

            // validate fragment

            if ( fragmentId >= m_receiveBlock->numFragments )
            {
                // The fragment id is out of range.
                SetErrorLevel( CHANNEL_ERROR_DESYNC );
                return;
            }

            if ( numFragments != m_receiveBlock->numFragments )
            {
                // The number of fragments is out of range.
                SetErrorLevel( CHANNEL_ERROR_DESYNC );
                return;
            }

            // receive the fragment

            if ( !m_receiveBlock->receivedFragment->GetBit( fragmentId ) )
            {
                m_receiveBlock->receivedFragment->SetBit( fragmentId );

                memcpy( m_receiveBlock->blockData + fragmentId * m_config.blockFragmentSize, fragmentData, fragmentBytes );

                if ( fragmentId == 0 )
                {
                    m_receiveBlock->messageType = messageType;
                }

                if ( fragmentId == m_receiveBlock->numFragments - 1 )
                {
                    m_receiveBlock->blockSize = ( m_receiveBlock->numFragments - 1 ) * m_config.blockFragmentSize + fragmentBytes;

                    if ( m_receiveBlock->blockSize > (uint32_t) m_config.maxBlockSize )
                    {
                        // The block size is outside range
                        SetErrorLevel( CHANNEL_ERROR_DESYNC );
                        return;
                    }
                }

                m_receiveBlock->numReceivedFragments++;

                if ( fragmentId == 0 )
                {
                    // save block message (sent with fragment 0)
                    m_receiveBlock->blockMessage = blockMessage;
                    m_messageFactory->AcquireMessage( m_receiveBlock->blockMessage );
                }

                if ( m_receiveBlock->numReceivedFragments == m_receiveBlock->numFragments )
                {
                    // finished receiving block

                    if ( m_messageReceiveQueue->GetAtIndex( m_messageReceiveQueue->GetIndex( messageId ) ) )
                    {
                        // Did you forget to dequeue messages on the receiver?
                        SetErrorLevel( CHANNEL_ERROR_DESYNC );
                        return;
                    }

                    blockMessage = m_receiveBlock->blockMessage;

                    yojimbo_assert( blockMessage );

                    uint8_t * blockData = (uint8_t*) YOJIMBO_ALLOCATE( m_messageFactory->GetAllocator(), m_receiveBlock->blockSize );

                    if ( !blockData )
                    {
                        // Not enough memory to allocate block data
                        SetErrorLevel( CHANNEL_ERROR_OUT_OF_MEMORY );
                        return;
                    }

                    memcpy( blockData, m_receiveBlock->blockData, m_receiveBlock->blockSize );

                    blockMessage->AttachBlock( m_messageFactory->GetAllocator(), blockData, m_receiveBlock->blockSize );

                    blockMessage->SetId( messageId );

                    MessageReceiveQueueEntry * entry = m_messageReceiveQueue->Insert( messageId );
                    yojimbo_assert( entry );
                    entry->message = blockMessage;
                    m_receiveBlock->active = false;
                    m_receiveBlock->blockMessage = NULL;
                }
            }
        }
    }
}
