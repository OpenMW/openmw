#include "server.hpp"
#include "libs/platform/strings.h"

using boost::asio::ip::tcp;

//
// Namespace for containing implementation details that the
// rest of OpenMW doesn't need to worry about
//
namespace OMW { namespace CommandServer { namespace Detail {

    struct Header
    {
        char   magic[4];
        size_t dataLength;
    } header;

    ///
    /// Tracks an active connection to the CommandServer
    ///
    class Connection
    {      
    public:
        Connection (boost::asio::io_service& io_service, Server* pServer);

        void         start();     
        void         stop();

        tcp::socket& socket();
        void         reply (std::string s);

    protected:
        void handle ();

        tcp::socket     mSocket;
        Server*         mpServer;
        boost::thread*  mpThread;
    };

    Connection::Connection (boost::asio::io_service& io_service, Server* pServer)
        : mSocket  (io_service)
        , mpServer (pServer)
    {
    }

    void Connection::start()
    {
        mpThread = new boost::thread(boost::bind(&Connection::handle, this));
    }
    
    ///
    /// Stops and disconnects the connection
    ///
    void Connection::stop()
    {
        mSocket.shutdown(boost::asio::socket_base::shutdown_both);
        mSocket.close();
        mpThread->join();
    }

    tcp::socket& Connection::socket()
    {
        return mSocket;
    }

    void Connection::reply (std::string reply)
    {
        const size_t plen = sizeof(Header) + reply.length() + 1;
        
        std::vector<char> packet(plen);
        Header* pHeader = reinterpret_cast<Header*>(&packet[0]);
        strncpy(pHeader->magic, "OMW0", 4);
        pHeader->dataLength = reply.length() + 1;   // Include the null terminator
        strncpy(&packet[8], reply.c_str(), pHeader->dataLength); 

        boost::system::error_code ec;
        boost::asio::write(mSocket, boost::asio::buffer(packet), 
                           boost::asio::transfer_all(), ec);     
        if (ec)
            std::cout << "Error: " << ec.message() << std::endl;        
    }
       
    void Connection::handle ()
    {
        bool bDone = false;
        while (!bDone)
        {
            // Read the header
            boost::system::error_code error;
            mSocket.read_some(boost::asio::buffer(&header, sizeof(Header)), error);

            if (error != boost::asio::error::eof)
            {
                if (strncmp(header.magic, "OMW0", 4) == 0)
                {
                    std::vector<char> msg;
                    msg.resize(header.dataLength);
                    
                    boost::system::error_code error;
                    mSocket.read_some(boost::asio::buffer(&msg[0], header.dataLength), error);
                    if (!error)
                        mpServer->postCommand(this, &msg[0]);
                    else
                        bDone = true;
                }
                else
                    throw std::runtime_error("Unexpected header!");
            }
            else
                bDone = true;
        }
        mpServer->removeConnection(this);
    }
   
}}}

namespace OMW { namespace CommandServer { 

    using namespace Detail;
       
    Server::Server (Deque* pCommandQueue, const int port)
        : mPort          (port)
        , mAcceptor      (mIOService, tcp::endpoint(tcp::v4(), mPort))
        , mbStopping     (false)
        , mpCommandQueue (pCommandQueue)
    {       
    }

    void Server::start()
    {
        mIOService.run();
        mpThread = new boost::thread(boost::bind(&Server::threadMain, this));
    }

    //
    // Helper function - see Server::stop()
    //
    static void connectAndDisconnect (int port)
    {
        char portString[64];
        snprintf(portString, 64, "%d", port);

        boost::asio::io_service ioService;
        tcp::resolver resolver(ioService);
        tcp::resolver::query query("localhost", portString);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;

        tcp::socket socket(ioService);
        boost::system::error_code error = boost::asio::error::host_not_found;
        while (error && endpoint_iterator != end)
        {
            socket.close();
            socket.connect(*endpoint_iterator++, error);
        }

        socket.close();
        ioService.stop();
    }

    void Server::stop()
    {
        // Boost.Asio doesn't have a way to cancel the blocking accept() call
        // in the listening thread.  Therefore, set an internal flag to let 
        // the server know it's stopping and then unblock it via a do-nothing
        // connect/disconnect.
        mbStopping = true;
        connectAndDisconnect(mPort);

        // (1) Stop accepting new connections
        // (2) Wait for the listener thread to finish
        mAcceptor.close();
        mpThread->join();

        // Now that no new connections are possible, close any existing
        // open connections
        {
            boost::mutex::scoped_lock lock(mConnectionsMutex);
            for (ConnectionSet::iterator it = mConnections.begin(); 
                 it != mConnections.end();
                 ++it)
            {
                (*it)->stop();
            }
        }
    }

    void Server::removeConnection (Connection* ptr)
    {
        // If the server is shutting down (rather the client closing the
        // connection), don't remove the connection from the list: that
        // would corrupt the iterator the server is using to shutdown all
        // clients.
        if (!mbStopping)
        {
            boost::mutex::scoped_lock lock(mConnectionsMutex);
            std::set<Connection*>::iterator it = mConnections.find(ptr);
            if (it != mConnections.end())
                mConnections.erase(it);
        }
        delete ptr;
    }

    void Server::postCommand (Connection* pConnection, const char* s)
    {
        Command cmd;
        cmd.mCommand = s;
        cmd.mReplyFunction = std::bind1st(std::mem_fun(&Connection::reply), pConnection);
        mpCommandQueue->push_back(cmd);
    }

    void Server::threadMain()
    {
        // Loop until accept() fails, which will cause the break statement to be hit
        while (true)
        {
            std::auto_ptr<Connection> spConnection(new Connection(mAcceptor.io_service(), this));
            boost::system::error_code ec;
            mAcceptor.accept(spConnection->socket(), ec);            
            if (!ec && !mbStopping)
            {
                boost::mutex::scoped_lock lock(mConnectionsMutex);
                mConnections.insert(spConnection.get());
                spConnection->start();
                spConnection.release();
            }
            else
                break;
        }
    }

}}
