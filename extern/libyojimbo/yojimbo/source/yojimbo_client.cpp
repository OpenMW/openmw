#include "yojimbo_client.h"
#include "yojimbo_connection.h"
#include "yojimbo_network_simulator.h"
#include "yojimbo_adapter.h"
#include "netcode.h"
#include "reliable.h"

namespace yojimbo
{
    Client::Client( Allocator & allocator, const Address & address, const ClientServerConfig & config, Adapter & adapter, double time ) 
        : BaseClient( allocator, config, adapter, time ), m_config( config ), m_address( address )
    {
        m_clientId = 0;
        m_client = NULL;
        m_boundAddress = m_address;
    }

    Client::~Client()
    {
        // IMPORTANT: Please disconnect the client before destroying it
        yojimbo_assert( m_client == NULL );
    }

    void Client::InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address & address )
    {
        InsecureConnect( privateKey, clientId, &address, 1 );
    }

    void Client::InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address serverAddresses[], int numServerAddresses )
    {
        yojimbo_assert( serverAddresses );
        yojimbo_assert( numServerAddresses > 0 );
        yojimbo_assert( numServerAddresses <= NETCODE_MAX_SERVERS_PER_CONNECT );
        Disconnect();
        CreateInternal();
        m_clientId = clientId;
        CreateClient( m_address );
        if ( !m_client )
        {
            Disconnect();
            return;
        }
        uint8_t connectToken[NETCODE_CONNECT_TOKEN_BYTES];
        if ( !GenerateInsecureConnectToken( connectToken, privateKey, clientId, serverAddresses, numServerAddresses ) )
        {
            yojimbo_printf( YOJIMBO_LOG_LEVEL_ERROR, "error: failed to generate insecure connect token\n" );
            SetClientState( CLIENT_STATE_ERROR );
            return;
        }
        netcode_client_connect( m_client, connectToken );
        SetClientState( CLIENT_STATE_CONNECTING );
    }

    bool Client::GenerateInsecureConnectToken( uint8_t * connectToken, 
                                               const uint8_t privateKey[], 
                                               uint64_t clientId, 
                                               const Address serverAddresses[], 
                                               int numServerAddresses )
    {
        char serverAddressStrings[NETCODE_MAX_SERVERS_PER_CONNECT][MaxAddressLength];
        const char * serverAddressStringPointers[NETCODE_MAX_SERVERS_PER_CONNECT];
        for ( int i = 0; i < numServerAddresses; ++i ) 
        {
            serverAddresses[i].ToString( serverAddressStrings[i], MaxAddressLength );
            serverAddressStringPointers[i] = serverAddressStrings[i];
        }

        uint8_t userData[256];
        memset( &userData, 0, sizeof(userData) );

        return netcode_generate_connect_token( numServerAddresses, 
                                               serverAddressStringPointers, 
                                               serverAddressStringPointers, 
                                               m_config.timeout,
                                               m_config.timeout, 
                                               clientId, 
                                               m_config.protocolId, 
                                               (uint8_t*)privateKey,
                                               &userData[0], 
                                               connectToken ) == NETCODE_OK;
    }

    void Client::Connect( uint64_t clientId, uint8_t * connectToken )
    {
        yojimbo_assert( connectToken );
        Disconnect();
        CreateInternal();
        m_clientId = clientId;
        CreateClient( m_address );
        netcode_client_connect( m_client, connectToken );
        if ( netcode_client_state( m_client ) > NETCODE_CLIENT_STATE_DISCONNECTED )
        {
            SetClientState( CLIENT_STATE_CONNECTING );
        }
        else
        {
            Disconnect();
        }
    }

    void Client::Disconnect()
    {
        BaseClient::Disconnect();
        DestroyClient();
        DestroyInternal();
        m_clientId = 0;
    }

    void Client::SendPackets()
    {
        if ( !IsConnected() )
            return;
        yojimbo_assert( m_client );
        uint8_t * packetData = GetPacketBuffer();
        int packetBytes;
        uint16_t packetSequence = reliable_endpoint_next_packet_sequence( GetEndpoint() );
        if ( GetConnection().GeneratePacket( GetContext(), packetSequence, packetData, m_config.maxPacketSize, packetBytes ) )
        {
            reliable_endpoint_send_packet( GetEndpoint(), packetData, packetBytes );
        }
    }

    void Client::ReceivePackets()
    {
        if ( !IsConnected() )
            return;
        yojimbo_assert( m_client );
        while ( true )
        {
            int packetBytes;
            uint64_t packetSequence;
            uint8_t * packetData = netcode_client_receive_packet( m_client, &packetBytes, &packetSequence );
            if ( !packetData )
                break;
            reliable_endpoint_receive_packet( GetEndpoint(), packetData, packetBytes );
            netcode_client_free_packet( m_client, packetData );
        }
    }

    void Client::AdvanceTime( double time )
    {
        BaseClient::AdvanceTime( time );
        if ( m_client )
        {
            netcode_client_update( m_client, time );
            const int state = netcode_client_state( m_client );
            if ( state < NETCODE_CLIENT_STATE_DISCONNECTED )
            {
                Disconnect();
                SetClientState( CLIENT_STATE_ERROR );
            }
            else if ( state == NETCODE_CLIENT_STATE_DISCONNECTED )
            {
                Disconnect();
                SetClientState( CLIENT_STATE_DISCONNECTED );
            }
            else if ( state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST || state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE )
            {
                SetClientState( CLIENT_STATE_CONNECTING );
            }
            else
            {
                SetClientState( CLIENT_STATE_CONNECTED );
            }
            NetworkSimulator * networkSimulator = GetNetworkSimulator();
            if ( networkSimulator && networkSimulator->IsActive() )
            {
                uint8_t ** packetData = (uint8_t**) alloca( sizeof( uint8_t*) * m_config.maxSimulatorPackets );
                int * packetBytes = (int*) alloca( sizeof(int) * m_config.maxSimulatorPackets );
                int numPackets = networkSimulator->ReceivePackets( m_config.maxSimulatorPackets, packetData, packetBytes, NULL );
                for ( int i = 0; i < numPackets; ++i )
                {
                    netcode_client_send_packet( m_client, (uint8_t*) packetData[i], packetBytes[i] );
                    YOJIMBO_FREE( networkSimulator->GetAllocator(), packetData[i] );
                }
            }
        }
    }

    int Client::GetClientIndex() const
    {
        return m_client ? netcode_client_index( m_client ) : -1;
    }

    void Client::ConnectLoopback( int clientIndex, uint64_t clientId, int maxClients )
    {
        Disconnect();
        CreateInternal();
        m_clientId = clientId;
        CreateClient( m_address );
        netcode_client_connect_loopback( m_client, clientIndex, maxClients );
        SetClientState( CLIENT_STATE_CONNECTED );
    }

    void Client::DisconnectLoopback()
    {
        netcode_client_disconnect_loopback( m_client );
        BaseClient::Disconnect();
        DestroyClient();
        DestroyInternal();
        m_clientId = 0;
    }

    bool Client::IsLoopback() const
    {
        return netcode_client_loopback( m_client ) != 0;
    }

    void Client::ProcessLoopbackPacket( const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        netcode_client_process_loopback_packet( m_client, packetData, packetBytes, packetSequence );
    }

    void Client::CreateClient( const Address & address )
    {
        DestroyClient();
        char addressString[MaxAddressLength];
        address.ToString( addressString, MaxAddressLength );

        struct netcode_client_config_t netcodeConfig;
        netcode_default_client_config(&netcodeConfig);
        netcodeConfig.allocator_context             = &GetClientAllocator();
        netcodeConfig.allocate_function             = StaticAllocateFunction;
        netcodeConfig.free_function                 = StaticFreeFunction;
        netcodeConfig.callback_context              = this;
        netcodeConfig.state_change_callback         = StaticStateChangeCallbackFunction;
        netcodeConfig.send_loopback_packet_callback = StaticSendLoopbackPacketCallbackFunction;
        m_client = netcode_client_create(addressString, &netcodeConfig, GetTime());
        
        if ( m_client )
        {
            m_boundAddress.SetPort( netcode_client_get_port( m_client ) );
        }
    }

    void Client::DestroyClient()
    {
        if ( m_client )
        {
            m_boundAddress = m_address;
            netcode_client_destroy( m_client );
            m_client = NULL;
        }
    }

    void Client::StateChangeCallbackFunction( int previous, int current )
    {
        (void) previous;
        (void) current;
    }

    void Client::StaticStateChangeCallbackFunction( void * context, int previous, int current )
    {
        Client * client = (Client*) context;
        client->StateChangeCallbackFunction( previous, current );
    }

    void Client::TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        (void) packetSequence;
        NetworkSimulator * networkSimulator = GetNetworkSimulator();
        if ( networkSimulator && networkSimulator->IsActive() )
        {
            networkSimulator->SendPacket( 0, packetData, packetBytes );
        }
        else
        {
            netcode_client_send_packet( m_client, packetData, packetBytes );
        }
    }

    int Client::ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes )
    {
        return (int) GetConnection().ProcessPacket( GetContext(), packetSequence, packetData, packetBytes );
    }

    void Client::SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        GetAdapter().ClientSendLoopbackPacket( clientIndex, packetData, packetBytes, packetSequence );
    }

    void Client::StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence )
    {
        Client * client = (Client*) context;
        client->SendLoopbackPacketCallbackFunction( clientIndex, packetData, packetBytes, packetSequence );
    }
}
