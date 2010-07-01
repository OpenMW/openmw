
#include "server.hpp"

using boost::asio::ip::tcp;

//
// Namespace for containing implementation details that the
// rest of OpenMW doesn't need to worry about
//
namespace OMW { namespace CommandServer { namespace Detail {

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
        mSocket.close();
        mpThread->join();
    }

    tcp::socket& Connection::socket()
    {
        return mSocket;
    }
       
    void Connection::handle ()
    {
        bool bDone = false;
        while (!bDone)
        {
            struct Header
            {
                char   magic[4];
                size_t dataLength;
            } header;

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
                        mpServer->postMessage( &msg[0] );
                    else
                        bDone = true;
                }
                else
                    throw std::exception("Unexpected header!");
            }
            else
                bDone = true;
        }
        mpServer->removeConnection(this);
    }
   
}}}

namespace OMW { namespace CommandServer { 

    using namespace Detail;
       
    Server::Server (Deque* pDeque, const int port)
        : mAcceptor   (mIOService, tcp::endpoint(tcp::v4(), port))
        , mpCommands  (pDeque)
        , mbStopping  (false)
    {       
    }
       

    void Server::start()
    {
        mIOService.run();
        mpThread = new boost::thread(boost::bind(&Server::threadMain, this));
    }

    void Server::stop()
    {
        // (1) Stop accepting new connections
        // (2) Wait for the listener thread to finish
        mAcceptor.close();
        mpThread->join();

        // Now that no new connections are possible, close any existing
        // open connections
        {
            boost::mutex::scoped_lock lock(mConnectionsMutex);
            mbStopping = true;
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

    void Server::postMessage (const char* s)
    {
        mpCommands->push_back(s);
    }

    void Server::threadMain()
    {
        // Loop until accept() fails, which will cause the break statement to be hit
        while (true)
        {
            std::auto_ptr<Connection> spConnection(new Connection(mAcceptor.io_service(), this));
            boost::system::error_code ec;
            mAcceptor.accept(spConnection->socket(), ec);            
            if (!ec)
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
