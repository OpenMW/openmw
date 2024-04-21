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

#include "yojimbo_address.h"
#include "yojimbo_platform.h"
#include "yojimbo_utils.h"

#if YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_WINDOWS

    #define NOMINMAX
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <ws2ipdef.h>
    #pragma comment( lib, "WS2_32.lib" )

    #ifdef SetPort
    #undef SetPort
    #endif // #ifdef SetPort

#elif YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_MAC || YOJIMBO_PLATFORM == YOJIMBO_PLATFORM_UNIX

    #include <netdb.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <net/if.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    
#else

    #error yojimbo unknown platform!

#endif

#include <memory.h>
#include <string.h>

namespace yojimbo
{
    Address::Address()
    {
        Clear();
    }

    Address::Address( uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port )
        : m_type( ADDRESS_IPV4 )
    {
        
        m_address.ipv4[0] = a;
        m_address.ipv4[1] = b;
        m_address.ipv4[2] = c;
        m_address.ipv4[3] = d;
        m_port = port;
    }

    Address::Address( const uint8_t address[], uint16_t port )
        : m_type( ADDRESS_IPV4 )
    {
        for ( int i = 0; i < 4; ++i )
            m_address.ipv4[i] = address[i];
        m_port = port;
    }

    Address::Address( uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f, uint16_t g, uint16_t h, uint16_t port )
        : m_type( ADDRESS_IPV6 )
    {
        m_address.ipv6[0] = a;
        m_address.ipv6[1] = b;
        m_address.ipv6[2] = c;
        m_address.ipv6[3] = d;
        m_address.ipv6[4] = e;
        m_address.ipv6[5] = f;
        m_address.ipv6[6] = g;
        m_address.ipv6[7] = h;
        m_port = port;
    }

    Address::Address( const uint16_t address[], uint16_t port )
        : m_type( ADDRESS_IPV6 )
    {
        for ( int i = 0; i < 8; ++i )
            m_address.ipv6[i] = address[i];
        m_port = port;
    }

    Address::Address( const char * address )
    {
        Parse( address );
    }

    Address::Address( const char * address, uint16_t port )
    {
        Parse( address );
        m_port = port;
    }

    void Address::Parse( const char * address_in )
    {
        // first try to parse as an IPv6 address:
        // 1. if the first character is '[' then it's probably an ipv6 in form "[addr6]:portnum"
        // 2. otherwise try to parse as raw IPv6 address, parse using inet_pton

        yojimbo_assert( address_in );

        char buffer[MaxAddressLength];
        char * address = buffer;
        yojimbo_copy_string( address, address_in, MaxAddressLength );

        int addressLength = (int) strlen( address );
        m_port = 0;
        if ( address[0] == '[' )
        {
            const int base_index = addressLength - 1;
            for ( int i = 0; i < 6; ++i )                 // note: no need to search past 6 characters as ":65535" is longest port value
            {
                const int index = base_index - i;
                if ( index < 3 )
                    break;
                if ( address[index] == ':' )
                {
                    m_port = uint16_t( atoi( &address[index + 1] ) );
                    address[index-1] = '\0';
                }
            }
            address += 1;
        }
        struct in6_addr sockaddr6;
        if ( inet_pton( AF_INET6, address, &sockaddr6 ) == 1 )
        {
            int i;
            for ( i = 0; i < 8; ++i )
            {
                m_address.ipv6[i] = ntohs( ( (uint16_t*) &sockaddr6 ) [i] );
            }
            m_type = ADDRESS_IPV6;
            return;
        }

        // otherwise it's probably an IPv4 address:
        // 1. look for ":portnum", if found save the portnum and strip it out
        // 2. parse remaining ipv4 address via inet_pton

        addressLength = (int) strlen( address );
        const int base_index = addressLength - 1;
        for ( int i = 0; i < 6; ++i )
        {
            const int index = base_index - i;
            if ( index < 0 )
                break;
            if ( address[index] == ':' )
            {
                m_port = (uint16_t) atoi( &address[index+1] );
                address[index] = '\0';
            }
        }

        struct sockaddr_in sockaddr4;
        if ( inet_pton( AF_INET, address, &sockaddr4.sin_addr ) == 1 )
        {
            m_type = ADDRESS_IPV4;
            m_address.ipv4[3] = (uint8_t) ( ( sockaddr4.sin_addr.s_addr & 0xFF000000 ) >> 24 );
            m_address.ipv4[2] = (uint8_t) ( ( sockaddr4.sin_addr.s_addr & 0x00FF0000 ) >> 16 );
            m_address.ipv4[1] = (uint8_t) ( ( sockaddr4.sin_addr.s_addr & 0x0000FF00 ) >> 8  );
            m_address.ipv4[0] = (uint8_t) ( ( sockaddr4.sin_addr.s_addr & 0x000000FF )       );
        }
        else
        {
            // Not a valid IPv4 address. Set address as invalid.
            Clear();
        }
    }

