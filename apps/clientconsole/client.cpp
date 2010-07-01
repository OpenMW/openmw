#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

#pragma warning( disable : 4966 )

class Client
{
protected:
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
        struct Header
        {
            char            magic[4];
            boost::uint32_t dataLength;
        };
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
            std::cout << "> ";
            char buffer[1024];
            gets(buffer);

            if (std::string(buffer) != "quit")
                bDone = !client.send(buffer);
            else
                bDone = true;
        } while (!bDone);

        client.disconnect();
    }
    else
        std::cout << "failed." << std::endl;
    
    return 0;
}
