#ifndef CONSOLESERVER_H
#define CONSOLESERVER_H

#include <iostream>
#include <string>
#include <deque>
#include <set>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "components/misc/tsdeque.hpp"
#include "components/commandserver/command.hpp"

namespace OMW { namespace CommandServer 
{
    //
    // Forward Declarations
    //
    namespace  Detail 
    {
        class Connection;
    }

    //
    // Server that opens a port to listen for string commands which will be 
    // put into the deque provided in the Server constructor.
    //
    class Server
    { 
    public:
        typedef TsDeque<Command> Deque;

        Server (Deque* pCommandQueue, const int port);

        void start();
        void stop();

    protected:
        friend class Detail::Connection;
        typedef std::set<Detail::Connection*> ConnectionSet;

        void removeConnection (Detail::Connection* ptr);
        void postCommand      (Detail::Connection*, const char* s);

        void threadMain();            

        // Objects used to set up the listening server
        boost::asio::io_service         mIOService;
        boost::asio::ip::tcp::acceptor  mAcceptor;
        boost::thread*                  mpThread;
        bool                            mbStopping;
        
        // Track active connections
        ConnectionSet                   mConnections;
        mutable boost::mutex            mConnectionsMutex;
        
        // Pointer to command queue
        Deque*                          mpCommandQueue;
    };

}}

#endif // CONSOLESERVER_H