    void Address::Clear()
    {
        m_type = ADDRESS_NONE;
        memset( &m_address, 0, sizeof( m_address ) );
        m_port = 0;
    }

    const uint8_t * Address::GetAddress4() const
    {
        yojimbo_assert( m_type == ADDRESS_IPV4 );
        return m_address.ipv4;
    }

    const uint16_t * Address::GetAddress6() const
    {
        yojimbo_assert( m_type == ADDRESS_IPV6 );
        return m_address.ipv6;
    }

    void Address::SetPort( uint16_t port )
    {
        m_port = port;
    }

    uint16_t Address::GetPort() const 
    {
        return m_port;
    }

    AddressType Address::GetType() const
    {
        return m_type;
    }

    const char * Address::ToString( char buffer[], int bufferSize ) const
    {
        yojimbo_assert( bufferSize >= MaxAddressLength );

        if ( m_type == ADDRESS_IPV4 )
        {
            const uint8_t a = m_address.ipv4[0];
            const uint8_t b = m_address.ipv4[1];
            const uint8_t c = m_address.ipv4[2];
            const uint8_t d = m_address.ipv4[3];
            if ( m_port != 0 )
                snprintf( buffer, bufferSize, "%d.%d.%d.%d:%d", a, b, c, d, m_port );
            else
                snprintf( buffer, bufferSize, "%d.%d.%d.%d", a, b, c, d );
            return buffer;
        }
        else if ( m_type == ADDRESS_IPV6 )
        {
            if ( m_port == 0 )
            {
                uint16_t address6[8];
                for ( int i = 0; i < 8; ++i )
                    address6[i] = ntohs( ((uint16_t*) &m_address.ipv6)[i] );
                inet_ntop( AF_INET6, address6, buffer, bufferSize );
                return buffer;
            }
            else
            {
                char addressString[INET6_ADDRSTRLEN];
                uint16_t address6[8];
                for ( int i = 0; i < 8; ++i )
                    address6[i] = ntohs( ((uint16_t*) &m_address.ipv6)[i] );
                inet_ntop( AF_INET6, address6, addressString, INET6_ADDRSTRLEN );
                snprintf( buffer, bufferSize, "[%s]:%d", addressString, m_port );
                return buffer;
            }
        }
        else
        {
            snprintf( buffer, bufferSize, "%s", "NONE" );
            return buffer;
        }
    }

    bool Address::IsValid() const
    {
        return m_type != ADDRESS_NONE;
    }

    bool Address::IsLinkLocal() const
    {
        return m_type == ADDRESS_IPV6 && m_address.ipv6[0] == 0xfe80;
    }

    bool Address::IsSiteLocal() const
    {
        return m_type == ADDRESS_IPV6 && m_address.ipv6[0] == 0xfec0;
    }

    bool Address::IsMulticast() const
    {
        return m_type == ADDRESS_IPV6 && m_address.ipv6[0] == 0xff00;
    }

    bool Address::IsLoopback() const
    {
        return ( m_type == ADDRESS_IPV4 && m_address.ipv4[0] == 127 
                                        && m_address.ipv4[1] == 0
                                        && m_address.ipv4[2] == 0
                                        && m_address.ipv4[3] == 1 )
                                            ||
               ( m_type == ADDRESS_IPV6 && m_address.ipv6[0] == 0
                                        && m_address.ipv6[1] == 0
                                        && m_address.ipv6[2] == 0
                                        && m_address.ipv6[3] == 0
                                        && m_address.ipv6[4] == 0
                                        && m_address.ipv6[5] == 0
                                        && m_address.ipv6[6] == 0
                                        && m_address.ipv6[7] == 0x0001 );
    }

    bool Address::IsGlobalUnicast() const
    {
        return m_type == ADDRESS_IPV6 && m_address.ipv6[0] != 0xfe80
                                      && m_address.ipv6[0] != 0xfec0
                                      && m_address.ipv6[0] != 0xff00
                                      && !IsLoopback();
    }

    bool Address::operator ==( const Address & other ) const
    {
        if ( m_type != other.m_type )
            return false;
        if ( m_port != other.m_port )
            return false;
        if ( m_type == ADDRESS_IPV4 && memcmp( m_address.ipv4, other.m_address.ipv4, sizeof( m_address.ipv4 ) ) == 0 )
            return true;
        else if ( m_type == ADDRESS_IPV6 && memcmp( m_address.ipv6, other.m_address.ipv6, sizeof( m_address.ipv6 ) ) == 0 )
            return true;
        else
            return false;
    }

    bool Address::operator !=( const Address & other ) const
    {
        return !( *this == other );
    }
}
