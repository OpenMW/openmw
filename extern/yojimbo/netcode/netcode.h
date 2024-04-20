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

#ifndef NETCODE_H
#define NETCODE_H

#include <stdint.h>
#include <stddef.h>

#if !defined(NETCODE_DEBUG) && !defined(NETCODE_RELEASE)
#if defined(NDEBUG)
#define NETCODE_RELEASE
#else
#define NETCODE_DEBUG
#endif
#elif defined(NETCODE_DEBUG) && defined(NETCODE_RELEASE)
#error Can only define one of debug & release
#endif

#if    defined(__386__) || defined(i386)    || defined(__i386__)  \
    || defined(__X86)   || defined(_M_IX86)                       \
    || defined(_M_X64)  || defined(__x86_64__)                    \
    || defined(alpha)   || defined(__alpha) || defined(__alpha__) \
    || defined(_M_ALPHA)                                          \
    || defined(ARM)     || defined(_ARM)    || defined(__arm__)   \
    || defined(__aarch64__) 								      \
    || defined(WIN32)   || defined(_WIN32)  || defined(__WIN32__) \
    || defined(_WIN32_WCE) || defined(__NT__)                     \
    || defined(__MIPSEL__)
  #define NETCODE_LITTLE_ENDIAN 1
#else
  #define NETCODE_BIG_ENDIAN 1
#endif

#define NETCODE_PLATFORM_WINDOWS    1
#define NETCODE_PLATFORM_MAC        2
#define NETCODE_PLATFORM_UNIX       3

#if defined(_WIN32)
#define NETCODE_PLATFORM NETCODE_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define NETCODE_PLATFORM NETCODE_PLATFORM_MAC
#else
#define NETCODE_PLATFORM NETCODE_PLATFORM_UNIX
#endif

#define NETCODE_CONNECT_TOKEN_BYTES 2048
#define NETCODE_KEY_BYTES 32
#define NETCODE_MAC_BYTES 16
#define NETCODE_USER_DATA_BYTES 256
#define NETCODE_MAX_SERVERS_PER_CONNECT 32

#define NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED              -6
#define NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN              -5
#define NETCODE_CLIENT_STATE_CONNECTION_TIMED_OUT               -4
#define NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT      -3
#define NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT       -2
#define NETCODE_CLIENT_STATE_CONNECTION_DENIED                  -1
#define NETCODE_CLIENT_STATE_DISCONNECTED                       0
#define NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST         1
#define NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE        2
#define NETCODE_CLIENT_STATE_CONNECTED                          3

#define NETCODE_MAX_CLIENTS         256
#define NETCODE_MAX_PACKET_SIZE     1200

#define NETCODE_LOG_LEVEL_NONE      0
#define NETCODE_LOG_LEVEL_ERROR     1
#define NETCODE_LOG_LEVEL_INFO      2
#define NETCODE_LOG_LEVEL_DEBUG     3

#define NETCODE_OK                  1
#define NETCODE_ERROR               0

#define NETCODE_ADDRESS_NONE        0
#define NETCODE_ADDRESS_IPV4        1
#define NETCODE_ADDRESS_IPV6        2

