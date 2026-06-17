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

#include "yojimbo_base_client.h"
#include "yojimbo_allocator.h"
#include "yojimbo_connection.h"
#include "yojimbo_network_simulator.h"
#include "yojimbo_adapter.h"
#include "yojimbo_utils.h"
#include "reliable.h"

namespace yojimbo
{
    BaseClient::BaseClient( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time ) : m_config( config )
    {
        m_allocator = &allocator;
        m_adapter = &adapter;
        m_time = time;
        m_context = NULL;
        m_clientMemory = NULL;
        m_clientAllocator = NULL;
        m_endpoint = NULL;
        m_connection = NULL;
        m_messageFactory = NULL;
        m_networkSimulator = NULL;
        m_clientState = CLIENT_STATE_DISCONNECTED;
        m_clientIndex = -1;
        m_packetBuffer = (uint8_t*) YOJIMBO_ALLOCATE( allocator, config.maxPacketSize );
    }

    BaseClient::~BaseClient()
    {
        // IMPORTANT: Please disconnect the client before destroying it
        yojimbo_assert( m_clientState <= CLIENT_STATE_DISCONNECTED );
        YOJIMBO_FREE( *m_allocator, m_packetBuffer );
        m_allocator = NULL;
    }

    void BaseClient::Disconnect()
    {
        SetClientState( CLIENT_STATE_DISCONNECTED );
        Reset();
    }

    void BaseClient::AdvanceTime( double time )
    {
        m_time = time;
        if ( m_endpoint )
        {
            m_connection->AdvanceTime( time );
            if ( m_connection->GetErrorLevel() != CONNECTION_ERROR_NONE )
            {
                yojimbo_printf( YOJIMBO_LOG_LEVEL_DEBUG, "connection error. disconnecting client\n" );
                Disconnect();
                return;
            }
            reliable_endpoint_update( m_endpoint, m_time );
            int numAcks;
            const uint16_t * acks = reliable_endpoint_get_acks( m_endpoint, &numAcks );
            m_connection->ProcessAcks( acks, numAcks );
            reliable_endpoint_clear_acks( m_endpoint );
        }
        NetworkSimulator * networkSimulator = GetNetworkSimulator();
        if ( networkSimulator )
        {
            networkSimulator->AdvanceTime( time );
        }
    }

