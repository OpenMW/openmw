#include "yojimbo_base_server.h"
#include "yojimbo_adapter.h"
#include "yojimbo_network_simulator.h"
#include "yojimbo_connection.h"
#include "yojimbo_network_info.h"
#include "yojimbo_utils.h"
#include "reliable.h"

namespace yojimbo
{
    BaseServer::BaseServer( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time ) : m_config( config )
    {
        m_allocator = &allocator;
        m_adapter = &adapter;
        m_context = NULL;
        m_time = time;
        m_running = false;
        m_maxClients = 0;
        m_globalMemory = NULL;
        m_globalAllocator = NULL;
        for ( int i = 0; i < MaxClients; ++i )
        {
            m_clientMemory[i] = NULL;
            m_clientAllocator[i] = NULL;
            m_clientMessageFactory[i] = NULL;
            m_clientConnection[i] = NULL;
            m_clientEndpoint[i] = NULL;
        }
        m_networkSimulator = NULL;
        m_packetBuffer = NULL;
    }

    BaseServer::~BaseServer()
    {
        // IMPORTANT: Please stop the server before destroying it!
        yojimbo_assert( !IsRunning () );
        m_allocator = NULL;
    }

    void BaseServer::SetContext( void * context )
    {
        yojimbo_assert( !IsRunning() );
        m_context = context;
    }

