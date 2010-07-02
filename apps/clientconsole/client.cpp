#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

#pragma warning( disable : 4966 )

class Client
{
protected:
    struct Header
    {
        char            magic[4];
        boost::uint32_t dataLength;
    };

    boost::asio::io_service mIOService;
    tcp::socket* mpSocket;

public:

    bool connect(const char* port)
    {
        tcp::resolver resolver(mIOService);
        tcp::resolver::query query("localhost", port);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;

        mpSocket = new tcp::socket(mIOService);
        boost::system::error_code error = boost::asio::error::host_not_found;
        while (error && endpoint_iterator != end)
        {
            mpSocket->close();
            mpSocket->connect(*endpoint_iterator++, error);
        }

        return (error) ? false : true;
    }
    void disconnect()
    {
        mpSocket->close();
        mIOService.stop();
    }

    bool send (const char* msg)
    {
        const size_t slen = strlen(msg);
        const size_t plen = sizeof(Header) + slen + 1;
        
        std::vector<char> packet(plen);
        Header* pHeader = reinterpret_cast<Header*>(&packet[0]);
        strncpy(pHeader->magic, "OMW0", 4);
        pHeader->dataLength = slen + 1;     // Include the null terminator
        strncpy(&packet[8], msg, pHeader->dataLength); 

        boost::system::error_code ec;
        boost::asio::write(*mpSocket, boost::asio::buffer(packet), 
                           boost::asio::transfer_all(), ec);     
        if (ec)
            std::cout << "Error: " << ec.message() << std::endl;
        
        return !ec;
    }

    bool receive (std::string& reply)
    {
        Header header;
        boost::system::error_code error;
        mpSocket->read_some(boost::asio::buffer(&header, sizeof(Header)), error);

        if (error != boost::asio::error::eof)
        {
            if (strncmp(header.magic, "OMW0", 4) == 0)
            {
                std::vector<char> msg;
                msg.resize(header.dataLength);
                
                boost::system::error_code error;
                mpSocket->read_some(boost::asio::buffer(&msg[0], header.dataLength), error);
                if (!error)
                {
                    reply = &msg[0];
                    return true;
                }
            }
            else
                throw std::exception("Unexpected header!");
        }       
        return false;
    }
};


int main(int argc, char* argv[])
{
    std::cout << "OpenMW client console" << std::endl;
    std::cout << "=====================" << std::endl;
    std::cout << "Type 'quit' to exit." << std::endl;
    std::cout << "Connecting...";

    Client client;
    if (client.connect("27917"))
    {
        std::cout << "success." << std::endl;
        
        bool bDone = false;
        do
        {
            std::cout << "Client> ";
            std::string buffer;
            std::getline(std::cin, buffer);

            if (buffer == "quit")
                bDone = true;
            else
            {
                if (client.send(buffer.c_str()))
                {
                    std::string reply;
                    if (client.receive(reply))
                        std::cout << "Server: " << reply << std::endl;
                    else
                        bDone = true;
                }
                else
                    bDone = true;
            }
                
        } while (!bDone);

        client.disconnect();
    }
    else
        std::cout << "failed." << std::endl;
    
    return 0;
}
