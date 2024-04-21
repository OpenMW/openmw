/*
    netcode reference implementation

    Copyright © 2017 - 2024, Mas Bandwidth LLC

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

#include "netcode.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>

#ifdef _MSC_VER
#define SODIUM_STATIC
#pragma warning(disable:4996)
#endif // #ifdef _MSC_VER

#include <sodium.h>

#define NETCODE_SOCKET_IPV6         1
#define NETCODE_SOCKET_IPV4         2

#define NETCODE_CONNECT_TOKEN_NONCE_BYTES 24
#define NETCODE_CONNECT_TOKEN_PRIVATE_BYTES 1024
#define NETCODE_CHALLENGE_TOKEN_BYTES 300
#define NETCODE_VERSION_INFO_BYTES 13
#define NETCODE_MAX_PACKET_BYTES 1300
#define NETCODE_MAX_PAYLOAD_BYTES 1200
#define NETCODE_MAX_ADDRESS_STRING_LENGTH 256
#define NETCODE_PACKET_QUEUE_SIZE 256
#define NETCODE_REPLAY_PROTECTION_BUFFER_SIZE 256
#define NETCODE_CLIENT_MAX_RECEIVE_PACKETS 64
#define NETCODE_SERVER_MAX_RECEIVE_PACKETS ( 64 * NETCODE_MAX_CLIENTS )
#define NETCODE_CLIENT_SOCKET_SNDBUF_SIZE ( 256 * 1024 )
#define NETCODE_CLIENT_SOCKET_RCVBUF_SIZE ( 256 * 1024 )
#define NETCODE_SERVER_SOCKET_SNDBUF_SIZE ( 4 * 1024 * 1024 )
#define NETCODE_SERVER_SOCKET_RCVBUF_SIZE ( 4 * 1024 * 1024 )

#define NETCODE_VERSION_INFO ( (uint8_t*) "NETCODE 1.02" )
#define NETCODE_PACKET_SEND_RATE 10.0
#define NETCODE_NUM_DISCONNECT_PACKETS 10

#ifndef NETCODE_ENABLE_TESTS
#define NETCODE_ENABLE_TESTS 0
#endif // #ifndef NETCODE_ENABLE_TESTS

#ifndef NETCODE_ENABLE_LOGGING
#define NETCODE_ENABLE_LOGGING 1
#endif // #ifndef NETCODE_ENABLE_LOGGING

// ------------------------------------------------------------------

static void netcode_default_assert_handler( NETCODE_CONST char * condition, NETCODE_CONST char * function, NETCODE_CONST char * file, int line )
{
    printf( "assert failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
    #if defined( __GNUC__ )
    __builtin_trap();
    #elif defined( _MSC_VER )
    __debugbreak();
    #endif
    exit( 1 );
}

static int log_level;
static int (*printf_function)( NETCODE_CONST char *, ... ) = ( int (*)( NETCODE_CONST char *, ... ) ) printf;
void (*netcode_assert_function)( NETCODE_CONST char *, NETCODE_CONST char *, NETCODE_CONST char * file, int line ) = netcode_default_assert_handler;

void netcode_log_level( int level )
{
    log_level = level;
}

void netcode_set_printf_function( int (*function)( NETCODE_CONST char *, ... ) )
{
    netcode_assert( function );
    printf_function = function;
}

void netcode_set_assert_function( void (*function)( NETCODE_CONST char *, NETCODE_CONST char *, NETCODE_CONST char * file, int line ) )
{
    netcode_assert_function = function;
}

#if NETCODE_ENABLE_LOGGING

void netcode_printf( int level, NETCODE_CONST char * format, ... ) 
{
    if ( level > log_level )
        return;
    va_list args;
    va_start( args, format );
    char buffer[4*1024];
    vsnprintf( buffer, sizeof(buffer), format, args );
    printf_function( "%s", buffer );
    va_end( args );
}

#else // #if NETCODE_ENABLE_LOGGING

void netcode_printf( int level, NETCODE_CONST char * format, ... ) 
{
    (void) level;
    (void) format;
}

#endif // #if NETCODE_ENABLE_LOGGING

void * netcode_default_allocate_function( void * context, size_t bytes )
{
    (void) context;
    return malloc( bytes );
}

void netcode_default_free_function( void * context, void * pointer )
{
    (void) context;
    free( pointer );
}

// ------------------------------------------------------------------

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    #define NOMINMAX
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <ws2ipdef.h>
    #include <iphlpapi.h>
    #pragma comment( lib, "WS2_32.lib" )
    #pragma comment( lib, "IPHLPAPI.lib" )

    #ifdef SetPort
    #undef SetPort
    #endif // #ifdef SetPort

    #include <iphlpapi.h>
    #pragma comment( lib, "IPHLPAPI.lib" )
    
#elif NETCODE_PLATFORM == NETCODE_PLATFORM_MAC || NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX

    #include <netdb.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>

#else

    #error netcode - unknown platform!

#endif

// ----------------------------------------------------------------

#ifdef __MINGW32__
    const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt)
    {
        if ( af == AF_INET )
        {
            struct sockaddr_in in;
            memset(&in, 0, sizeof(in));
            in.sin_family = AF_INET;
            memcpy(&in.sin_addr, src, sizeof(struct in_addr));
            getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in), dst, cnt, NULL, 0, NI_NUMERICHOST);
            return dst;
        } 
        else if ( af == AF_INET6 )
        {
            struct sockaddr_in6 in;
            memset(&in, 0, sizeof(in));
            in.sin6_family = AF_INET6;
            memcpy(&in.sin6_addr, src, sizeof(struct in_addr6));
            getnameinfo((struct sockaddr *)&in, sizeof(struct sockaddr_in6), dst, cnt, NULL, 0, NI_NUMERICHOST);
            return dst;
        }

        return NULL;
    }

    #define NS_INADDRSZ  4
    #define NS_IN6ADDRSZ 16
    #define NS_INT16SZ   2

    int inet_pton4(const char *src, char *dst)
    {
        uint8_t tmp[NS_INADDRSZ], *tp;
        int saw_digit = 0;
        int octets = 0;
        *(tp = tmp) = 0;
        int ch;

        while ((ch = *src++) != '\0')
        {
            if ( ch >= '0' && ch <= '9' )
            {
                uint32_t n = *tp * 10 + (ch - '0');

                if ( saw_digit && *tp == 0 )
                    return 0;
                if ( n > 255 )
                    return 0;
                *tp = n;
                if ( !saw_digit )
                {
                    if ( ++octets > 4 )
                        return 0;
                    saw_digit = 1;
                }
            }
            else if ( ch == '.' && saw_digit )
            {
                if ( octets == 4 )
                    return 0;
                *++tp = 0;
                saw_digit = 0;
            }
            else
                return 0;
        }
        if ( octets < 4 )
            return 0;

        memcpy(dst, tmp, NS_INADDRSZ);

        return 1;
    }

    int inet_pton6(const char *src, char *dst)
    {
        static const char xdigits[] = "0123456789abcdef";
        uint8_t tmp[NS_IN6ADDRSZ];
        uint8_t *tp = (uint8_t*) memset(tmp, '\0', NS_IN6ADDRSZ);
        uint8_t *endp = tp + NS_IN6ADDRSZ;
        uint8_t *colonp = NULL;

        // Leading :: requires some special handling
        if ( *src == ':' )
        {
            if ( *++src != ':' )
                return 0;
        }

        const char *curtok = src;
        int saw_xdigit = 0;
        uint32_t val = 0;
        int ch;

        while ( (ch = tolower(*src++)) != '\0' )
        {
            const char *pch = strchr(xdigits, ch);
            if ( pch != NULL )
            {
                val <<= 4;
                val |= (pch - xdigits);
                if ( val > 0xffff )
                    return 0;
                saw_xdigit = 1;
                continue;
            }
            if ( ch == ':' )
            {
                curtok = src;
                if ( !saw_xdigit )
                {
                    if ( colonp )
                        return 0;
                    colonp = tp;
                    continue;
                }
                else if ( *src == '\0' )
                {
                    return 0;
                }
                if ( tp + NS_INT16SZ > endp )
                    return 0;
                *tp++ = (uint8_t) (val >> 8) & 0xff;
                *tp++ = (uint8_t) val & 0xff;
                saw_xdigit = 0;
                val = 0;
                continue;
            }
            if ( ch == '.' && ((tp + NS_INADDRSZ) <= endp) && inet_pton4(curtok, (char*) tp) > 0 )
            {
                tp += NS_INADDRSZ;
                saw_xdigit = 0;
                break;
            }
            return 0;
        }

        if ( saw_xdigit )
        {
            if ( tp + NS_INT16SZ > endp )
                return 0;
            *tp++ = (uint8_t) (val >> 8) & 0xff;
            *tp++ = (uint8_t) val & 0xff;
        }

        if ( colonp != NULL )
        {
            const int n = tp - colonp;
            if ( tp == endp )
                return 0;
            for (int i = 1; i <= n; i++)
            {
                endp[-i] = colonp[n - i];
                colonp[n - i] = 0;
            }
            tp = endp;
        }
        if ( tp != endp )
            return 0;

        memcpy(dst, tmp, NS_IN6ADDRSZ);

        return 1;
    }

    int inet_pton(int af, const char *src, void *dst)
    {
        switch ( af )
        {
            case AF_INET:
                return inet_pton4(src, (char *) dst);

            case AF_INET6:
                return inet_pton6(src, (char *) dst);

            default:
                return -1;
        }
    }
#endif

// ----------------------------------------------------------------

int netcode_parse_address( NETCODE_CONST char * address_string_in, struct netcode_address_t * address )
{
    netcode_assert( address_string_in );
    netcode_assert( address );

    memset( address, 0, sizeof( struct netcode_address_t ) );

    // first try to parse the string as an IPv6 address:
    // 1. if the first character is '[' then it's probably an ipv6 in form "[addr6]:portnum"
    // 2. otherwise try to parse as a raw IPv6 address using inet_pton

    #define NETCODE_ADDRESS_BUFFER_SAFETY 32

    char buffer[NETCODE_MAX_ADDRESS_STRING_LENGTH + NETCODE_ADDRESS_BUFFER_SAFETY*2];

    char * address_string = buffer + NETCODE_ADDRESS_BUFFER_SAFETY;
    strncpy( address_string, address_string_in, NETCODE_MAX_ADDRESS_STRING_LENGTH - 1 );
    address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH-1] = '\0';

    int address_string_length = (int) strlen( address_string );

    if ( address_string[0] == '[' )
    {
        int base_index = address_string_length - 1;
        
        int i;
        for ( i = 0; i < 6; ++i )         // note: no need to search past 6 characters as ":65535" is longest possible port value
        {
            int index = base_index - i;
            if ( index < 3 )
                return NETCODE_ERROR;
            if ( address_string[index] == ':' )
            {
                address->port = (uint16_t) ( atoi( &address_string[index + 1] ) );
                address_string[index-1] = '\0';
            }
        }
        address_string += 1;
    }

    struct in6_addr sockaddr6;
    if ( inet_pton( AF_INET6, address_string, &sockaddr6 ) == 1 )
    {
        address->type = NETCODE_ADDRESS_IPV6;
        int i;
        for ( i = 0; i < 8; ++i )
        {
            address->data.ipv6[i] = ntohs( ( (uint16_t*) &sockaddr6 ) [i] );
        }
        return NETCODE_OK;
    }

    // otherwise it's probably an IPv4 address:
    // 1. look for ":portnum", if found save the portnum and strip it out
    // 2. parse remaining ipv4 address via inet_pton

    address_string_length = (int) strlen( address_string );
    int base_index = address_string_length - 1;
    int i;
    for ( i = 0; i < 6; ++i )
    {
        int index = base_index - i;
        if ( index < 0 )
            break;
        if ( address_string[index] == ':' )
        {
            address->port = (uint16_t) atoi( &address_string[index+1] );
            address_string[index] = '\0';
        }
    }

    struct sockaddr_in sockaddr4;
    if ( inet_pton( AF_INET, address_string, &sockaddr4.sin_addr ) == 1 )
    {
        address->type = NETCODE_ADDRESS_IPV4;
        address->data.ipv4[3] = (uint8_t) ( ( sockaddr4.sin_addr.s_addr & 0xFF000000 ) >> 24 );
        address->data.ipv4[2] = (uint8_t) ( ( sockaddr4.sin_addr.s_addr & 0x00FF0000 ) >> 16 );
        address->data.ipv4[1] = (uint8_t) ( ( sockaddr4.sin_addr.s_addr & 0x0000FF00 ) >> 8  );
        address->data.ipv4[0] = (uint8_t) ( ( sockaddr4.sin_addr.s_addr & 0x000000FF )       );
        return NETCODE_OK;
    }

    return NETCODE_ERROR;
}

char * netcode_address_to_string( struct netcode_address_t * address, char * buffer )
{
    netcode_assert( address );
    netcode_assert( buffer );

    if ( address->type == NETCODE_ADDRESS_IPV6 )
    {
        if ( address->port == 0 )
        {
            uint16_t ipv6_network_order[8];
            int i;
            for ( i = 0; i < 8; ++i )
                ipv6_network_order[i] = htons( address->data.ipv6[i] );
            inet_ntop( AF_INET6, (void*) ipv6_network_order, buffer, NETCODE_MAX_ADDRESS_STRING_LENGTH );
            return buffer;
        }
        else
        {
            char address_string[INET6_ADDRSTRLEN];
            uint16_t ipv6_network_order[8];
            int i;
            for ( i = 0; i < 8; ++i )
                ipv6_network_order[i] = htons( address->data.ipv6[i] );
            inet_ntop( AF_INET6, (void*) ipv6_network_order, address_string, INET6_ADDRSTRLEN );
            snprintf( buffer, NETCODE_MAX_ADDRESS_STRING_LENGTH, "[%s]:%d", address_string, address->port );
            return buffer;
        }
    }
    else if ( address->type == NETCODE_ADDRESS_IPV4 )
    {
        if ( address->port != 0 )
        {
            snprintf( buffer, NETCODE_MAX_ADDRESS_STRING_LENGTH, "%d.%d.%d.%d:%d", 
                address->data.ipv4[0], 
                address->data.ipv4[1], 
                address->data.ipv4[2], 
                address->data.ipv4[3], 
                address->port );
        }
        else
        {
            snprintf( buffer, NETCODE_MAX_ADDRESS_STRING_LENGTH, "%d.%d.%d.%d", 
                address->data.ipv4[0], 
                address->data.ipv4[1], 
                address->data.ipv4[2], 
                address->data.ipv4[3] );
        }
        return buffer;
    }
    else
    {
        snprintf( buffer, NETCODE_MAX_ADDRESS_STRING_LENGTH, "%s", "NONE" );
        return buffer;
    }
}

int netcode_address_equal( struct netcode_address_t * a, struct netcode_address_t * b )
{
    netcode_assert( a );
    netcode_assert( b );

    if ( a->type != b->type )
        return 0;

    if ( a->port != b->port )
        return 0;

    if ( a->type == NETCODE_ADDRESS_IPV4 )
    {
        int i;
        for ( i = 0; i < 4; ++i )
        {
            if ( a->data.ipv4[i] != b->data.ipv4[i] )
                return 0;
        }
    }
    else if ( a->type == NETCODE_ADDRESS_IPV6 )
    {
        int i;
        for ( i = 0; i < 8; ++i )
        {
            if ( a->data.ipv6[i] != b->data.ipv6[i] )
                return 0;
        }
    }
    else
    {
        return 0;
    }

    return 1;
}

// ----------------------------------------------------------------

struct netcode_t
{
    int initialized;
};

static struct netcode_t netcode;

int netcode_init()
{
    netcode_assert( !netcode.initialized );

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    WSADATA WsaData;         
    if ( WSAStartup( MAKEWORD(2,2), &WsaData ) != NO_ERROR )
        return NETCODE_ERROR;
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    if ( sodium_init() == -1 )
        return NETCODE_ERROR;

    netcode.initialized = 1;

    return NETCODE_OK;
}

void netcode_term()
{
    netcode_assert( netcode.initialized );

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    WSACleanup();
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    netcode.initialized = 0;
}

// ----------------------------------------------------------------

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
typedef uint32_t netcode_socket_handle_t;
#else // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
typedef size_t netcode_socket_handle_t;
#endif // #if NETCODE_PLATFORM == NETCODe_PLATFORM_WINDOWS

struct netcode_socket_t
{
    struct netcode_address_t address;
    netcode_socket_handle_t handle;
};

struct netcode_socket_holder_t
{
    struct netcode_socket_t ipv4;
    struct netcode_socket_t ipv6;
};

#define NETCODE_SOCKET_ERROR_NONE                               0
#define NETCODE_SOCKET_ERROR_CREATE_FAILED                      1
#define NETCODE_SOCKET_ERROR_SET_NON_BLOCKING_FAILED            2
#define NETCODE_SOCKET_ERROR_SOCKOPT_IPV6_ONLY_FAILED           3
#define NETCODE_SOCKET_ERROR_SOCKOPT_RCVBUF_FAILED              4
#define NETCODE_SOCKET_ERROR_SOCKOPT_SNDBUF_FAILED              5
#define NETCODE_SOCKET_ERROR_BIND_IPV4_FAILED                   6
#define NETCODE_SOCKET_ERROR_BIND_IPV6_FAILED                   7
#define NETCODE_SOCKET_ERROR_GET_SOCKNAME_IPV4_FAILED           8
#define NETCODE_SOCKET_ERROR_GET_SOCKNAME_IPV6_FAILED           7

void netcode_socket_destroy( struct netcode_socket_t * socket )
{
    netcode_assert( socket );
    netcode_assert( netcode.initialized );

    if ( socket->handle != 0 )
    {
        #if NETCODE_PLATFORM == NETCODE_PLATFORM_MAC || NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX
        close( socket->handle );
        #elif NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
        closesocket( socket->handle );
        #else
        #error unsupported platform
        #endif
        socket->handle = 0;
    }
}

int netcode_socket_create( struct netcode_socket_t * s, struct netcode_address_t * address, int send_buffer_size, int receive_buffer_size )
{
    netcode_assert( s );
    netcode_assert( address );
    netcode_assert( netcode.initialized );

    netcode_assert( address->type != NETCODE_ADDRESS_NONE );

    s->address = *address;

    // create socket

    s->handle = socket( ( address->type == NETCODE_ADDRESS_IPV6 ) ? AF_INET6 : AF_INET, SOCK_DGRAM, IPPROTO_UDP );

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    if ( s->handle == INVALID_SOCKET )
#else // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    if ( s->handle <= 0 )
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to create socket\n" );
        return NETCODE_SOCKET_ERROR_CREATE_FAILED;
    }

    // IMPORTANT: tell windows we don't want to receive any connection reset messages
    // for this socket, otherwise recvfrom errors out when client sockets disconnect hard
    // in response to ICMP messages.
#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    #define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    WSAIoctl( s->handle, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL );
#endif

    // force IPv6 only if necessary

    if ( address->type == NETCODE_ADDRESS_IPV6 )
    {
        int yes = 1;
        if ( setsockopt( s->handle, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&yes, sizeof(yes) ) != 0 )
        {
            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to set socket ipv6 only\n" );
            netcode_socket_destroy( s );
            return NETCODE_SOCKET_ERROR_SOCKOPT_IPV6_ONLY_FAILED;
        }
    }

    // increase socket send and receive buffer sizes

    if ( setsockopt( s->handle, SOL_SOCKET, SO_SNDBUF, (char*)&send_buffer_size, sizeof(int) ) != 0 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to set socket send buffer size\n" );
        netcode_socket_destroy( s );
        return NETCODE_SOCKET_ERROR_SOCKOPT_SNDBUF_FAILED;
    }

    if ( setsockopt( s->handle, SOL_SOCKET, SO_RCVBUF, (char*)&receive_buffer_size, sizeof(int) ) != 0 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to set socket receive buffer size\n" );
        netcode_socket_destroy( s );
        return NETCODE_SOCKET_ERROR_SOCKOPT_RCVBUF_FAILED;
    }

    // bind to port

    if ( address->type == NETCODE_ADDRESS_IPV6 )
    {
        struct sockaddr_in6 socket_address;
        memset( &socket_address, 0, sizeof( struct sockaddr_in6 ) );
        socket_address.sin6_family = AF_INET6;
        int i;
        for ( i = 0; i < 8; ++i )
        {
            ( (uint16_t*) &socket_address.sin6_addr ) [i] = htons( address->data.ipv6[i] );
        }
        socket_address.sin6_port = htons( address->port );

        if ( bind( s->handle, (struct sockaddr*) &socket_address, sizeof( socket_address ) ) < 0 )
        {
            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to bind socket (ipv6)\n" );
            netcode_socket_destroy( s );
            return NETCODE_SOCKET_ERROR_BIND_IPV6_FAILED;
        }
    }
    else
    {
        struct sockaddr_in socket_address;
        memset( &socket_address, 0, sizeof( socket_address ) );
        socket_address.sin_family = AF_INET;
        socket_address.sin_addr.s_addr = ( ( (uint32_t) address->data.ipv4[0] ) )       | 
                                         ( ( (uint32_t) address->data.ipv4[1] ) << 8 )  | 
                                         ( ( (uint32_t) address->data.ipv4[2] ) << 16 ) | 
                                         ( ( (uint32_t) address->data.ipv4[3] ) << 24 );
        socket_address.sin_port = htons( address->port );

        if ( bind( s->handle, (struct sockaddr*) &socket_address, sizeof( socket_address ) ) < 0 )
        {
            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to bind socket (ipv4)\n" );
            netcode_socket_destroy( s );
            return NETCODE_SOCKET_ERROR_BIND_IPV4_FAILED;
        }
    }

    // if bound to port 0 find the actual port we got

    if ( address->port == 0 )
    {
        if ( address->type == NETCODE_ADDRESS_IPV6 )
        {
            struct sockaddr_in6 sin;
            socklen_t len = sizeof( sin );
            if ( getsockname( s->handle, (struct sockaddr*)&sin, &len ) == -1 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to get socket port (ipv6)\n" );
                netcode_socket_destroy( s );
                return NETCODE_SOCKET_ERROR_GET_SOCKNAME_IPV6_FAILED;
            }
            s->address.port = ntohs( sin.sin6_port );
        }
        else
        {
            struct sockaddr_in sin;
            socklen_t len = sizeof( sin );
            if ( getsockname( s->handle, (struct sockaddr*)&sin, &len ) == -1 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to get socket port (ipv4)\n" );
                netcode_socket_destroy( s );
                return NETCODE_SOCKET_ERROR_GET_SOCKNAME_IPV4_FAILED;
            }
            s->address.port = ntohs( sin.sin_port );
        }
    }

    // set non-blocking io

#if NETCODE_PLATFORM == NETCODE_PLATFORM_MAC || NETCODE_PLATFORM == NETCODE_PLATFORM_UNIX

    int non_blocking = 1;
    if ( fcntl( s->handle, F_SETFL, O_NONBLOCK, non_blocking ) == -1 )
    {
        netcode_socket_destroy( s );
        return NETCODE_SOCKET_ERROR_SET_NON_BLOCKING_FAILED;
    }

#elif NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    DWORD nonBlocking = 1;
    if ( ioctlsocket( s->handle, FIONBIO, &nonBlocking ) != 0 )
    {
        netcode_socket_destroy( s );
        return NETCODE_SOCKET_ERROR_SET_NON_BLOCKING_FAILED;
    }

#else

    #error unsupported platform

#endif

    return NETCODE_SOCKET_ERROR_NONE;
}

void netcode_socket_send_packet( struct netcode_socket_t * socket, struct netcode_address_t * to, void * packet_data, int packet_bytes )
{
    netcode_assert( socket );
    netcode_assert( socket->handle != 0 );
    netcode_assert( to );
    netcode_assert( to->type == NETCODE_ADDRESS_IPV6 || to->type == NETCODE_ADDRESS_IPV4 );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes > 0 );

    if ( to->type == NETCODE_ADDRESS_IPV6 )
    {
        struct sockaddr_in6 socket_address;
        memset( &socket_address, 0, sizeof( socket_address ) );
        socket_address.sin6_family = AF_INET6;
        int i;
        for ( i = 0; i < 8; ++i )
        {
            ( (uint16_t*) &socket_address.sin6_addr ) [i] = htons( to->data.ipv6[i] );
        }
        socket_address.sin6_port = htons( to->port );
        int result = sendto( socket->handle, (char*) packet_data, packet_bytes, 0, (struct sockaddr*) &socket_address, sizeof( struct sockaddr_in6 ) );
        (void) result;
    }
    else if ( to->type == NETCODE_ADDRESS_IPV4 )
    {
        struct sockaddr_in socket_address;
        memset( &socket_address, 0, sizeof( socket_address ) );
        socket_address.sin_family = AF_INET;
        socket_address.sin_addr.s_addr = ( ( (uint32_t) to->data.ipv4[0] ) )        | 
                                         ( ( (uint32_t) to->data.ipv4[1] ) << 8 )   | 
                                         ( ( (uint32_t) to->data.ipv4[2] ) << 16 )  | 
                                         ( ( (uint32_t) to->data.ipv4[3] ) << 24 );
        socket_address.sin_port = htons( to->port );
        int result = sendto( socket->handle, (NETCODE_CONST char*) packet_data, packet_bytes, 0, (struct sockaddr*) &socket_address, sizeof( struct sockaddr_in ) );
        (void) result;
    }
}

int netcode_socket_receive_packet( struct netcode_socket_t * socket, struct netcode_address_t * from, void * packet_data, int max_packet_size )
{
    netcode_assert( socket );
    netcode_assert( socket->handle != 0 );
    netcode_assert( from );
    netcode_assert( packet_data );
    netcode_assert( max_packet_size > 0 );

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    typedef int socklen_t;
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    
    struct sockaddr_storage sockaddr_from;
    socklen_t from_length = sizeof( sockaddr_from );

    int result = recvfrom( socket->handle, (char*) packet_data, max_packet_size, 0, (struct sockaddr*) &sockaddr_from, &from_length );

#if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    if ( result == SOCKET_ERROR )
    {
        int error = WSAGetLastError();

        if ( error == WSAEWOULDBLOCK )
            return 0;

        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: recvfrom failed with error %d\n", error );

        return 0;
    }
#else // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS
    if ( result <= 0 )
    {
        if ( errno == EAGAIN )
            return 0;

        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: recvfrom failed with error %d\n", errno );

        return 0;
    }
#endif // #if NETCODE_PLATFORM == NETCODE_PLATFORM_WINDOWS

    if ( sockaddr_from.ss_family == AF_INET6 )
    {
        struct sockaddr_in6 * addr_ipv6 = (struct sockaddr_in6*) &sockaddr_from;
        from->type = NETCODE_ADDRESS_IPV6;
        int i;
        for ( i = 0; i < 8; ++i )
        {
            from->data.ipv6[i] = ntohs( ( (uint16_t*) &addr_ipv6->sin6_addr ) [i] );
        }
        from->port = ntohs( addr_ipv6->sin6_port );
    }
    else if ( sockaddr_from.ss_family == AF_INET )
    {
        struct sockaddr_in * addr_ipv4 = (struct sockaddr_in*) &sockaddr_from;
        from->type = NETCODE_ADDRESS_IPV4;
        from->data.ipv4[0] = (uint8_t) ( ( addr_ipv4->sin_addr.s_addr & 0x000000FF ) );
        from->data.ipv4[1] = (uint8_t) ( ( addr_ipv4->sin_addr.s_addr & 0x0000FF00 ) >> 8 );
        from->data.ipv4[2] = (uint8_t) ( ( addr_ipv4->sin_addr.s_addr & 0x00FF0000 ) >> 16 );
        from->data.ipv4[3] = (uint8_t) ( ( addr_ipv4->sin_addr.s_addr & 0xFF000000 ) >> 24 );
        from->port = ntohs( addr_ipv4->sin_port );
    }
    else
    {
        netcode_assert( 0 );
        return 0;
    }
  
    netcode_assert( result >= 0 );

    int bytes_read = result;

    return bytes_read;
}

// ----------------------------------------------------------------

void netcode_write_uint8( uint8_t ** p, uint8_t value )
{
    **p = value;
    ++(*p);
}

void netcode_write_uint16( uint8_t ** p, uint16_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = value >> 8;
    *p += 2;
}

void netcode_write_uint32( uint8_t ** p, uint32_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = ( value >> 8  ) & 0xFF;
    (*p)[2] = ( value >> 16 ) & 0xFF;
    (*p)[3] = value >> 24;
    *p += 4;
}

void netcode_write_uint64( uint8_t ** p, uint64_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = ( value >> 8  ) & 0xFF;
    (*p)[2] = ( value >> 16 ) & 0xFF;
    (*p)[3] = ( value >> 24 ) & 0xFF;
    (*p)[4] = ( value >> 32 ) & 0xFF;
    (*p)[5] = ( value >> 40 ) & 0xFF;
    (*p)[6] = ( value >> 48 ) & 0xFF;
    (*p)[7] = value >> 56;
    *p += 8;
}

void netcode_write_bytes( uint8_t ** p, uint8_t * byte_array, int num_bytes )
{
    int i;
    for ( i = 0; i < num_bytes; ++i )
    {
        netcode_write_uint8( p, byte_array[i] );
    }
}

uint8_t netcode_read_uint8( uint8_t ** p )
{
    uint8_t value = **p;
    ++(*p);
    return value;
}

uint16_t netcode_read_uint16( uint8_t ** p )
{
    uint16_t value;
    value = (*p)[0];
    value |= ( ( (uint16_t)( (*p)[1] ) ) << 8 );
    *p += 2;
    return value;
}

uint32_t netcode_read_uint32( uint8_t ** p )
{
    uint32_t value;
    value  = (*p)[0];
    value |= ( ( (uint32_t)( (*p)[1] ) ) << 8 );
    value |= ( ( (uint32_t)( (*p)[2] ) ) << 16 );
    value |= ( ( (uint32_t)( (*p)[3] ) ) << 24 );
    *p += 4;
    return value;
}

uint64_t netcode_read_uint64( uint8_t ** p )
{
    uint64_t value;
    value  = (*p)[0];
    value |= ( ( (uint64_t)( (*p)[1] ) ) << 8  );
    value |= ( ( (uint64_t)( (*p)[2] ) ) << 16 );
    value |= ( ( (uint64_t)( (*p)[3] ) ) << 24 );
    value |= ( ( (uint64_t)( (*p)[4] ) ) << 32 );
    value |= ( ( (uint64_t)( (*p)[5] ) ) << 40 );
    value |= ( ( (uint64_t)( (*p)[6] ) ) << 48 );
    value |= ( ( (uint64_t)( (*p)[7] ) ) << 56 );
    *p += 8;
    return value;
}

void netcode_read_bytes( uint8_t ** p, uint8_t * byte_array, int num_bytes )
{
    int i;
    for ( i = 0; i < num_bytes; ++i )
    {
        byte_array[i] = netcode_read_uint8( p );
    }
}

// ----------------------------------------------------------------

void netcode_generate_key( uint8_t * key )
{
    netcode_assert( key );
    randombytes_buf( key, NETCODE_KEY_BYTES );
}

void netcode_generate_nonce( uint8_t * nonce )
{
    netcode_assert( nonce );
    randombytes_buf( nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
}

void netcode_random_bytes( uint8_t * data, int bytes )
{
    netcode_assert( data );
    netcode_assert( bytes > 0 );
    randombytes_buf( data, bytes );
}

int netcode_encrypt_aead_bignonce( uint8_t * message, uint64_t message_length, 
                                   uint8_t * additional, uint64_t additional_length,
                                   NETCODE_CONST uint8_t * nonce,
                                   NETCODE_CONST uint8_t * key )
{
    unsigned long long encrypted_length;

    int result = crypto_aead_xchacha20poly1305_ietf_encrypt( message, &encrypted_length,
                                                             message, (unsigned long long) message_length,
                                                             additional, (unsigned long long) additional_length,
                                                             NULL, nonce, key );
    
    if ( result != 0 )
        return NETCODE_ERROR;

    netcode_assert( encrypted_length == message_length + NETCODE_MAC_BYTES );

    return NETCODE_OK;
}

int netcode_decrypt_aead_bignonce( uint8_t * message, uint64_t message_length, 
                                   uint8_t * additional, uint64_t additional_length,
                                   uint8_t * nonce,
                                   uint8_t * key )
{
    unsigned long long decrypted_length;

    int result = crypto_aead_xchacha20poly1305_ietf_decrypt( message, &decrypted_length,
                                                             NULL,
                                                             message, (unsigned long long) message_length,
                                                             additional, (unsigned long long) additional_length,
                                                             nonce, key );

    if ( result != 0 )
        return NETCODE_ERROR;

    netcode_assert( decrypted_length == message_length - NETCODE_MAC_BYTES );

    return NETCODE_OK;
}

int netcode_encrypt_aead( uint8_t * message, uint64_t message_length, 
                          uint8_t * additional, uint64_t additional_length,
                          NETCODE_CONST uint8_t * nonce,
                          NETCODE_CONST uint8_t * key )
{
    unsigned long long encrypted_length;

    int result = crypto_aead_chacha20poly1305_ietf_encrypt( message, &encrypted_length,
                                                            message, (unsigned long long) message_length,
                                                            additional, (unsigned long long) additional_length,
                                                            NULL, nonce, key );
    
    if ( result != 0 )
        return NETCODE_ERROR;

    netcode_assert( encrypted_length == message_length + NETCODE_MAC_BYTES );

    return NETCODE_OK;
}

int netcode_decrypt_aead( uint8_t * message, uint64_t message_length, 
                          uint8_t * additional, uint64_t additional_length,
                          uint8_t * nonce,
                          uint8_t * key )
{
    unsigned long long decrypted_length;

    int result = crypto_aead_chacha20poly1305_ietf_decrypt( message, &decrypted_length,
                                                            NULL,
                                                            message, (unsigned long long) message_length,
                                                            additional, (unsigned long long) additional_length,
                                                            nonce, key );

    if ( result != 0 )
        return NETCODE_ERROR;

    netcode_assert( decrypted_length == message_length - NETCODE_MAC_BYTES );

    return NETCODE_OK;
}

// ----------------------------------------------------------------

struct netcode_connect_token_private_t
{
    uint64_t client_id;
    int timeout_seconds;
    int num_server_addresses;
    struct netcode_address_t server_addresses[NETCODE_MAX_SERVERS_PER_CONNECT];
    uint8_t client_to_server_key[NETCODE_KEY_BYTES];
    uint8_t server_to_client_key[NETCODE_KEY_BYTES];
    uint8_t user_data[NETCODE_USER_DATA_BYTES];
};

void netcode_generate_connect_token_private( struct netcode_connect_token_private_t * connect_token, 
                                             uint64_t client_id, 
                                             int timeout_seconds,
                                             int num_server_addresses, 
                                             struct netcode_address_t * server_addresses, 
                                             uint8_t * user_data )
{
    netcode_assert( connect_token );
    netcode_assert( num_server_addresses > 0 );
    netcode_assert( num_server_addresses <= NETCODE_MAX_SERVERS_PER_CONNECT );
    netcode_assert( server_addresses );
    netcode_assert( user_data );

    connect_token->client_id = client_id;
    connect_token->timeout_seconds = timeout_seconds;
    connect_token->num_server_addresses = num_server_addresses;
    
    int i;
    for ( i = 0; i < num_server_addresses; ++i )
    {
        memcpy( &connect_token->server_addresses[i], &server_addresses[i], sizeof( struct netcode_address_t ) );
    }

    netcode_generate_key( connect_token->client_to_server_key );
    netcode_generate_key( connect_token->server_to_client_key );

    if ( user_data != NULL )
    {
        memcpy( connect_token->user_data, user_data, NETCODE_USER_DATA_BYTES );
    }
    else
    {
        memset( connect_token->user_data, 0, NETCODE_USER_DATA_BYTES );
    }
}

void netcode_write_connect_token_private( struct netcode_connect_token_private_t * connect_token, uint8_t * buffer, int buffer_length )
{
    (void) buffer_length;

    netcode_assert( connect_token );
    netcode_assert( connect_token->num_server_addresses > 0 );
    netcode_assert( connect_token->num_server_addresses <= NETCODE_MAX_SERVERS_PER_CONNECT );
    netcode_assert( buffer );
    netcode_assert( buffer_length >= NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    uint8_t * start = buffer;

    (void) start;

    netcode_write_uint64( &buffer, connect_token->client_id );

    netcode_write_uint32( &buffer, connect_token->timeout_seconds );

    netcode_write_uint32( &buffer, connect_token->num_server_addresses );

    int i,j;

    for ( i = 0; i < connect_token->num_server_addresses; ++i )
    {
        if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV4 )
        {
            netcode_write_uint8( &buffer, NETCODE_ADDRESS_IPV4 );
            for ( j = 0; j < 4; ++j )
            {
                netcode_write_uint8( &buffer, connect_token->server_addresses[i].data.ipv4[j] );
            }
            netcode_write_uint16( &buffer, connect_token->server_addresses[i].port );
        }
        else if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV6 )
        {
            netcode_write_uint8( &buffer, NETCODE_ADDRESS_IPV6 );
            for ( j = 0; j < 8; ++j )
            {
                netcode_write_uint16( &buffer, connect_token->server_addresses[i].data.ipv6[j] );
            }
            netcode_write_uint16( &buffer, connect_token->server_addresses[i].port );
        }
        else
        {
            netcode_assert( 0 );
        }
    }

    netcode_write_bytes( &buffer, connect_token->client_to_server_key, NETCODE_KEY_BYTES );

    netcode_write_bytes( &buffer, connect_token->server_to_client_key, NETCODE_KEY_BYTES );

    netcode_write_bytes( &buffer, connect_token->user_data, NETCODE_USER_DATA_BYTES );

    netcode_assert( buffer - start <= NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - NETCODE_MAC_BYTES );

    memset( buffer, 0, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - ( buffer - start ) );
}

int netcode_encrypt_connect_token_private( uint8_t * buffer, 
                                           int buffer_length, 
                                           uint8_t * version_info, 
                                           uint64_t protocol_id, 
                                           uint64_t expire_timestamp, 
                                           NETCODE_CONST uint8_t * nonce, 
                                           NETCODE_CONST uint8_t * key )
{
    netcode_assert( buffer );
    netcode_assert( buffer_length == NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
    netcode_assert( key );

    (void) buffer_length;

    uint8_t additional_data[NETCODE_VERSION_INFO_BYTES+8+8];
    {
        uint8_t * p = additional_data;
        netcode_write_bytes( &p, version_info, NETCODE_VERSION_INFO_BYTES );
        netcode_write_uint64( &p, protocol_id );
        netcode_write_uint64( &p, expire_timestamp );
    }

    return netcode_encrypt_aead_bignonce( buffer, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - NETCODE_MAC_BYTES, additional_data, sizeof( additional_data ), nonce, key );
}

int netcode_decrypt_connect_token_private( uint8_t * buffer, 
                                           int buffer_length, 
                                           uint8_t * version_info, 
                                           uint64_t protocol_id, 
                                           uint64_t expire_timestamp, 
                                           uint8_t * nonce, 
                                           uint8_t * key )
{
    netcode_assert( buffer );
    netcode_assert( buffer_length == NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
    netcode_assert( key );

    (void) buffer_length;

    uint8_t additional_data[NETCODE_VERSION_INFO_BYTES+8+8];
    {
        uint8_t * p = additional_data;
        netcode_write_bytes( &p, version_info, NETCODE_VERSION_INFO_BYTES );
        netcode_write_uint64( &p, protocol_id );
        netcode_write_uint64( &p, expire_timestamp );
    }
    return netcode_decrypt_aead_bignonce( buffer, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, additional_data, sizeof( additional_data ), nonce, key );
}

int netcode_read_connect_token_private( uint8_t * buffer, int buffer_length, struct netcode_connect_token_private_t * connect_token )
{
    netcode_assert( buffer );
    netcode_assert( connect_token );

    if ( buffer_length < NETCODE_CONNECT_TOKEN_PRIVATE_BYTES )
        return NETCODE_ERROR;
    
    connect_token->client_id = netcode_read_uint64( &buffer );

    connect_token->timeout_seconds = (int) netcode_read_uint32( &buffer );

    connect_token->num_server_addresses = netcode_read_uint32( &buffer );

    if ( connect_token->num_server_addresses <= 0 )
        return NETCODE_ERROR;

    if ( connect_token->num_server_addresses > NETCODE_MAX_SERVERS_PER_CONNECT )
        return NETCODE_ERROR;

    int i,j;

    for ( i = 0; i < connect_token->num_server_addresses; ++i )
    {
        connect_token->server_addresses[i].type = netcode_read_uint8( &buffer );

        if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV4 )
        {
            for ( j = 0; j < 4; ++j )
            {
                connect_token->server_addresses[i].data.ipv4[j] = netcode_read_uint8( &buffer );
            }
            connect_token->server_addresses[i].port = netcode_read_uint16( &buffer );
        }
        else if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV6 )
        {
            for ( j = 0; j < 8; ++j )
            {
                connect_token->server_addresses[i].data.ipv6[j] = netcode_read_uint16( &buffer );
            }
            connect_token->server_addresses[i].port = netcode_read_uint16( &buffer );
        }
        else
        {
            return NETCODE_ERROR;
        }
    }

    netcode_read_bytes( &buffer, connect_token->client_to_server_key, NETCODE_KEY_BYTES );

    netcode_read_bytes( &buffer, connect_token->server_to_client_key, NETCODE_KEY_BYTES );

    netcode_read_bytes( &buffer, connect_token->user_data, NETCODE_USER_DATA_BYTES );

    return NETCODE_OK;
}

// -----------------------------------------------

struct netcode_challenge_token_t
{
    uint64_t client_id;
    uint8_t user_data[NETCODE_USER_DATA_BYTES];
};

void netcode_write_challenge_token( struct netcode_challenge_token_t * challenge_token, uint8_t * buffer, int buffer_length )
{
    (void) buffer_length;

    netcode_assert( challenge_token );
    netcode_assert( buffer );
    netcode_assert( buffer_length >= NETCODE_CHALLENGE_TOKEN_BYTES );

    memset( buffer, 0, NETCODE_CHALLENGE_TOKEN_BYTES );

    uint8_t * start = buffer;

    (void) start;

    netcode_write_uint64( &buffer, challenge_token->client_id );

    netcode_write_bytes( &buffer, challenge_token->user_data, NETCODE_USER_DATA_BYTES ); 

    netcode_assert( buffer - start <= NETCODE_CHALLENGE_TOKEN_BYTES - NETCODE_MAC_BYTES );
}

int netcode_encrypt_challenge_token( uint8_t * buffer, int buffer_length, uint64_t sequence, uint8_t * key )
{
    netcode_assert( buffer );
    netcode_assert( buffer_length >= NETCODE_CHALLENGE_TOKEN_BYTES );
    netcode_assert( key );

    (void) buffer_length;

    uint8_t nonce[12];
    {
        uint8_t * p = nonce;
        netcode_write_uint32( &p, 0 );
        netcode_write_uint64( &p, sequence );
    }

    return netcode_encrypt_aead( buffer, NETCODE_CHALLENGE_TOKEN_BYTES - NETCODE_MAC_BYTES, NULL, 0, nonce, key );
}

int netcode_decrypt_challenge_token( uint8_t * buffer, int buffer_length, uint64_t sequence, uint8_t * key )
{
    netcode_assert( buffer );
    netcode_assert( buffer_length >= NETCODE_CHALLENGE_TOKEN_BYTES );
    netcode_assert( key );

    (void) buffer_length;

    uint8_t nonce[12];
    {
        uint8_t * p = nonce;
        netcode_write_uint32( &p, 0 );
        netcode_write_uint64( &p, sequence );
    }

    return netcode_decrypt_aead( buffer, NETCODE_CHALLENGE_TOKEN_BYTES, NULL, 0, nonce, key );
}

int netcode_read_challenge_token( uint8_t * buffer, int buffer_length, struct netcode_challenge_token_t * challenge_token )
{
    netcode_assert( buffer );
    netcode_assert( challenge_token );

    if ( buffer_length < NETCODE_CHALLENGE_TOKEN_BYTES )
        return NETCODE_ERROR;

    uint8_t * start = buffer;

    (void) start;
    
    challenge_token->client_id = netcode_read_uint64( &buffer );

    netcode_read_bytes( &buffer, challenge_token->user_data, NETCODE_USER_DATA_BYTES );

    netcode_assert( buffer - start == 8 + NETCODE_USER_DATA_BYTES );

    return NETCODE_OK;
}

// ----------------------------------------------------------------

#define NETCODE_CONNECTION_REQUEST_PACKET           0
#define NETCODE_CONNECTION_DENIED_PACKET            1
#define NETCODE_CONNECTION_CHALLENGE_PACKET         2
#define NETCODE_CONNECTION_RESPONSE_PACKET          3
#define NETCODE_CONNECTION_KEEP_ALIVE_PACKET        4
#define NETCODE_CONNECTION_PAYLOAD_PACKET           5
#define NETCODE_CONNECTION_DISCONNECT_PACKET        6
#define NETCODE_CONNECTION_NUM_PACKETS              7

struct netcode_connection_request_packet_t
{
    uint8_t packet_type;
    uint8_t version_info[NETCODE_VERSION_INFO_BYTES];
    uint64_t protocol_id;
    uint64_t connect_token_expire_timestamp;
    uint8_t connect_token_nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    uint8_t connect_token_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
};

struct netcode_connection_denied_packet_t
{
    uint8_t packet_type;
};

struct netcode_connection_challenge_packet_t
{
    uint8_t packet_type;
    uint64_t challenge_token_sequence;
    uint8_t challenge_token_data[NETCODE_CHALLENGE_TOKEN_BYTES];
};

struct netcode_connection_response_packet_t
{
    uint8_t packet_type;
    uint64_t challenge_token_sequence;
    uint8_t challenge_token_data[NETCODE_CHALLENGE_TOKEN_BYTES];
};

struct netcode_connection_keep_alive_packet_t
{
    uint8_t packet_type;
    int client_index;
    int max_clients;
};

struct netcode_connection_payload_packet_t
{
    uint8_t packet_type;
    uint32_t payload_bytes;
    uint8_t payload_data[1];
};

struct netcode_connection_disconnect_packet_t
{
    uint8_t packet_type;
};

struct netcode_connection_payload_packet_t * netcode_create_payload_packet( int payload_bytes, void * allocator_context, void* (*allocate_function)(void*,size_t) )
{
    netcode_assert( payload_bytes >= 0 );
    netcode_assert( payload_bytes <= NETCODE_MAX_PAYLOAD_BYTES );

    if ( allocate_function == NULL )
    {
        allocate_function = netcode_default_allocate_function;
    }

    struct netcode_connection_payload_packet_t * packet = (struct netcode_connection_payload_packet_t*) 
        allocate_function( allocator_context, sizeof( struct netcode_connection_payload_packet_t ) + payload_bytes );

    if ( !packet )
        return NULL;
    
    packet->packet_type = NETCODE_CONNECTION_PAYLOAD_PACKET;
    packet->payload_bytes = payload_bytes;

    return packet;
}

struct netcode_context_t
{
    uint8_t write_packet_key[NETCODE_KEY_BYTES];
    uint8_t read_packet_key[NETCODE_KEY_BYTES];
};

int netcode_sequence_number_bytes_required( uint64_t sequence )
{
    int i;
    uint64_t mask = 0xFF00000000000000UL;
    for ( i = 0; i < 7; ++i )
    {
        if ( sequence & mask )
            break;
        mask >>= 8;
    }
    return 8 - i;
}

int netcode_write_packet( void * packet, uint8_t * buffer, int buffer_length, uint64_t sequence, uint8_t * write_packet_key, uint64_t protocol_id )
{
    netcode_assert( packet );
    netcode_assert( buffer );
    netcode_assert( write_packet_key );

    (void) buffer_length;

    uint8_t packet_type = ((uint8_t*)packet)[0];

    if ( packet_type == NETCODE_CONNECTION_REQUEST_PACKET )
    {
        // connection request packet: first byte is zero

        netcode_assert( buffer_length >= 1 + 13 + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

        struct netcode_connection_request_packet_t * p = (struct netcode_connection_request_packet_t*) packet;

        uint8_t * start = buffer;

        netcode_write_uint8( &buffer, NETCODE_CONNECTION_REQUEST_PACKET );
        netcode_write_bytes( &buffer, p->version_info, NETCODE_VERSION_INFO_BYTES );
        netcode_write_uint64( &buffer, p->protocol_id );
        netcode_write_uint64( &buffer, p->connect_token_expire_timestamp );
        netcode_write_bytes( &buffer, p->connect_token_nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
        netcode_write_bytes( &buffer, p->connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

        netcode_assert( buffer - start == 1 + 13 + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

        return (int) ( buffer - start );
    }
    else
    {
        // *** encrypted packets ***

        // write the prefix byte (this is a combination of the packet type and number of sequence bytes)

        uint8_t * start = buffer;

        uint8_t sequence_bytes = (uint8_t) netcode_sequence_number_bytes_required( sequence );

        netcode_assert( sequence_bytes >= 1 );
        netcode_assert( sequence_bytes <= 8 );

        netcode_assert( packet_type <= 0xF );

        uint8_t prefix_byte = packet_type | ( sequence_bytes << 4 );

        netcode_write_uint8( &buffer, prefix_byte );

        // write the variable length sequence number [1,8] bytes.

        uint64_t sequence_temp = sequence;

        int i;
        for ( i = 0; i < sequence_bytes; ++i )
        {
            netcode_write_uint8( &buffer, (uint8_t) ( sequence_temp & 0xFF ) );
            sequence_temp >>= 8;
        }

        // write packet data according to type. this data will be encrypted.

        uint8_t * encrypted_start = buffer;

        switch ( packet_type )
        {
            case NETCODE_CONNECTION_DENIED_PACKET:
            {
                // ...
            }
            break;

            case NETCODE_CONNECTION_CHALLENGE_PACKET:
            {
                struct netcode_connection_challenge_packet_t * p = (struct netcode_connection_challenge_packet_t*) packet;
                netcode_write_uint64( &buffer, p->challenge_token_sequence );
                netcode_write_bytes( &buffer, p->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
            }
            break;

            case NETCODE_CONNECTION_RESPONSE_PACKET:
            {
                struct netcode_connection_response_packet_t * p = (struct netcode_connection_response_packet_t*) packet;
                netcode_write_uint64( &buffer, p->challenge_token_sequence );
                netcode_write_bytes( &buffer, p->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
            }
            break;

            case NETCODE_CONNECTION_KEEP_ALIVE_PACKET:
            {
                struct netcode_connection_keep_alive_packet_t * p = (struct netcode_connection_keep_alive_packet_t*) packet;
                netcode_write_uint32( &buffer, p->client_index );
                netcode_write_uint32( &buffer, p->max_clients );
            }
            break;

            case NETCODE_CONNECTION_PAYLOAD_PACKET:
            {
                struct netcode_connection_payload_packet_t * p = (struct netcode_connection_payload_packet_t*) packet;

                netcode_assert( p->payload_bytes <= NETCODE_MAX_PAYLOAD_BYTES );

                netcode_write_bytes( &buffer, p->payload_data, p->payload_bytes );
            }
            break;

            case NETCODE_CONNECTION_DISCONNECT_PACKET:
            {
                // ...
            }
            break;

            default:
                netcode_assert( 0 );
        }

        netcode_assert( buffer - start <= buffer_length - NETCODE_MAC_BYTES );

        uint8_t * encrypted_finish = buffer;

        // encrypt the per-packet packet written with the prefix byte, protocol id and version as the associated data. this must match to decrypt.

        uint8_t additional_data[NETCODE_VERSION_INFO_BYTES+8+1];
        {
            uint8_t * p = additional_data;
            netcode_write_bytes( &p, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
            netcode_write_uint64( &p, protocol_id );
            netcode_write_uint8( &p, prefix_byte );
        }

        uint8_t nonce[12];
        {
            uint8_t * p = nonce;
            netcode_write_uint32( &p, 0 );
            netcode_write_uint64( &p, sequence );
        }

        if ( netcode_encrypt_aead( encrypted_start, 
                                   encrypted_finish - encrypted_start, 
                                   additional_data, sizeof( additional_data ), 
                                   nonce, write_packet_key ) != NETCODE_OK )
        {
            return NETCODE_ERROR;
        }

        buffer += NETCODE_MAC_BYTES;

        netcode_assert( buffer - start <= buffer_length );

        return (int) ( buffer - start );
    }
}

struct netcode_replay_protection_t
{
    uint64_t most_recent_sequence;
    uint64_t received_packet[NETCODE_REPLAY_PROTECTION_BUFFER_SIZE];
};

void netcode_replay_protection_reset( struct netcode_replay_protection_t * replay_protection )
{
    netcode_assert( replay_protection );
    replay_protection->most_recent_sequence = 0;
    memset( replay_protection->received_packet, 0xFF, sizeof( replay_protection->received_packet ) );
}

int netcode_replay_protection_already_received( struct netcode_replay_protection_t * replay_protection, uint64_t sequence )
{
    netcode_assert( replay_protection );

    if ( sequence + NETCODE_REPLAY_PROTECTION_BUFFER_SIZE <= replay_protection->most_recent_sequence )
        return 1;
    
    int index = (int) ( sequence % NETCODE_REPLAY_PROTECTION_BUFFER_SIZE );

    if ( replay_protection->received_packet[index] == UINT64_MAX )
        return 0;

    if ( replay_protection->received_packet[index] >= sequence )
        return 1;

    return 0;
}

void netcode_replay_protection_advance_sequence( struct netcode_replay_protection_t * replay_protection, uint64_t sequence )
{
    netcode_assert( replay_protection );

    if ( sequence > replay_protection->most_recent_sequence )
        replay_protection->most_recent_sequence = sequence;

    int index = (int) ( sequence % NETCODE_REPLAY_PROTECTION_BUFFER_SIZE );

    replay_protection->received_packet[index] = sequence;
}

void * netcode_read_packet( uint8_t * buffer, 
                            int buffer_length, 
                            uint64_t * sequence, 
                            uint8_t * read_packet_key, 
                            uint64_t protocol_id, 
                            uint64_t current_timestamp, 
                            uint8_t * private_key, 
                            uint8_t * allowed_packets, 
                            struct netcode_replay_protection_t * replay_protection, 
                            void * allocator_context, 
                            void* (*allocate_function)(void*,size_t) )
{
    netcode_assert( sequence );
    netcode_assert( allowed_packets );

    *sequence = 0;

    if ( allocate_function == NULL )
    {
        allocate_function = netcode_default_allocate_function;
    }

    if ( buffer_length < 1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored packet. buffer length is less than 1\n" );
        return NULL;
    }

    uint8_t * start = buffer;

    uint8_t prefix_byte = netcode_read_uint8( &buffer );

    if ( prefix_byte == NETCODE_CONNECTION_REQUEST_PACKET )
    {
        // connection request packet: first byte is zero

        if ( !allowed_packets[NETCODE_CONNECTION_REQUEST_PACKET] )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. packet type is not allowed\n" );
            return NULL;
        }

        if ( buffer_length != 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. bad packet length (expected %d, got %d)\n", 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + 8 + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, buffer_length );
            return NULL;
        }

        if ( !private_key )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. no private key\n" );
            return NULL;
        }

        uint8_t version_info[NETCODE_VERSION_INFO_BYTES];
        netcode_read_bytes( &buffer, version_info, NETCODE_VERSION_INFO_BYTES );
        if ( version_info[0]  != 'N' || 
             version_info[1]  != 'E' || 
             version_info[2]  != 'T' || 
             version_info[3]  != 'C' || 
             version_info[4]  != 'O' ||
             version_info[5]  != 'D' ||
             version_info[6]  != 'E' ||
             version_info[7]  != ' ' || 
             version_info[8]  != '1' ||
             version_info[9]  != '.' ||
             version_info[10] != '0' ||
             version_info[11] != '2' ||
             version_info[12] != '\0' )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. bad version info\n" );
            return NULL;
        }

        uint64_t packet_protocol_id = netcode_read_uint64( &buffer );
        if ( packet_protocol_id != protocol_id )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. wrong protocol id. expected %.16" PRIx64 ", got %.16" PRIx64 "\n", 
                protocol_id, packet_protocol_id );
            return NULL;
        }

        uint64_t packet_connect_token_expire_timestamp = netcode_read_uint64( &buffer );
        if ( packet_connect_token_expire_timestamp <= current_timestamp )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. connect token expired\n" );
            return NULL;
        }

        uint8_t packet_connect_token_nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
        netcode_read_bytes(&buffer, packet_connect_token_nonce, sizeof(packet_connect_token_nonce));

        netcode_assert( buffer - start == 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES );

        if ( netcode_decrypt_connect_token_private( buffer, 
                                                    NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, 
                                                    version_info, 
                                                    protocol_id, 
                                                    packet_connect_token_expire_timestamp, 
                                                    packet_connect_token_nonce, 
                                                    private_key ) != NETCODE_OK )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. connect token failed to decrypt\n" );
            return NULL;
        }

        struct netcode_connection_request_packet_t * packet = (struct netcode_connection_request_packet_t*) 
            allocate_function( allocator_context, sizeof( struct netcode_connection_request_packet_t ) );

        if ( !packet )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection request packet. failed to allocate packet\n" );
            return NULL;
        }

        packet->packet_type = NETCODE_CONNECTION_REQUEST_PACKET;
        memcpy( packet->version_info, version_info, NETCODE_VERSION_INFO_BYTES );
        packet->protocol_id = packet_protocol_id;
        packet->connect_token_expire_timestamp = packet_connect_token_expire_timestamp;
        memcpy( packet->connect_token_nonce, packet_connect_token_nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
        netcode_read_bytes( &buffer, packet->connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

        netcode_assert( buffer - start == 1 + NETCODE_VERSION_INFO_BYTES + 8 + 8 + NETCODE_CONNECT_TOKEN_NONCE_BYTES + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

        return packet;
    }
    else
    {
        // *** encrypted packets ***

        if ( !read_packet_key )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. no read packet key for this address\n" );
            return NULL;
        }

        if ( buffer_length < 1 + 1 + NETCODE_MAC_BYTES )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. packet is too small to be valid (%d bytes)\n", buffer_length );
            return NULL;
        }

        // extract the packet type and number of sequence bytes from the prefix byte

        int packet_type = prefix_byte & 0xF;

        if ( packet_type >= NETCODE_CONNECTION_NUM_PACKETS )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. packet type %d is invalid\n", packet_type );
            return NULL;
        }

        if ( !allowed_packets[packet_type] )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. packet type %d is not allowed\n", packet_type );
            return NULL;
        }

        int sequence_bytes = prefix_byte >> 4;

        if ( sequence_bytes < 1 || sequence_bytes > 8 )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. sequence bytes %d is out of range [1,8]\n", sequence_bytes );
            return NULL;
        }

        if ( buffer_length < 1 + sequence_bytes + NETCODE_MAC_BYTES )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. buffer is too small for sequence bytes + encryption mac\n" );
            return NULL;
        }

        // read variable length sequence number [1,8]

        int i;
        for ( i = 0; i < sequence_bytes; ++i )
        {
            uint8_t value = netcode_read_uint8( &buffer );
            (*sequence) |= ( uint64_t) ( value ) << ( 8 * i );
        }

        // ignore the packet if it has already been received

        if ( replay_protection && packet_type >= NETCODE_CONNECTION_KEEP_ALIVE_PACKET )
        {
            if ( netcode_replay_protection_already_received( replay_protection, *sequence ) )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored packet. sequence %.16" PRIx64 " already received (replay protection)\n", *sequence );
                return NULL;
            }
        }

        // decrypt the per-packet type data

        uint8_t additional_data[NETCODE_VERSION_INFO_BYTES+8+1];
        {
            uint8_t * p = additional_data;
            netcode_write_bytes( &p, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
            netcode_write_uint64( &p, protocol_id );
            netcode_write_uint8( &p, prefix_byte );
        }

        uint8_t nonce[12];
        {
            uint8_t * p = nonce;
            netcode_write_uint32( &p, 0 );
            netcode_write_uint64( &p, *sequence );
        }

        int encrypted_bytes = (int) ( buffer_length - ( buffer - start ) );

        if ( encrypted_bytes < NETCODE_MAC_BYTES )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. encrypted payload is too small\n" );
            return NULL;
        }

        if ( netcode_decrypt_aead( buffer, encrypted_bytes, additional_data, sizeof( additional_data ), nonce, read_packet_key ) != NETCODE_OK )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored encrypted packet. failed to decrypt\n" );
            return NULL;
        }

        int decrypted_bytes = encrypted_bytes - NETCODE_MAC_BYTES;

        // update the latest replay protection sequence #

        if ( replay_protection && packet_type >= NETCODE_CONNECTION_KEEP_ALIVE_PACKET )
        {
            netcode_replay_protection_advance_sequence( replay_protection, *sequence );
        }

        // process the per-packet type data that was just decrypted
        
        switch ( packet_type )
        {
            case NETCODE_CONNECTION_DENIED_PACKET:
            {
                if ( decrypted_bytes != 0 )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection denied packet. decrypted packet data is wrong size\n" );
                    return NULL;
                }

                struct netcode_connection_denied_packet_t * packet = (struct netcode_connection_denied_packet_t*) 
                    allocate_function( allocator_context, sizeof( struct netcode_connection_denied_packet_t ) );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection denied packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                packet->packet_type = NETCODE_CONNECTION_DENIED_PACKET;
                
                return packet;
            }
            break;

            case NETCODE_CONNECTION_CHALLENGE_PACKET:
            {
                if ( decrypted_bytes != 8 + NETCODE_CHALLENGE_TOKEN_BYTES )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection challenge packet. decrypted packet data is wrong size\n" );
                    return NULL;
                }

                struct netcode_connection_challenge_packet_t * packet = (struct netcode_connection_challenge_packet_t*) 
                    allocate_function( allocator_context, sizeof( struct netcode_connection_challenge_packet_t ) );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection challenge packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                packet->packet_type = NETCODE_CONNECTION_CHALLENGE_PACKET;
                packet->challenge_token_sequence = netcode_read_uint64( &buffer );
                netcode_read_bytes( &buffer, packet->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
                
                return packet;
            }
            break;

            case NETCODE_CONNECTION_RESPONSE_PACKET:
            {
                if ( decrypted_bytes != 8 + NETCODE_CHALLENGE_TOKEN_BYTES )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection response packet. decrypted packet data is wrong size\n" );
                    return NULL;
                }

                struct netcode_connection_response_packet_t * packet = (struct netcode_connection_response_packet_t*) 
                    allocate_function( allocator_context, sizeof( struct netcode_connection_response_packet_t ) );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection response packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                packet->packet_type = NETCODE_CONNECTION_RESPONSE_PACKET;
                packet->challenge_token_sequence = netcode_read_uint64( &buffer );
                netcode_read_bytes( &buffer, packet->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
                
                return packet;
            }
            break;

            case NETCODE_CONNECTION_KEEP_ALIVE_PACKET:
            {
                if ( decrypted_bytes != 8 )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection keep alive packet. decrypted packet data is wrong size\n" );
                    return NULL;
                }

                struct netcode_connection_keep_alive_packet_t * packet = (struct netcode_connection_keep_alive_packet_t*) 
                    allocate_function( allocator_context, sizeof( struct netcode_connection_keep_alive_packet_t ) );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection keep alive packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                packet->packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
                packet->client_index = netcode_read_uint32( &buffer );
                packet->max_clients = netcode_read_uint32( &buffer );
                
                return packet;
            }
            break;
            
            case NETCODE_CONNECTION_PAYLOAD_PACKET:
            {
                if ( decrypted_bytes < 1 )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection payload packet. payload is too small\n" );
                    return NULL;
                }

                if ( decrypted_bytes > NETCODE_MAX_PAYLOAD_BYTES )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection payload packet. payload is too large\n" );
                    return NULL;
                }

                struct netcode_connection_payload_packet_t * packet = netcode_create_payload_packet( decrypted_bytes, allocator_context, allocate_function );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection payload packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                memcpy( packet->payload_data, buffer, decrypted_bytes );
                
                return packet;
            }
            break;

            case NETCODE_CONNECTION_DISCONNECT_PACKET:
            {
                if ( decrypted_bytes != 0 )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection disconnect packet. decrypted packet data is wrong size\n" );
                    return NULL;
                }

                struct netcode_connection_disconnect_packet_t * packet = (struct netcode_connection_disconnect_packet_t*) 
                    allocate_function( allocator_context, sizeof( struct netcode_connection_disconnect_packet_t ) );

                if ( !packet )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "ignored connection disconnect packet. could not allocate packet struct\n" );
                    return NULL;
                }
                
                packet->packet_type = NETCODE_CONNECTION_DISCONNECT_PACKET;
                
                return packet;
            }
            break;

            default:
                return NULL;
        }
    }
}

// ----------------------------------------------------------------

struct netcode_connect_token_t
{
    uint8_t version_info[NETCODE_VERSION_INFO_BYTES];
    uint64_t protocol_id;
    uint64_t create_timestamp;
    uint64_t expire_timestamp;
    uint8_t nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    uint8_t private_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
    int timeout_seconds;
    int num_server_addresses;
    struct netcode_address_t server_addresses[NETCODE_MAX_SERVERS_PER_CONNECT];
    uint8_t client_to_server_key[NETCODE_KEY_BYTES];
    uint8_t server_to_client_key[NETCODE_KEY_BYTES];
};

void netcode_write_connect_token( struct netcode_connect_token_t * connect_token, uint8_t * buffer, int buffer_length )
{
    netcode_assert( connect_token );
    netcode_assert( buffer );
    netcode_assert( buffer_length >= NETCODE_CONNECT_TOKEN_BYTES );

    uint8_t * start = buffer;

    (void) start;
    (void) buffer_length;

    netcode_write_bytes( &buffer, connect_token->version_info, NETCODE_VERSION_INFO_BYTES );

    netcode_write_uint64( &buffer, connect_token->protocol_id );

    netcode_write_uint64( &buffer, connect_token->create_timestamp );

    netcode_write_uint64( &buffer, connect_token->expire_timestamp );

    netcode_write_bytes( &buffer, connect_token->nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );

    netcode_write_bytes( &buffer, connect_token->private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    int i,j;

    netcode_write_uint32( &buffer, connect_token->timeout_seconds );

    netcode_write_uint32( &buffer, connect_token->num_server_addresses );

    for ( i = 0; i < connect_token->num_server_addresses; ++i )
    {
        if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV4 )
        {
            netcode_write_uint8( &buffer, NETCODE_ADDRESS_IPV4 );
            for ( j = 0; j < 4; ++j )
            {
                netcode_write_uint8( &buffer, connect_token->server_addresses[i].data.ipv4[j] );
            }
            netcode_write_uint16( &buffer, connect_token->server_addresses[i].port );
        }
        else if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV6 )
        {
            netcode_write_uint8( &buffer, NETCODE_ADDRESS_IPV6 );
            for ( j = 0; j < 8; ++j )
            {
                netcode_write_uint16( &buffer, connect_token->server_addresses[i].data.ipv6[j] );
            }
            netcode_write_uint16( &buffer, connect_token->server_addresses[i].port );
        }
        else
        {
            netcode_assert( 0 );
        }
    }

    netcode_write_bytes( &buffer, connect_token->client_to_server_key, NETCODE_KEY_BYTES );

    netcode_write_bytes( &buffer, connect_token->server_to_client_key, NETCODE_KEY_BYTES );

    netcode_assert( buffer - start <= NETCODE_CONNECT_TOKEN_BYTES );

    memset( buffer, 0, NETCODE_CONNECT_TOKEN_BYTES - ( buffer - start ) );
}

int netcode_read_connect_token( uint8_t * buffer, int buffer_length, struct netcode_connect_token_t * connect_token )
{
    netcode_assert( buffer );
    netcode_assert( connect_token );

    if ( buffer_length != NETCODE_CONNECT_TOKEN_BYTES )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: read connect data has bad buffer length (%d)\n", buffer_length );
        return NETCODE_ERROR;
    }

    netcode_read_bytes( &buffer, connect_token->version_info, NETCODE_VERSION_INFO_BYTES );
    if ( connect_token->version_info[0]  != 'N' || 
         connect_token->version_info[1]  != 'E' || 
         connect_token->version_info[2]  != 'T' || 
         connect_token->version_info[3]  != 'C' || 
         connect_token->version_info[4]  != 'O' ||
         connect_token->version_info[5]  != 'D' ||
         connect_token->version_info[6]  != 'E' ||
         connect_token->version_info[7]  != ' ' || 
         connect_token->version_info[8]  != '1' ||
         connect_token->version_info[9]  != '.' ||
         connect_token->version_info[10] != '0' ||
         connect_token->version_info[11] != '2' ||
         connect_token->version_info[12] != '\0' )
    {
        connect_token->version_info[12] = '\0';
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: read connect data has bad version info (got %s, expected %s)\n", connect_token->version_info, NETCODE_VERSION_INFO );
        return NETCODE_ERROR;
    }

    connect_token->protocol_id = netcode_read_uint64( &buffer );

    connect_token->create_timestamp = netcode_read_uint64( &buffer );

    connect_token->expire_timestamp = netcode_read_uint64( &buffer );

    if ( connect_token->create_timestamp > connect_token->expire_timestamp )
        return NETCODE_ERROR;

    netcode_read_bytes( &buffer, connect_token->nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );

    netcode_read_bytes( &buffer, connect_token->private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    connect_token->timeout_seconds = (int) netcode_read_uint32( &buffer );

    connect_token->num_server_addresses = netcode_read_uint32( &buffer );

    if ( connect_token->num_server_addresses <= 0 || connect_token->num_server_addresses > NETCODE_MAX_SERVERS_PER_CONNECT )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: read connect data has bad number of server addresses (%d)\n", connect_token->num_server_addresses );
        return NETCODE_ERROR;
    }

    int i,j;

    for ( i = 0; i < connect_token->num_server_addresses; ++i )
    {
        connect_token->server_addresses[i].type = netcode_read_uint8( &buffer );

        if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV4 )
        {
            for ( j = 0; j < 4; ++j )
            {
                connect_token->server_addresses[i].data.ipv4[j] = netcode_read_uint8( &buffer );
            }
            connect_token->server_addresses[i].port = netcode_read_uint16( &buffer );
        }
        else if ( connect_token->server_addresses[i].type == NETCODE_ADDRESS_IPV6 )
        {
            for ( j = 0; j < 8; ++j )
            {
                connect_token->server_addresses[i].data.ipv6[j] = netcode_read_uint16( &buffer );
            }
            connect_token->server_addresses[i].port = netcode_read_uint16( &buffer );
        }
        else
        {
            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: read connect data has bad address type (%d)\n", connect_token->server_addresses[i].type );
            return NETCODE_ERROR;
        }
    }

    netcode_read_bytes( &buffer, connect_token->client_to_server_key, NETCODE_KEY_BYTES );

    netcode_read_bytes( &buffer, connect_token->server_to_client_key, NETCODE_KEY_BYTES );
    
    return NETCODE_OK;
}

// ----------------------------------------------------------------

struct netcode_packet_queue_t
{
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
    int num_packets;
    int start_index;
    void * packet_data[NETCODE_PACKET_QUEUE_SIZE];
    uint64_t packet_sequence[NETCODE_PACKET_QUEUE_SIZE];
};

void netcode_packet_queue_init( struct netcode_packet_queue_t * queue, 
                                void * allocator_context, 
                                void * (*allocate_function)(void*,size_t), 
                                void (*free_function)(void*,void*) )
{
    if ( allocate_function == NULL )
    {
        allocate_function = netcode_default_allocate_function;
    }

    if ( free_function == NULL )
    {
        free_function = netcode_default_free_function;
    }

    netcode_assert( queue );

    queue->allocator_context = allocator_context;
    queue->allocate_function = allocate_function;
    queue->free_function = free_function;
    queue->num_packets = 0;
    queue->start_index = 0;
    memset( queue->packet_data, 0, sizeof( queue->packet_data ) );
    memset( queue->packet_sequence, 0, sizeof( queue->packet_sequence ) );
}

void netcode_packet_queue_clear( struct netcode_packet_queue_t * queue )
{
    int i;
    for ( i = 0; i < queue->num_packets; ++i )
    {
        queue->free_function( queue->allocator_context, queue->packet_data[i] );
    }
    queue->num_packets = 0;
    queue->start_index = 0;
    memset( queue->packet_data, 0, sizeof( queue->packet_data ) );
    memset( queue->packet_sequence, 0, sizeof( queue->packet_sequence ) );
}

int netcode_packet_queue_push( struct netcode_packet_queue_t * queue, void * packet_data, uint64_t packet_sequence )
{
    netcode_assert( queue );
    netcode_assert( packet_data );
    if ( queue->num_packets == NETCODE_PACKET_QUEUE_SIZE )
    {
        queue->free_function( queue->allocator_context, packet_data );
        return 0;
    }
    int index = ( queue->start_index + queue->num_packets ) % NETCODE_PACKET_QUEUE_SIZE;
    queue->packet_data[index] = packet_data;
    queue->packet_sequence[index] = packet_sequence;
    queue->num_packets++;
    return 1;
}

void * netcode_packet_queue_pop( struct netcode_packet_queue_t * queue, uint64_t * packet_sequence )
{
    if ( queue->num_packets == 0 )
        return NULL;
    void * packet = queue->packet_data[queue->start_index];
    if ( packet_sequence )
        *packet_sequence = queue->packet_sequence[queue->start_index];
    queue->start_index = ( queue->start_index + 1 ) % NETCODE_PACKET_QUEUE_SIZE;
    queue->num_packets--;
    return packet;
}

// ----------------------------------------------------------------

#define NETCODE_NETWORK_SIMULATOR_NUM_PACKET_ENTRIES ( NETCODE_MAX_CLIENTS * 256 )
#define NETCODE_NETWORK_SIMULATOR_NUM_PENDING_RECEIVE_PACKETS ( NETCODE_MAX_CLIENTS * 64 )

struct netcode_network_simulator_packet_entry_t
{
    struct netcode_address_t from;
    struct netcode_address_t to;
    double delivery_time;
    uint8_t * packet_data;
    int packet_bytes;
};

struct netcode_network_simulator_t
{
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
    float latency_milliseconds;
    float jitter_milliseconds;
    float packet_loss_percent;
    float duplicate_packet_percent;
    double time;
    int current_index;
    int num_pending_receive_packets;
    struct netcode_network_simulator_packet_entry_t packet_entries[NETCODE_NETWORK_SIMULATOR_NUM_PACKET_ENTRIES];
    struct netcode_network_simulator_packet_entry_t pending_receive_packets[NETCODE_NETWORK_SIMULATOR_NUM_PENDING_RECEIVE_PACKETS];
};

struct netcode_network_simulator_t * netcode_network_simulator_create( void * allocator_context, 
                                                                       void * (*allocate_function)(void*,size_t), 
                                                                       void (*free_function)(void*,void*) )
{
    if ( allocate_function == NULL )
    {
        allocate_function = netcode_default_allocate_function;
    }

    if ( free_function == NULL )
    {
        free_function = netcode_default_free_function;
    }

    struct netcode_network_simulator_t * network_simulator = (struct netcode_network_simulator_t*) 
        allocate_function( allocator_context, sizeof( struct netcode_network_simulator_t ) );

    netcode_assert( network_simulator );

    memset( network_simulator, 0, sizeof( struct netcode_network_simulator_t ) );

    network_simulator->allocator_context = allocator_context;
    network_simulator->allocate_function = allocate_function;
    network_simulator->free_function = free_function;

    return network_simulator;
}

void netcode_network_simulator_reset( struct netcode_network_simulator_t * network_simulator )
{
    netcode_assert( network_simulator );

    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "network simulator reset\n" );

    int i;
    for ( i = 0; i < NETCODE_NETWORK_SIMULATOR_NUM_PACKET_ENTRIES; ++i )
    {
        network_simulator->free_function( network_simulator->allocator_context, network_simulator->packet_entries[i].packet_data );
        memset( &network_simulator->packet_entries[i], 0, sizeof( struct netcode_network_simulator_packet_entry_t ) );
    }

    for ( i = 0; i < network_simulator->num_pending_receive_packets; ++i )
    {
        network_simulator->free_function( network_simulator->allocator_context, network_simulator->pending_receive_packets[i].packet_data );
        memset( &network_simulator->pending_receive_packets[i], 0, sizeof( struct netcode_network_simulator_packet_entry_t ) );
    }

    network_simulator->current_index = 0;
    network_simulator->num_pending_receive_packets = 0;
}

void netcode_network_simulator_destroy( struct netcode_network_simulator_t * network_simulator )
{
    netcode_assert( network_simulator );
    netcode_network_simulator_reset( network_simulator );
    network_simulator->free_function( network_simulator->allocator_context, network_simulator );
}

float netcode_random_float( float a, float b )
{
    netcode_assert( a < b );
    float random = ( (float) rand() ) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

void netcode_network_simulator_queue_packet( struct netcode_network_simulator_t * network_simulator, 
                                             struct netcode_address_t * from, 
                                             struct netcode_address_t * to, 
                                             uint8_t * packet_data, 
                                             int packet_bytes, 
                                             float delay )
{
    network_simulator->packet_entries[network_simulator->current_index].from = *from;
    network_simulator->packet_entries[network_simulator->current_index].to = *to;
    network_simulator->packet_entries[network_simulator->current_index].packet_data = 
        (uint8_t*) network_simulator->allocate_function( network_simulator->allocator_context, packet_bytes );
    memcpy( network_simulator->packet_entries[network_simulator->current_index].packet_data, packet_data, packet_bytes );
    network_simulator->packet_entries[network_simulator->current_index].packet_bytes = packet_bytes;
    network_simulator->packet_entries[network_simulator->current_index].delivery_time = network_simulator->time + delay;
    network_simulator->current_index++;
    network_simulator->current_index %= NETCODE_NETWORK_SIMULATOR_NUM_PACKET_ENTRIES;
}

void netcode_network_simulator_send_packet( struct netcode_network_simulator_t * network_simulator, 
                                            struct netcode_address_t * from, 
                                            struct netcode_address_t * to, 
                                            uint8_t * packet_data, 
                                            int packet_bytes )
{
    netcode_assert( network_simulator );
    netcode_assert( from );
    netcode_assert( from->type != 0 );
    netcode_assert( to );
    netcode_assert( to->type != 0 );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes > 0 );
    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_BYTES );

    if ( netcode_random_float( 0.0f, 100.0f ) <= network_simulator->packet_loss_percent )
        return;

    if ( network_simulator->packet_entries[network_simulator->current_index].packet_data )
    {
        network_simulator->free_function( network_simulator->allocator_context, network_simulator->packet_entries[network_simulator->current_index].packet_data );
        network_simulator->packet_entries[network_simulator->current_index].packet_data = NULL;
    }

    float delay = network_simulator->latency_milliseconds / 1000.0f;

    if ( network_simulator->jitter_milliseconds > 0.0 )
        delay += netcode_random_float( -network_simulator->jitter_milliseconds, +network_simulator->jitter_milliseconds ) / 1000.0f;

    netcode_network_simulator_queue_packet( network_simulator, from, to, packet_data, packet_bytes, delay );

    if ( netcode_random_float( 0.0f, 100.0f ) <= network_simulator->duplicate_packet_percent )
    {
        netcode_network_simulator_queue_packet( network_simulator, from, to, packet_data, packet_bytes, delay + netcode_random_float( 0, 1.0 ) );
    }
}

int netcode_network_simulator_receive_packets( struct netcode_network_simulator_t * network_simulator, 
                                               struct netcode_address_t * to, 
                                               int max_packets, 
                                               uint8_t ** packet_data, 
                                               int * packet_bytes, 
                                               struct netcode_address_t * from )
{
    netcode_assert( network_simulator );
    netcode_assert( max_packets >= 0 );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes );
    netcode_assert( from );
    netcode_assert( to );

    int num_packets = 0;

    int i;
    for ( i = 0; i < network_simulator->num_pending_receive_packets; ++i )
    {
        if ( num_packets == max_packets )
            break;

        if ( !network_simulator->pending_receive_packets[i].packet_data )
            continue;

        if ( !netcode_address_equal( &network_simulator->pending_receive_packets[i].to, to ) )
            continue;

        packet_data[num_packets] = network_simulator->pending_receive_packets[i].packet_data;
        packet_bytes[num_packets] = network_simulator->pending_receive_packets[i].packet_bytes;
        from[num_packets] = network_simulator->pending_receive_packets[i].from;

        network_simulator->pending_receive_packets[i].packet_data = NULL;

        num_packets++;
    }

    netcode_assert( num_packets <= max_packets );

    return num_packets;
}

void netcode_network_simulator_update( struct netcode_network_simulator_t * network_simulator, double time )
{   
    netcode_assert( network_simulator );

    network_simulator->time = time;

    // discard any pending receive packets that are still in the buffer

    int i;
    for ( i = 0; i < network_simulator->num_pending_receive_packets; ++i )
    {
        if ( network_simulator->pending_receive_packets[i].packet_data )
        {
            network_simulator->free_function( network_simulator->allocator_context, network_simulator->pending_receive_packets[i].packet_data );
            network_simulator->pending_receive_packets[i].packet_data = NULL;
        }
    }

    network_simulator->num_pending_receive_packets = 0;

    // walk across packet entries and move any that are ready to be received into the pending receive buffer

    for ( i = 0; i < NETCODE_NETWORK_SIMULATOR_NUM_PACKET_ENTRIES; ++i )
    {
        if ( !network_simulator->packet_entries[i].packet_data )
            continue;

        if ( network_simulator->num_pending_receive_packets == NETCODE_NETWORK_SIMULATOR_NUM_PENDING_RECEIVE_PACKETS )
            break;

        if ( network_simulator->packet_entries[i].packet_data && network_simulator->packet_entries[i].delivery_time <= time )
        {
            network_simulator->pending_receive_packets[network_simulator->num_pending_receive_packets] = network_simulator->packet_entries[i];
            network_simulator->num_pending_receive_packets++;
            network_simulator->packet_entries[i].packet_data = NULL;
        }
    }
}

// ----------------------------------------------------------------
    
NETCODE_CONST char * netcode_client_state_name( int client_state )
{
    switch ( client_state )
    {
        case NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED:                return "connect token expired";
        case NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN:                return "invalid connect token";
        case NETCODE_CLIENT_STATE_CONNECTION_TIMED_OUT:                 return "connection timed out";
        case NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT:         return "connection request timed out";
        case NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT:        return "connection response timed out";
        case NETCODE_CLIENT_STATE_CONNECTION_DENIED:                    return "connection denied";
        case NETCODE_CLIENT_STATE_DISCONNECTED:                         return "disconnected";
        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST:           return "sending connection request";
        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE:          return "sending connection response";
        case NETCODE_CLIENT_STATE_CONNECTED:                            return "connected";
        default:
            netcode_assert( 0 );
            return "???";
    }
}

void netcode_default_client_config( struct netcode_client_config_t * config )
{
    netcode_assert( config );
    config->allocator_context = NULL;
    config->allocate_function = netcode_default_allocate_function;
    config->free_function = netcode_default_free_function;
    config->network_simulator = NULL;
    config->callback_context = NULL;
    config->state_change_callback = NULL;
    config->send_loopback_packet_callback = NULL;
    config->override_send_and_receive = 0;
    config->send_packet_override = NULL;
    config->receive_packet_override = NULL;
};

struct netcode_client_t
{
    struct netcode_client_config_t config;
    int state;
    double time;
    double connect_start_time;
    double last_packet_send_time;
    double last_packet_receive_time;
    int should_disconnect;
    int should_disconnect_state;
    uint64_t sequence;
    int client_index;
    int max_clients;
    int server_address_index;
    struct netcode_address_t address;
    struct netcode_address_t server_address;
    struct netcode_connect_token_t connect_token;
    struct netcode_socket_holder_t socket_holder;
    struct netcode_context_t context;
    struct netcode_replay_protection_t replay_protection;
    struct netcode_packet_queue_t packet_receive_queue;
    uint64_t challenge_token_sequence;
    uint8_t challenge_token_data[NETCODE_CHALLENGE_TOKEN_BYTES];
    uint8_t * receive_packet_data[NETCODE_CLIENT_MAX_RECEIVE_PACKETS];
    int receive_packet_bytes[NETCODE_CLIENT_MAX_RECEIVE_PACKETS];
    struct netcode_address_t receive_from[NETCODE_CLIENT_MAX_RECEIVE_PACKETS];
    int loopback;
};

int netcode_client_socket_create( struct netcode_socket_t * socket,
                                  struct netcode_address_t * address,
                                  int send_buffer_size,
                                  int receive_buffer_size,
                                  NETCODE_CONST struct netcode_client_config_t * config )
{
    netcode_assert( socket );
    netcode_assert( address );
    netcode_assert( config );

    if ( !config->network_simulator )
    {
        if ( !config->override_send_and_receive )
        {
            if ( netcode_socket_create( socket, address, send_buffer_size, receive_buffer_size ) != NETCODE_SOCKET_ERROR_NONE )
            {
                return 0;
            }
        }
    }
    else
    {
        if ( address->port == 0 )
        {
            netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: must bind to a specific port when using network simulator\n" );
            return 0;
        }
    }

    return 1;
}

struct netcode_client_t * netcode_client_create_overload( NETCODE_CONST char * address1_string,
                                                          NETCODE_CONST char * address2_string,
                                                          NETCODE_CONST struct netcode_client_config_t * config,
                                                          double time )
{
    netcode_assert( config );
    netcode_assert( netcode.initialized );

    struct netcode_address_t address1;
    struct netcode_address_t address2;

    memset( &address1, 0, sizeof( address1 ) );
    memset( &address2, 0, sizeof( address2 ) );

    if ( netcode_parse_address( address1_string, &address1 ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to parse client address\n" );
        return NULL;
    }

    if ( address2_string != NULL && netcode_parse_address( address2_string, &address2 ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to parse client address2\n" );
        return NULL;
    }


    struct netcode_socket_t socket_ipv4;
    struct netcode_socket_t socket_ipv6;

    memset( &socket_ipv4, 0, sizeof( socket_ipv4 ) );
    memset( &socket_ipv6, 0, sizeof( socket_ipv6 ) );

    if ( address1.type == NETCODE_ADDRESS_IPV4 || address2.type == NETCODE_ADDRESS_IPV4 )
    {
        if ( !netcode_client_socket_create( &socket_ipv4, address1.type == NETCODE_ADDRESS_IPV4 ? &address1 : &address2, NETCODE_CLIENT_SOCKET_SNDBUF_SIZE, NETCODE_CLIENT_SOCKET_RCVBUF_SIZE, config ) )
        {
            return NULL;
        }
    }

    if ( address1.type == NETCODE_ADDRESS_IPV6 || address2.type == NETCODE_ADDRESS_IPV6 )
    {
        if ( !netcode_client_socket_create( &socket_ipv6, address1.type == NETCODE_ADDRESS_IPV6 ? &address1 : &address2, NETCODE_CLIENT_SOCKET_SNDBUF_SIZE, NETCODE_CLIENT_SOCKET_RCVBUF_SIZE, config ) )
        {
            return NULL;
        }
    }

    struct netcode_client_t * client = (struct netcode_client_t*) config->allocate_function( config->allocator_context, sizeof( struct netcode_client_t ) );

    if ( !client )
    {
        netcode_socket_destroy( &socket_ipv4 );
        netcode_socket_destroy( &socket_ipv6 );
        return NULL;
    }

    struct netcode_address_t socket_address = address1.type == NETCODE_ADDRESS_IPV4 ? socket_ipv4.address : socket_ipv6.address;

    if ( !config->network_simulator )
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "client started on port %d\n", socket_address.port );
    }
    else
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "client started on port %d (network simulator)\n", socket_address.port );
    }

    client->config = *config;
    client->socket_holder.ipv4 = socket_ipv4;
    client->socket_holder.ipv6 = socket_ipv6;
    client->address = config->network_simulator ? address1 : socket_address;
    client->state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->time = time;
    client->connect_start_time = 0.0;
    client->last_packet_send_time = -1000.0;
    client->last_packet_receive_time = -1000.0;
    client->should_disconnect = 0;
    client->should_disconnect_state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->sequence = 0;
    client->client_index = 0;
    client->max_clients = 0;
    client->server_address_index = 0;
    client->challenge_token_sequence = 0;
    client->loopback = 0;
    memset( &client->server_address, 0, sizeof( struct netcode_address_t ) );
    memset( &client->connect_token, 0, sizeof( struct netcode_connect_token_t ) );
    memset( &client->context, 0, sizeof( struct netcode_context_t ) );
    memset( client->challenge_token_data, 0, NETCODE_CHALLENGE_TOKEN_BYTES );

    netcode_packet_queue_init( &client->packet_receive_queue, config->allocator_context, config->allocate_function, config->free_function );

    netcode_replay_protection_reset( &client->replay_protection );

    return client;
}

struct netcode_client_t * netcode_client_create( NETCODE_CONST char * address,
                                                 NETCODE_CONST struct netcode_client_config_t * config,
                                                 double time )
{
    return netcode_client_create_overload( address, NULL, config, time );
}

void netcode_client_destroy( struct netcode_client_t * client )
{
    netcode_assert( client );
    if ( !client->loopback )
        netcode_client_disconnect( client );
    else
        netcode_client_disconnect_loopback( client );
    netcode_socket_destroy( &client->socket_holder.ipv4 );
    netcode_socket_destroy( &client->socket_holder.ipv6 );
    netcode_packet_queue_clear( &client->packet_receive_queue );
    client->config.free_function( client->config.allocator_context, client );
}

void netcode_client_set_state( struct netcode_client_t * client, int client_state )
{
    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client changed state from '%s' to '%s'\n", 
        netcode_client_state_name( client->state ), netcode_client_state_name( client_state ) );

    if ( client->config.state_change_callback )
    {
        client->config.state_change_callback( client->config.callback_context, client->state, client_state );
    }

    client->state = client_state;
}

void netcode_client_reset_before_next_connect( struct netcode_client_t * client )
{
    client->connect_start_time = client->time;
    client->last_packet_send_time = client->time - 1.0f;
    client->last_packet_receive_time = client->time;
    client->should_disconnect = 0;
    client->should_disconnect_state = NETCODE_CLIENT_STATE_DISCONNECTED;
    client->challenge_token_sequence = 0;

    memset( client->challenge_token_data, 0, NETCODE_CHALLENGE_TOKEN_BYTES );

    netcode_replay_protection_reset( &client->replay_protection );
}

void netcode_client_reset_connection_data( struct netcode_client_t * client, int client_state )
{
    netcode_assert( client );

    client->sequence = 0;
    client->loopback = 0;
    client->client_index = 0;
    client->max_clients = 0;
    client->connect_start_time = 0.0;
    client->server_address_index = 0;
    memset( &client->server_address, 0, sizeof( struct netcode_address_t ) );
    memset( &client->connect_token, 0, sizeof( struct netcode_connect_token_t ) );
    memset( &client->context, 0, sizeof( struct netcode_context_t ) );

    netcode_client_set_state( client, client_state );

    netcode_client_reset_before_next_connect( client );

    while ( 1 )
    {
        void * packet = netcode_packet_queue_pop( &client->packet_receive_queue, NULL );
        if ( !packet )
            break;
        client->config.free_function( client->config.allocator_context, packet );
    }

    netcode_packet_queue_clear( &client->packet_receive_queue );
}

void netcode_client_disconnect_internal( struct netcode_client_t * client, int destination_state, int send_disconnect_packets );

void netcode_client_connect( struct netcode_client_t * client, uint8_t * connect_token )
{
    netcode_assert( client );
    netcode_assert( connect_token );

    netcode_client_disconnect( client );

    if ( netcode_read_connect_token( connect_token, NETCODE_CONNECT_TOKEN_BYTES, &client->connect_token ) != NETCODE_OK )
    {
        netcode_client_set_state( client, NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN );
        return;
    }

    client->server_address_index = 0;
    client->server_address = client->connect_token.server_addresses[0];

    char server_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connecting to server %s [%d/%d]\n", 
        netcode_address_to_string( &client->server_address, server_address_string ), client->server_address_index + 1, client->connect_token.num_server_addresses );

    memcpy( client->context.read_packet_key, client->connect_token.server_to_client_key, NETCODE_KEY_BYTES );
    memcpy( client->context.write_packet_key, client->connect_token.client_to_server_key, NETCODE_KEY_BYTES );

    netcode_client_reset_before_next_connect( client );

    netcode_client_set_state( client, NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST );
}

void netcode_client_process_packet_internal( struct netcode_client_t * client, struct netcode_address_t * from, uint8_t * packet, uint64_t sequence )
{
    netcode_assert( client );
    netcode_assert( packet );

    uint8_t packet_type = ( (uint8_t*) packet ) [0];

    switch ( packet_type )
    {
        case NETCODE_CONNECTION_DENIED_PACKET:
        {
            if ( ( client->state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST || 
                   client->state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE ) 
                                                && 
                      netcode_address_equal( from, &client->server_address ) )
            {
                client->should_disconnect = 1;
                client->should_disconnect_state = NETCODE_CLIENT_STATE_CONNECTION_DENIED;
                client->last_packet_receive_time = client->time;
            }
        }
        break;

        case NETCODE_CONNECTION_CHALLENGE_PACKET:
        {
            if ( client->state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST && netcode_address_equal( from, &client->server_address ) )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client received connection challenge packet from server\n" );

                struct netcode_connection_challenge_packet_t * p = (struct netcode_connection_challenge_packet_t*) packet;
                client->challenge_token_sequence = p->challenge_token_sequence;
                memcpy( client->challenge_token_data, p->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
                client->last_packet_receive_time = client->time;

                netcode_client_set_state( client, NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE );
            }
        }
        break;

        case NETCODE_CONNECTION_KEEP_ALIVE_PACKET:
        {
            if ( netcode_address_equal( from, &client->server_address ) )
            {
                struct netcode_connection_keep_alive_packet_t * p = (struct netcode_connection_keep_alive_packet_t*) packet;

                if ( client->state == NETCODE_CLIENT_STATE_CONNECTED )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client received connection keep alive packet from server\n" );

                    client->last_packet_receive_time = client->time;
                }
                else if ( client->state == NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client received connection keep alive packet from server\n" );

                    client->last_packet_receive_time = client->time;
                    client->client_index = p->client_index;
                    client->max_clients = p->max_clients;

                    netcode_client_set_state( client, NETCODE_CLIENT_STATE_CONNECTED );

                    netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connected to server\n" );
                }
            }
        }
        break;

        case NETCODE_CONNECTION_PAYLOAD_PACKET:
        {
            if ( client->state == NETCODE_CLIENT_STATE_CONNECTED && netcode_address_equal( from, &client->server_address ) )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client received connection payload packet from server\n" );

                netcode_packet_queue_push( &client->packet_receive_queue, packet, sequence );

                client->last_packet_receive_time = client->time;

                return;
            }
        }
        break;

        case NETCODE_CONNECTION_DISCONNECT_PACKET:
        {
            if ( client->state == NETCODE_CLIENT_STATE_CONNECTED && netcode_address_equal( from, &client->server_address ) )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client received disconnect packet from server\n" );

                client->should_disconnect = 1;
                client->should_disconnect_state = NETCODE_CLIENT_STATE_DISCONNECTED;
                client->last_packet_receive_time = client->time;
            }
        }
        break;

        default:
            break;
    }

    client->config.free_function( client->config.allocator_context, packet );    
}

void netcode_client_process_packet( struct netcode_client_t * client, struct netcode_address_t * from, uint8_t * packet_data, int packet_bytes )
{
    (void) client;
    (void) from;
    (void) packet_data;
    (void) packet_bytes;

    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packets, 0, sizeof( allowed_packets ) );
    allowed_packets[NETCODE_CONNECTION_DENIED_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_CHALLENGE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_KEEP_ALIVE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_PAYLOAD_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_DISCONNECT_PACKET] = 1;

    uint64_t current_timestamp = (uint64_t) time( NULL );

    uint64_t sequence;

    void * packet = netcode_read_packet( packet_data, 
                                         packet_bytes, 
                                         &sequence, 
                                         client->context.read_packet_key, 
                                         client->connect_token.protocol_id, 
                                         current_timestamp, 
                                         NULL, 
                                         allowed_packets, 
                                         &client->replay_protection, 
                                         client->config.allocator_context, 
                                         client->config.allocate_function );

    if ( !packet )
        return;
    
    netcode_client_process_packet_internal( client, from, (uint8_t*)packet, sequence );
}

void netcode_client_receive_packets( struct netcode_client_t * client )
{
    netcode_assert( client );
    netcode_assert( !client->loopback );

    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packets, 0, sizeof( allowed_packets ) );
    allowed_packets[NETCODE_CONNECTION_DENIED_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_CHALLENGE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_KEEP_ALIVE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_PAYLOAD_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_DISCONNECT_PACKET] = 1;

    uint64_t current_timestamp = (uint64_t) time( NULL );

    if ( !client->config.network_simulator )
    {
        // process packets received from socket

        while ( 1 )
        {
            struct netcode_address_t from;
            uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];
            int packet_bytes = 0;

            if ( client->config.override_send_and_receive )
            {
                packet_bytes = client->config.receive_packet_override( client->config.callback_context, &from, packet_data, NETCODE_MAX_PACKET_BYTES );
            }
            else if ( client->server_address.type == NETCODE_ADDRESS_IPV4 )
            {
                packet_bytes = netcode_socket_receive_packet( &client->socket_holder.ipv4, &from, packet_data, NETCODE_MAX_PACKET_BYTES );
            }
            else if ( client->server_address.type == NETCODE_ADDRESS_IPV6 )
            {
                packet_bytes = netcode_socket_receive_packet( &client->socket_holder.ipv6, &from, packet_data, NETCODE_MAX_PACKET_BYTES );
            }

            if ( packet_bytes == 0 )
                break;

            uint64_t sequence;
            void * packet = netcode_read_packet( packet_data, 
                                                 packet_bytes, 
                                                 &sequence, 
                                                 client->context.read_packet_key, 
                                                 client->connect_token.protocol_id, 
                                                 current_timestamp, 
                                                 NULL, 
                                                 allowed_packets, 
                                                 &client->replay_protection, 
                                                 client->config.allocator_context, 
                                                 client->config.allocate_function );

            if ( !packet )
                continue;

            netcode_client_process_packet_internal( client, &from, (uint8_t*)packet, sequence );
        }
    }
    else
    {
        // process packets received from network simulator

        int num_packets_received = netcode_network_simulator_receive_packets( client->config.network_simulator, 
                                                                              &client->address, 
                                                                              NETCODE_CLIENT_MAX_RECEIVE_PACKETS, 
                                                                              client->receive_packet_data, 
                                                                              client->receive_packet_bytes, 
                                                                              client->receive_from );

        int i;
        for ( i = 0; i < num_packets_received; ++i )
        {
            uint64_t sequence;

            void * packet = netcode_read_packet( client->receive_packet_data[i], 
                                                 client->receive_packet_bytes[i], 
                                                 &sequence, 
                                                 client->context.read_packet_key, 
                                                 client->connect_token.protocol_id, 
                                                 current_timestamp, 
                                                 NULL, 
                                                 allowed_packets, 
                                                 &client->replay_protection, 
                                                 client->config.allocator_context, 
                                                 client->config.allocate_function );

            client->config.free_function( client->config.allocator_context, client->receive_packet_data[i] );

            if ( !packet )
                continue;

            netcode_client_process_packet_internal( client, &client->receive_from[i], (uint8_t*)packet, sequence );
        }
    }
}

void netcode_client_send_packet_to_server_internal( struct netcode_client_t * client, void * packet )
{
    netcode_assert( client );
    netcode_assert( !client->loopback );
    
    uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];

    int packet_bytes = netcode_write_packet( packet, 
                                             packet_data, 
                                             NETCODE_MAX_PACKET_BYTES, 
                                             client->sequence++, 
                                             client->context.write_packet_key, 
                                             client->connect_token.protocol_id );

    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_BYTES );

    if ( client->config.network_simulator )
    {
        netcode_network_simulator_send_packet( client->config.network_simulator, &client->address, &client->server_address, packet_data, packet_bytes );
    }
    else
    {
        if ( client->config.override_send_and_receive )
        {
            client->config.send_packet_override( client->config.callback_context, &client->server_address, packet_data, packet_bytes );
        }
        else if ( client->server_address.type == NETCODE_ADDRESS_IPV4 )
        {
            netcode_socket_send_packet( &client->socket_holder.ipv4, &client->server_address, packet_data, packet_bytes );
        }
        else if ( client->server_address.type == NETCODE_ADDRESS_IPV6 )
        {
            netcode_socket_send_packet( &client->socket_holder.ipv6, &client->server_address, packet_data, packet_bytes );
        }
    }

    client->last_packet_send_time = client->time;
}

void netcode_client_send_packets( struct netcode_client_t * client )
{
    netcode_assert( client );
    netcode_assert( !client->loopback );

    switch ( client->state )
    {
        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST:
        {
            if ( client->last_packet_send_time + ( 1.0 / NETCODE_PACKET_SEND_RATE ) >= client->time )
                return;

            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client sent connection request packet to server\n" );

            struct netcode_connection_request_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_REQUEST_PACKET;
            memcpy( packet.version_info, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
            packet.protocol_id = client->connect_token.protocol_id;
            packet.connect_token_expire_timestamp = client->connect_token.expire_timestamp;
            memcpy( packet.connect_token_nonce, client->connect_token.nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
            memcpy( packet.connect_token_data, client->connect_token.private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

            netcode_client_send_packet_to_server_internal( client, &packet );
        }
        break;

        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE:
        {
            if ( client->last_packet_send_time + ( 1.0 / NETCODE_PACKET_SEND_RATE ) >= client->time )
                return;

            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client sent connection response packet to server\n" );

            struct netcode_connection_response_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_RESPONSE_PACKET;
            packet.challenge_token_sequence = client->challenge_token_sequence;
            memcpy( packet.challenge_token_data, client->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );

            netcode_client_send_packet_to_server_internal( client, &packet );
        }
        break;

        case NETCODE_CLIENT_STATE_CONNECTED:
        {
            if ( client->last_packet_send_time + ( 1.0 / NETCODE_PACKET_SEND_RATE ) >= client->time )
                return;

            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client sent connection keep-alive packet to server\n" );

            struct netcode_connection_keep_alive_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
            packet.client_index = 0;
            packet.max_clients = 0;

            netcode_client_send_packet_to_server_internal( client, &packet );
        }
        break;
        
        default:
            break;
    }
}

int netcode_client_connect_to_next_server( struct netcode_client_t * client )
{
    netcode_assert( client );

    if ( client->server_address_index + 1 >= client->connect_token.num_server_addresses )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client has no more servers to connect to\n" );
        return 0;
    }

    client->server_address_index++;
    client->server_address = client->connect_token.server_addresses[client->server_address_index];

    netcode_client_reset_before_next_connect( client );

    char server_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connecting to next server %s [%d/%d]\n", 
        netcode_address_to_string( &client->server_address, server_address_string ), 
        client->server_address_index + 1, 
        client->connect_token.num_server_addresses );

    netcode_client_set_state( client, NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST );

    return 1;
}

void netcode_client_update( struct netcode_client_t * client, double time )
{
    netcode_assert( client );

    client->time = time;

    if ( client->loopback )
        return;

    netcode_client_receive_packets( client );

    netcode_client_send_packets( client );

    if ( client->state > NETCODE_CLIENT_STATE_DISCONNECTED && client->state < NETCODE_CLIENT_STATE_CONNECTED )
    {
        uint64_t connect_token_expire_seconds = ( client->connect_token.expire_timestamp - client->connect_token.create_timestamp );            
        if ( client->time - client->connect_start_time >= connect_token_expire_seconds )
        {
            netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connect failed. connect token expired\n" );
            netcode_client_disconnect_internal( client, NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED, 0 );
            return;
        }
    }

    if ( client->should_disconnect )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client should disconnect -> %s\n", netcode_client_state_name( client->should_disconnect_state ) );
        if ( netcode_client_connect_to_next_server( client ) )
            return;
        netcode_client_disconnect_internal( client, client->should_disconnect_state, 0 );
        return;
    }

    switch ( client->state )
    {
        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST:
        {
            if ( client->connect_token.timeout_seconds > 0 && client->last_packet_receive_time + client->connect_token.timeout_seconds < time )
            {
                netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connect failed. connection request timed out\n" );
                if ( netcode_client_connect_to_next_server( client ) )
                    return;
                netcode_client_disconnect_internal( client, NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT, 0 );
                return;
            }
        }
        break;

        case NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE:
        {
            if ( client->connect_token.timeout_seconds > 0 && client->last_packet_receive_time + client->connect_token.timeout_seconds < time )
            {
                netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connect failed. connection response timed out\n" );
                if ( netcode_client_connect_to_next_server( client ) )
                    return;
                netcode_client_disconnect_internal( client, NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT, 0 );
                return;
            }
        }
        break;

        case NETCODE_CLIENT_STATE_CONNECTED:
        {
            if ( client->connect_token.timeout_seconds > 0 && client->last_packet_receive_time + client->connect_token.timeout_seconds < time )
            {
                netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connection timed out\n" );
                netcode_client_disconnect_internal( client, NETCODE_CLIENT_STATE_CONNECTION_TIMED_OUT, 0 );
                return;
            }
        }
        break;

        default:
            break;
    }
}

uint64_t netcode_client_next_packet_sequence( struct netcode_client_t * client )
{
    netcode_assert( client );
    return client->sequence;  
}

void netcode_client_send_packet( struct netcode_client_t * client, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    netcode_assert( client );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes >= 0 );
    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_SIZE );

    if ( client->state != NETCODE_CLIENT_STATE_CONNECTED )
        return;

    if ( !client->loopback )
    {
        uint8_t buffer[NETCODE_MAX_PAYLOAD_BYTES*2];

        struct netcode_connection_payload_packet_t * packet = (struct netcode_connection_payload_packet_t*) buffer;

        packet->packet_type = NETCODE_CONNECTION_PAYLOAD_PACKET;
        packet->payload_bytes = packet_bytes;
        memcpy( packet->payload_data, packet_data, packet_bytes );

        netcode_client_send_packet_to_server_internal( client, packet );
    }
    else
    {
        client->config.send_loopback_packet_callback( client->config.callback_context, 
                                                      client->client_index, 
                                                      packet_data,
                                                      packet_bytes,
                                                      client->sequence++ );
    }
}

uint8_t * netcode_client_receive_packet( struct netcode_client_t * client, int * packet_bytes, uint64_t * packet_sequence )
{
    netcode_assert( client );
    netcode_assert( packet_bytes );

    struct netcode_connection_payload_packet_t * packet = (struct netcode_connection_payload_packet_t*) 
        netcode_packet_queue_pop( &client->packet_receive_queue, packet_sequence );
    
    if ( packet )
    {
        netcode_assert( packet->packet_type == NETCODE_CONNECTION_PAYLOAD_PACKET );
        *packet_bytes = packet->payload_bytes;
        netcode_assert( *packet_bytes >= 0 );
        netcode_assert( *packet_bytes <= NETCODE_MAX_PAYLOAD_BYTES );
        return (uint8_t*) &packet->payload_data;
    }
    else
    {
        return NULL;
    }
}

void netcode_client_free_packet( struct netcode_client_t * client, void * packet )
{
    netcode_assert( client );
    netcode_assert( packet );
    uint8_t * packet_data = (uint8_t*) packet;
    int offset = offsetof( struct netcode_connection_payload_packet_t, payload_data );
    client->config.free_function( client->config.allocator_context, packet_data - offset );
}

void netcode_client_disconnect( struct netcode_client_t * client )
{
    netcode_assert( client );
    netcode_assert( !client->loopback );
    netcode_client_disconnect_internal( client, NETCODE_CLIENT_STATE_DISCONNECTED, 1 );
}

void netcode_client_disconnect_internal( struct netcode_client_t * client, int destination_state, int send_disconnect_packets )
{
    netcode_assert( !client->loopback );
    netcode_assert( destination_state <= NETCODE_CLIENT_STATE_DISCONNECTED );

    if ( client->state <= NETCODE_CLIENT_STATE_DISCONNECTED || client->state == destination_state )
        return;

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "client disconnected\n" );

    if ( !client->loopback && send_disconnect_packets && client->state > NETCODE_CLIENT_STATE_DISCONNECTED )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client sent disconnect packets to server\n" );

        int i;
        for ( i = 0; i < NETCODE_NUM_DISCONNECT_PACKETS; ++i )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client sent disconnect packet %d\n", i );

            struct netcode_connection_disconnect_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_DISCONNECT_PACKET;

            netcode_client_send_packet_to_server_internal( client, &packet );
        }
    }

    netcode_client_reset_connection_data( client, destination_state );
}

int netcode_client_state( struct netcode_client_t * client )
{
    netcode_assert( client );
    return client->state;
}

int netcode_client_index( struct netcode_client_t * client )
{
    netcode_assert( client );
    return client->client_index;
}

int netcode_client_max_clients( struct netcode_client_t * client )
{   
    netcode_assert( client );
    return client->max_clients;
}

void netcode_client_connect_loopback( struct netcode_client_t * client, int client_index, int max_clients )
{
    netcode_assert( client );
    netcode_assert( client->state <= NETCODE_CLIENT_STATE_DISCONNECTED );
    netcode_printf( NETCODE_LOG_LEVEL_INFO, "client connected to server via loopback as client %d\n", client_index );
    client->state = NETCODE_CLIENT_STATE_CONNECTED;
    client->client_index = client_index;
    client->max_clients = max_clients;
    client->loopback = 1;
}

void netcode_client_disconnect_loopback( struct netcode_client_t * client )
{
    netcode_assert( client );
    netcode_assert( client->loopback );
    netcode_client_reset_connection_data( client, NETCODE_CLIENT_STATE_DISCONNECTED );
}

int netcode_client_loopback( struct netcode_client_t * client )
{
    netcode_assert( client );
    return client->loopback;
}

void netcode_client_process_loopback_packet( struct netcode_client_t * client, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence )
{
    netcode_assert( client );
    netcode_assert( client->loopback );
    struct netcode_connection_payload_packet_t * packet = netcode_create_payload_packet( packet_bytes, client->config.allocator_context, client->config.allocate_function );
    if ( !packet )
        return;
    memcpy( packet->payload_data, packet_data, packet_bytes );
    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "client processing loopback packet from server\n" );
    netcode_packet_queue_push( &client->packet_receive_queue, packet, packet_sequence );
}

uint16_t netcode_client_get_port( struct netcode_client_t * client )
{
    netcode_assert( client );
    return client->address.type == NETCODE_ADDRESS_IPV4 ? client->socket_holder.ipv4.address.port : client->socket_holder.ipv6.address.port;
}

struct netcode_address_t * netcode_client_server_address( struct netcode_client_t * client )
{
    netcode_assert( client );
    return &client->server_address;
}

// ----------------------------------------------------------------

#define NETCODE_MAX_ENCRYPTION_MAPPINGS ( NETCODE_MAX_CLIENTS * 4 )

struct netcode_encryption_manager_t
{
    int num_encryption_mappings;
    int timeout[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    double expire_time[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    double last_access_time[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    struct netcode_address_t address[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    int client_index[NETCODE_MAX_ENCRYPTION_MAPPINGS];
    uint8_t send_key[NETCODE_KEY_BYTES*NETCODE_MAX_ENCRYPTION_MAPPINGS];
    uint8_t receive_key[NETCODE_KEY_BYTES*NETCODE_MAX_ENCRYPTION_MAPPINGS];
};

void netcode_encryption_manager_reset( struct netcode_encryption_manager_t * encryption_manager )
{
    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "reset encryption manager\n" );

    netcode_assert( encryption_manager );

    encryption_manager->num_encryption_mappings = 0;
    
    int i;
    for ( i = 0; i < NETCODE_MAX_ENCRYPTION_MAPPINGS; ++i )
    {
        encryption_manager->client_index[i] = -1;
        encryption_manager->expire_time[i] = -1.0;
        encryption_manager->last_access_time[i] = -1000.0;
        memset( &encryption_manager->address[i], 0, sizeof( struct netcode_address_t ) );
    }

    memset( encryption_manager->timeout, 0, sizeof( encryption_manager->timeout ) );    
    memset( encryption_manager->send_key, 0, sizeof( encryption_manager->send_key ) );
    memset( encryption_manager->receive_key, 0, sizeof( encryption_manager->receive_key ) );
}

int netcode_encryption_manager_entry_expired( struct netcode_encryption_manager_t * encryption_manager, int index, double time )
{
    return ( encryption_manager->timeout[index] > 0 && ( encryption_manager->last_access_time[index] + encryption_manager->timeout[index] ) < time ) ||
           ( encryption_manager->expire_time[index] >= 0.0 && encryption_manager->expire_time[index] < time );
}

int netcode_encryption_manager_add_encryption_mapping( struct netcode_encryption_manager_t * encryption_manager, 
                                                       struct netcode_address_t * address, 
                                                       uint8_t * send_key, 
                                                       uint8_t * receive_key, 
                                                       double time, 
                                                       double expire_time,
                                                       int timeout )
{
    int i;
    for ( i = 0; i < encryption_manager->num_encryption_mappings; ++i )
    {
        if ( netcode_address_equal( &encryption_manager->address[i], address ) && !netcode_encryption_manager_entry_expired( encryption_manager, i, time ) )
        {
            encryption_manager->timeout[i] = timeout;
            encryption_manager->expire_time[i] = expire_time;
            encryption_manager->last_access_time[i] = time;
            memcpy( encryption_manager->send_key + i * NETCODE_KEY_BYTES, send_key, NETCODE_KEY_BYTES );
            memcpy( encryption_manager->receive_key + i * NETCODE_KEY_BYTES, receive_key, NETCODE_KEY_BYTES );
            return 1;
        }
    }

    for ( i = 0; i < NETCODE_MAX_ENCRYPTION_MAPPINGS; ++i )
    {
        if ( encryption_manager->address[i].type == NETCODE_ADDRESS_NONE || 
        	( netcode_encryption_manager_entry_expired( encryption_manager, i, time ) && encryption_manager->client_index[i] == -1 ) )
        {
            encryption_manager->timeout[i] = timeout;
            encryption_manager->address[i] = *address;
            encryption_manager->expire_time[i] = expire_time;
            encryption_manager->last_access_time[i] = time;
            memcpy( encryption_manager->send_key + i * NETCODE_KEY_BYTES, send_key, NETCODE_KEY_BYTES );
            memcpy( encryption_manager->receive_key + i * NETCODE_KEY_BYTES, receive_key, NETCODE_KEY_BYTES );
            if ( i + 1 > encryption_manager->num_encryption_mappings )
                encryption_manager->num_encryption_mappings = i + 1;
            return 1;
        }
    }

    return 0;
}

int netcode_encryption_manager_remove_encryption_mapping( struct netcode_encryption_manager_t * encryption_manager, struct netcode_address_t * address, double time )
{
    netcode_assert( encryption_manager );
    netcode_assert( address );

    int i;
    for ( i = 0; i < encryption_manager->num_encryption_mappings; ++i )
    {
        if ( netcode_address_equal( &encryption_manager->address[i], address ) )
        {
            encryption_manager->expire_time[i] = -1.0;
            encryption_manager->last_access_time[i] = -1000.0;
            memset( &encryption_manager->address[i], 0, sizeof( struct netcode_address_t ) );
            memset( encryption_manager->send_key + i * NETCODE_KEY_BYTES, 0, NETCODE_KEY_BYTES );
            memset( encryption_manager->receive_key + i * NETCODE_KEY_BYTES, 0, NETCODE_KEY_BYTES );

            if ( i + 1 == encryption_manager->num_encryption_mappings )
            {
                int index = i - 1;
                while ( index >= 0 )
                {
                    if ( !netcode_encryption_manager_entry_expired( encryption_manager, index, time ) || encryption_manager->client_index[index] != -1 )
                    {
                        break;
                    }
                    encryption_manager->address[index].type = NETCODE_ADDRESS_NONE;
                    index--;
                }
                encryption_manager->num_encryption_mappings = index + 1;
            }

            return 1;
        }
    }

    return 0;
}

int netcode_encryption_manager_find_encryption_mapping( struct netcode_encryption_manager_t * encryption_manager, struct netcode_address_t * address, double time )
{
    int i;
    for ( i = 0; i < encryption_manager->num_encryption_mappings; ++i )
    {
        if ( netcode_address_equal( &encryption_manager->address[i], address ) && !netcode_encryption_manager_entry_expired( encryption_manager, i, time ) )
        {
            encryption_manager->last_access_time[i] = time;
            return i;
        }
    }
    return -1;
}

int netcode_encryption_manager_touch( struct netcode_encryption_manager_t * encryption_manager, int index, struct netcode_address_t * address, double time )
{
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    if ( !netcode_address_equal( &encryption_manager->address[index], address ) )
        return 0;
    encryption_manager->last_access_time[index] = time;
    return 1;
}

void netcode_encryption_manager_set_expire_time( struct netcode_encryption_manager_t * encryption_manager, int index, double expire_time )
{
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    encryption_manager->expire_time[index] = expire_time;
}


uint8_t * netcode_encryption_manager_get_send_key( struct netcode_encryption_manager_t * encryption_manager, int index )
{
    netcode_assert( encryption_manager );
    if ( index == -1 )
        return NULL;
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    return encryption_manager->send_key + index * NETCODE_KEY_BYTES;
}

uint8_t * netcode_encryption_manager_get_receive_key( struct netcode_encryption_manager_t * encryption_manager, int index )
{
    netcode_assert( encryption_manager );
    if ( index == -1 )
        return NULL;
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    return encryption_manager->receive_key + index * NETCODE_KEY_BYTES;
}

int netcode_encryption_manager_get_timeout( struct netcode_encryption_manager_t * encryption_manager, int index )
{
    netcode_assert( encryption_manager );
    if ( index == -1 )
        return 0;
    netcode_assert( index >= 0 );
    netcode_assert( index < encryption_manager->num_encryption_mappings );
    return encryption_manager->timeout[index];
}

// ----------------------------------------------------------------

#define NETCODE_MAX_CONNECT_TOKEN_ENTRIES ( NETCODE_MAX_CLIENTS * 8 )

struct netcode_connect_token_entry_t
{
    double time;
    uint8_t mac[NETCODE_MAC_BYTES];
    struct netcode_address_t address;
};

void netcode_connect_token_entries_reset( struct netcode_connect_token_entry_t * connect_token_entries )
{
    int i;
    for ( i = 0; i < NETCODE_MAX_CONNECT_TOKEN_ENTRIES; ++i )
    {
        connect_token_entries[i].time = -1000.0;
        memset( connect_token_entries[i].mac, 0, NETCODE_MAC_BYTES );
        memset( &connect_token_entries[i].address, 0, sizeof( struct netcode_address_t ) );
    }
}

int netcode_connect_token_entries_find_or_add( struct netcode_connect_token_entry_t * connect_token_entries, 
                                               struct netcode_address_t * address, 
                                               uint8_t * mac, 
                                               double time )
{
    netcode_assert( connect_token_entries );
    netcode_assert( address );
    netcode_assert( mac );

    // find the matching entry for the token mac and the oldest token entry. constant time worst case. This is intentional!

    int matching_token_index = -1;
    int oldest_token_index = -1;
    double oldest_token_time = 0.0;

    int i;
    for ( i = 0; i < NETCODE_MAX_CONNECT_TOKEN_ENTRIES; ++i )
    {
        if ( memcmp( mac, connect_token_entries[i].mac, NETCODE_MAC_BYTES ) == 0 )
            matching_token_index = i;
        
        if ( oldest_token_index == -1 || connect_token_entries[i].time < oldest_token_time )
        {
            oldest_token_time = connect_token_entries[i].time;
            oldest_token_index = i;
        }
    }

    // if no entry is found with the mac, this is a new connect token. replace the oldest token entry.

    netcode_assert( oldest_token_index != -1 );

    if ( matching_token_index == -1 )
    {
        connect_token_entries[oldest_token_index].time = time;
        connect_token_entries[oldest_token_index].address = *address;
        memcpy( connect_token_entries[oldest_token_index].mac, mac, NETCODE_MAC_BYTES );
        return 1;
    }

    // allow connect tokens we have already seen from the same address

    netcode_assert( matching_token_index >= 0 );
    netcode_assert( matching_token_index < NETCODE_MAX_CONNECT_TOKEN_ENTRIES );
    if ( netcode_address_equal( &connect_token_entries[matching_token_index].address, address ) )
        return 1;

    return 0;
}

// ----------------------------------------------------------------

#define NETCODE_SERVER_FLAG_IGNORE_CONNECTION_REQUEST_PACKETS       1
#define NETCODE_SERVER_FLAG_IGNORE_CONNECTION_RESPONSE_PACKETS      (1<<1)

void netcode_default_server_config( struct netcode_server_config_t * config )
{
    netcode_assert( config );
    config->allocator_context = NULL;
    config->allocate_function = netcode_default_allocate_function;
    config->free_function = netcode_default_free_function;
    config->network_simulator = NULL;
    config->callback_context = NULL;
    config->connect_disconnect_callback = NULL;
    config->send_loopback_packet_callback = NULL;
    config->override_send_and_receive = 0;
    config->send_packet_override = NULL;
    config->receive_packet_override = NULL;
};

struct netcode_server_t
{
    struct netcode_server_config_t config;
    struct netcode_socket_holder_t socket_holder;
    struct netcode_address_t address;
    uint32_t flags;
    double time;
    int running;
    int max_clients;
    int num_connected_clients;
    uint64_t global_sequence;
    uint64_t challenge_sequence;
    uint8_t challenge_key[NETCODE_KEY_BYTES];
    int client_connected[NETCODE_MAX_CLIENTS];
    int client_timeout[NETCODE_MAX_CLIENTS];
    int client_loopback[NETCODE_MAX_CLIENTS];
    int client_confirmed[NETCODE_MAX_CLIENTS];
    int client_encryption_index[NETCODE_MAX_CLIENTS];
    uint64_t client_id[NETCODE_MAX_CLIENTS];
    uint64_t client_sequence[NETCODE_MAX_CLIENTS];
    double client_last_packet_send_time[NETCODE_MAX_CLIENTS];
    double client_last_packet_receive_time[NETCODE_MAX_CLIENTS];
    uint8_t client_user_data[NETCODE_MAX_CLIENTS][NETCODE_USER_DATA_BYTES];
    struct netcode_replay_protection_t client_replay_protection[NETCODE_MAX_CLIENTS];
    struct netcode_packet_queue_t client_packet_queue[NETCODE_MAX_CLIENTS];
    struct netcode_address_t client_address[NETCODE_MAX_CLIENTS];
    struct netcode_connect_token_entry_t connect_token_entries[NETCODE_MAX_CONNECT_TOKEN_ENTRIES];
    struct netcode_encryption_manager_t encryption_manager;
    uint8_t * receive_packet_data[NETCODE_SERVER_MAX_RECEIVE_PACKETS];
    int receive_packet_bytes[NETCODE_SERVER_MAX_RECEIVE_PACKETS];
    struct netcode_address_t receive_from[NETCODE_SERVER_MAX_RECEIVE_PACKETS];
};

int netcode_server_socket_create( struct netcode_socket_t * socket,
                                  struct netcode_address_t * address,
                                  int send_buffer_size,
                                  int receive_buffer_size,
                                  NETCODE_CONST struct netcode_server_config_t * config )
{
    netcode_assert( socket );
    netcode_assert( address );
    netcode_assert( config );

    if ( !config->network_simulator )
    {
        if ( !config->override_send_and_receive )
        {
            if ( netcode_socket_create( socket, address, send_buffer_size, receive_buffer_size ) != NETCODE_SOCKET_ERROR_NONE )
            {
                return 0;
            }
        }
    }

    return 1;
}

struct netcode_server_t * netcode_server_create_overload( NETCODE_CONST char * server_address1_string, NETCODE_CONST char * server_address2_string, NETCODE_CONST struct netcode_server_config_t * config, double time )
{
    netcode_assert( config );
    netcode_assert( netcode.initialized );

    struct netcode_address_t server_address1;
    struct netcode_address_t server_address2;

    memset( &server_address1, 0, sizeof( server_address1 ) );
    memset( &server_address2, 0, sizeof( server_address2 ) );

    if ( netcode_parse_address( server_address1_string, &server_address1 ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to parse server public address\n" );
        return NULL;
    }

    if ( server_address2_string != NULL && netcode_parse_address( server_address2_string, &server_address2 ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: failed to parse server public address2\n" );
        return NULL;
    }

    struct netcode_address_t bind_address_ipv4;
    struct netcode_address_t bind_address_ipv6;

    memset( &bind_address_ipv4, 0, sizeof( bind_address_ipv4 ) );
    memset( &bind_address_ipv6, 0, sizeof( bind_address_ipv6 ) );

    struct netcode_socket_t socket_ipv4;
    struct netcode_socket_t socket_ipv6;

    memset( &socket_ipv4, 0, sizeof( socket_ipv4 ) );
    memset( &socket_ipv6, 0, sizeof( socket_ipv6 ) );

    if ( server_address1.type == NETCODE_ADDRESS_IPV4 || server_address2.type == NETCODE_ADDRESS_IPV4 )
    {
        bind_address_ipv4.type = NETCODE_ADDRESS_IPV4;
        bind_address_ipv4.port = server_address1.type == NETCODE_ADDRESS_IPV4 ? server_address1.port : server_address2.port;

        if ( !netcode_server_socket_create( &socket_ipv4, &bind_address_ipv4, NETCODE_SERVER_SOCKET_SNDBUF_SIZE, NETCODE_SERVER_SOCKET_RCVBUF_SIZE, config ) )
        {
            return NULL;
        }
    }

    if ( server_address1.type == NETCODE_ADDRESS_IPV6 || server_address2.type == NETCODE_ADDRESS_IPV6 )
    {
        bind_address_ipv6.type = NETCODE_ADDRESS_IPV6;
        bind_address_ipv6.port = server_address1.type == NETCODE_ADDRESS_IPV6 ? server_address1.port : server_address2.port;

        if ( !netcode_server_socket_create( &socket_ipv6, &bind_address_ipv6, NETCODE_SERVER_SOCKET_SNDBUF_SIZE, NETCODE_SERVER_SOCKET_RCVBUF_SIZE, config ) )
        {
            return NULL;
        }
    }

    struct netcode_server_t * server = (struct netcode_server_t*) config->allocate_function( config->allocator_context, sizeof( struct netcode_server_t ) );
    if ( !server )
    {
        netcode_socket_destroy( &socket_ipv4 );
        netcode_socket_destroy( &socket_ipv6 );
        return NULL;
    }

    if ( !config->network_simulator )
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "server listening on %s\n", server_address1_string );
    }
    else
    {
        netcode_printf( NETCODE_LOG_LEVEL_INFO, "server listening on %s (network simulator)\n", server_address1_string );
    }

    server->config = *config;
    server->socket_holder.ipv4 = socket_ipv4;
    server->socket_holder.ipv6 = socket_ipv6;
    server->address = server_address1;
    server->flags = 0;
    server->time = time;
    server->running = 0;
    server->max_clients = 0;
    server->num_connected_clients = 0;
    server->global_sequence = 1ULL << 63;

    memset( server->client_connected, 0, sizeof( server->client_connected ) );
    memset( server->client_loopback, 0, sizeof( server->client_loopback ) );
    memset( server->client_confirmed, 0, sizeof( server->client_confirmed ) );
    memset( server->client_id, 0, sizeof( server->client_id ) );
    memset( server->client_sequence, 0, sizeof( server->client_sequence ) );
    memset( server->client_last_packet_send_time, 0, sizeof( server->client_last_packet_send_time ) );
    memset( server->client_last_packet_receive_time, 0, sizeof( server->client_last_packet_receive_time ) );
    memset( server->client_address, 0, sizeof( server->client_address ) );
    memset( server->client_user_data, 0, sizeof( server->client_user_data ) );

    int i;
    for ( i = 0; i < NETCODE_MAX_CLIENTS; ++i )
        server->client_encryption_index[i] = -1;

    netcode_connect_token_entries_reset( server->connect_token_entries );

    netcode_encryption_manager_reset( &server->encryption_manager );

    for ( i = 0; i < NETCODE_MAX_CLIENTS; ++i )
        netcode_replay_protection_reset( &server->client_replay_protection[i] );

    memset( &server->client_packet_queue, 0, sizeof( server->client_packet_queue ) );

    return server;
}

struct netcode_server_t * netcode_server_create( NETCODE_CONST char * server_address_string, NETCODE_CONST struct netcode_server_config_t * config, double time )
{
    return netcode_server_create_overload( server_address_string, NULL, config, time );
}

void netcode_server_stop( struct netcode_server_t * server );

void netcode_server_destroy( struct netcode_server_t * server )
{
    netcode_assert( server );

    netcode_server_stop( server );

    netcode_socket_destroy( &server->socket_holder.ipv4 );
    netcode_socket_destroy( &server->socket_holder.ipv6 );

    server->config.free_function( server->config.allocator_context, server );
}

void netcode_server_start( struct netcode_server_t * server, int max_clients )
{
    netcode_assert( server );
    netcode_assert( max_clients > 0 );
    netcode_assert( max_clients <= NETCODE_MAX_CLIENTS );

    if ( server->running )
        netcode_server_stop( server );

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server started with %d client slots\n", max_clients );

    server->running = 1;
    server->max_clients = max_clients;
    server->num_connected_clients = 0;
    server->challenge_sequence = 0;    
    netcode_generate_key( server->challenge_key );

    int i;
    for ( i = 0; i < server->max_clients; ++i )
    {
        netcode_packet_queue_init( &server->client_packet_queue[i], server->config.allocator_context, server->config.allocate_function, server->config.free_function );
    }
}

void netcode_server_send_global_packet( struct netcode_server_t * server, void * packet, struct netcode_address_t * to, uint8_t * packet_key )
{
    netcode_assert( server );
    netcode_assert( packet );
    netcode_assert( to );
    netcode_assert( packet_key );

    uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];

    int packet_bytes = netcode_write_packet( packet, packet_data, NETCODE_MAX_PACKET_BYTES, server->global_sequence, packet_key, server->config.protocol_id );

    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_BYTES );

    if ( server->config.network_simulator )
    {
        netcode_network_simulator_send_packet( server->config.network_simulator, &server->address, to, packet_data, packet_bytes );
    }
    else
    {
        if ( server->config.override_send_and_receive )
        {
            server->config.send_packet_override( server->config.callback_context, to, packet_data, packet_bytes );
        }
        else if ( to->type == NETCODE_ADDRESS_IPV4 )
        {
            netcode_socket_send_packet( &server->socket_holder.ipv4, to, packet_data, packet_bytes );
        }
        else if ( to->type == NETCODE_ADDRESS_IPV6 )
        {
            netcode_socket_send_packet( &server->socket_holder.ipv6, to, packet_data, packet_bytes );
        }
    }

    server->global_sequence++;
}

void netcode_server_send_client_packet( struct netcode_server_t * server, void * packet, int client_index )
{
    netcode_assert( server );
    netcode_assert( packet );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( server->client_connected[client_index] );
    netcode_assert( !server->client_loopback[client_index] );

    uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];

    if ( !netcode_encryption_manager_touch( &server->encryption_manager, 
                                            server->client_encryption_index[client_index], 
                                            &server->client_address[client_index], 
                                            server->time ) )
    {
        netcode_printf( NETCODE_LOG_LEVEL_ERROR, "error: encryption mapping is out of date for client %d\n", client_index );
        return;
    }

    uint8_t * packet_key = netcode_encryption_manager_get_send_key( &server->encryption_manager, server->client_encryption_index[client_index] );

    int packet_bytes = netcode_write_packet( packet, packet_data, NETCODE_MAX_PACKET_BYTES, server->client_sequence[client_index], packet_key, server->config.protocol_id );

    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_BYTES );

    if ( server->config.network_simulator )
    {
        netcode_network_simulator_send_packet( server->config.network_simulator, &server->address, &server->client_address[client_index], packet_data, packet_bytes );
    }
    else
    {
        if ( server->config.override_send_and_receive )
        {
            server->config.send_packet_override( server->config.callback_context, &server->client_address[client_index], packet_data, packet_bytes );
        }
        else
        {
            if ( server->client_address[client_index].type == NETCODE_ADDRESS_IPV4 )
            {
                netcode_socket_send_packet( &server->socket_holder.ipv4, &server->client_address[client_index], packet_data, packet_bytes );
            }
            else if ( server->client_address[client_index].type == NETCODE_ADDRESS_IPV6 )
            {
                netcode_socket_send_packet( &server->socket_holder.ipv6, &server->client_address[client_index], packet_data, packet_bytes );
            }
        }
    }

    server->client_sequence[client_index]++;

    server->client_last_packet_send_time[client_index] = server->time;
}

void netcode_server_disconnect_client_internal( struct netcode_server_t * server, int client_index, int send_disconnect_packets )
{
    netcode_assert( server );
    netcode_assert( server->running );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( server->client_connected[client_index] );
    netcode_assert( !server->client_loopback[client_index] );
    netcode_assert( server->encryption_manager.client_index[server->client_encryption_index[client_index]] == client_index );

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server disconnected client %d\n", client_index );

    if ( server->config.connect_disconnect_callback )
    {
        server->config.connect_disconnect_callback( server->config.callback_context, client_index, 0 );
    }

    if ( send_disconnect_packets )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server sent disconnect packets to client %d\n", client_index );

        int i;
        for ( i = 0; i < NETCODE_NUM_DISCONNECT_PACKETS; ++i )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server sent disconnect packet %d\n", i );

            struct netcode_connection_disconnect_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_DISCONNECT_PACKET;

            netcode_server_send_client_packet( server, &packet, client_index );
        }
    }

    while ( 1 )
    {
        void * packet = netcode_packet_queue_pop( &server->client_packet_queue[client_index], NULL );
        if ( !packet )
            break;
        server->config.free_function( server->config.allocator_context, packet );
    }

    netcode_packet_queue_clear( &server->client_packet_queue[client_index] );

    netcode_replay_protection_reset( &server->client_replay_protection[client_index] );

    server->encryption_manager.client_index[server->client_encryption_index[client_index]] = -1;

    netcode_encryption_manager_remove_encryption_mapping( &server->encryption_manager, &server->client_address[client_index], server->time );

    server->client_connected[client_index] = 0;
    server->client_confirmed[client_index] = 0;
    server->client_id[client_index] = 0;
    server->client_sequence[client_index] = 0;
    server->client_last_packet_send_time[client_index] = 0.0;
    server->client_last_packet_receive_time[client_index] = 0.0;
    memset( &server->client_address[client_index], 0, sizeof( struct netcode_address_t ) );
    server->client_encryption_index[client_index] = -1;
    memset( server->client_user_data[client_index], 0, NETCODE_USER_DATA_BYTES );

    server->num_connected_clients--;

    netcode_assert( server->num_connected_clients >= 0 );
}

void netcode_server_disconnect_client( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );

    if ( !server->running )
        return;

    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( server->client_loopback[client_index] == 0 );

    if ( !server->client_connected[client_index] )
        return;

    if ( server->client_loopback[client_index] )
        return;

    netcode_server_disconnect_client_internal( server, client_index, 1 );
}

void netcode_server_disconnect_all_clients( struct netcode_server_t * server )
{
    netcode_assert( server );

    if ( !server->running )
        return;

    int i;
    for ( i = 0; i < server->max_clients; ++i )
    {
        if ( server->client_connected[i] && !server->client_loopback[i] )
        {
            netcode_server_disconnect_client_internal( server, i, 1 );
        }
    }
}

void netcode_server_stop( struct netcode_server_t * server )
{
    netcode_assert( server );

    if ( !server->running )
        return;

    netcode_server_disconnect_all_clients( server );

    server->running = 0;
    server->max_clients = 0;
    server->num_connected_clients = 0;

    server->global_sequence = 0;
    server->challenge_sequence = 0;
    memset( server->challenge_key, 0, NETCODE_KEY_BYTES );

    netcode_connect_token_entries_reset( server->connect_token_entries );

    netcode_encryption_manager_reset( &server->encryption_manager );

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server stopped\n" );
}

int netcode_server_find_client_index_by_id( struct netcode_server_t * server, uint64_t client_id )
{
    netcode_assert( server );

    int i;
    for ( i = 0; i < server->max_clients; ++i )
    {   
        if ( server->client_connected[i] && server->client_id[i] == client_id )
            return i;
    }

    return -1;
}

int netcode_server_find_client_index_by_address( struct netcode_server_t * server, struct netcode_address_t * address )
{
    netcode_assert( server );
    netcode_assert( address );

    if ( address->type == 0 )
        return -1;

    int i;
    for ( i = 0; i < server->max_clients; ++i )
    {   
        if ( server->client_connected[i] && netcode_address_equal( &server->client_address[i], address ) )
            return i;
    }

    return -1;
}

void netcode_server_process_connection_request_packet( struct netcode_server_t * server, 
                                                       struct netcode_address_t * from, 
                                                       struct netcode_connection_request_packet_t * packet )
{
    netcode_assert( server );

    (void) from;

    struct netcode_connect_token_private_t connect_token_private;
    if ( netcode_read_connect_token_private( packet->connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, &connect_token_private ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. failed to read connect token\n" );
        return;
    }

    int found_server_address = 0;
    int i;
    for ( i = 0; i < connect_token_private.num_server_addresses; ++i )
    {
        if ( netcode_address_equal( &server->address, &connect_token_private.server_addresses[i] ) )
        {
            found_server_address = 1;
        }
    }
    if ( !found_server_address )
    {   
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. server address not in connect token whitelist\n" );
        return;
    }

    if ( netcode_server_find_client_index_by_address( server, from ) != -1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. a client with this address is already connected\n" );
        return;
    }

    if ( netcode_server_find_client_index_by_id( server, connect_token_private.client_id ) != -1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. a client with this id is already connected\n" );
        return;
    }

    if ( !netcode_connect_token_entries_find_or_add( server->connect_token_entries, 
                                                     from, 
                                                     packet->connect_token_data + NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - NETCODE_MAC_BYTES, 
                                                     server->time ) )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. connect token has already been used\n" );
        return;
    }

    if ( server->num_connected_clients == server->max_clients )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server denied connection request. server is full\n" );

        struct netcode_connection_denied_packet_t p;
        p.packet_type = NETCODE_CONNECTION_DENIED_PACKET;
        
        netcode_server_send_global_packet( server, &p, from, connect_token_private.server_to_client_key );

        return;
    }

    double expire_time = ( connect_token_private.timeout_seconds >= 0 ) ? server->time + connect_token_private.timeout_seconds : -1.0;

    if ( !netcode_encryption_manager_add_encryption_mapping( &server->encryption_manager, 
                                                             from, 
                                                             connect_token_private.server_to_client_key, 
                                                             connect_token_private.client_to_server_key, 
                                                             server->time, 
                                                             expire_time,
                                                             connect_token_private.timeout_seconds ) )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. failed to add encryption mapping\n" );
        return;
    }

    struct netcode_challenge_token_t challenge_token;
    challenge_token.client_id = connect_token_private.client_id;
    memcpy( challenge_token.user_data, connect_token_private.user_data, NETCODE_USER_DATA_BYTES );

    struct netcode_connection_challenge_packet_t challenge_packet;
    challenge_packet.packet_type = NETCODE_CONNECTION_CHALLENGE_PACKET;
    challenge_packet.challenge_token_sequence = server->challenge_sequence;
    netcode_write_challenge_token( &challenge_token, challenge_packet.challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );
    if ( netcode_encrypt_challenge_token( challenge_packet.challenge_token_data, 
                                          NETCODE_CHALLENGE_TOKEN_BYTES, 
                                          server->challenge_sequence, 
                                          server->challenge_key ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection request. failed to encrypt challenge token\n" );
        return;
    }

    server->challenge_sequence++;

    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server sent connection challenge packet\n" );

    netcode_server_send_global_packet( server, &challenge_packet, from, connect_token_private.server_to_client_key );
}

int netcode_server_find_free_client_index( struct netcode_server_t * server )
{
    netcode_assert( server );

    int i;
    for ( i = 0; i < server->max_clients; ++i )
    {
        if ( !server->client_connected[i] )
            return i;
    }

    return -1;
}

void netcode_server_connect_client( struct netcode_server_t * server, 
                                    int client_index, 
                                    struct netcode_address_t * address, 
                                    uint64_t client_id, 
                                    int encryption_index,
                                    int timeout_seconds, 
                                    void * user_data )
{
    netcode_assert( server );
    netcode_assert( server->running );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( address );
    netcode_assert( encryption_index != -1 );
    netcode_assert( user_data );
    netcode_assert( server->encryption_manager.client_index[encryption_index] == -1 );

    server->num_connected_clients++;

    netcode_assert( server->num_connected_clients <= server->max_clients );

    netcode_assert( server->client_connected[client_index] == 0 );

    netcode_encryption_manager_set_expire_time( &server->encryption_manager, encryption_index, -1.0 );
    
    server->encryption_manager.client_index[encryption_index] = client_index;

    server->client_connected[client_index] = 1;
    server->client_timeout[client_index] = timeout_seconds;
    server->client_encryption_index[client_index] = encryption_index;
    server->client_id[client_index] = client_id;
    server->client_sequence[client_index] = 0;
    server->client_address[client_index] = *address;
    server->client_last_packet_send_time[client_index] = server->time;
    server->client_last_packet_receive_time[client_index] = server->time;
    memcpy( server->client_user_data[client_index], user_data, NETCODE_USER_DATA_BYTES );

    char address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server accepted client %s %.16" PRIx64 " in slot %d\n", 
        netcode_address_to_string( address, address_string ), client_id, client_index );

    struct netcode_connection_keep_alive_packet_t packet;
    packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
    packet.client_index = client_index;
    packet.max_clients = server->max_clients;

    netcode_server_send_client_packet( server, &packet, client_index );

    if ( server->config.connect_disconnect_callback )
    {
        server->config.connect_disconnect_callback( server->config.callback_context, client_index, 1 );
    }
}

void netcode_server_process_connection_response_packet( struct netcode_server_t * server, 
                                                        struct netcode_address_t * from, 
                                                        struct netcode_connection_response_packet_t * packet, 
                                                        int encryption_index )
{
    netcode_assert( server );

    if ( netcode_decrypt_challenge_token( packet->challenge_token_data, 
                                          NETCODE_CHALLENGE_TOKEN_BYTES, 
                                          packet->challenge_token_sequence, 
                                          server->challenge_key ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection response. failed to decrypt challenge token\n" );
        return;
    }

    struct netcode_challenge_token_t challenge_token;
    if ( netcode_read_challenge_token( packet->challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES, &challenge_token ) != NETCODE_OK )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection response. failed to read challenge token\n" );
        return;
    }

    uint8_t * packet_send_key = netcode_encryption_manager_get_send_key( &server->encryption_manager, encryption_index );

    if ( !packet_send_key )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection response. no packet send key\n" );
        return;
    }

    if ( netcode_server_find_client_index_by_address( server, from ) != -1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection response. a client with this address is already connected\n" );
        return;
    }

    if ( netcode_server_find_client_index_by_id( server, challenge_token.client_id ) != -1 )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server ignored connection response. a client with this id is already connected\n" );
        return;
    }

    if ( server->num_connected_clients == server->max_clients )
    {
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server denied connection response. server is full\n" );

        struct netcode_connection_denied_packet_t p;
        p.packet_type = NETCODE_CONNECTION_DENIED_PACKET;

        netcode_server_send_global_packet( server, &p, from, packet_send_key );

        return;
    }

    int client_index = netcode_server_find_free_client_index( server );

    netcode_assert( client_index != -1 );

    int timeout_seconds = netcode_encryption_manager_get_timeout( &server->encryption_manager, encryption_index );

    netcode_server_connect_client( server, client_index, from, challenge_token.client_id, encryption_index, timeout_seconds, challenge_token.user_data );
}

void netcode_server_process_packet_internal( struct netcode_server_t * server, 
                                             struct netcode_address_t * from, 
                                             void * packet, 
                                             uint64_t sequence, 
                                             int encryption_index, 
                                             int client_index )
{
    netcode_assert( server );
    netcode_assert( packet );

    (void) from;
    (void) sequence;

    uint8_t packet_type = ( (uint8_t*) packet ) [0];

    switch ( packet_type )
    {
        case NETCODE_CONNECTION_REQUEST_PACKET:
        {    
            if ( ( server->flags & NETCODE_SERVER_FLAG_IGNORE_CONNECTION_REQUEST_PACKETS ) == 0 )
            {
                char from_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received connection request from %s\n", netcode_address_to_string( from, from_address_string ) );
                netcode_server_process_connection_request_packet( server, from, (struct netcode_connection_request_packet_t*) packet );
            }
        }
        break;

        case NETCODE_CONNECTION_RESPONSE_PACKET:
        {    
            if ( ( server->flags & NETCODE_SERVER_FLAG_IGNORE_CONNECTION_RESPONSE_PACKETS ) == 0 )
            {
                char from_address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received connection response from %s\n", netcode_address_to_string( from, from_address_string ) );
                netcode_server_process_connection_response_packet( server, from, (struct netcode_connection_response_packet_t*) packet, encryption_index );
            }
        }
        break;

        case NETCODE_CONNECTION_KEEP_ALIVE_PACKET:
        {
            if ( client_index != -1 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received connection keep alive packet from client %d\n", client_index );
                server->client_last_packet_receive_time[client_index] = server->time;
                if ( !server->client_confirmed[client_index] )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server confirmed connection with client %d\n", client_index );
                    server->client_confirmed[client_index] = 1;
                }
            }
        }
        break;

        case NETCODE_CONNECTION_PAYLOAD_PACKET:
        {
            if ( client_index != -1 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received connection payload packet from client %d\n", client_index );
                server->client_last_packet_receive_time[client_index] = server->time;
                if ( !server->client_confirmed[client_index] )
                {
                    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server confirmed connection with client %d\n", client_index );
                    server->client_confirmed[client_index] = 1;
                }
                netcode_packet_queue_push( &server->client_packet_queue[client_index], packet, sequence );
                return;
            }
        }
        break;

        case NETCODE_CONNECTION_DISCONNECT_PACKET:
        {
            if ( client_index != -1 )
            {
                netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server received disconnect packet from client %d\n", client_index );
                netcode_server_disconnect_client_internal( server, client_index, 0 );
           }
        }
        break;

        default:
            break;
    }

    server->config.free_function( server->config.allocator_context, packet );
}

void netcode_server_process_packet( struct netcode_server_t * server, struct netcode_address_t * from, uint8_t * packet_data, int packet_bytes )
{
    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packets, 0, sizeof( allowed_packets ) );
    allowed_packets[NETCODE_CONNECTION_REQUEST_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_RESPONSE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_KEEP_ALIVE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_PAYLOAD_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_DISCONNECT_PACKET] = 1;

    uint64_t current_timestamp = (uint64_t) time( NULL );

    uint64_t sequence;

    int encryption_index = -1;
    int client_index = netcode_server_find_client_index_by_address( server, from );
    if ( client_index != -1 )
    {
        netcode_assert( client_index >= 0 );
        netcode_assert( client_index < server->max_clients );
        encryption_index = server->client_encryption_index[client_index];
    }
    else
    {
        encryption_index = netcode_encryption_manager_find_encryption_mapping( &server->encryption_manager, from, server->time );
    }
    
    uint8_t * read_packet_key = netcode_encryption_manager_get_receive_key( &server->encryption_manager, encryption_index );

    if ( !read_packet_key && packet_data[0] != 0 )
    {
        char address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server could not process packet because no encryption mapping exists for %s\n", netcode_address_to_string( from, address_string ) );
        return;
    }

    void * packet = netcode_read_packet( packet_data, 
                                         packet_bytes, 
                                         &sequence, 
                                         read_packet_key, 
                                         server->config.protocol_id, 
                                         current_timestamp, 
                                         server->config.private_key, 
                                         allowed_packets, 
                                         ( client_index != -1 ) ? &server->client_replay_protection[client_index] : NULL, 
                                         server->config.allocator_context, 
                                         server->config.allocate_function );

    if ( !packet )
        return;

    netcode_server_process_packet_internal( server, from, packet, sequence, encryption_index, client_index );
}

void netcode_server_read_and_process_packet( struct netcode_server_t * server, 
                                             struct netcode_address_t * from, 
                                             uint8_t * packet_data, 
                                             int packet_bytes, 
                                             uint64_t current_timestamp, 
                                             uint8_t * allowed_packets )
{
    if ( !server->running )
        return;

    if ( packet_bytes <= 1 )
        return;

    uint64_t sequence;

    int encryption_index = -1;
    int client_index = netcode_server_find_client_index_by_address( server, from );
    if ( client_index != -1 )
    {
        netcode_assert( client_index >= 0 );
        netcode_assert( client_index < server->max_clients );
        encryption_index = server->client_encryption_index[client_index];
    }
    else
    {
        encryption_index = netcode_encryption_manager_find_encryption_mapping( &server->encryption_manager, from, server->time );
    }
    
    uint8_t * read_packet_key = netcode_encryption_manager_get_receive_key( &server->encryption_manager, encryption_index );

    if ( !read_packet_key && packet_data[0] != 0 )
    {
        char address_string[NETCODE_MAX_ADDRESS_STRING_LENGTH];
        netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server could not process packet because no encryption mapping exists for %s\n", netcode_address_to_string( from, address_string ) );
        return;
    }

    void * packet = netcode_read_packet( packet_data, 
                                         packet_bytes, 
                                         &sequence, 
                                         read_packet_key, 
                                         server->config.protocol_id, 
                                         current_timestamp, 
                                         server->config.private_key, 
                                         allowed_packets, 
                                         ( client_index != -1 ) ? &server->client_replay_protection[client_index] : NULL, 
                                         server->config.allocator_context, 
                                         server->config.allocate_function );

    if ( !packet )
        return;

    netcode_server_process_packet_internal( server, from, packet, sequence, encryption_index, client_index );
}

void netcode_server_receive_packets( struct netcode_server_t * server )
{
    netcode_assert( server );

    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packets, 0, sizeof( allowed_packets ) );
    allowed_packets[NETCODE_CONNECTION_REQUEST_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_RESPONSE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_KEEP_ALIVE_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_PAYLOAD_PACKET] = 1;
    allowed_packets[NETCODE_CONNECTION_DISCONNECT_PACKET] = 1;

    uint64_t current_timestamp = (uint64_t) time( NULL );

    if ( !server->config.network_simulator )
    {
        // process packets received from socket

        while ( 1 )
        {
            struct netcode_address_t from;
            
            uint8_t packet_data[NETCODE_MAX_PACKET_BYTES];
            
            int packet_bytes = 0;
            
            if ( server->config.override_send_and_receive )
            {
                packet_bytes = server->config.receive_packet_override( server->config.callback_context, &from, packet_data, NETCODE_MAX_PACKET_BYTES );
            }
            else
            {
                if (server->socket_holder.ipv4.handle != 0)
                    packet_bytes = netcode_socket_receive_packet( &server->socket_holder.ipv4, &from, packet_data, NETCODE_MAX_PACKET_BYTES );

                if ( packet_bytes == 0 && server->socket_holder.ipv6.handle != 0)
                    packet_bytes = netcode_socket_receive_packet( &server->socket_holder.ipv6, &from, packet_data, NETCODE_MAX_PACKET_BYTES );
            }

            if ( packet_bytes == 0 )
                break;

            netcode_server_read_and_process_packet( server, &from, packet_data, packet_bytes, current_timestamp, allowed_packets );
        }
    }
    else
    {
        // process packets received from network simulator

        int num_packets_received = netcode_network_simulator_receive_packets( server->config.network_simulator, 
                                                                              &server->address, 
                                                                              NETCODE_SERVER_MAX_RECEIVE_PACKETS, 
                                                                              server->receive_packet_data, 
                                                                              server->receive_packet_bytes, 
                                                                              server->receive_from );

        int i;
        for ( i = 0; i < num_packets_received; ++i )
        {
            netcode_server_read_and_process_packet( server, 
                                                    &server->receive_from[i], 
                                                    server->receive_packet_data[i], 
                                                    server->receive_packet_bytes[i], 
                                                    current_timestamp, 
                                                    allowed_packets );

            server->config.free_function( server->config.allocator_context, server->receive_packet_data[i] );
        }
    }
}

void netcode_server_send_packets( struct netcode_server_t * server )
{
    netcode_assert( server );

    if ( !server->running )
        return;

    int i;
    for ( i = 0; i < server->max_clients; ++i )
    {
        if ( server->client_connected[i] && !server->client_loopback[i] &&
             ( server->client_last_packet_send_time[i] + ( 1.0 / NETCODE_PACKET_SEND_RATE ) <= server->time ) )
        {
            netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server sent connection keep alive packet to client %d\n", i );
            struct netcode_connection_keep_alive_packet_t packet;
            packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
            packet.client_index = i;
            packet.max_clients = server->max_clients;
            netcode_server_send_client_packet( server, &packet, i );
        }
    }
}

void netcode_server_check_for_timeouts( struct netcode_server_t * server )
{
    netcode_assert( server );

    if ( !server->running )
        return;

    int i;
    for ( i = 0; i < server->max_clients; ++i )
    {
        if ( server->client_connected[i] && server->client_timeout[i] > 0 && !server->client_loopback[i] &&
             ( server->client_last_packet_receive_time[i] + server->client_timeout[i] <= server->time ) )
        {
            netcode_printf( NETCODE_LOG_LEVEL_INFO, "server timed out client %d\n", i );
            netcode_server_disconnect_client_internal( server, i, 0 );
        }
    }
}

int netcode_server_client_connected( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );

    if ( !server->running )
        return 0;

    if ( client_index < 0 || client_index >= server->max_clients )
        return 0;

    return server->client_connected[client_index];
}

uint64_t netcode_server_client_id( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );

    if ( !server->running )
        return 0;

    if ( client_index < 0 || client_index >= server->max_clients )
        return 0;

    return server->client_id[client_index];
}

struct netcode_address_t * netcode_server_client_address( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );

    if (!server->running)
        return NULL;

    if (client_index < 0 || client_index >= server->max_clients)
        return NULL;

    return &server->client_address[client_index];
}

uint64_t netcode_server_next_packet_sequence( struct netcode_server_t * server, int client_index )
{
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    if ( !server->client_connected[client_index] )
        return 0;
    return server->client_sequence[client_index];    
}

void netcode_server_send_packet( struct netcode_server_t * server, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes )
{
    netcode_assert( server );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes >= 0 );
    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_SIZE );

    if ( !server->running )
        return;

    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    if ( !server->client_connected[client_index] )
        return;

    if ( !server->client_loopback[client_index] )
    {
        uint8_t buffer[NETCODE_MAX_PAYLOAD_BYTES*2];

        struct netcode_connection_payload_packet_t * packet = (struct netcode_connection_payload_packet_t*) buffer;

        packet->packet_type = NETCODE_CONNECTION_PAYLOAD_PACKET;
        packet->payload_bytes = packet_bytes;
        memcpy( packet->payload_data, packet_data, packet_bytes );

        if ( !server->client_confirmed[client_index] )
        {
            struct netcode_connection_keep_alive_packet_t keep_alive_packet;
            keep_alive_packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
            keep_alive_packet.client_index = client_index;
            keep_alive_packet.max_clients = server->max_clients;
            netcode_server_send_client_packet( server, &keep_alive_packet, client_index );
        }

        netcode_server_send_client_packet( server, packet, client_index );
    }
    else
    {
        netcode_assert( server->config.send_loopback_packet_callback );

        server->config.send_loopback_packet_callback( server->config.callback_context,
                                                      client_index, 
                                                      packet_data, 
                                                      packet_bytes, 
                                                      server->client_sequence[client_index]++ );

        server->client_last_packet_send_time[client_index] = server->time;
    }
}

uint8_t * netcode_server_receive_packet( struct netcode_server_t * server, int client_index, int * packet_bytes, uint64_t * packet_sequence )
{
    netcode_assert( server );
    netcode_assert( packet_bytes );

    if ( !server->running )
        return NULL;

    if ( !server->client_connected[client_index] )
        return NULL;

    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );

    struct netcode_connection_payload_packet_t * packet = (struct netcode_connection_payload_packet_t*) 
        netcode_packet_queue_pop( &server->client_packet_queue[client_index], packet_sequence );
    
    if ( packet )
    {
        netcode_assert( packet->packet_type == NETCODE_CONNECTION_PAYLOAD_PACKET );
        *packet_bytes = packet->payload_bytes;
        netcode_assert( *packet_bytes >= 0 );
        netcode_assert( *packet_bytes <= NETCODE_MAX_PAYLOAD_BYTES );
        return (uint8_t*) &packet->payload_data;
    }
    else
    {
        return NULL;
    }
}

void netcode_server_free_packet( struct netcode_server_t * server, void * packet )
{
    netcode_assert( server );
    netcode_assert( packet );
    (void) server;
    int offset = offsetof( struct netcode_connection_payload_packet_t, payload_data );
    server->config.free_function( server->config.allocator_context, ( (uint8_t*) packet ) - offset );
}

int netcode_server_num_connected_clients( struct netcode_server_t * server )
{
    netcode_assert( server );
    return server->num_connected_clients;
}

void * netcode_server_client_user_data( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    return server->client_user_data[client_index];
}

int netcode_server_running( struct netcode_server_t * server )
{
    netcode_assert( server );
    return server->running;
}

int netcode_server_max_clients( struct netcode_server_t * server )
{
    return server->max_clients;
}

void netcode_server_update( struct netcode_server_t * server, double time )
{
    netcode_assert( server );
    server->time = time;
    netcode_server_receive_packets( server );
    netcode_server_send_packets( server );
    netcode_server_check_for_timeouts( server );
}

void netcode_server_connect_loopback_client( struct netcode_server_t * server, int client_index, uint64_t client_id, NETCODE_CONST uint8_t * user_data )
{
    netcode_assert( server );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( server->running );
    netcode_assert( !server->client_connected[client_index] );

    server->num_connected_clients++;

    netcode_assert( server->num_connected_clients <= server->max_clients );

    server->client_loopback[client_index] = 1;
    server->client_connected[client_index] = 1;
    server->client_confirmed[client_index] = 1;
    server->client_encryption_index[client_index] = -1;
    server->client_id[client_index] = client_id;
    server->client_sequence[client_index] = 0;
    memset( &server->client_address[client_index], 0, sizeof( struct netcode_address_t ) );
    server->client_last_packet_send_time[client_index] = server->time;
    server->client_last_packet_receive_time[client_index] = server->time;

    if ( user_data )
    {
        memcpy( server->client_user_data[client_index], user_data, NETCODE_USER_DATA_BYTES );
    }
    else
    {
        memset( server->client_user_data[client_index], 0, NETCODE_USER_DATA_BYTES );
    }

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server connected loopback client %.16" PRIx64 " in slot %d\n", client_id, client_index );

    if ( server->config.connect_disconnect_callback )
    {
        server->config.connect_disconnect_callback( server->config.callback_context, client_index, 1 );
    }
}

void netcode_server_disconnect_loopback_client( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( server->running );
    netcode_assert( server->client_connected[client_index] );
    netcode_assert( server->client_loopback[client_index] );

    netcode_printf( NETCODE_LOG_LEVEL_INFO, "server disconnected loopback client %d\n", client_index );

    if ( server->config.connect_disconnect_callback )
    {
        server->config.connect_disconnect_callback( server->config.callback_context, client_index, 0 );
    }

    while ( 1 )
    {
        void * packet = netcode_packet_queue_pop( &server->client_packet_queue[client_index], NULL );
        if ( !packet )
            break;
        server->config.free_function( server->config.allocator_context, packet );
    }

    netcode_packet_queue_clear( &server->client_packet_queue[client_index] );

    server->client_connected[client_index] = 0;
    server->client_loopback[client_index] = 0;
    server->client_confirmed[client_index] = 0;
    server->client_id[client_index] = 0;
    server->client_sequence[client_index] = 0;
    server->client_last_packet_send_time[client_index] = 0.0;
    server->client_last_packet_receive_time[client_index] = 0.0;
    memset( &server->client_address[client_index], 0, sizeof( struct netcode_address_t ) );
    server->client_encryption_index[client_index] = -1;
    memset( server->client_user_data[client_index], 0, NETCODE_USER_DATA_BYTES );

    server->num_connected_clients--;

    netcode_assert( server->num_connected_clients >= 0 );
}

int netcode_server_client_loopback( struct netcode_server_t * server, int client_index )
{
    netcode_assert( server );
    netcode_assert( server->running );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    return server->client_loopback[client_index];
}

void netcode_server_process_loopback_packet( struct netcode_server_t * server, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence )
{
    netcode_assert( server );
    netcode_assert( client_index >= 0 );
    netcode_assert( client_index < server->max_clients );
    netcode_assert( packet_data );
    netcode_assert( packet_bytes >= 0 );
    netcode_assert( packet_bytes <= NETCODE_MAX_PACKET_SIZE );
    netcode_assert( server->client_connected[client_index] );
    netcode_assert( server->client_loopback[client_index] );
    netcode_assert( server->running );

    struct netcode_connection_payload_packet_t * packet = netcode_create_payload_packet( packet_bytes, server->config.allocator_context, server->config.allocate_function );
    if ( !packet )
        return;

    memcpy( packet->payload_data, packet_data, packet_bytes );

    netcode_printf( NETCODE_LOG_LEVEL_DEBUG, "server processing loopback packet from client %d\n", client_index );

    server->client_last_packet_receive_time[client_index] = server->time;

    netcode_packet_queue_push( &server->client_packet_queue[client_index], packet, packet_sequence );
}

uint16_t netcode_server_get_port( struct netcode_server_t * server )
{
    netcode_assert( server );
    return server->address.type == NETCODE_ADDRESS_IPV4 ? server->socket_holder.ipv4.address.port : server->socket_holder.ipv6.address.port;
}

// ----------------------------------------------------------------

int netcode_generate_connect_token( int num_server_addresses, 
                                    NETCODE_CONST char ** public_server_addresses, 
                                    NETCODE_CONST char ** internal_server_addresses, 
                                    int expire_seconds, 
                                    int timeout_seconds,
                                    uint64_t client_id, 
                                    uint64_t protocol_id, 
                                    NETCODE_CONST uint8_t * private_key, 
                                    uint8_t * user_data, 
                                    uint8_t * output_buffer )
{
    netcode_assert( num_server_addresses > 0 );
    netcode_assert( num_server_addresses <= NETCODE_MAX_SERVERS_PER_CONNECT );
    netcode_assert( public_server_addresses );
    netcode_assert( internal_server_addresses );
    netcode_assert( private_key );
    netcode_assert( user_data );
    netcode_assert( output_buffer );

    // parse public server addresses

    struct netcode_address_t parsed_public_server_addresses[NETCODE_MAX_SERVERS_PER_CONNECT];
    int i;
    for ( i = 0; i < num_server_addresses; ++i )
    {
        if ( netcode_parse_address( public_server_addresses[i], &parsed_public_server_addresses[i] ) != NETCODE_OK )
        {
            return NETCODE_ERROR;
        }
    }

    // parse internal server addresses

     struct netcode_address_t parsed_internal_server_addresses[NETCODE_MAX_SERVERS_PER_CONNECT];
    for ( i = 0; i < num_server_addresses; ++i )
    {
        if ( netcode_parse_address( internal_server_addresses[i], &parsed_internal_server_addresses[i] ) != NETCODE_OK )
        {
            return NETCODE_ERROR;
        }
    }

    // generate a connect token

    uint8_t nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    netcode_generate_nonce(nonce);

    struct netcode_connect_token_private_t connect_token_private;
    netcode_generate_connect_token_private( &connect_token_private, client_id, timeout_seconds, num_server_addresses, parsed_internal_server_addresses, user_data );

    // write it to a buffer

    uint8_t connect_token_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
    netcode_write_connect_token_private( &connect_token_private, connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    // encrypt the buffer

    uint64_t create_timestamp = time( NULL );
    uint64_t expire_timestamp = ( expire_seconds >= 0 ) ? ( create_timestamp + expire_seconds ) : 0xFFFFFFFFFFFFFFFFULL;
    if ( netcode_encrypt_connect_token_private( connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, NETCODE_VERSION_INFO, protocol_id, expire_timestamp, nonce, private_key ) != NETCODE_OK )
        return NETCODE_ERROR;

    // wrap a connect token around the private connect token data

    struct netcode_connect_token_t connect_token;
    memcpy( connect_token.version_info, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
    connect_token.protocol_id = protocol_id;
    connect_token.create_timestamp = create_timestamp;
    connect_token.expire_timestamp = expire_timestamp;
    memcpy( connect_token.nonce, nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
    memcpy( connect_token.private_data, connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
    connect_token.num_server_addresses = num_server_addresses;
    for ( i = 0; i < num_server_addresses; ++i )
        connect_token.server_addresses[i] = parsed_public_server_addresses[i];
    memcpy( connect_token.client_to_server_key, connect_token_private.client_to_server_key, NETCODE_KEY_BYTES );
    memcpy( connect_token.server_to_client_key, connect_token_private.server_to_client_key, NETCODE_KEY_BYTES );
    connect_token.timeout_seconds = timeout_seconds;

    // write the connect token to the output buffer

    netcode_write_connect_token( &connect_token, output_buffer, NETCODE_CONNECT_TOKEN_BYTES );

    return NETCODE_OK;
}

// ---------------------------------------------------------------

#if __APPLE__

// MacOS

#include <unistd.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

void netcode_sleep( double time )
{
    struct timespec ts;
    ts.tv_sec = (time_t) time;
    ts.tv_nsec = (long) ((time - (double) ( ts.tv_sec )) * 1000000000.0);
    nanosleep( &ts, NULL );
}

static uint64_t start = 0;
static mach_timebase_info_data_t timebase_info;

double netcode_time()
{
    if ( start == 0 )
    {
        mach_timebase_info( &timebase_info );
        start = mach_absolute_time();
        return 0.0;
    }
    uint64_t current = mach_absolute_time();
    return ( (double) ( current - start ) ) * ( (double) timebase_info.numer ) / ( (double) timebase_info.denom ) / 1000000000.0;
}

#elif __linux

// linux

#include <unistd.h>

void netcode_sleep( double time )
{
    struct timespec ts;
    ts.tv_sec = (time_t) time;
    ts.tv_nsec = (long) ((time - (double) ( ts.tv_sec )) * 1000000000.0);
    nanosleep( &ts, NULL );
}

double netcode_time()
{
    static double start = -1;
    if ( start == -1 )
    {
        struct timespec ts;
        clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
        start = ts.tv_sec + ( (double) ( ts.tv_nsec ) ) / 1000000000.0;
        return 0.0;
    }
    struct timespec ts;
    clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
    double current = ts.tv_sec + ( (double) ( ts.tv_nsec ) ) / 1000000000.0;
    return current - start;
}

#elif defined( _WIN32 )

// windows

#define NOMINMAX
#include <windows.h>

void netcode_sleep( double time )
{
    int milliseconds = (int) ( time * 1000 );
    Sleep( milliseconds );
}

static int timer_initialized = 0;
static LARGE_INTEGER timer_frequency;
static LARGE_INTEGER timer_start;

double netcode_time()
{
    if ( !timer_initialized )
    {
        QueryPerformanceFrequency( &timer_frequency );
        QueryPerformanceCounter( &timer_start );
        timer_initialized = 1;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter( &now );
    return ( (double) ( now.QuadPart - timer_start.QuadPart ) ) / ( (double) ( timer_frequency.QuadPart ) );
}

#else

#error unsupported platform!

#endif

// ---------------------------------------------------------------

#if NETCODE_ENABLE_TESTS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>

static void check_handler( NETCODE_CONST char * condition, 
                           NETCODE_CONST char * function,
                           NETCODE_CONST char * file,
                           int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#ifdef NETCODE_DEBUG
    #if defined( __GNUC__ )
        __builtin_trap();
    #elif defined( _MSC_VER )
        __debugbreak();
    #endif
#endif
    exit( 1 );
}

#define check( condition )                                                                                      \
do                                                                                                              \
{                                                                                                               \
    if ( !(condition) )                                                                                         \
    {                                                                                                           \
        check_handler( #condition, (NETCODE_CONST char*) __FUNCTION__, (char*) __FILE__, __LINE__ );            \
    }                                                                                                           \
} while(0)

static void test_queue()
{
    struct netcode_packet_queue_t queue;

    netcode_packet_queue_init( &queue, NULL, NULL, NULL );

    check( queue.num_packets == 0 );
    check( queue.start_index == 0 );

    // attempting to pop a packet off an empty queue should return NULL

    check( netcode_packet_queue_pop( &queue, NULL ) == NULL );

    // add some packets to the queue and make sure they pop off in the correct order
    {
        #define NUM_PACKETS 100

        void * packets[NUM_PACKETS];

        int i;
        for ( i = 0; i < NUM_PACKETS; ++i )
        {
            packets[i] = malloc( (i+1) * 256 );
            check( netcode_packet_queue_push( &queue, packets[i], (uint64_t) i ) == 1 );
        }

        check( queue.num_packets == NUM_PACKETS );

        for ( i = 0; i < NUM_PACKETS; ++i )
        {
            uint64_t sequence = 0;
            void * packet = netcode_packet_queue_pop( &queue, &sequence );
            check( sequence == (uint64_t) i ) ;
            check( packet == packets[i] );
            free( packet );
        }
    }

    // after all entries are popped off, the queue is empty, so calls to pop should return NULL

    check( queue.num_packets == 0 );

    check( netcode_packet_queue_pop( &queue, NULL ) == NULL );

    // test that the packet queue can be filled to max capacity

    void * packets[NETCODE_PACKET_QUEUE_SIZE];

    int i;
    for ( i = 0; i < NETCODE_PACKET_QUEUE_SIZE; ++i )
    {
        packets[i] = malloc( i * 256 );
        check( netcode_packet_queue_push( &queue, packets[i], (uint64_t) i ) == 1 );
    }

    check( queue.num_packets == NETCODE_PACKET_QUEUE_SIZE );

    // when the queue is full, attempting to push a packet should fail and return 0

    check( netcode_packet_queue_push( &queue, malloc( 100 ), 0 ) == 0 );

    // make sure all packets pop off in the correct order

    for ( i = 0; i < NETCODE_PACKET_QUEUE_SIZE; ++i )
    {
        uint64_t sequence = 0;
        void * packet = netcode_packet_queue_pop( &queue, &sequence );
        check( sequence == (uint64_t) i );
        check( packet == packets[i] );
        free( packet );
    }

    // add some packets again

    for ( i = 0; i < NETCODE_PACKET_QUEUE_SIZE; ++i )
    {
        packets[i] = malloc( i * 256 );
        check( netcode_packet_queue_push( &queue, packets[i], (uint64_t) i ) == 1 );
    }

    // clear the queue and make sure that all packets are freed

    netcode_packet_queue_clear( &queue );

    check( queue.start_index == 0 );
    check( queue.num_packets == 0 );
    for ( i = 0; i < NETCODE_PACKET_QUEUE_SIZE; ++i )
        check( queue.packet_data[i] == NULL );
}

static void test_endian()
{
    uint32_t value = 0x11223344;

    char * bytes = (char*) &value;

#if NETCODE_LITTLE_ENDIAN

    check( bytes[0] == 0x44 );
    check( bytes[1] == 0x33 );
    check( bytes[2] == 0x22 );
    check( bytes[3] == 0x11 );

#else // #if NETCODE_LITTLE_ENDIAN

    check( bytes[3] == 0x44 );
    check( bytes[2] == 0x33 );
    check( bytes[1] == 0x22 );
    check( bytes[0] == 0x11 );

#endif // #if NETCODE_LITTLE_ENDIAN
}

static void test_sequence()
{
    check( netcode_sequence_number_bytes_required( 0 ) == 1 );
    check( netcode_sequence_number_bytes_required( 0x11 ) == 1 );
    check( netcode_sequence_number_bytes_required( 0x1122 ) == 2 );
    check( netcode_sequence_number_bytes_required( 0x112233 ) == 3 );
    check( netcode_sequence_number_bytes_required( 0x11223344 ) == 4 );
    check( netcode_sequence_number_bytes_required( 0x1122334455 ) == 5 );
    check( netcode_sequence_number_bytes_required( 0x112233445566 ) == 6 );
    check( netcode_sequence_number_bytes_required( 0x11223344556677 ) == 7 );
    check( netcode_sequence_number_bytes_required( 0x1122334455667788 ) == 8 );
}

static void test_address()
{
    {
        struct netcode_address_t address;
        check( netcode_parse_address( "", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "[", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "[]", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "[]:", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( ":", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "1", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "12", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "123", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "1234", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "1234.0.12313.0000", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "1234.0.12313.0000.0.0.0.0.0", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "1312313:123131:1312313:123131:1312313:123131:1312313:123131:1312313:123131:1312313:123131", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( ".", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "..", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "...", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( "....", &address ) == NETCODE_ERROR );
        check( netcode_parse_address( ".....", &address ) == NETCODE_ERROR );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "107.77.207.77", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV4 );
        check( address.port == 0 );
        check( address.data.ipv4[0] == 107 );
        check( address.data.ipv4[1] == 77 );
        check( address.data.ipv4[2] == 207 );
        check( address.data.ipv4[3] == 77 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "127.0.0.1", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV4 );
        check( address.port == 0 );
        check( address.data.ipv4[0] == 127 );
        check( address.data.ipv4[1] == 0 );
        check( address.data.ipv4[2] == 0 );
        check( address.data.ipv4[3] == 1 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "107.77.207.77:40000", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV4 );
        check( address.port == 40000 );
        check( address.data.ipv4[0] == 107 );
        check( address.data.ipv4[1] == 77 );
        check( address.data.ipv4[2] == 207 );
        check( address.data.ipv4[3] == 77 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "127.0.0.1:40000", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV4 );
        check( address.port == 40000 );
        check( address.data.ipv4[0] == 127 );
        check( address.data.ipv4[1] == 0 );
        check( address.data.ipv4[2] == 0 );
        check( address.data.ipv4[3] == 1 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "fe80::202:b3ff:fe1e:8329", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 0 );
        check( address.data.ipv6[0] == 0xfe80 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0202 );
        check( address.data.ipv6[5] == 0xb3ff );
        check( address.data.ipv6[6] == 0xfe1e );
        check( address.data.ipv6[7] == 0x8329 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "::", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 0 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0000 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "::1", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 0 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0001 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[fe80::202:b3ff:fe1e:8329]:40000", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 40000 );
        check( address.data.ipv6[0] == 0xfe80 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0202 );
        check( address.data.ipv6[5] == 0xb3ff );
        check( address.data.ipv6[6] == 0xfe1e );
        check( address.data.ipv6[7] == 0x8329 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[::]:40000", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 40000 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0000 );
    }

    {
        struct netcode_address_t address;
        check( netcode_parse_address( "[::1]:40000", &address ) == NETCODE_OK );
        check( address.type == NETCODE_ADDRESS_IPV6 );
        check( address.port == 40000 );
        check( address.data.ipv6[0] == 0x0000 );
        check( address.data.ipv6[1] == 0x0000 );
        check( address.data.ipv6[2] == 0x0000 );
        check( address.data.ipv6[3] == 0x0000 );
        check( address.data.ipv6[4] == 0x0000 );
        check( address.data.ipv6[5] == 0x0000 );
        check( address.data.ipv6[6] == 0x0000 );
        check( address.data.ipv6[7] == 0x0001 );
    }
}

#define TEST_PROTOCOL_ID            0x1122334455667788ULL
#define TEST_CLIENT_ID              0x1ULL
#define TEST_SERVER_PORT            40000
#define TEST_CONNECT_TOKEN_EXPIRY   30
#define TEST_TIMEOUT_SECONDS        15

static void test_connect_token()
{
    // generate a connect token

    struct netcode_address_t server_address;
    server_address.type = NETCODE_ADDRESS_IPV4;
    server_address.data.ipv4[0] = 127;
    server_address.data.ipv4[1] = 0;
    server_address.data.ipv4[2] = 0;
    server_address.data.ipv4[3] = 1;
    server_address.port = TEST_SERVER_PORT;

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    struct netcode_connect_token_private_t input_token;

    netcode_generate_connect_token_private( &input_token, TEST_CLIENT_ID, TEST_TIMEOUT_SECONDS, 1, &server_address, user_data );

    check( input_token.client_id == TEST_CLIENT_ID );
    check( input_token.num_server_addresses == 1 );
    check( memcmp( input_token.user_data, user_data, NETCODE_USER_DATA_BYTES ) == 0 );
    check( netcode_address_equal( &input_token.server_addresses[0], &server_address ) );

    // write it to a buffer

    uint8_t buffer[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];

    netcode_write_connect_token_private( &input_token, buffer, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    // encrypt the buffer

    uint64_t expire_timestamp = time( NULL ) + 30;
    uint8_t nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    netcode_generate_nonce(nonce);
    uint8_t key[NETCODE_KEY_BYTES];
    netcode_generate_key( key );    

    check( netcode_encrypt_connect_token_private( buffer, 
                                                  NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, 
                                                  NETCODE_VERSION_INFO, 
                                                  TEST_PROTOCOL_ID, 
                                                  expire_timestamp, 
                                                  nonce, 
                                                  key ) == NETCODE_OK );

    // decrypt the buffer

    check( netcode_decrypt_connect_token_private( buffer, 
                                                  NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, 
                                                  NETCODE_VERSION_INFO, 
                                                  TEST_PROTOCOL_ID, 
                                                  expire_timestamp, 
                                                  nonce, 
                                                  key ) == NETCODE_OK );

    // read the connect token back in

    struct netcode_connect_token_private_t output_token;

    check( netcode_read_connect_token_private( buffer, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, &output_token ) == NETCODE_OK );

    // make sure that everything matches the original connect token

    check( output_token.client_id == input_token.client_id );
    check( output_token.timeout_seconds == input_token.timeout_seconds );
    check( output_token.num_server_addresses == input_token.num_server_addresses );
    check( netcode_address_equal( &output_token.server_addresses[0], &input_token.server_addresses[0] ) );
    check( memcmp( output_token.client_to_server_key, input_token.client_to_server_key, NETCODE_KEY_BYTES ) == 0 );
    check( memcmp( output_token.server_to_client_key, input_token.server_to_client_key, NETCODE_KEY_BYTES ) == 0 );
    check( memcmp( output_token.user_data, input_token.user_data, NETCODE_USER_DATA_BYTES ) == 0 );
}

static void test_challenge_token()
{
    // generate a challenge token

    struct netcode_challenge_token_t input_token;

    input_token.client_id = TEST_CLIENT_ID;
    netcode_random_bytes( input_token.user_data, NETCODE_USER_DATA_BYTES );

    // write it to a buffer

    uint8_t buffer[NETCODE_CHALLENGE_TOKEN_BYTES];

    netcode_write_challenge_token( &input_token, buffer, NETCODE_CHALLENGE_TOKEN_BYTES );

    // encrypt the buffer

    uint64_t sequence = 1000;
    uint8_t key[NETCODE_KEY_BYTES]; 
    netcode_generate_key( key );    

    check( netcode_encrypt_challenge_token( buffer, NETCODE_CHALLENGE_TOKEN_BYTES, sequence, key ) == NETCODE_OK );

    // decrypt the buffer

    check( netcode_decrypt_challenge_token( buffer, NETCODE_CHALLENGE_TOKEN_BYTES, sequence, key ) == NETCODE_OK );

    // read the challenge token back in

    struct netcode_challenge_token_t output_token;

    check( netcode_read_challenge_token( buffer, NETCODE_CHALLENGE_TOKEN_BYTES, &output_token ) == NETCODE_OK );

    // make sure that everything matches the original challenge token

    check( output_token.client_id == input_token.client_id );
    check( memcmp( output_token.user_data, input_token.user_data, NETCODE_USER_DATA_BYTES ) == 0 );
}

static void test_connection_request_packet()
{
    // generate a connect token

    struct netcode_address_t server_address;
    server_address.type = NETCODE_ADDRESS_IPV4;
    server_address.data.ipv4[0] = 127;
    server_address.data.ipv4[1] = 0;
    server_address.data.ipv4[2] = 0;
    server_address.data.ipv4[3] = 1;
    server_address.port = TEST_SERVER_PORT;

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    struct netcode_connect_token_private_t input_token;

    netcode_generate_connect_token_private( &input_token, TEST_CLIENT_ID, TEST_TIMEOUT_SECONDS, 1, &server_address, user_data );

    check( input_token.client_id == TEST_CLIENT_ID );
    check( input_token.num_server_addresses == 1 );
    check( memcmp( input_token.user_data, user_data, NETCODE_USER_DATA_BYTES ) == 0 );
    check( netcode_address_equal( &input_token.server_addresses[0], &server_address ) );

    // write the conect token to a buffer (non-encrypted)

    uint8_t connect_token_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];

    netcode_write_connect_token_private( &input_token, connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    // copy to a second buffer then encrypt it in place (we need the unencrypted token for verification later on)

    uint8_t encrypted_connect_token_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];

    memcpy( encrypted_connect_token_data, connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    uint64_t connect_token_expire_timestamp = time( NULL ) + 30;
    uint8_t connect_token_nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    netcode_generate_nonce(connect_token_nonce);
    uint8_t connect_token_key[NETCODE_KEY_BYTES];
    netcode_generate_key( connect_token_key );

    check( netcode_encrypt_connect_token_private( encrypted_connect_token_data, 
                                                  NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, 
                                                  NETCODE_VERSION_INFO, 
                                                  TEST_PROTOCOL_ID, 
                                                  connect_token_expire_timestamp, 
                                                  connect_token_nonce, 
                                                  connect_token_key ) == NETCODE_OK );

    // setup a connection request packet wrapping the encrypted connect token

    struct netcode_connection_request_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_REQUEST_PACKET;
    memcpy( input_packet.version_info, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
    input_packet.protocol_id = TEST_PROTOCOL_ID;
    input_packet.connect_token_expire_timestamp = connect_token_expire_timestamp;
    memcpy( input_packet.connect_token_nonce, connect_token_nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
    memcpy( input_packet.connect_token_data, encrypted_connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    // write the connection request packet to a buffer

    uint8_t buffer[2048];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the connection request packet back in from the buffer (the connect token data is decrypted as part of the read packet validation)

    uint64_t sequence = 1000;

    uint8_t allowed_packets[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packets, 1, sizeof( allowed_packets ) );

    struct netcode_connection_request_packet_t * output_packet = (struct netcode_connection_request_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, time( NULL ), connect_token_key, allowed_packets, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_REQUEST_PACKET );
    check( memcmp( output_packet->version_info, input_packet.version_info, NETCODE_VERSION_INFO_BYTES ) == 0 );
    check( output_packet->protocol_id == input_packet.protocol_id );
    check( output_packet->connect_token_expire_timestamp == input_packet.connect_token_expire_timestamp );
    check( memcmp( output_packet->connect_token_nonce, input_packet.connect_token_nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES ) == 0 );
    check( memcmp( output_packet->connect_token_data, connect_token_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES - NETCODE_MAC_BYTES ) == 0 );

    free( output_packet );
}

void test_connection_denied_packet()
{
    // setup a connection denied packet

    struct netcode_connection_denied_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_DENIED_PACKET;

    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    struct netcode_connection_denied_packet_t * output_packet = (struct netcode_connection_denied_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, time( NULL ), NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_DENIED_PACKET );

    free( output_packet );
}

void test_connection_challenge_packet()
{
    // setup a connection challenge packet

    struct netcode_connection_challenge_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_CHALLENGE_PACKET;
    input_packet.challenge_token_sequence = 0;
    netcode_random_bytes( input_packet.challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );

    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    struct netcode_connection_challenge_packet_t * output_packet = (struct netcode_connection_challenge_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, time( NULL ), NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_CHALLENGE_PACKET );
    check( output_packet->challenge_token_sequence == input_packet.challenge_token_sequence );
    check( memcmp( output_packet->challenge_token_data, input_packet.challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES ) == 0 );

    free( output_packet );
}

void test_connection_response_packet()
{
    // setup a connection response packet

    struct netcode_connection_response_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_RESPONSE_PACKET;
    input_packet.challenge_token_sequence = 0;
    netcode_random_bytes( input_packet.challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES );

    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );
    
    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    struct netcode_connection_response_packet_t * output_packet = (struct netcode_connection_response_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, time( NULL ), NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_RESPONSE_PACKET );
    check( output_packet->challenge_token_sequence == input_packet.challenge_token_sequence );
    check( memcmp( output_packet->challenge_token_data, input_packet.challenge_token_data, NETCODE_CHALLENGE_TOKEN_BYTES ) == 0 );

    free( output_packet );
}

void test_connection_keep_alive_packet()
{
    // setup a connection keep alive packet

    struct netcode_connection_keep_alive_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_KEEP_ALIVE_PACKET;
    input_packet.client_index = 10;
    input_packet.max_clients = 16;

    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );
    
    struct netcode_connection_keep_alive_packet_t * output_packet = (struct netcode_connection_keep_alive_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, time( NULL ), NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_KEEP_ALIVE_PACKET );
    check( output_packet->client_index == input_packet.client_index );
    check( output_packet->max_clients == input_packet.max_clients );

    free( output_packet );
}

void test_connection_payload_packet()
{
    // setup a connection payload packet

    struct netcode_connection_payload_packet_t * input_packet = netcode_create_payload_packet( NETCODE_MAX_PAYLOAD_BYTES, NULL, NULL );

    check( input_packet->packet_type == NETCODE_CONNECTION_PAYLOAD_PACKET );
    check( input_packet->payload_bytes == NETCODE_MAX_PAYLOAD_BYTES );

    netcode_random_bytes( input_packet->payload_data, NETCODE_MAX_PAYLOAD_BYTES );
    
    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    struct netcode_connection_payload_packet_t * output_packet = (struct netcode_connection_payload_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, time( NULL ), NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_PAYLOAD_PACKET );
    check( output_packet->payload_bytes == input_packet->payload_bytes );
    check( memcmp( output_packet->payload_data, input_packet->payload_data, NETCODE_MAX_PAYLOAD_BYTES ) == 0 );

    free( input_packet );
    free( output_packet );
}

void test_connection_disconnect_packet()
{
    // setup a connection disconnect packet

    struct netcode_connection_disconnect_packet_t input_packet;

    input_packet.packet_type = NETCODE_CONNECTION_DISCONNECT_PACKET;

    // write the packet to a buffer

    uint8_t buffer[NETCODE_MAX_PACKET_BYTES];

    uint8_t packet_key[NETCODE_KEY_BYTES];

    netcode_generate_key( packet_key );

    int bytes_written = netcode_write_packet( &input_packet, buffer, sizeof( buffer ), 1000, packet_key, TEST_PROTOCOL_ID );

    check( bytes_written > 0 );

    // read the packet back in from the buffer

    uint64_t sequence;

    uint8_t allowed_packet_types[NETCODE_CONNECTION_NUM_PACKETS];
    memset( allowed_packet_types, 1, sizeof( allowed_packet_types ) );

    struct netcode_connection_disconnect_packet_t * output_packet = (struct netcode_connection_disconnect_packet_t*) 
        netcode_read_packet( buffer, bytes_written, &sequence, packet_key, TEST_PROTOCOL_ID, time( NULL ), NULL, allowed_packet_types, NULL, NULL, NULL );

    check( output_packet );

    // make sure the read packet matches what was written
    
    check( output_packet->packet_type == NETCODE_CONNECTION_DISCONNECT_PACKET );

    free( output_packet );
}

void test_connect_token_public()
{
    // generate a private connect token

    struct netcode_address_t server_address;
    server_address.type = NETCODE_ADDRESS_IPV4;
    server_address.data.ipv4[0] = 127;
    server_address.data.ipv4[1] = 0;
    server_address.data.ipv4[2] = 0;
    server_address.data.ipv4[3] = 1;
    server_address.port = TEST_SERVER_PORT;

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes( user_data, NETCODE_USER_DATA_BYTES );

    struct netcode_connect_token_private_t connect_token_private;

    netcode_generate_connect_token_private( &connect_token_private, TEST_CLIENT_ID, TEST_TIMEOUT_SECONDS, 1, &server_address, user_data );

    check( connect_token_private.client_id == TEST_CLIENT_ID );
    check( connect_token_private.num_server_addresses == 1 );
    check( memcmp( connect_token_private.user_data, user_data, NETCODE_USER_DATA_BYTES ) == 0 );
    check( netcode_address_equal( &connect_token_private.server_addresses[0], &server_address ) );

    // write it to a buffer

    uint8_t connect_token_private_data[NETCODE_CONNECT_TOKEN_PRIVATE_BYTES];
    netcode_write_connect_token_private( &connect_token_private, connect_token_private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );

    // encrypt the buffer

    uint64_t create_timestamp = time( NULL );
    uint64_t expire_timestamp = create_timestamp + 30;
    uint8_t connect_token_nonce[NETCODE_CONNECT_TOKEN_NONCE_BYTES];
    netcode_generate_nonce( connect_token_nonce );    
    uint8_t key[NETCODE_KEY_BYTES];
    netcode_generate_key( key );    
    check( netcode_encrypt_connect_token_private( connect_token_private_data, 
                                                  NETCODE_CONNECT_TOKEN_PRIVATE_BYTES, 
                                                  NETCODE_VERSION_INFO, 
                                                  TEST_PROTOCOL_ID, 
                                                  expire_timestamp, 
                                                  connect_token_nonce, 
                                                  key ) == 1 );

    // wrap a public connect token around the private connect token data

    struct netcode_connect_token_t input_connect_token;
    memset( &input_connect_token, 0, sizeof( struct netcode_connect_token_t ) );
    memcpy( input_connect_token.version_info, NETCODE_VERSION_INFO, NETCODE_VERSION_INFO_BYTES );
    input_connect_token.protocol_id = TEST_PROTOCOL_ID;
    input_connect_token.create_timestamp = create_timestamp;
    input_connect_token.expire_timestamp = expire_timestamp;
    memcpy( input_connect_token.nonce, connect_token_nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES );
    memcpy( input_connect_token.private_data, connect_token_private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES );
    input_connect_token.num_server_addresses = 1;
    input_connect_token.server_addresses[0] = server_address;
    memcpy( input_connect_token.client_to_server_key, connect_token_private.client_to_server_key, NETCODE_KEY_BYTES );
    memcpy( input_connect_token.server_to_client_key, connect_token_private.server_to_client_key, NETCODE_KEY_BYTES );
    input_connect_token.timeout_seconds = (int) TEST_TIMEOUT_SECONDS;

    // write the connect token to a buffer

    uint8_t buffer[NETCODE_CONNECT_TOKEN_BYTES];
    netcode_write_connect_token( &input_connect_token, buffer, NETCODE_CONNECT_TOKEN_BYTES );

    // read the buffer back in

    struct netcode_connect_token_t output_connect_token;
    memset( &output_connect_token, 0, sizeof( struct netcode_connect_token_t ) );
    check( netcode_read_connect_token( buffer, NETCODE_CONNECT_TOKEN_BYTES, &output_connect_token ) == 1 );

    // make sure the public connect token matches what was written

    check( memcmp( output_connect_token.version_info, input_connect_token.version_info, NETCODE_VERSION_INFO_BYTES ) == 0 );
    check( output_connect_token.protocol_id == input_connect_token.protocol_id );
    check( output_connect_token.create_timestamp == input_connect_token.create_timestamp );
    check( output_connect_token.expire_timestamp == input_connect_token.expire_timestamp );
    check( memcmp( output_connect_token.nonce, input_connect_token.nonce, NETCODE_CONNECT_TOKEN_NONCE_BYTES ) == 0 );
    check( memcmp( output_connect_token.private_data, input_connect_token.private_data, NETCODE_CONNECT_TOKEN_PRIVATE_BYTES ) == 0 );
    check( output_connect_token.num_server_addresses == input_connect_token.num_server_addresses );
    check( netcode_address_equal( &output_connect_token.server_addresses[0], &input_connect_token.server_addresses[0] ) );
    check( memcmp( output_connect_token.client_to_server_key, input_connect_token.client_to_server_key, NETCODE_KEY_BYTES ) == 0 );
    check( memcmp( output_connect_token.server_to_client_key, input_connect_token.server_to_client_key, NETCODE_KEY_BYTES ) == 0 );
    check( output_connect_token.timeout_seconds == input_connect_token.timeout_seconds );
}

void test_encryption_manager()
{
    struct netcode_encryption_manager_t encryption_manager;

    netcode_encryption_manager_reset( &encryption_manager );

    double time = 100.0;

    // generate some test encryption mappings

    struct encryption_mapping_t
    {
        struct netcode_address_t address;
        uint8_t send_key[NETCODE_KEY_BYTES];
        uint8_t receive_key[NETCODE_KEY_BYTES];
    };

    #define NUM_ENCRYPTION_MAPPINGS 5

    struct encryption_mapping_t encryption_mapping[NUM_ENCRYPTION_MAPPINGS];
    memset( encryption_mapping, 0, sizeof( encryption_mapping ) );
    int i;
    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; ++i )
    {
        encryption_mapping[i].address.type = NETCODE_ADDRESS_IPV6;
        encryption_mapping[i].address.data.ipv6[7] = 1;
        encryption_mapping[i].address.port = ( uint16_t) ( 20000 + i );
        netcode_generate_key( encryption_mapping[i].send_key );
        netcode_generate_key( encryption_mapping[i].receive_key );
    }

    // add the encryption mappings to the manager and make sure they can be looked up by address

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; ++i )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        check( encryption_index == -1 );

        check( netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index ) == NULL );
        check( netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index ) == NULL );

        check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, 
                                                                  &encryption_mapping[i].address, 
                                                                  encryption_mapping[i].send_key, 
                                                                  encryption_mapping[i].receive_key, 
                                                                  time, 
                                                                  -1.0,
                                                                  TEST_TIMEOUT_SECONDS ) );

        encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        check( send_key );
        check( receive_key );

        check( memcmp( send_key, encryption_mapping[i].send_key, NETCODE_KEY_BYTES ) == 0 );
        check( memcmp( receive_key, encryption_mapping[i].receive_key, NETCODE_KEY_BYTES ) == 0 );
    }

    // removing an encryption mapping that doesn't exist should return 0
    {
        struct netcode_address_t address;
        address.type = NETCODE_ADDRESS_IPV6;
        address.data.ipv6[7] = 1;
        address.port = 50000;

        check( netcode_encryption_manager_remove_encryption_mapping( &encryption_manager, &address, time ) == 0 );
    }

    // remove the first and last encryption mappings

    check( netcode_encryption_manager_remove_encryption_mapping( &encryption_manager, &encryption_mapping[0].address, time ) == 1 );

    check( netcode_encryption_manager_remove_encryption_mapping( &encryption_manager, &encryption_mapping[NUM_ENCRYPTION_MAPPINGS-1].address, time ) == 1 );

    // make sure the encryption mappings that were removed can no longer be looked up by address

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; ++i )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        if ( i != 0 && i != NUM_ENCRYPTION_MAPPINGS - 1 )
        {
            check( send_key );
            check( receive_key );

            check( memcmp( send_key, encryption_mapping[i].send_key, NETCODE_KEY_BYTES ) == 0 );
            check( memcmp( receive_key, encryption_mapping[i].receive_key, NETCODE_KEY_BYTES ) == 0 );
        }
        else
        {
            check( !send_key );
            check( !receive_key );
        }
    }

    // add the encryption mappings back in
    
    check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, 
                                                              &encryption_mapping[0].address, 
                                                              encryption_mapping[0].send_key, 
                                                              encryption_mapping[0].receive_key, 
                                                              time, 
                                                              -1.0,
                                                              TEST_TIMEOUT_SECONDS ) );
    
    check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, 
                                                              &encryption_mapping[NUM_ENCRYPTION_MAPPINGS-1].address, 
                                                              encryption_mapping[NUM_ENCRYPTION_MAPPINGS-1].send_key, 
                                                              encryption_mapping[NUM_ENCRYPTION_MAPPINGS-1].receive_key, 
                                                              time, 
                                                              -1.0,
                                                              TEST_TIMEOUT_SECONDS ) );

    // all encryption mappings should be able to be looked up by address again

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; ++i )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        check( send_key );
        check( receive_key );

        check( memcmp( send_key, encryption_mapping[i].send_key, NETCODE_KEY_BYTES ) == 0 );
        check( memcmp( receive_key, encryption_mapping[i].receive_key, NETCODE_KEY_BYTES ) == 0 );
    }

    // check that encryption mappings time out properly

    time += TEST_TIMEOUT_SECONDS * 2;

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; ++i )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        check( !send_key );
        check( !receive_key );
    }

    // add the same encryption mappings after timeout

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; ++i )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        check( encryption_index == -1 );

        check( netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index ) == NULL );
        check( netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index ) == NULL );

        check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, 
                                                                  &encryption_mapping[i].address, 
                                                                  encryption_mapping[i].send_key, 
                                                                  encryption_mapping[i].receive_key, 
                                                                  time, 
                                                                  -1.0,
                                                                  TEST_TIMEOUT_SECONDS ) );

        encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        check( send_key );
        check( receive_key );

        check( memcmp( send_key, encryption_mapping[i].send_key, NETCODE_KEY_BYTES ) == 0 );
        check( memcmp( receive_key, encryption_mapping[i].receive_key, NETCODE_KEY_BYTES ) == 0 );
    }

    // reset the encryption mapping and verify that all encryption mappings have been removed

    netcode_encryption_manager_reset( &encryption_manager );

    for ( i = 0; i < NUM_ENCRYPTION_MAPPINGS; ++i )
    {
        int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[i].address, time );

        uint8_t * send_key = netcode_encryption_manager_get_send_key( &encryption_manager, encryption_index );
        uint8_t * receive_key = netcode_encryption_manager_get_receive_key( &encryption_manager, encryption_index );

        check( !send_key );
        check( !receive_key );
    }

    // test the expire time for encryption mapping works as expected

    check( netcode_encryption_manager_add_encryption_mapping( &encryption_manager, 
                                                              &encryption_mapping[0].address, 
                                                              encryption_mapping[0].send_key, 
                                                              encryption_mapping[0].receive_key, 
                                                              time, 
                                                              time + 1.0,
                                                              TEST_TIMEOUT_SECONDS ) );

    int encryption_index = netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[0].address, time );

    check( encryption_index != -1 );

    check( netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[0].address, time + 1.1f ) == -1 );

    netcode_encryption_manager_set_expire_time( &encryption_manager, encryption_index, -1.0 );

    check( netcode_encryption_manager_find_encryption_mapping( &encryption_manager, &encryption_mapping[0].address, time ) == encryption_index );
}

void test_replay_protection()
{
    struct netcode_replay_protection_t replay_protection;

    int i;
    for ( i = 0; i < 2; ++i )
    {
        netcode_replay_protection_reset( &replay_protection );

        check( replay_protection.most_recent_sequence == 0 );

        // the first time we receive packets, they should not be already received

        #define MAX_SEQUENCE ( NETCODE_REPLAY_PROTECTION_BUFFER_SIZE * 4 )

        uint64_t sequence;
        for ( sequence = 0; sequence < MAX_SEQUENCE; ++sequence )
        {
            check( netcode_replay_protection_already_received( &replay_protection, sequence ) == 0 );
            netcode_replay_protection_advance_sequence( &replay_protection, sequence );
        }

        // old packets outside buffer should be considered already received

        check( netcode_replay_protection_already_received( &replay_protection, 0 ) == 1 );

        // packets received a second time should be flagged already received

        for ( sequence = MAX_SEQUENCE - 10; sequence < MAX_SEQUENCE; ++sequence )
        {
            check( netcode_replay_protection_already_received( &replay_protection, sequence ) == 1 );
        }

        // jumping ahead to a much higher sequence should be considered not already received

        check( netcode_replay_protection_already_received( &replay_protection, MAX_SEQUENCE + NETCODE_REPLAY_PROTECTION_BUFFER_SIZE ) == 0 );

        // old packets should be considered already received

        for ( sequence = 0; sequence < MAX_SEQUENCE; ++sequence )
        {
            check( netcode_replay_protection_already_received( &replay_protection, sequence ) == 1 );
        }
    }
}

void test_client_create()
{
    {
        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create( "127.0.0.1:40000", &client_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "127.0.0.1:40000", &test_address );

        check( client );
        check( client->socket_holder.ipv4.handle != 0 );
        check( client->socket_holder.ipv6.handle == 0 );
        check( netcode_address_equal( &client->address, &test_address ) );

        netcode_client_destroy( client );
    }

    {
        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "[::]:50000", &test_address );

        check( client );
        check( client->socket_holder.ipv4.handle == 0 );
        check( client->socket_holder.ipv6.handle != 0 );
        check( netcode_address_equal( &client->address, &test_address ) );

        netcode_client_destroy( client );
    }

    {
        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create_overload( "127.0.0.1:40000", "[::]:50000", &client_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "127.0.0.1:40000", &test_address );

        check( client );
        check( client->socket_holder.ipv4.handle != 0 );
        check( client->socket_holder.ipv6.handle != 0 );
        check( netcode_address_equal( &client->address, &test_address ) );

        netcode_client_destroy( client );
    }

    {
        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create_overload( "[::]:50000", "127.0.0.1:40000", &client_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "[::]:50000", &test_address );

        check( client );
        check( client->socket_holder.ipv4.handle != 0 );
        check( client->socket_holder.ipv6.handle != 0 );
        check( netcode_address_equal( &client->address, &test_address ) );

        netcode_client_destroy( client );
    }
}

void test_server_create()
{
    {
        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );

        struct netcode_server_t * server = netcode_server_create( "127.0.0.1:40000", &server_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "127.0.0.1:40000", &test_address );

        check( server );
        check( server->socket_holder.ipv4.handle != 0 );
        check( server->socket_holder.ipv6.handle == 0 );
        check( netcode_address_equal( &server->address, &test_address ) );

        netcode_server_destroy( server );
    }

    {
        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );

        struct netcode_server_t * server = netcode_server_create( "[::1]:50000", &server_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "[::1]:50000", &test_address );

        check( server );
        check( server->socket_holder.ipv4.handle == 0 );
        check( server->socket_holder.ipv6.handle != 0 );
        check( netcode_address_equal( &server->address, &test_address ) );

        netcode_server_destroy( server );
    }

    {
        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );

        struct netcode_server_t * server = netcode_server_create_overload( "127.0.0.1:40000", "[::1]:50000", &server_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "127.0.0.1:40000", &test_address );

        check( server );
        check( server->socket_holder.ipv4.handle != 0 );
        check( server->socket_holder.ipv6.handle != 0 );
        check( netcode_address_equal( &server->address, &test_address ) );

        netcode_server_destroy( server );
    }

    {
        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );

        struct netcode_server_t * server = netcode_server_create_overload( "[::1]:50000", "127.0.0.1:40000", &server_config, 0.0 );

        struct netcode_address_t test_address;
        netcode_parse_address( "[::1]:50000", &test_address );

        check( server );
        check( server->socket_holder.ipv4.handle != 0 );
        check( server->socket_holder.ipv6.handle != 0 );
        check( netcode_address_equal( &server->address, &test_address ) );

        netcode_server_destroy( server );
    }
}

static uint8_t private_key[NETCODE_KEY_BYTES] = { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                                                  0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                                                  0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                                                  0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

void test_client_server_connect()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    int server_num_packets_received = 0;
    int client_num_packets_received = 0;

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; ++i )
        packet_data[i] = (uint8_t) i;

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        netcode_client_send_packet( client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            client_num_packets_received++;
            netcode_client_free_packet( client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        if ( client_num_packets_received >= 10 && server_num_packets_received >= 10 )
        {
            if ( netcode_server_client_connected( server, 0 ) )
            {
                netcode_server_disconnect_client( server, 0 );
            }
        }

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( client_num_packets_received >= 10 && server_num_packets_received >= 10 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_server_ipv4_socket_connect()
{
    {
        double time = 0.0;
        double delta_time = 1.0 / 10.0;

        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create( "0.0.0.0:50000", &client_config, time );

        check( client );

        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );
        server_config.protocol_id = TEST_PROTOCOL_ID;
        memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

        struct netcode_server_t * server = netcode_server_create( "127.0.0.1:40000", &server_config, time );

        check( server );

        netcode_server_start( server, 1 );

        NETCODE_CONST char * server_address = "127.0.0.1:40000";

        uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

        check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

        netcode_client_connect( client, connect_token );

        while ( 1 )
        {
            netcode_client_update( client, time );

            netcode_server_update( server, time );

            if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                break;

            if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
                break;

            time += delta_time;
        }

        netcode_server_destroy( server );

        netcode_client_destroy( client );
    }

    {
        double time = 0.0;
        double delta_time = 1.0 / 10.0;

        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create( "0.0.0.0:50000", &client_config, time );

        check( client );

        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );
        server_config.protocol_id = TEST_PROTOCOL_ID;
        memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

        struct netcode_server_t * server = netcode_server_create_overload( "127.0.0.1:40000", "[::1]:40000", &server_config, time );

        check( server );

        netcode_server_start( server, 1 );

        NETCODE_CONST char * server_address = "127.0.0.1:40000";

        uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

        check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

        netcode_client_connect( client, connect_token );

        while ( 1 )
        {
            netcode_client_update( client, time );

            netcode_server_update( server, time );

            if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                break;

            if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
                break;

            time += delta_time;
        }

        netcode_server_destroy( server );

        netcode_client_destroy( client );
    }

    {
        double time = 0.0;
        double delta_time = 1.0 / 10.0;

        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create_overload( "0.0.0.0:50000", "[::]:50000", &client_config, time );

        check( client );

        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );
        server_config.protocol_id = TEST_PROTOCOL_ID;
        memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

        struct netcode_server_t * server = netcode_server_create( "127.0.0.1:40000", &server_config, time );

        check( server );

        netcode_server_start( server, 1 );

        NETCODE_CONST char * server_address = "127.0.0.1:40000";

        uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

        check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

        netcode_client_connect( client, connect_token );

        while ( 1 )
        {
            netcode_client_update( client, time );

            netcode_server_update( server, time );

            if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                break;

            if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
                break;

            time += delta_time;
        }

        netcode_server_destroy( server );

        netcode_client_destroy( client );
    }

    {
        double time = 0.0;
        double delta_time = 1.0 / 10.0;

        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create_overload( "0.0.0.0:50000", "[::]:50000", &client_config, time );

        check( client );

        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );
        server_config.protocol_id = TEST_PROTOCOL_ID;
        memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

        struct netcode_server_t * server = netcode_server_create_overload( "127.0.0.1:40000", "[::1]:40000", &server_config, time );

        check( server );

        netcode_server_start( server, 1 );

        NETCODE_CONST char * server_address = "127.0.0.1:40000";

        uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

        check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

        netcode_client_connect( client, connect_token );

        while ( 1 )
        {
            netcode_client_update( client, time );

            netcode_server_update( server, time );

            if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                break;

            if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
                break;

            time += delta_time;
        }

        netcode_server_destroy( server );

        netcode_client_destroy( client );
    }
}

void test_client_server_ipv6_socket_connect()
{
    {
        double time = 0.0;
        double delta_time = 1.0 / 10.0;

        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

        check( client );

        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );
        server_config.protocol_id = TEST_PROTOCOL_ID;
        memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

        struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

        check( server );

        netcode_server_start( server, 1 );

        NETCODE_CONST char * server_address = "[::1]:40000";

        uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

        check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

        netcode_client_connect( client, connect_token );

        while ( 1 )
        {
            netcode_client_update( client, time );

            netcode_server_update( server, time );

            if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                break;

            if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
                break;

            time += delta_time;
        }

        netcode_server_destroy( server );

        netcode_client_destroy( client );
    }
    
    {
        double time = 0.0;
        double delta_time = 1.0 / 10.0;

        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

        check( client );

        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );
        server_config.protocol_id = TEST_PROTOCOL_ID;
        memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

        struct netcode_server_t * server = netcode_server_create_overload( "127.0.0.1:40000", "[::1]:40000", &server_config, time );

        check( server );

        netcode_server_start( server, 1 );

        NETCODE_CONST char * server_address = "[::1]:40000";

        uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

        check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

        netcode_client_connect( client, connect_token );

        while ( 1 )
        {
            netcode_client_update( client, time );

            netcode_server_update( server, time );

            if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                break;

            if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
                break;

            time += delta_time;
        }

        netcode_server_destroy( server );

        netcode_client_destroy( client );
    }
    
    {
        double time = 0.0;
        double delta_time = 1.0 / 10.0;

        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create_overload( "0.0.0.0:50000", "[::]:50000", &client_config, time );

        check( client );

        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );
        server_config.protocol_id = TEST_PROTOCOL_ID;
        memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

        struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

        check( server );

        netcode_server_start( server, 1 );

        NETCODE_CONST char * server_address = "[::1]:40000";

        uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

        check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

        netcode_client_connect( client, connect_token );

        while ( 1 )
        {
            netcode_client_update( client, time );

            netcode_server_update( server, time );

            if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                break;

            if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
                break;

            time += delta_time;
        }

        netcode_server_destroy( server );

        netcode_client_destroy( client );
    }
    
    {
        double time = 0.0;
        double delta_time = 1.0 / 10.0;

        struct netcode_client_config_t client_config;
        netcode_default_client_config( &client_config );

        struct netcode_client_t * client = netcode_client_create_overload( "0.0.0.0:50000", "[::]:50000", &client_config, time );

        check( client );

        struct netcode_server_config_t server_config;
        netcode_default_server_config( &server_config );
        server_config.protocol_id = TEST_PROTOCOL_ID;
        memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

        struct netcode_server_t * server = netcode_server_create_overload( "127.0.0.1:40000", "[::1]:40000", &server_config, time );

        check( server );

        netcode_server_start( server, 1 );

        NETCODE_CONST char * server_address = "[::1]:40000";

        uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );

        uint8_t user_data[NETCODE_USER_DATA_BYTES];
        netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

        check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

        netcode_client_connect( client, connect_token );

        while ( 1 )
        {
            netcode_client_update( client, time );

            netcode_server_update( server, time );

            if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                break;

            if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
                break;

            time += delta_time;
        }

        netcode_server_destroy( server );

        netcode_client_destroy( client );
    }
}

void test_client_server_keep_alive()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    // connect client to server

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // pump the client and server long enough that they would timeout without keep alive packets

    int num_iterations = (int) ( 1.25f * TEST_TIMEOUT_SECONDS / delta_time ) + 1;

    int i;
    for ( i = 0; i < num_iterations; ++i )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_server_multiple_clients()
{
    #define NUM_START_STOP_ITERATIONS 3

    int max_clients[NUM_START_STOP_ITERATIONS] = { 2, 32, 5 };

    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    int i;
    for ( i = 0; i < NUM_START_STOP_ITERATIONS; ++i )
    {
        // start the server with max # of clients for this iteration

        netcode_server_start( server, max_clients[i] );

        // create # of client objects for this iteration and connect to server

        struct netcode_client_t ** client = (struct netcode_client_t **) malloc( sizeof( struct netcode_client_t* ) * max_clients[i] );

        check( client );

        int j;
        for ( j = 0; j < max_clients[i]; ++j )
        {
            char client_address[NETCODE_MAX_ADDRESS_STRING_LENGTH];
            snprintf( client_address, sizeof(client_address), "[::]:%d", 50000 + j );

            struct netcode_client_config_t client_config;
            netcode_default_client_config( &client_config );
            client_config.network_simulator = network_simulator;

            client[j] = netcode_client_create( client_address, &client_config, time );

            check( client[j] );

            uint64_t client_id = j;
            netcode_random_bytes( (uint8_t*) &client_id, 8 );

            NETCODE_CONST char * server_address = "[::1]:40000";

            uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

            uint8_t user_data[NETCODE_USER_DATA_BYTES];
            netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

            check( netcode_generate_connect_token( 1, 
                                                   &server_address, 
                                                   &server_address, 
                                                   TEST_CONNECT_TOKEN_EXPIRY, 
                                                   TEST_TIMEOUT_SECONDS,
                                                   client_id, 
                                                   TEST_PROTOCOL_ID, 
                                                   private_key, 
                                                   user_data, 
                                                   connect_token ) );

            netcode_client_connect( client[j], connect_token );
        }

        // make sure all clients can connect

        while ( 1 )
        {
            netcode_network_simulator_update( network_simulator, time );

            for ( j = 0; j < max_clients[i]; ++j )
            {
                netcode_client_update( client[j], time );
            }

            netcode_server_update( server, time );

            int num_connected_clients = 0;

            for ( j = 0; j < max_clients[i]; ++j )
            {
                if ( netcode_client_state( client[j] ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                    break;

                if ( netcode_client_state( client[j] ) == NETCODE_CLIENT_STATE_CONNECTED )
                    num_connected_clients++;
            }

            if ( num_connected_clients == max_clients[i] )
                break;

            time += delta_time;
        }

        check( netcode_server_num_connected_clients( server ) == max_clients[i] );

        for ( j = 0; j < max_clients[i]; ++j )
        {
            check( netcode_client_state( client[j] ) == NETCODE_CLIENT_STATE_CONNECTED );
            check( netcode_server_client_connected( server, j ) == 1 );
        }

        // make sure all clients can exchange packets with the server

        int * server_num_packets_received = (int*) malloc( sizeof(int) * max_clients[i] );
        int * client_num_packets_received = (int*) malloc( sizeof(int) * max_clients[i] );

        memset( server_num_packets_received, 0, sizeof(int) * max_clients[i] );
        memset( client_num_packets_received, 0, sizeof(int) * max_clients[i] );

        uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
        for ( j = 0; j < NETCODE_MAX_PACKET_SIZE; ++j )
            packet_data[j] = (uint8_t) j;

        while ( 1 )
        {
            netcode_network_simulator_update( network_simulator, time );

            for ( j = 0; j < max_clients[i]; ++j )
            {
                netcode_client_update( client[j], time );
            }

            netcode_server_update( server, time );

            for ( j = 0; j < max_clients[i]; ++j )
            {
                netcode_client_send_packet( client[j], packet_data, NETCODE_MAX_PACKET_SIZE );
            }

            for ( j = 0; j < max_clients[i]; ++j )
            {
                netcode_server_send_packet( server, j, packet_data, NETCODE_MAX_PACKET_SIZE );
            }

            for ( j = 0; j < max_clients[i]; ++j )
            {
                while ( 1 )             
                {
                    int packet_bytes;
                    uint64_t packet_sequence;
                    uint8_t * packet = netcode_client_receive_packet( client[j], &packet_bytes, &packet_sequence );
                    if ( !packet )
                        break;
                    (void) packet_sequence;
                    netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
                    netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
                    client_num_packets_received[j]++;
                    netcode_client_free_packet( client[j], packet );
                }
            }

            for ( j = 0; j < max_clients[i]; ++j )
            {
                while ( 1 )             
                {
                    int packet_bytes;
                    uint64_t packet_sequence;
                    void * packet = netcode_server_receive_packet( server, j, &packet_bytes, &packet_sequence );
                    if ( !packet )
                        break;
                    (void) packet_sequence;
                    netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
                    netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
                    server_num_packets_received[j]++;
                    netcode_server_free_packet( server, packet );
                }
            }

            int num_clients_ready = 0;

            for ( j = 0; j < max_clients[i]; ++j )
            {
                if ( client_num_packets_received[j] >= 1 && server_num_packets_received[j] >= 1 )
                {
                    num_clients_ready++;
                }
            }

            if ( num_clients_ready == max_clients[i] )
                break;

            for ( j = 0; j < max_clients[i]; ++j )
            {
                if ( netcode_client_state( client[j] ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
                    break;
            }

            time += delta_time;
        }

        int num_clients_ready = 0;

        for ( j = 0; j < max_clients[i]; ++j )
        {
            if ( client_num_packets_received[j] >= 1 && server_num_packets_received[j] >= 1 )
            {
                num_clients_ready++;
            }
        }

        check( num_clients_ready == max_clients[i] );

        free( server_num_packets_received );
        free( client_num_packets_received );
        
        netcode_network_simulator_reset( network_simulator );

        for ( j = 0; j < max_clients[i]; ++j )
        {
            netcode_client_destroy( client[j] );
        }

        free( client );

        netcode_server_stop( server );
    }

    netcode_server_destroy( server );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_server_multiple_servers()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address[] = { "10.10.10.10:1000", "100.100.100.100:50000", "[::1]:40000" };

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 3, server_address, server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    int server_num_packets_received = 0;
    int client_num_packets_received = 0;

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; ++i )
        packet_data[i] = (uint8_t) i;

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        netcode_client_send_packet( client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            client_num_packets_received++;
            netcode_client_free_packet( client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        if ( client_num_packets_received >= 10 && server_num_packets_received >= 10 )
        {
            if ( netcode_server_client_connected( server, 0 ) )
            {
                netcode_server_disconnect_client( server, 0 );
            }
        }

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( client_num_packets_received >= 10 && server_num_packets_received >= 10 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_connect_token_expired()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, 0, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    netcode_client_update( client, time );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_invalid_connect_token()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    netcode_random_bytes( connect_token, NETCODE_CONNECT_TOKEN_BYTES );

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    netcode_client_connect( client, connect_token );

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_connection_timed_out()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    // connect a client to the server

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // now disable updating the server and verify that the client times out

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTION_TIMED_OUT );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_connection_response_timeout()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    server->flags = NETCODE_SERVER_FLAG_IGNORE_CONNECTION_RESPONSE_PACKETS;

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED  )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_connection_request_timeout()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 60.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    server->flags = NETCODE_SERVER_FLAG_IGNORE_CONNECTION_REQUEST_PACKETS;

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED  )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_error_connection_denied()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    // start a server and connect one client

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // now attempt to connect a second client. the connection should be denied.

    struct netcode_client_t * client2 = netcode_client_create( "[::]:50001", &client_config, time );

    check( client2 );

    uint8_t connect_token2[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id2 = 0;
    netcode_random_bytes( (uint8_t*) &client_id2, 8 );

    uint8_t user_data2[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data2, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id2, TEST_PROTOCOL_ID, private_key, user_data2, connect_token2 ) );

    netcode_client_connect( client2, connect_token2 );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_client_update( client2, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client2 ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_state( client2 ) == NETCODE_CLIENT_STATE_CONNECTION_DENIED );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );
    
    netcode_client_destroy( client2 );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_side_disconnect()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    // start a server and connect one client

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // disconnect client side and verify that the server sees that client disconnect cleanly, rather than timing out.

    netcode_client_disconnect( client );

    int i;
    for ( i = 0; i < 10; ++i )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_server_client_connected( server, 0 ) == 0 )
            break;

        time += delta_time;
    }

    check( netcode_server_client_connected( server, 0 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 0 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_server_side_disconnect()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    // start a server and connect one client

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // disconnect server side and verify that the client disconnects cleanly, rather than timing out.

    netcode_server_disconnect_client( server, 0 );

    int i;
    for ( i = 0; i < 10; ++i )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_DISCONNECTED );
    check( netcode_server_client_connected( server, 0 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 0 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_client_reconnect()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    // start a server and connect one client

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // disconnect client on the server-side and wait until client sees the disconnect

    netcode_network_simulator_reset( network_simulator );

    netcode_server_disconnect_client( server, 0 );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_DISCONNECTED );
    check( netcode_server_client_connected( server, 0 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 0 );

    // now reconnect the client and verify they connect

    netcode_network_simulator_reset( network_simulator );

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

struct test_loopback_context_t
{
    struct netcode_client_t * client;
    struct netcode_server_t * server;
    int num_loopback_packets_sent_to_client;
    int num_loopback_packets_sent_to_server;
};

void client_send_loopback_packet_callback( void * _context, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence )
{
    (void) packet_sequence;
    check( _context );
    check( client_index == 0 );
    check( packet_data );
    check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
    int i;
    for ( i = 0; i < packet_bytes; ++i )
    {
        check( packet_data[i] == (uint8_t) i );
    }
    struct test_loopback_context_t * context = (struct test_loopback_context_t*) _context;
    context->num_loopback_packets_sent_to_server++;
    netcode_server_process_loopback_packet( context->server, client_index, packet_data, packet_bytes, packet_sequence );
}

void server_send_loopback_packet_callback( void * _context, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence )
{
    (void) packet_sequence;
    check( _context );
    check( client_index == 0 );
    check( packet_data );
    check( packet_bytes == NETCODE_MAX_PACKET_SIZE );
    int i;
    for ( i = 0; i < packet_bytes; ++i )
    {
        check( packet_data[i] == (uint8_t) i );
    }
    struct test_loopback_context_t * context = (struct test_loopback_context_t*) _context;
    context->num_loopback_packets_sent_to_client++;
    netcode_client_process_loopback_packet( context->client, packet_data, packet_bytes, packet_sequence );
}

void test_disable_timeout()
{
    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * client = netcode_client_create( "[::]:50000", &client_config, time );

    check( client );

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    netcode_server_start( server, 1 );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, -1, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( client ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    int server_num_packets_received = 0;
    int client_num_packets_received = 0;

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; ++i )
        packet_data[i] = (uint8_t) i;

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( client, time );

        netcode_server_update( server, time );

        netcode_client_send_packet( client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            client_num_packets_received++;
            netcode_client_free_packet( client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        if ( client_num_packets_received >= 10 && server_num_packets_received >= 10 )
        {
            if ( netcode_server_client_connected( server, 0 ) )
            {
                netcode_server_disconnect_client( server, 0 );
            }
        }

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += 1000.0f;        // normally this would timeout the client
    }

    check( client_num_packets_received >= 10 && server_num_packets_received >= 10 );

    netcode_server_destroy( server );

    netcode_client_destroy( client );

    netcode_network_simulator_destroy( network_simulator );
}

void test_loopback()
{
    struct test_loopback_context_t context;
    memset( &context, 0, sizeof( context ) );

    struct netcode_network_simulator_t * network_simulator = netcode_network_simulator_create( NULL, NULL, NULL );

    network_simulator->latency_milliseconds = 250;
    network_simulator->jitter_milliseconds = 250;
    network_simulator->packet_loss_percent = 5;
    network_simulator->duplicate_packet_percent = 10;

    double time = 0.0;
    double delta_time = 1.0 / 10.0;

    // start the server

    struct netcode_server_config_t server_config;
    netcode_default_server_config( &server_config );
    server_config.protocol_id = TEST_PROTOCOL_ID;
    server_config.network_simulator = network_simulator;
    server_config.callback_context = &context;
    server_config.send_loopback_packet_callback = server_send_loopback_packet_callback;
    memcpy( &server_config.private_key, private_key, NETCODE_KEY_BYTES );

    struct netcode_server_t * server = netcode_server_create( "[::1]:40000", &server_config, time );

    check( server );

    int max_clients = 2;

    netcode_server_start( server, max_clients );

    context.server = server;

    // connect a loopback client in slot 0

    struct netcode_client_config_t client_config;
    netcode_default_client_config( &client_config );
    client_config.callback_context = &context;
    client_config.send_loopback_packet_callback = client_send_loopback_packet_callback;
    client_config.network_simulator = network_simulator;

    struct netcode_client_t * loopback_client = netcode_client_create( "[::]:50000", &client_config, time );
    check( loopback_client );
    netcode_client_connect_loopback( loopback_client, 0, max_clients );
    context.client = loopback_client;

    check( netcode_client_index( loopback_client ) == 0 );
    check( netcode_client_loopback( loopback_client ) == 1 );
    check( netcode_client_max_clients( loopback_client ) == max_clients );
    check( netcode_client_state( loopback_client ) == NETCODE_CLIENT_STATE_CONNECTED );

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );
    netcode_server_connect_loopback_client( server, 0, client_id, NULL );

    check( netcode_server_client_loopback( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    // connect a regular client in the other slot

    struct netcode_client_t * regular_client = netcode_client_create( "[::]:50001", &client_config, time );

    check( regular_client );

    NETCODE_CONST char * server_address = "[::1]:40000";

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    netcode_random_bytes( (uint8_t*) &client_id, 8 );

    uint8_t user_data[NETCODE_USER_DATA_BYTES];
    netcode_random_bytes(user_data, NETCODE_USER_DATA_BYTES);

    check( netcode_generate_connect_token( 1, &server_address, &server_address, TEST_CONNECT_TOKEN_EXPIRY, TEST_TIMEOUT_SECONDS, client_id, TEST_PROTOCOL_ID, private_key, user_data, connect_token ) );

    netcode_client_connect( regular_client, connect_token );

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( regular_client, time );

        netcode_server_update( server, time );

        if ( netcode_client_state( regular_client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        if ( netcode_client_state( regular_client ) == NETCODE_CLIENT_STATE_CONNECTED )
            break;

        time += delta_time;
    }

    check( netcode_client_state( regular_client ) == NETCODE_CLIENT_STATE_CONNECTED );
    check( netcode_client_index( regular_client ) == 1 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 1 ) == 1 );
    check( netcode_server_client_loopback( server, 0 ) == 1 );
    check( netcode_server_client_loopback( server, 1 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 2 );

    // test that we can exchange packets for the regular client and the loopback client

    int loopback_client_num_packets_received = 0;
    int loopback_server_num_packets_received = 0;
    int regular_server_num_packets_received = 0;
    int regular_client_num_packets_received = 0;

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; ++i )
        packet_data[i] = (uint8_t) i;

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( regular_client, time );

        netcode_server_update( server, time );

        netcode_client_send_packet( loopback_client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_client_send_packet( regular_client, packet_data, NETCODE_MAX_PACKET_SIZE );
        
        netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );
        
        netcode_server_send_packet( server, 1, packet_data, NETCODE_MAX_PACKET_SIZE );

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( loopback_client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            loopback_client_num_packets_received++;
            netcode_client_free_packet( loopback_client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( regular_client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            regular_client_num_packets_received++;
            netcode_client_free_packet( regular_client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            loopback_server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 1, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            regular_server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        if ( loopback_client_num_packets_received >= 10 && loopback_server_num_packets_received >= 10 &&
             regular_client_num_packets_received >= 10 && regular_server_num_packets_received >= 10 )
            break;

        if ( netcode_client_state( regular_client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( loopback_client_num_packets_received >= 10 );
    check( loopback_server_num_packets_received >= 10 );
    check( regular_client_num_packets_received >= 10 );
    check( regular_server_num_packets_received >= 10 );
    check( context.num_loopback_packets_sent_to_client >= 10 );
    check( context.num_loopback_packets_sent_to_server >= 10 );

    // verify that we can disconnect the loopback client

    check( netcode_server_client_loopback( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 2 );

    netcode_server_disconnect_loopback_client( server, 0 );    

    check( netcode_server_client_loopback( server, 0 ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 0 );
    check( netcode_server_num_connected_clients( server ) == 1 );

    netcode_client_disconnect_loopback( loopback_client );

    check( netcode_client_state( loopback_client ) == NETCODE_CLIENT_STATE_DISCONNECTED );

    // verify that we can reconnect the loopback client

    netcode_random_bytes( (uint8_t*) &client_id, 8 );
    netcode_server_connect_loopback_client( server, 0, client_id, NULL );

    check( netcode_server_client_loopback( server, 0 ) == 1 );
    check( netcode_server_client_loopback( server, 1 ) == 0 );
    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 1 ) == 1 );
    check( netcode_server_num_connected_clients( server ) == 2 );

    netcode_client_connect_loopback( loopback_client, 0, max_clients );
    
    check( netcode_client_index( loopback_client ) == 0 );
    check( netcode_client_loopback( loopback_client ) == 1 );
    check( netcode_client_max_clients( loopback_client ) == max_clients );
    check( netcode_client_state( loopback_client ) == NETCODE_CLIENT_STATE_CONNECTED );

    // verify that we can exchange packets for both regular and loopback client post reconnect

    loopback_server_num_packets_received = 0;
    loopback_client_num_packets_received = 0;
    regular_server_num_packets_received = 0;
    regular_client_num_packets_received = 0;
    context.num_loopback_packets_sent_to_client = 0;
    context.num_loopback_packets_sent_to_server = 0;

    while ( 1 )
    {
        netcode_network_simulator_update( network_simulator, time );

        netcode_client_update( regular_client, time );

        netcode_server_update( server, time );

        netcode_client_send_packet( loopback_client, packet_data, NETCODE_MAX_PACKET_SIZE );

        netcode_client_send_packet( regular_client, packet_data, NETCODE_MAX_PACKET_SIZE );
        
        netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );
        
        netcode_server_send_packet( server, 1, packet_data, NETCODE_MAX_PACKET_SIZE );

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( loopback_client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            loopback_client_num_packets_received++;
            netcode_client_free_packet( loopback_client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            uint8_t * packet = netcode_client_receive_packet( regular_client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            regular_client_num_packets_received++;
            netcode_client_free_packet( regular_client, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 0, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            loopback_server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_server_receive_packet( server, 1, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            netcode_assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            netcode_assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            regular_server_num_packets_received++;
            netcode_server_free_packet( server, packet );
        }

        if ( loopback_client_num_packets_received >= 10 && loopback_server_num_packets_received >= 10 &&
             regular_client_num_packets_received >= 10 && regular_server_num_packets_received >= 10 )
            break;

        if ( netcode_client_state( regular_client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        time += delta_time;
    }

    check( loopback_client_num_packets_received >= 10 );
    check( loopback_server_num_packets_received >= 10 );
    check( regular_client_num_packets_received >= 10 );
    check( regular_server_num_packets_received >= 10 );
    check( context.num_loopback_packets_sent_to_client >= 10 );
    check( context.num_loopback_packets_sent_to_server >= 10 );

    // verify the regular client times out but loopback client doesn't

    time += 100000.0;

    netcode_server_update( server, time );

    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 1 ) == 0 );

    netcode_client_update( loopback_client, time );

    check( netcode_client_state( loopback_client ) == NETCODE_CLIENT_STATE_CONNECTED );

    // verify that disconnect all clients leaves loopback clients alone

    netcode_server_disconnect_all_clients( server );

    check( netcode_server_client_connected( server, 0 ) == 1 );
    check( netcode_server_client_connected( server, 1 ) == 0 );
    check( netcode_server_client_loopback( server, 0 ) == 1 );

    // clean up

    netcode_client_destroy( regular_client );

    netcode_client_destroy( loopback_client );

    netcode_server_destroy( server );

    netcode_network_simulator_destroy( network_simulator );
}

#define RUN_TEST( test_function )                                           \
    do                                                                      \
    {                                                                       \
        printf( #test_function "\n" );                                      \
        test_function();                                                    \
    }                                                                       \
    while (0)

void netcode_test()
{
    //while ( 1 )
    {
        RUN_TEST( test_queue );
        RUN_TEST( test_endian );
        RUN_TEST( test_address );
        RUN_TEST( test_sequence );
        RUN_TEST( test_connect_token );
        RUN_TEST( test_challenge_token );
        RUN_TEST( test_connection_request_packet );
        RUN_TEST( test_connection_denied_packet );
        RUN_TEST( test_connection_challenge_packet );
        RUN_TEST( test_connection_response_packet );
        RUN_TEST( test_connection_payload_packet );
        RUN_TEST( test_connection_disconnect_packet );
        RUN_TEST( test_connect_token_public );
        RUN_TEST( test_encryption_manager );
        RUN_TEST( test_replay_protection );
        RUN_TEST( test_client_create );
        RUN_TEST( test_server_create );
        RUN_TEST( test_client_server_connect );
        RUN_TEST( test_client_server_ipv4_socket_connect );
        RUN_TEST( test_client_server_ipv6_socket_connect );
        RUN_TEST( test_client_server_keep_alive );
        RUN_TEST( test_client_server_multiple_clients );
        RUN_TEST( test_client_server_multiple_servers );
        RUN_TEST( test_client_error_connect_token_expired );
        RUN_TEST( test_client_error_invalid_connect_token );
        RUN_TEST( test_client_error_connection_timed_out );
        RUN_TEST( test_client_error_connection_response_timeout );
        RUN_TEST( test_client_error_connection_request_timeout );
        RUN_TEST( test_client_error_connection_denied );
        RUN_TEST( test_client_side_disconnect );
        RUN_TEST( test_server_side_disconnect );
        RUN_TEST( test_client_reconnect );
        RUN_TEST( test_disable_timeout );
        RUN_TEST( test_loopback );
    }
}

#endif // #if NETCODE_ENABLE_TESTS