    void BaseClient::SetLatency( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetLatency( milliseconds );
        }
    }

    void BaseClient::SetJitter( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetJitter( milliseconds );
        }
    }

    void BaseClient::SetPacketLoss( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetPacketLoss( percent );
        }
    }

    void BaseClient::SetDuplicates( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetDuplicates( percent );
        }
    }

    void BaseClient::SetClientState( ClientState clientState )
    {
        m_clientState = clientState;
    }

    void BaseClient::CreateInternal()
    {
        yojimbo_assert( m_allocator );
        yojimbo_assert( m_adapter );
        yojimbo_assert( m_clientMemory == NULL );
        yojimbo_assert( m_clientAllocator == NULL );
        yojimbo_assert( m_messageFactory == NULL );
        m_clientMemory = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.clientMemory );
        m_clientAllocator = m_adapter->CreateAllocator( *m_allocator, m_clientMemory, m_config.clientMemory );
        m_messageFactory = m_adapter->CreateMessageFactory( *m_clientAllocator );
        m_connection = YOJIMBO_NEW( *m_clientAllocator, Connection, *m_clientAllocator, *m_messageFactory, m_config, m_time );
        yojimbo_assert( m_connection );
        if ( m_config.networkSimulator )
        {
            m_networkSimulator = YOJIMBO_NEW( *m_clientAllocator, NetworkSimulator, *m_clientAllocator, m_config.maxSimulatorPackets, m_time );
        }
        reliable_config_t reliable_config;
        reliable_default_config( &reliable_config );
        yojimbo_copy_string( reliable_config.name, "client endpoint", sizeof( reliable_config.name ) );
        reliable_config.context = (void*) this;
        reliable_config.max_packet_size = m_config.maxPacketSize;
        reliable_config.fragment_above = m_config.fragmentPacketsAbove;
        reliable_config.max_fragments = m_config.maxPacketFragments;
        reliable_config.fragment_size = m_config.packetFragmentSize; 
        reliable_config.ack_buffer_size = m_config.ackedPacketsBufferSize;
        reliable_config.received_packets_buffer_size = m_config.receivedPacketsBufferSize;
        reliable_config.fragment_reassembly_buffer_size = m_config.packetReassemblyBufferSize;
        reliable_config.rtt_smoothing_factor = m_config.rttSmoothingFactor;
        reliable_config.transmit_packet_function = BaseClient::StaticTransmitPacketFunction;
        reliable_config.process_packet_function = BaseClient::StaticProcessPacketFunction;
        reliable_config.allocator_context = m_clientAllocator;
        reliable_config.allocate_function = BaseClient::StaticAllocateFunction;
        reliable_config.free_function = BaseClient::StaticFreeFunction;
        m_endpoint = reliable_endpoint_create( &reliable_config, m_time );
        reliable_endpoint_reset( m_endpoint );
    }

    void BaseClient::DestroyInternal()
    {
        yojimbo_assert( m_allocator );
        if ( m_endpoint )
        {
            reliable_endpoint_destroy( m_endpoint ); 
            m_endpoint = NULL;
        }
        YOJIMBO_DELETE( *m_clientAllocator, NetworkSimulator, m_networkSimulator );
        YOJIMBO_DELETE( *m_clientAllocator, Connection, m_connection );
        YOJIMBO_DELETE( *m_clientAllocator, MessageFactory, m_messageFactory );
        YOJIMBO_DELETE( *m_allocator, Allocator, m_clientAllocator );
        YOJIMBO_FREE( *m_allocator, m_clientMemory );
    }

    void BaseClient::StaticTransmitPacketFunction( void * context, uint64_t index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) index;
        BaseClient * client = (BaseClient*) context;
        client->TransmitPacketFunction( packetSequence, packetData, packetBytes );
    }
    
    int BaseClient::StaticProcessPacketFunction( void * context, uint64_t index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) index;
        BaseClient * client = (BaseClient*) context;
        return client->ProcessPacketFunction( packetSequence, packetData, packetBytes );
    }

    void * BaseClient::StaticAllocateFunction( void * context, size_t bytes )
    {
        yojimbo_assert( context );
        Allocator * allocator = (Allocator*) context;
        return YOJIMBO_ALLOCATE( *allocator, bytes );
    }
    
    void BaseClient::StaticFreeFunction( void * context, void * pointer )
    {
        yojimbo_assert( context );
        yojimbo_assert( pointer );
        Allocator * allocator = (Allocator*) context;
        YOJIMBO_FREE( *allocator, pointer );
    }

    Message * BaseClient::CreateMessage( int type )
    {
        yojimbo_assert( m_messageFactory );
        return m_messageFactory->CreateMessage( type );
    }

    uint8_t * BaseClient::AllocateBlock( int bytes )
    {
        return (uint8_t*) YOJIMBO_ALLOCATE( *m_clientAllocator, bytes );
    }

    void BaseClient::AttachBlockToMessage( Message * message, uint8_t * block, int bytes )
    {
        yojimbo_assert( message );
        yojimbo_assert( block );
        yojimbo_assert( bytes > 0 );
        yojimbo_assert( message->IsBlockMessage() );
        BlockMessage * blockMessage = (BlockMessage*) message;
        blockMessage->AttachBlock( *m_clientAllocator, block, bytes );
    }

    void BaseClient::FreeBlock( uint8_t * block )
    {
        YOJIMBO_FREE( *m_clientAllocator, block );
    }

    bool BaseClient::CanSendMessage( int channelIndex ) const
    {
        yojimbo_assert( m_connection );
        return m_connection->CanSendMessage( channelIndex );
    }

    bool BaseClient::HasMessagesToSend( int channelIndex ) const
    {
        yojimbo_assert( m_connection );
        return m_connection->HasMessagesToSend( channelIndex );
    }

    void BaseClient::SendMessage( int channelIndex, Message * message )
    {
        yojimbo_assert( m_connection );
        m_connection->SendMessage( channelIndex, message, GetContext() );
    }

    Message * BaseClient::ReceiveMessage( int channelIndex )
    {
        yojimbo_assert( m_connection );
        return m_connection->ReceiveMessage( channelIndex );
    }

    void BaseClient::ReleaseMessage( Message * message )
    {
        yojimbo_assert( m_connection );
        m_connection->ReleaseMessage( message );
    }

    void BaseClient::GetNetworkInfo( NetworkInfo & info ) const
    {
        memset( &info, 0, sizeof( info ) );
        if ( m_connection )
        {
            yojimbo_assert( m_endpoint );
            const uint64_t * counters = reliable_endpoint_counters( m_endpoint );
            info.numPacketsSent = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT];
            info.numPacketsReceived = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED];
            info.numPacketsAcked = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED];
            info.RTT = reliable_endpoint_rtt( m_endpoint );
            info.packetLoss = reliable_endpoint_packet_loss( m_endpoint );
            reliable_endpoint_bandwidth( m_endpoint, &info.sentBandwidth, &info.receivedBandwidth, &info.ackedBandwidth );
        }
    }

    void BaseClient::Reset()
    {
        if ( m_connection )
        {
            m_connection->Reset();
        }
    }
}
