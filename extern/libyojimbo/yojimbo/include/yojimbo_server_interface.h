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

#ifndef YOJIMBO_SERVER_INTERFACE_H
#define YOJIMBO_SERVER_INTERFACE_H

#include "yojimbo_config.h"

struct netcode_address_t;

// fucking windows =p
#ifdef SendMessage
#undef SendMessage
#endif

namespace yojimbo
{
    /**
        The server interface.
     */

    class ServerInterface
    {
    public:

        virtual ~ServerInterface() {}

        /**
            Set the context for reading and writing packets.
            This is optional. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets via Stream::GetContext.
            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time.
            If you do use a context, make sure the same context data is set on client and server, and include a checksum of the context data in the protocol id.
         */

        virtual void SetContext( void * context ) = 0;

        /**
            Start the server and allocate client slots.
            Each client that connects to this server occupies one of the client slots allocated by this function.
            @param maxClients The number of client slots to allocate. Must be in range [1,MaxClients]
            @see Server::Stop
         */

        virtual void Start( int maxClients ) = 0;

        /**
            Stop the server and free client slots.
            Any clients that are connected at the time you call stop will be disconnected.
            When the server is stopped, clients cannot connect to the server.
            @see Server::Start.
         */

        virtual void Stop() = 0;

        /**
            Disconnect the client at the specified client index.
            @param clientIndex The index of the client to disconnect in range [0,maxClients-1], where maxClients is the number of client slots allocated in Server::Start.
            @see Server::IsClientConnected
         */

        virtual void DisconnectClient( int clientIndex ) = 0;

        /**
            Disconnect all clients from the server.
            Client slots remain allocated as per the last call to Server::Start, they are simply made available for new clients to connect.
         */

        virtual void DisconnectAllClients() = 0;

        /**
            Send packets to connected clients.
            This function drives the sending of packets that transmit messages to clients.
         */

        virtual void SendPackets() = 0;

        /**
            Receive packets from connected clients.
            This function drives the procesing of messages included in packets received from connected clients.
         */

        virtual void ReceivePackets() = 0;

        /**
            Advance server time.
            Call this at the end of each frame to advance the server time forward.
            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Is the server running?
            The server is running after you have called Server::Start. It is not running before the first server start, and after you call Server::Stop.
            Clients can only connect to the server while it is running.
            @returns true if the server is currently running.
         */

        virtual bool IsRunning() const = 0;

        /**
            Get the maximum number of clients that can connect to the server.
            Corresponds to the maxClients parameter passed into the last call to Server::Start.
            @returns The maximum number of clients that can connect to the server. In other words, the number of client slots.
         */

        virtual int GetMaxClients() const = 0;

        /**
            Is a client connected to a client slot?
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns True if the client is connected.
         */

        virtual bool IsClientConnected( int clientIndex ) const = 0;

        /**
            Get the unique id of the client
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns The unique id of the client.
         */

        virtual uint64_t GetClientId( int clientIndex ) const = 0;

        /**
            Get the user data of the client.
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns The user data of the client.
         */
        const uint8_t * GetClientUserData( int clientIndex ) const;

        /**
            Get the address of the client
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns The address of the client.
         */

        virtual netcode_address_t * GetClientAddress( int clientIndex ) const = 0;

        /**
            Get the number of clients that are currently connected to the server.
            @returns the number of connected clients.
         */

        virtual int GetNumConnectedClients() const = 0;

        /**
            Gets the current server time.
            @see Server::AdvanceTime
         */

        virtual double GetTime() const = 0;

        /**
            Create a message of the specified type for a specific client.
            @param clientIndex The index of the client this message belongs to. Determines which client heap is used to allocate the message.
            @param type The type of the message to create. The message types corresponds to the message factory created by the adapter set on the server.
         */

        virtual class Message * CreateMessage( int clientIndex, int type ) = 0;

        /**
            Helper function to allocate a data block.
            This is typically used to create blocks of data to attach to block messages. See BlockMessage for details.
            @param clientIndex The index of the client this message belongs to. Determines which client heap is used to allocate the data.
            @param bytes The number of bytes to allocate.
            @returns The pointer to the data block. This must be attached to a message via Client::AttachBlockToMessage, or freed via Client::FreeBlock.
         */

        virtual uint8_t * AllocateBlock( int clientIndex, int bytes ) = 0;

        /**
            Attach data block to message.
            @param clientIndex The index of the client this block belongs to.
            @param message The message to attach the block to. This message must be derived from BlockMessage.
            @param block Pointer to the block of data to attach. Must be created via Client::AllocateBlock.
            @param bytes Length of the block of data in bytes.
         */

        virtual void AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes ) = 0;

        /**
            Free a block of memory.
            @param clientIndex The index of the client this block belongs to.
            @param block The block of memory created by Client::AllocateBlock.
         */

        virtual void FreeBlock( int clientIndex, uint8_t * block ) = 0;

        /**
            Can we send a message to a particular client on a channel?
            @param clientIndex The index of the client to send a message to.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns True if a message can be sent over the channel, false otherwise.
         */

        virtual bool CanSendMessage( int clientIndex, int channelIndex ) const = 0;

        /**
            Send a message to a client over a channel.
            @param clientIndex The index of the client to send a message to.
            @param channelIndex The channel index in range [0,numChannels-1].
            @param message The message to send.
         */

        virtual void SendMessage( int clientIndex, int channelIndex, Message * message ) = 0;

        /**
            Receive a message from a client over a channel.
            @param clientIndex The index of the client to receive messages from.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns The message received, or NULL if no message is available. Make sure to release this message by calling Server::ReleaseMessage.
         */

        virtual Message * ReceiveMessage( int clientIndex, int channelIndex ) = 0;

        /**
            Release a message.
            Call this for messages received by Server::ReceiveMessage.
            @param clientIndex The index of the client that the message belongs to.
            @param message The message to release.
         */

        virtual void ReleaseMessage( int clientIndex, class Message * message ) = 0;

        /**
            Get client network info.
            Call this to receive information about the client network connection, eg. round trip time, packet loss %, # of packets sent and so on.
            @param clientIndex The index of the client.
            @param info The struct to be filled with network info [out].
         */

        virtual void GetNetworkInfo( int clientIndex, struct NetworkInfo & info ) const = 0;

        /**
            Connect a loopback client.
            This allows you to have local clients connected to a server, for example for integrated server or singleplayer.
            @param clientIndex The index of the client.
            @param clientId The unique client id.
            @param userData User data for this client. Optional. Pass NULL if not needed.
         */

        virtual void ConnectLoopbackClient( int clientIndex, uint64_t clientId, const uint8_t * userData ) = 0;

        /**
            Disconnect a loopback client.
            Loopback clients are not disconnected by regular Disconnect or DisconnectAllClient calls. You need to call this function instead.
            @param clientIndex The index of the client to disconnect. Must already be a connected loopback client.
         */

        virtual void DisconnectLoopbackClient( int clientIndex ) = 0;

        /**
            Is this client a loopback client?
            @param clientIndex The client index.
            @returns true if the client is a connected loopback client, false otherwise.
         */

        virtual bool IsLoopbackClient( int clientIndex ) const = 0;

        /**
            Process loopback packet.
            Use this to pass packets from a client directly to the loopback client slot on the server.
            @param clientIndex The client index. Must be an already connected loopback client.
            @param packetData The packet data to process.
            @param packetBytes The number of bytes of packet data.
            @param packetSequence The packet sequence number.
         */

        virtual void ProcessLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence ) = 0;
    };
}

#endif // #ifndef YOJIMBO_SERVER_INTERFACE_H
