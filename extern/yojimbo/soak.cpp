/*
    Yojimbo Soak Test.

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

#include "shared.h"
#include <signal.h>

const int MaxPacketSize = 16 * 1024;
const int MaxSnapshotSize = 8 * 1024;
const int MaxBlockSize = 10 * 1024;

static volatile int quit = 0;

void interrupt_handler( int /*dummy*/ )
{
    quit = 1;
}

static const int UNRELIABLE_UNORDERED_CHANNEL = 0;
static const int RELIABLE_ORDERED_CHANNEL = 1;

int SoakMain()
{
    srand( (unsigned int) time( NULL ) );

    ClientServerConfig config;
    config.maxPacketSize = MaxPacketSize;
    config.clientMemory = 100 * 1024 * 1024;
    config.serverGlobalMemory = 10 * 1024 * 1024;
    config.serverPerClientMemory = 100 * 1024 * 1024;
    config.numChannels = 2;
    config.channel[UNRELIABLE_UNORDERED_CHANNEL].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    config.channel[UNRELIABLE_UNORDERED_CHANNEL].maxBlockSize = MaxSnapshotSize;
    config.channel[RELIABLE_ORDERED_CHANNEL].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    config.channel[RELIABLE_ORDERED_CHANNEL].maxBlockSize = MaxBlockSize;
    config.channel[RELIABLE_ORDERED_CHANNEL].blockFragmentSize = 1024;

    uint8_t privateKey[KeyBytes];
    memset( privateKey, 0, KeyBytes );

    double time = 0.0;

    Address serverAddress( "127.0.0.1", ServerPort );

    Server server( GetDefaultAllocator(), privateKey, serverAddress, config, adapter, time );

    server.SetLatency( 1000.0f );
    server.SetJitter( 100.0f );
    server.SetPacketLoss( 25.0f );
    server.SetDuplicates( 25.0f );

    server.Start( 1 );

    uint64_t clientId = 0;
    yojimbo_random_bytes( (uint8_t*) &clientId, 8 );

    Client client( GetDefaultAllocator(), Address("0.0.0.0"), config, adapter, time );

    client.SetLatency( 1000.0f );
    client.SetJitter( 100.0f );
    client.SetPacketLoss( 25.0f );
    client.SetDuplicates( 25.0f );

    client.InsecureConnect( privateKey, clientId, serverAddress );

    uint64_t numMessagesSentToServer = 0;
    uint64_t numMessagesSentToClient = 0;
    uint64_t numMessagesReceivedFromClient = 0;
    uint64_t numMessagesReceivedFromServer = 0;

    signal( SIGINT, interrupt_handler );

    bool clientConnected = false;
    bool serverConnected = false;

    double timeForNextClientMessage = 0;
    double timeForNextServerMessage = 0;

    while ( !quit )
    {
        client.SendPackets();
        server.SendPackets();

        client.ReceivePackets();
        server.ReceivePackets();

        if ( client.ConnectionFailed() )
        {
            printf( "error: client connect failed!\n" );
            break;
        }

        time += 0.1f;

        if ( client.IsConnected() )
        {
            if ( ( rand() % 100000 ) == 0 )
            {
                printf( "client reconnect\n" );
                client.Disconnect();
                client.InsecureConnect( privateKey, clientId, serverAddress );
                clientConnected = false;
                numMessagesSentToServer = 0;
                numMessagesSentToClient = 0;
                numMessagesReceivedFromClient = 0;
                numMessagesReceivedFromServer = 0;
            }
        }

        if ( client.IsConnected() )
        {
            clientConnected = true;

            if ( timeForNextClientMessage < time && ( rand() % 1000 ) == 0 )
            {
                timeForNextClientMessage = time + yojimbo_random_int( 1, 1000 );
            }

            if ( timeForNextClientMessage <= time )
			{
				const int messagesToSend = yojimbo_random_int( 0, 64 );

				for ( int i = 0; i < messagesToSend; ++i )
				{
					if ( !client.CanSendMessage( RELIABLE_ORDERED_CHANNEL ) )
						break;

					if ( rand() % 100 )
					{
						TestMessage * message = (TestMessage*) client.CreateMessage( TEST_MESSAGE );
						if ( message )
						{
							message->sequence = (uint16_t) numMessagesSentToServer;
							client.SendMessage( RELIABLE_ORDERED_CHANNEL, message );
							numMessagesSentToServer++;
						}
					}
					else
					{
                        int numBlocks = yojimbo_random_int( 1, 3 );

                        for ( int k = 0; k < numBlocks; k++ )
                        {
                            if ( !client.CanSendMessage( RELIABLE_ORDERED_CHANNEL ) )
                                break;

    						TestBlockMessage * blockMessage = (TestBlockMessage*) client.CreateMessage( TEST_BLOCK_MESSAGE );
    						if ( blockMessage )
    						{
    							blockMessage->sequence = (uint16_t) numMessagesSentToServer;
    							const int blockSize = 1 + ( int( numMessagesSentToServer ) * 33 ) % MaxBlockSize;
    							uint8_t * blockData = client.AllocateBlock( blockSize );
    							if ( blockData )
    							{
    								for ( int j = 0; j < blockSize; ++j )
    									blockData[j] = uint8_t( numMessagesSentToServer + j );
    								client.AttachBlockToMessage( blockMessage, blockData, blockSize );
    								client.SendMessage( RELIABLE_ORDERED_CHANNEL, blockMessage );
    								numMessagesSentToServer++;
    							}
    							else
    							{
    								client.ReleaseMessage( blockMessage );
    							}
    						}
    					}
    				}
                }
			}

            const int clientIndex = client.GetClientIndex();

            if ( timeForNextServerMessage < time && ( rand() % 1000 ) == 0 )
            {
                int delay = yojimbo_random_int( 1, 1000 );
                
                timeForNextServerMessage = time + delay;
            }

            if ( server.IsClientConnected( clientIndex ) )
            {
                serverConnected = true;

                if ( timeForNextServerMessage <= time )
                {
                    const int messagesToSend = yojimbo_random_int( 0, 64 );

                    for ( int i = 0; i < messagesToSend; ++i )
                    {
                        if ( !server.CanSendMessage( clientIndex, RELIABLE_ORDERED_CHANNEL ) )
                            break;

                        if ( rand() % 100 )
                        {
                            TestMessage * message = (TestMessage*) server.CreateMessage( clientIndex, TEST_MESSAGE );
                            if ( message )
                            {
                                message->sequence = (uint16_t) numMessagesSentToClient;
                                server.SendMessage( clientIndex, RELIABLE_ORDERED_CHANNEL, message );
                                numMessagesSentToClient++;
                            }
                        }
                        else
                        {
                            int numBlocks = yojimbo_random_int( 1, 3 );

                            for ( int k = 0; k < numBlocks; k++ )
                            {
                                if ( !server.CanSendMessage( clientIndex, RELIABLE_ORDERED_CHANNEL ) )
                                    break;

                                TestBlockMessage * blockMessage = (TestBlockMessage*) server.CreateMessage( clientIndex, TEST_BLOCK_MESSAGE );
                                if ( blockMessage )
                                {
                                    blockMessage->sequence = (uint16_t) numMessagesSentToClient;
                                    const int blockSize = 1 + ( int( numMessagesSentToClient ) * 33 ) % MaxBlockSize;
                                    uint8_t * blockData = server.AllocateBlock( clientIndex, blockSize );
                                    if ( blockData )
                                    {
                                        for ( int j = 0; j < blockSize; ++j )
                                            blockData[j] = uint8_t( numMessagesSentToClient + j );
                                        server.AttachBlockToMessage( clientIndex, blockMessage, blockData, blockSize );
                                        server.SendMessage( clientIndex, RELIABLE_ORDERED_CHANNEL, blockMessage );
                                        numMessagesSentToClient++;
                                    }
                                    else
                                    {
                                        server.ReleaseMessage( clientIndex, blockMessage );
                                    }
                                }
                            }
                        }
                    }
                }

                while ( true )
                {
                    Message * message = server.ReceiveMessage( clientIndex, RELIABLE_ORDERED_CHANNEL );
                    if ( !message )
                        break;

                    yojimbo_assert( message->GetId() == (uint16_t) numMessagesReceivedFromClient );

                    switch ( message->GetType() )
                    {
                        case TEST_MESSAGE:
                        {
                            TestMessage * testMessage = (TestMessage*) message;
                            yojimbo_assert( testMessage->sequence == uint16_t( numMessagesReceivedFromClient ) );
                            printf( "server received message %d\n", testMessage->sequence );
                            server.ReleaseMessage( clientIndex, message );
                            numMessagesReceivedFromClient++;
                        }
                        break;

                        case TEST_BLOCK_MESSAGE:
                        {
                            TestBlockMessage * blockMessage = (TestBlockMessage*) message;
                            yojimbo_assert( blockMessage->sequence == uint16_t( numMessagesReceivedFromClient ) );
                            const int blockSize = blockMessage->GetBlockSize();
                            const int expectedBlockSize = 1 + ( int( numMessagesReceivedFromClient ) * 33 ) % MaxBlockSize;
                            if ( blockSize  != expectedBlockSize )
                            {
                                printf( "error: block size mismatch. expected %d, got %d\n", expectedBlockSize, blockSize );
                                return 1;
                            }
                            const uint8_t * blockData = blockMessage->GetBlockData();
                            yojimbo_assert( blockData );
                            for ( int i = 0; i < blockSize; ++i )
                            {
                                if ( blockData[i] != uint8_t( numMessagesReceivedFromClient + i ) )
                                {
                                    printf( "error: block data mismatch. expected %d, but blockData[%d] = %d\n", uint8_t( numMessagesReceivedFromClient + i ), i, blockData[i] );
                                    return 1;
                                }
                            }
                            printf( "server received message %d\n", uint16_t( numMessagesReceivedFromClient ) );
                            server.ReleaseMessage( clientIndex, message );
                            numMessagesReceivedFromClient++;
                        }
                        break;
                    }
                }
            }

            while ( true )
            {
                Message * message = client.ReceiveMessage( RELIABLE_ORDERED_CHANNEL );

                if ( !message )
                    break;

                yojimbo_assert( message->GetId() == (uint16_t) numMessagesReceivedFromServer );

                switch ( message->GetType() )
                {
                    case TEST_MESSAGE:
                    {
                        TestMessage * testMessage = (TestMessage*) message;
                        yojimbo_assert( testMessage->sequence == uint16_t( numMessagesReceivedFromServer ) );
                        printf( "client received message %d\n", testMessage->sequence );
                        client.ReleaseMessage( message );
                        numMessagesReceivedFromServer++;
                    }
                    break;

                    case TEST_BLOCK_MESSAGE:
                    {
                        TestBlockMessage * blockMessage = (TestBlockMessage*) message;
                        yojimbo_assert( blockMessage->sequence == uint16_t( numMessagesReceivedFromServer ) );
                        const int blockSize = blockMessage->GetBlockSize();
                        const int expectedBlockSize = 1 + ( int( numMessagesReceivedFromServer ) * 33 ) % MaxBlockSize;
                        if ( blockSize  != expectedBlockSize )
                        {
                            printf( "error: block size mismatch. expected %d, got %d\n", expectedBlockSize, blockSize );
                            return 1;
                        }
                        const uint8_t * blockData = blockMessage->GetBlockData();
                        yojimbo_assert( blockData );
                        for ( int i = 0; i < blockSize; ++i )
                        {
                            if ( blockData[i] != uint8_t( numMessagesReceivedFromServer + i ) )
                            {
                                printf( "error: block data mismatch. expected %d, but blockData[%d] = %d\n", uint8_t( numMessagesReceivedFromServer + i ), i, blockData[i] );
                                return 1;
                            }
                        }
                        printf( "client received message %d\n", uint16_t( numMessagesReceivedFromServer ) );
                        client.ReleaseMessage( message );
                        numMessagesReceivedFromServer++;
                    }
                    break;
                }
            }

            if ( clientConnected && !client.IsConnected() )
                break;

            if ( serverConnected && server.GetNumConnectedClients() == 0 )
                break;
        }

        client.AdvanceTime( time );
        server.AdvanceTime( time );
    }

    if ( quit )
    {
        printf( "\nstopped\n" );
    }

    client.Disconnect();
    
    server.Stop();

    return 0;
}

int main()
{
    printf( "\nsoak\n" );

    if ( !InitializeYojimbo() )
    {
        printf( "error: failed to initialize Yojimbo!\n" );
        return 1;
    }

    yojimbo_log_level( YOJIMBO_LOG_LEVEL_INFO );

    srand( (unsigned int) time( NULL ) );

    int result = SoakMain();

    ShutdownYojimbo();

    printf( "\n" );

    return result;
}