#ifdef __cplusplus
#define NETCODE_CONST const
extern "C" {
#else
#if defined(__STDC__)
#define NETCODE_CONST const
#else
#define NETCODE_CONST
#endif
#endif

int netcode_init();

void netcode_term();

struct netcode_address_t
{
    union { uint8_t ipv4[4]; uint16_t ipv6[8]; } data;
    uint16_t port;
    uint8_t type;
};

int netcode_parse_address( NETCODE_CONST char * address_string_in, struct netcode_address_t * address );

char * netcode_address_to_string( struct netcode_address_t * address, char * buffer );

int netcode_address_equal( struct netcode_address_t * a, struct netcode_address_t * b );

struct netcode_client_config_t
{
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
    struct netcode_network_simulator_t * network_simulator;
    void * callback_context;
    void (*state_change_callback)(void*,int,int);
    void (*send_loopback_packet_callback)(void*,int,NETCODE_CONST uint8_t*,int,uint64_t);
    int override_send_and_receive;
    void (*send_packet_override)(void*,struct netcode_address_t*,NETCODE_CONST uint8_t*,int);
    int (*receive_packet_override)(void*,struct netcode_address_t*,uint8_t*,int);
};

void netcode_default_client_config( struct netcode_client_config_t * config );

struct netcode_client_t * netcode_client_create( NETCODE_CONST char * address, NETCODE_CONST struct netcode_client_config_t * config, double time );

void netcode_client_destroy( struct netcode_client_t * client );

void netcode_client_connect( struct netcode_client_t * client, uint8_t * connect_token );

void netcode_client_update( struct netcode_client_t * client, double time );

uint64_t netcode_client_next_packet_sequence( struct netcode_client_t * client );

void netcode_client_send_packet( struct netcode_client_t * client, NETCODE_CONST uint8_t * packet_data, int packet_bytes );

uint8_t * netcode_client_receive_packet( struct netcode_client_t * client, int * packet_bytes, uint64_t * packet_sequence );

void netcode_client_free_packet( struct netcode_client_t * client, void * packet );

void netcode_client_disconnect( struct netcode_client_t * client );

int netcode_client_state( struct netcode_client_t * client );

int netcode_client_index( struct netcode_client_t * client );

int netcode_client_max_clients( struct netcode_client_t * client );

void netcode_client_connect_loopback( struct netcode_client_t * client, int client_index, int max_clients );

void netcode_client_disconnect_loopback( struct netcode_client_t * client );

void netcode_client_process_packet( struct netcode_client_t * client, struct netcode_address_t * from, uint8_t * packet_data, int packet_bytes );

int netcode_client_loopback( struct netcode_client_t * client );

void netcode_client_process_loopback_packet( struct netcode_client_t * client, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence );

uint16_t netcode_client_get_port( struct netcode_client_t * client );

struct netcode_address_t * netcode_client_server_address( struct netcode_client_t * client );

int netcode_generate_connect_token( int num_server_addresses, 
                                    NETCODE_CONST char ** public_server_addresses, 
                                    NETCODE_CONST char ** internal_server_addresses, 
                                    int expire_seconds,
                                    int timeout_seconds, 
                                    uint64_t client_id, 
                                    uint64_t protocol_id, 
                                    NETCODE_CONST uint8_t * private_key, 
                                    uint8_t * user_data, 
                                    uint8_t * connect_token );

struct netcode_server_config_t
{
    uint64_t protocol_id;
    uint8_t private_key[NETCODE_KEY_BYTES];
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
    struct netcode_network_simulator_t * network_simulator;
    void * callback_context;
    void (*connect_disconnect_callback)(void*,int,int);
    void (*send_loopback_packet_callback)(void*,int,NETCODE_CONST uint8_t*,int,uint64_t);
    int override_send_and_receive;
    void (*send_packet_override)(void*,struct netcode_address_t*,NETCODE_CONST uint8_t*,int);
    int (*receive_packet_override)(void*,struct netcode_address_t*,uint8_t*,int);
};

void netcode_default_server_config( struct netcode_server_config_t * config );

struct netcode_server_t * netcode_server_create( NETCODE_CONST char * server_address, NETCODE_CONST struct netcode_server_config_t * config, double time );

void netcode_server_destroy( struct netcode_server_t * server );

void netcode_server_start( struct netcode_server_t * server, int max_clients );

void netcode_server_stop( struct netcode_server_t * server );

int netcode_server_running( struct netcode_server_t * server );

int netcode_server_max_clients( struct netcode_server_t * server );

void netcode_server_update( struct netcode_server_t * server, double time );

int netcode_server_client_connected( struct netcode_server_t * server, int client_index );

uint64_t netcode_server_client_id( struct netcode_server_t * server, int client_index );

struct netcode_address_t * netcode_server_client_address( struct netcode_server_t * server, int client_index );

void netcode_server_disconnect_client( struct netcode_server_t * server, int client_index );

void netcode_server_disconnect_all_clients( struct netcode_server_t * server );

uint64_t netcode_server_next_packet_sequence( struct netcode_server_t * server, int client_index );

void netcode_server_send_packet( struct netcode_server_t * server, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes );

uint8_t * netcode_server_receive_packet( struct netcode_server_t * server, int client_index, int * packet_bytes, uint64_t * packet_sequence );

void netcode_server_free_packet( struct netcode_server_t * server, void * packet );

int netcode_server_num_connected_clients( struct netcode_server_t * server );

void * netcode_server_client_user_data( struct netcode_server_t * server, int client_index );

void netcode_server_process_packet( struct netcode_server_t * server, struct netcode_address_t * from, uint8_t * packet_data, int packet_bytes );

void netcode_server_connect_loopback_client( struct netcode_server_t * server, int client_index, uint64_t client_id, NETCODE_CONST uint8_t * user_data );

void netcode_server_disconnect_loopback_client( struct netcode_server_t * server, int client_index );

int netcode_server_client_loopback( struct netcode_server_t * server, int client_index );

void netcode_server_process_loopback_packet( struct netcode_server_t * server, int client_index, NETCODE_CONST uint8_t * packet_data, int packet_bytes, uint64_t packet_sequence );

uint16_t netcode_server_get_port( struct netcode_server_t * server );

void netcode_log_level( int level );

void netcode_set_printf_function( int (*function)( NETCODE_CONST char *, ... ) );

extern void (*netcode_assert_function)( NETCODE_CONST char *, NETCODE_CONST char *, NETCODE_CONST char * file, int line );

#ifndef NDEBUG
#define netcode_assert( condition )                                                         \
do                                                                                          \
{                                                                                           \
    if ( !(condition) )                                                                     \
    {                                                                                       \
        netcode_assert_function( #condition, __FUNCTION__, __FILE__, __LINE__ );            \
        exit(1);                                                                            \
    }                                                                                       \
} while(0)
#else
#define netcode_assert( ignore ) ((void)0)
#endif

void netcode_set_assert_function( void (*function)( NETCODE_CONST char * /*condition*/, 
                                  NETCODE_CONST char * /*function*/, 
                                  NETCODE_CONST char * /*file*/, 
                                  int /*line*/ ) );

void netcode_random_bytes( uint8_t * data, int bytes );

void netcode_sleep( double seconds );

double netcode_time();

#ifdef __cplusplus
}
#endif

#endif // #ifndef NETCODE_H
