/*
 *  https://github.com/eidheim/Simple-Web-Server/
 *
 *  The MIT License (MIT)
 *  Copyright (c) 2014-2016 Ole Christian Eidheim
 */

#ifndef SERVER_HTTP_HPP
#define SERVER_HTTP_HPP

#include "base_server.hpp"

namespace SimpleWeb
{

    template<class socket_type>
    class Server : public ServerBase<socket_type> {};

    typedef boost::asio::ip::tcp::socket HTTP;

    template<>
    class Server<HTTP> : public ServerBase<HTTP>
    {
    public:
        Server() : ServerBase<HTTP>::ServerBase(80)
        {}

    protected:
        virtual void accept()
        {
            //Create new socket for this connection
            //Shared_ptr is used to pass temporary objects to the asynchronous functions
            auto socket = std::make_shared<HTTP>(*io_service);

            acceptor->async_accept(*socket, [this, socket](const boost::system::error_code &ec)
            {
                //Immediately start accepting a new connection (if io_service hasn't been stopped)
                if (ec != boost::asio::error::operation_aborted)
                    accept();

                if (!ec)
                {
                    boost::asio::ip::tcp::no_delay option(true);
                    socket->set_option(option);

                    this->read_request_and_content(socket);
                }
                else if (on_error)
                    on_error(std::shared_ptr<Request>(new Request(*socket)), ec);
            });
        }
    };
}

#endif //SERVER_HTTP_HPP
