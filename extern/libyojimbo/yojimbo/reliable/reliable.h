/*
    reliable

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

#ifndef RELIABLE_H
#define RELIABLE_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

#if !defined(RELIABLE_DEBUG) && !defined(RELIABLE_RELEASE)
#if defined(NDEBUG)
#define RELIABLE_RELEASE
#else
#define RELIABLE_DEBUG
#endif
#elif defined(RELIABLE_DEBUG) && defined(RELIABLE_RELEASE)
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
  #define RELIABLE_LITTLE_ENDIAN 1
#else
  #define RELIABLE_BIG_ENDIAN 1
#endif

#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT                          0
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED                      1
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED                         2
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_STALE                         3
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_INVALID                       4
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_SEND             5
#define RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_RECEIVE          6
#define RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_SENT                        7
#define RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_RECEIVED                    8
#define RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID                     9
#define RELIABLE_ENDPOINT_NUM_COUNTERS                                      10

#define RELIABLE_MAX_PACKET_HEADER_BYTES 9
#define RELIABLE_FRAGMENT_HEADER_BYTES 5

#define RELIABLE_LOG_LEVEL_NONE     0
#define RELIABLE_LOG_LEVEL_ERROR    1
#define RELIABLE_LOG_LEVEL_INFO     2
#define RELIABLE_LOG_LEVEL_DEBUG    3

#define RELIABLE_OK         1
#define RELIABLE_ERROR      0

#ifdef __cplusplus
#define RELIABLE_CONST const
extern "C" {
#else
#if defined(__STDC__)
#define RELIABLE_CONST const
#else
#define RELIABLE_CONST
#endif
#endif

int reliable_init(void);

void reliable_term(void);

struct reliable_config_t
{
    char name[256];
    void * context;
    uint64_t id;
    int max_packet_size;
    int fragment_above;
    int max_fragments;
    int fragment_size;
    int ack_buffer_size;
    int sent_packets_buffer_size;
    int received_packets_buffer_size;
    int fragment_reassembly_buffer_size;
    float rtt_smoothing_factor;
    float packet_loss_smoothing_factor;
    float bandwidth_smoothing_factor;
    int packet_header_size;
    void (*transmit_packet_function)(void*,uint64_t,uint16_t,uint8_t*,int);
    int (*process_packet_function)(void*,uint64_t,uint16_t,uint8_t*,int);
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
};

void reliable_default_config( struct reliable_config_t * config );

struct reliable_endpoint_t * reliable_endpoint_create( struct reliable_config_t * config, double time );

uint16_t reliable_endpoint_next_packet_sequence( struct reliable_endpoint_t * endpoint );

void reliable_endpoint_send_packet( struct reliable_endpoint_t * endpoint, uint8_t * packet_data, int packet_bytes );

void reliable_endpoint_receive_packet( struct reliable_endpoint_t * endpoint, uint8_t * packet_data, int packet_bytes );

void reliable_endpoint_free_packet( struct reliable_endpoint_t * endpoint, void * packet );

uint16_t * reliable_endpoint_get_acks( struct reliable_endpoint_t * endpoint, int * num_acks );

void reliable_endpoint_clear_acks( struct reliable_endpoint_t * endpoint );

void reliable_endpoint_reset( struct reliable_endpoint_t * endpoint );

void reliable_endpoint_update( struct reliable_endpoint_t * endpoint, double time );

float reliable_endpoint_rtt( struct reliable_endpoint_t * endpoint );

float reliable_endpoint_packet_loss( struct reliable_endpoint_t * endpoint );

void reliable_endpoint_bandwidth( struct reliable_endpoint_t * endpoint, float * sent_bandwidth_kbps, float * received_bandwidth_kbps, float * acked_bandwidth_kpbs );

RELIABLE_CONST uint64_t * reliable_endpoint_counters( struct reliable_endpoint_t * endpoint );

void reliable_endpoint_destroy( struct reliable_endpoint_t * endpoint );

void reliable_log_level( int level );

void reliable_set_printf_function( int (*function)( RELIABLE_CONST char *, ... ) );

extern void (*reliable_assert_function)( RELIABLE_CONST char *, RELIABLE_CONST char *, RELIABLE_CONST char * file, int line );

#ifdef RELIABLE_DEBUG
#define reliable_assert( condition )                                                        \
do                                                                                          \
{                                                                                           \
    if ( !(condition) )                                                                     \
    {                                                                                       \
        reliable_assert_function( #condition, __FUNCTION__, __FILE__, __LINE__ );           \
        exit(1);                                                                            \
    }                                                                                       \
} while(0)
#else
#define reliable_assert( ignore ) ((void)0)
#endif

void reliable_set_assert_function( void (*function)( RELIABLE_CONST char * /*condition*/, 
                                   RELIABLE_CONST char * /*function*/, 
                                   RELIABLE_CONST char * /*file*/, 
                                   int /*line*/ ) );

void reliable_copy_string( char * dest, RELIABLE_CONST char * source, size_t dest_size );

#ifdef __cplusplus
}
#endif

#endif // #ifndef RELIABLE_H
