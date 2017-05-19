#ifndef HTTPS_SERVER_HPP
#define HTTPS_SERVER_HPP

#include "base_server.hpp"
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>
#include <algorithm>

namespace SimpleWeb
{
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> HTTPS;

    template<>
    class Server<HTTPS> : public ServerBase<HTTPS>
    {
        std::string session_id_context;
        bool set_session_id_context = false;
    public:
        Server(const std::string &cert_file, const std::string &private_key_file,
               const std::string &verify_file = std::string()) : ServerBase<HTTPS>::ServerBase(443),
                                                                 context(boost::asio::ssl::context::tlsv12)
        {
            context.use_certificate_chain_file(cert_file);
            context.use_private_key_file(private_key_file, boost::asio::ssl::context::pem);

            if (verify_file.size() > 0)
            {
                context.load_verify_file(verify_file);
                context.set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert |
                                        boost::asio::ssl::verify_client_once);
                set_session_id_context = true;
            }
        }

        void start()
        {
            if (set_session_id_context)
            {
                // Creating session_id_context from address:port but reversed due to small SSL_MAX_SSL_SESSION_ID_LENGTH
                session_id_context = std::to_string(config.port) + ':';
                session_id_context.append(config.address.rbegin(), config.address.rend());
                SSL_CTX_set_session_id_context(context.native_handle(),
                                               reinterpret_cast<const unsigned char *>(session_id_context.data()),
                                               std::min<size_t>(session_id_context.size(),
                                                                SSL_MAX_SSL_SESSION_ID_LENGTH));
            }
            ServerBase::start();
        }

    protected:
        boost::asio::ssl::context context;

        virtual void accept()
        {
            //Create new socket for this connection
            //Shared_ptr is used to pass temporary objects to the asynchronous functions
            auto socket = std::make_shared<HTTPS>(*io_service, context);

            acceptor->async_accept((*socket).lowest_layer(), [this, socket](const boost::system::error_code &ec)
            {
                //Immediately start accepting a new connection (if io_service hasn't been stopped)
                if (ec != boost::asio::error::operation_aborted)
                    accept();


                if (!ec)
                {
                    boost::asio::ip::tcp::no_delay option(true);
                    socket->lowest_layer().set_option(option);

                    //Set timeout on the following boost::asio::ssl::stream::async_handshake
                    auto timer = get_timeout_timer(socket, config.timeout_request);
                    socket->async_handshake(boost::asio::ssl::stream_base::server, [this, socket, timer]
                            (const boost::system::error_code &ec)
                    {
                        if (timer)
                            timer->cancel();
                        if (!ec)
                            read_request_and_content(socket);
                        else if (on_error)
                            on_error(std::shared_ptr<Request>(new Request(*socket)), ec);
                    });
                }
                else if (on_error)
                    on_error(std::shared_ptr<Request>(new Request(*socket)), ec);
            });
        }
    };
}

#endif //HTTPS_SERVER_HPP