    void BaseServer::Start( int maxClients )
    {
        Stop();
        m_running = true;
        m_maxClients = maxClients;
        yojimbo_assert( !m_globalMemory );
        yojimbo_assert( !m_globalAllocator );
        m_globalMemory = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.serverGlobalMemory );
        m_globalAllocator = m_adapter->CreateAllocator( *m_allocator, m_globalMemory, m_config.serverGlobalMemory );
        yojimbo_assert( m_globalAllocator );
        if ( m_config.networkSimulator )
        {
            m_networkSimulator = YOJIMBO_NEW( *m_globalAllocator, NetworkSimulator, *m_globalAllocator, m_config.maxSimulatorPackets, m_time );
        }
        for ( int i = 0; i < m_maxClients; ++i )
        {
            yojimbo_assert( !m_clientMemory[i] );
            yojimbo_assert( !m_clientAllocator[i] );
            
            m_clientMemory[i] = (uint8_t*) YOJIMBO_ALLOCATE( *m_allocator, m_config.serverPerClientMemory );
            m_clientAllocator[i] = m_adapter->CreateAllocator( *m_allocator, m_clientMemory[i], m_config.serverPerClientMemory );
            yojimbo_assert( m_clientAllocator[i] );
            
            m_clientMessageFactory[i] = m_adapter->CreateMessageFactory( *m_clientAllocator[i] );
            yojimbo_assert( m_clientMessageFactory[i] );
            
            m_clientConnection[i] = YOJIMBO_NEW( *m_clientAllocator[i], Connection, *m_clientAllocator[i], *m_clientMessageFactory[i], m_config, m_time );
            yojimbo_assert( m_clientConnection[i] );

            reliable_config_t reliable_config;
            reliable_default_config( &reliable_config );
            yojimbo_copy_string( reliable_config.name, "server endpoint", sizeof( reliable_config.name ) );
            reliable_config.context = (void*) this;
            reliable_config.id = i;
            reliable_config.max_packet_size = m_config.maxPacketSize;
            reliable_config.fragment_above = m_config.fragmentPacketsAbove;
            reliable_config.max_fragments = m_config.maxPacketFragments;
            reliable_config.fragment_size = m_config.packetFragmentSize; 
            reliable_config.ack_buffer_size = m_config.ackedPacketsBufferSize;
            reliable_config.received_packets_buffer_size = m_config.receivedPacketsBufferSize;
            reliable_config.fragment_reassembly_buffer_size = m_config.packetReassemblyBufferSize;
            reliable_config.rtt_smoothing_factor = m_config.rttSmoothingFactor;
            reliable_config.transmit_packet_function = BaseServer::StaticTransmitPacketFunction;
            reliable_config.process_packet_function = BaseServer::StaticProcessPacketFunction;
            reliable_config.allocator_context = &GetGlobalAllocator();
            reliable_config.allocate_function = BaseServer::StaticAllocateFunction;
            reliable_config.free_function = BaseServer::StaticFreeFunction;
            m_clientEndpoint[i] = reliable_endpoint_create( &reliable_config, m_time );
            reliable_endpoint_reset( m_clientEndpoint[i] );
        }
        m_packetBuffer = (uint8_t*) YOJIMBO_ALLOCATE( *m_globalAllocator, m_config.maxPacketSize );
    }

    void BaseServer::Stop()
    {
        if ( IsRunning() )
        {
            YOJIMBO_FREE( *m_globalAllocator, m_packetBuffer );
            yojimbo_assert( m_globalMemory );
            yojimbo_assert( m_globalAllocator );
            YOJIMBO_DELETE( *m_globalAllocator, NetworkSimulator, m_networkSimulator );
            for ( int i = 0; i < m_maxClients; ++i )
            {
                yojimbo_assert( m_clientMemory[i] );
                yojimbo_assert( m_clientAllocator[i] );
                yojimbo_assert( m_clientMessageFactory[i] );
                yojimbo_assert( m_clientEndpoint[i] );
                reliable_endpoint_destroy( m_clientEndpoint[i] ); m_clientEndpoint[i] = NULL;
                YOJIMBO_DELETE( *m_clientAllocator[i], Connection, m_clientConnection[i] );
                YOJIMBO_DELETE( *m_clientAllocator[i], MessageFactory, m_clientMessageFactory[i] );
                YOJIMBO_DELETE( *m_allocator, Allocator, m_clientAllocator[i] );
                YOJIMBO_FREE( *m_allocator, m_clientMemory[i] );
            }
            YOJIMBO_DELETE( *m_allocator, Allocator, m_globalAllocator );
            YOJIMBO_FREE( *m_allocator, m_globalMemory );
        }
        m_running = false;
        m_maxClients = 0;
        m_packetBuffer = NULL;
    }

    void BaseServer::AdvanceTime( double time )
    {
        m_time = time;
        if ( IsRunning() )
        {
            for ( int i = 0; i < m_maxClients; ++i )
            {
                m_clientConnection[i]->AdvanceTime( time );
                if ( m_clientConnection[i]->GetErrorLevel() != CONNECTION_ERROR_NONE )
                {
                    yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "client %d connection is in error state. disconnecting client\n", i );
                    DisconnectClient( i );
                    continue;
                }
                reliable_endpoint_update( m_clientEndpoint[i], m_time );
                int numAcks;
                const uint16_t * acks = reliable_endpoint_get_acks( m_clientEndpoint[i], &numAcks );
                m_clientConnection[i]->ProcessAcks( acks, numAcks );
                reliable_endpoint_clear_acks( m_clientEndpoint[i] );
            }
            NetworkSimulator * networkSimulator = GetNetworkSimulator();
            if ( networkSimulator )
            {
                networkSimulator->AdvanceTime( time );
            }        
        }
    }

    void BaseServer::SetLatency( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetLatency( milliseconds );
        }
    }

    void BaseServer::SetJitter( float milliseconds )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetJitter( milliseconds );
        }
    }

    void BaseServer::SetPacketLoss( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetPacketLoss( percent );
        }
    }

    void BaseServer::SetDuplicates( float percent )
    {
        if ( m_networkSimulator )
        {
            m_networkSimulator->SetDuplicates( percent );
        }
    }

    Message * BaseServer::CreateMessage( int clientIndex, int type )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientMessageFactory[clientIndex] );
        return m_clientMessageFactory[clientIndex]->CreateMessage( type );
    }

    uint8_t * BaseServer::AllocateBlock( int clientIndex, int bytes )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientAllocator[clientIndex] );
        return (uint8_t*) YOJIMBO_ALLOCATE( *m_clientAllocator[clientIndex], bytes );
    }

    void BaseServer::AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( message );
        yojimbo_assert( block );
        yojimbo_assert( bytes > 0 );
        yojimbo_assert( message->IsBlockMessage() );
        BlockMessage * blockMessage = (BlockMessage*) message;
        blockMessage->AttachBlock( *m_clientAllocator[clientIndex], block, bytes );
    }

    void BaseServer::FreeBlock( int clientIndex, uint8_t * block )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        YOJIMBO_FREE( *m_clientAllocator[clientIndex], block );
    }

    bool BaseServer::CanSendMessage( int clientIndex, int channelIndex ) const
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->CanSendMessage( channelIndex );
    }

    bool BaseServer::HasMessagesToSend( int clientIndex, int channelIndex ) const
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->HasMessagesToSend( channelIndex );
    }

    void BaseServer::SendMessage( int clientIndex, int channelIndex, Message * message )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->SendMessage( channelIndex, message, GetContext() );
    }

    Message * BaseServer::ReceiveMessage( int clientIndex, int channelIndex )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return m_clientConnection[clientIndex]->ReceiveMessage( channelIndex );
    }

    void BaseServer::ReleaseMessage( int clientIndex, Message * message )
    {
        yojimbo_assert( clientIndex >= 0 );
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        m_clientConnection[clientIndex]->ReleaseMessage( message );
    }

    void BaseServer::GetNetworkInfo( int clientIndex, NetworkInfo & info ) const
    {
        yojimbo_assert( IsRunning() );
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        memset( &info, 0, sizeof( info ) );
        if ( IsClientConnected( clientIndex ) )
        {
            yojimbo_assert( m_clientEndpoint[clientIndex] );
            const uint64_t * counters = reliable_endpoint_counters( m_clientEndpoint[clientIndex] );
            info.numPacketsSent = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT];
            info.numPacketsReceived = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED];
            info.numPacketsAcked = counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED];
            info.RTT = reliable_endpoint_rtt( m_clientEndpoint[clientIndex] );
            info.packetLoss = reliable_endpoint_packet_loss( m_clientEndpoint[clientIndex] );
            reliable_endpoint_bandwidth( m_clientEndpoint[clientIndex], &info.sentBandwidth, &info.receivedBandwidth, &info.ackedBandwidth );
        }
    }

    MessageFactory & BaseServer::GetClientMessageFactory( int clientIndex ) 
    { 
        yojimbo_assert( IsRunning() ); 
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        return *m_clientMessageFactory[clientIndex];
    }

    reliable_endpoint_t * BaseServer::GetClientEndpoint( int clientIndex )
    {
        yojimbo_assert( IsRunning() ); 
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        return m_clientEndpoint[clientIndex];
    }

    Connection & BaseServer::GetClientConnection( int clientIndex )
    {
        yojimbo_assert( IsRunning() ); 
        yojimbo_assert( clientIndex >= 0 ); 
        yojimbo_assert( clientIndex < m_maxClients );
        yojimbo_assert( m_clientConnection[clientIndex] );
        return *m_clientConnection[clientIndex];
    }

    void BaseServer::StaticTransmitPacketFunction( void * context, uint64_t index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        BaseServer * server = (BaseServer*) context;
        server->TransmitPacketFunction( index, packetSequence, packetData, packetBytes );
    }
    
    int BaseServer::StaticProcessPacketFunction( void * context, uint64_t index, uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        BaseServer * server = (BaseServer*) context;
        return server->ProcessPacketFunction( index, packetSequence, packetData, packetBytes );
    }

    void * BaseServer::StaticAllocateFunction( void * context, size_t bytes )
    {
        yojimbo_assert( context );
        Allocator * allocator = (Allocator*) context;
        return YOJIMBO_ALLOCATE( *allocator, bytes );
    }
    
    void BaseServer::StaticFreeFunction( void * context, void * pointer )
    {
        yojimbo_assert( context );
        yojimbo_assert( pointer );
        Allocator * allocator = (Allocator*) context;
        YOJIMBO_FREE( *allocator, pointer );
    }

    void BaseServer::ResetClient( int clientIndex )
    {
        m_clientConnection[clientIndex]->Reset();
    }
}
