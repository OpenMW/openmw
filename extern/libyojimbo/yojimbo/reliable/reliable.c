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

#include "reliable.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>

#ifndef RELIABLE_ENABLE_TESTS
#define RELIABLE_ENABLE_TESTS 0
#endif // #ifndef RELIABLE_ENABLE_TESTS

#ifndef RELIABLE_ENABLE_LOGGING
#define RELIABLE_ENABLE_LOGGING 1
#endif // #ifndef RELIABLE_ENABLE_LOGGING

// ------------------------------------------------------------------

static void default_assert_handler( RELIABLE_CONST char * condition, RELIABLE_CONST char * function, RELIABLE_CONST char * file, int line )
{
    printf( "assert failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
    #if defined( __GNUC__ )
    __builtin_trap();
    #elif defined( _MSC_VER )
    __debugbreak();
    #endif
    exit( 1 );
}

static int log_level = 0;
static int (*printf_function)( RELIABLE_CONST char *, ... ) = ( int (*)( RELIABLE_CONST char *, ... ) ) printf;
void (*reliable_assert_function)( RELIABLE_CONST char *, RELIABLE_CONST char *, RELIABLE_CONST char * file, int line ) = default_assert_handler;

void reliable_log_level( int level )
{
    log_level = level;
}

void reliable_set_printf_function( int (*function)( RELIABLE_CONST char *, ... ) )
{
    reliable_assert( function );
    printf_function = function;
}

void reliable_set_assert_function( void (*function)( RELIABLE_CONST char *, RELIABLE_CONST char *, RELIABLE_CONST char * file, int line ) )
{
    reliable_assert_function = function;
}

#if RELIABLE_ENABLE_LOGGING

void reliable_printf( int level, RELIABLE_CONST char * format, ... ) 
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

#else // #if RELIABLE_ENABLE_LOGGING

void reliable_printf( int level, RELIABLE_CONST char * format, ... ) 
{
    (void) level;
    (void) format;
}

#endif // #if RELIABLE_ENABLE_LOGGING

void * reliable_default_allocate_function( void * context, size_t bytes )
{
    (void) context;
    return malloc( bytes );
}

void reliable_default_free_function( void * context, void * pointer )
{
    (void) context;
    free( pointer );
}

// ------------------------------------------------------------------

int reliable_init(void)
{
    return RELIABLE_OK;
}

void reliable_term(void)
{
}

// ---------------------------------------------------------------

int reliable_sequence_greater_than( uint16_t s1, uint16_t s2 )
{
    return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) || 
           ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
}

int reliable_sequence_less_than( uint16_t s1, uint16_t s2 )
{
    return reliable_sequence_greater_than( s2, s1 );
}

// ---------------------------------------------------------------

struct reliable_sequence_buffer_t
{
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
    uint16_t sequence;
    int num_entries;
    int entry_stride;
    uint32_t * entry_sequence;
    uint8_t * entry_data;
};

struct reliable_sequence_buffer_t * reliable_sequence_buffer_create( int num_entries, 
                                                                     int entry_stride, 
                                                                     void * allocator_context, 
                                                                     void * (*allocate_function)(void*,size_t), 
                                                                     void (*free_function)(void*,void*) )
{
    reliable_assert( num_entries > 0 );
    reliable_assert( entry_stride > 0 );

    if ( allocate_function == NULL )
    {
        allocate_function = reliable_default_allocate_function;
    }

    if ( free_function == NULL )
    {
        free_function = reliable_default_free_function;
    }

    struct reliable_sequence_buffer_t * sequence_buffer = (struct reliable_sequence_buffer_t*) 
        allocate_function( allocator_context, sizeof( struct reliable_sequence_buffer_t ) );

    sequence_buffer->allocator_context = allocator_context;
    sequence_buffer->allocate_function = allocate_function;
    sequence_buffer->free_function = free_function;
    sequence_buffer->sequence = 0;
    sequence_buffer->num_entries = num_entries;
    sequence_buffer->entry_stride = entry_stride;
    sequence_buffer->entry_sequence = (uint32_t*) allocate_function( allocator_context, num_entries * sizeof( uint32_t ) );
    sequence_buffer->entry_data = (uint8_t*) allocate_function( allocator_context, num_entries * entry_stride );
    reliable_assert( sequence_buffer->entry_sequence );
    reliable_assert( sequence_buffer->entry_data );
    memset( sequence_buffer->entry_sequence, 0xFF, sizeof( uint32_t) * sequence_buffer->num_entries );
    memset( sequence_buffer->entry_data, 0, num_entries * entry_stride );

    return sequence_buffer;
}

void reliable_sequence_buffer_destroy( struct reliable_sequence_buffer_t * sequence_buffer )
{
    reliable_assert( sequence_buffer );
    sequence_buffer->free_function( sequence_buffer->allocator_context, sequence_buffer->entry_sequence );
    sequence_buffer->free_function( sequence_buffer->allocator_context, sequence_buffer->entry_data );
    sequence_buffer->free_function( sequence_buffer->allocator_context, sequence_buffer );
}

void reliable_sequence_buffer_reset( struct reliable_sequence_buffer_t * sequence_buffer )
{
    reliable_assert( sequence_buffer );
    sequence_buffer->sequence = 0;
    memset( sequence_buffer->entry_sequence, 0xFF, sizeof( uint32_t) * sequence_buffer->num_entries );
}

void reliable_sequence_buffer_remove_entries( struct reliable_sequence_buffer_t * sequence_buffer, 
                                              int start_sequence, 
                                              int finish_sequence, 
                                              void (*cleanup_function)(void*,void*,void(*free_function)(void*,void*)) )
{
    reliable_assert( sequence_buffer );
    if ( finish_sequence < start_sequence ) 
    {
        finish_sequence += 65536;
    }
    if ( finish_sequence - start_sequence < sequence_buffer->num_entries )
    {
        int sequence;
        for ( sequence = start_sequence; sequence <= finish_sequence; ++sequence )
        {
            if ( cleanup_function )
            {
                cleanup_function( sequence_buffer->entry_data + sequence_buffer->entry_stride * ( sequence % sequence_buffer->num_entries ), 
                                  sequence_buffer->allocator_context, 
                                  sequence_buffer->free_function );
            }
            sequence_buffer->entry_sequence[ sequence % sequence_buffer->num_entries ] = 0xFFFFFFFF;
        }
    }
    else
    {
        int i;
        for ( i = 0; i < sequence_buffer->num_entries; ++i )
        {
            if ( cleanup_function )
            {
                cleanup_function( sequence_buffer->entry_data + sequence_buffer->entry_stride * i, 
                                  sequence_buffer->allocator_context, 
                                  sequence_buffer->free_function );
            }
            sequence_buffer->entry_sequence[i] = 0xFFFFFFFF;
        }
    }
}

int reliable_sequence_buffer_test_insert( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    return reliable_sequence_less_than( sequence, sequence_buffer->sequence - ((uint16_t)sequence_buffer->num_entries) ) ? ((uint16_t)0) : ((uint16_t)1);
}

void * reliable_sequence_buffer_insert( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    reliable_assert( sequence_buffer );
    if ( reliable_sequence_less_than( sequence, sequence_buffer->sequence - ((uint16_t)sequence_buffer->num_entries) ) )
    {
        return NULL;
    }
    if ( reliable_sequence_greater_than( sequence + 1, sequence_buffer->sequence ) )
    {
        reliable_sequence_buffer_remove_entries( sequence_buffer, sequence_buffer->sequence, sequence, NULL );
        sequence_buffer->sequence = sequence + 1;
    }
    int index = sequence % sequence_buffer->num_entries;
    sequence_buffer->entry_sequence[index] = sequence;
    return sequence_buffer->entry_data + index * sequence_buffer->entry_stride;
}

void reliable_sequence_buffer_advance( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    reliable_assert( sequence_buffer );
    if ( reliable_sequence_greater_than( sequence + 1, sequence_buffer->sequence ) )
    {
        reliable_sequence_buffer_remove_entries( sequence_buffer, sequence_buffer->sequence, sequence, NULL );
        sequence_buffer->sequence = sequence + 1;
    }
}

void * reliable_sequence_buffer_insert_with_cleanup( struct reliable_sequence_buffer_t * sequence_buffer, 
                                                     uint16_t sequence, 
                                                     void (*cleanup_function)(void*,void*,void(*free_function)(void*,void*)) )
{
    reliable_assert( sequence_buffer );
    if ( reliable_sequence_greater_than( sequence + 1, sequence_buffer->sequence ) )
    {
        reliable_sequence_buffer_remove_entries( sequence_buffer, sequence_buffer->sequence, sequence, cleanup_function );
        sequence_buffer->sequence = sequence + 1;
    }
    else if ( reliable_sequence_less_than( sequence, sequence_buffer->sequence - ((uint16_t)sequence_buffer->num_entries) ) )
    {
        return NULL;
    }
    int index = sequence % sequence_buffer->num_entries;
    if ( sequence_buffer->entry_sequence[index] != 0xFFFFFFFF )
    {
        cleanup_function( sequence_buffer->entry_data + sequence_buffer->entry_stride * ( sequence % sequence_buffer->num_entries ), 
                          sequence_buffer->allocator_context, 
                          sequence_buffer->free_function );
    }
    sequence_buffer->entry_sequence[index] = sequence;
    return sequence_buffer->entry_data + index * sequence_buffer->entry_stride;
}

void reliable_sequence_buffer_advance_with_cleanup( struct reliable_sequence_buffer_t * sequence_buffer,
                                                    uint16_t sequence,
                                                    void (*cleanup_function)(void*,void*,void(*free_function)(void*,void*)) )
{
    reliable_assert( sequence_buffer );
    if ( reliable_sequence_greater_than( sequence + 1, sequence_buffer->sequence ) )
    {
        reliable_sequence_buffer_remove_entries( sequence_buffer, sequence_buffer->sequence, sequence, cleanup_function );
        sequence_buffer->sequence = sequence + 1;
    }
}

void reliable_sequence_buffer_remove( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    reliable_assert( sequence_buffer );
    sequence_buffer->entry_sequence[ sequence % sequence_buffer->num_entries ] = 0xFFFFFFFF;
}

void reliable_sequence_buffer_remove_with_cleanup( struct reliable_sequence_buffer_t * sequence_buffer, 
                                                   uint16_t sequence, 
                                                   void (*cleanup_function)(void*,void*,void(*free_function)(void*,void*)) )
{
    reliable_assert( sequence_buffer );
    int index = sequence % sequence_buffer->num_entries;
    if ( sequence_buffer->entry_sequence[index] != 0xFFFFFFFF )
    {
        sequence_buffer->entry_sequence[index] = 0xFFFFFFFF;
        cleanup_function( sequence_buffer->entry_data + sequence_buffer->entry_stride * index, sequence_buffer->allocator_context, sequence_buffer->free_function );
    }
}

int reliable_sequence_buffer_available( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    reliable_assert( sequence_buffer );
    return sequence_buffer->entry_sequence[ sequence % sequence_buffer->num_entries ] == 0xFFFFFFFF;
}

int reliable_sequence_buffer_exists( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    reliable_assert( sequence_buffer );
    return sequence_buffer->entry_sequence[ sequence % sequence_buffer->num_entries ] == (uint32_t) sequence;
}

void * reliable_sequence_buffer_find( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t sequence )
{
    reliable_assert( sequence_buffer );
    int index = sequence % sequence_buffer->num_entries;
    return ( ( sequence_buffer->entry_sequence[index] == (uint32_t) sequence ) ) ? ( sequence_buffer->entry_data + index * sequence_buffer->entry_stride ) : NULL;

}

void * reliable_sequence_buffer_at_index( struct reliable_sequence_buffer_t * sequence_buffer, int index )
{
    reliable_assert( sequence_buffer );
    reliable_assert( index >= 0 );
    reliable_assert( index < sequence_buffer->num_entries );
    return sequence_buffer->entry_sequence[index] != 0xFFFFFFFF ? ( sequence_buffer->entry_data + index * sequence_buffer->entry_stride ) : NULL;
}

void reliable_sequence_buffer_generate_ack_bits( struct reliable_sequence_buffer_t * sequence_buffer, uint16_t * ack, uint32_t * ack_bits )
{
    reliable_assert( sequence_buffer );
    reliable_assert( ack );
    reliable_assert( ack_bits );
    *ack = sequence_buffer->sequence - 1;
    *ack_bits = 0;
    uint32_t mask = 1;
    int i;
    for ( i = 0; i < 32; ++i )
    {
        uint16_t sequence = *ack - ((uint16_t)i);
        if ( reliable_sequence_buffer_exists( sequence_buffer, sequence ) )
            *ack_bits |= mask;
        mask <<= 1;
    }
}

// ---------------------------------------------------------------

void reliable_write_uint8( uint8_t ** p, uint8_t value )
{
    **p = value;
    ++(*p);
}

void reliable_write_uint16( uint8_t ** p, uint16_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = value >> 8;
    *p += 2;
}

void reliable_write_uint32( uint8_t ** p, uint32_t value )
{
    (*p)[0] = value & 0xFF;
    (*p)[1] = ( value >> 8  ) & 0xFF;
    (*p)[2] = ( value >> 16 ) & 0xFF;
    (*p)[3] = value >> 24;
    *p += 4;
}

void reliable_write_uint64( uint8_t ** p, uint64_t value )
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

void reliable_write_bytes( uint8_t ** p, uint8_t * byte_array, int num_bytes )
{
    int i;
    for ( i = 0; i < num_bytes; ++i )
    {
        reliable_write_uint8( p, byte_array[i] );
    }
}

uint8_t reliable_read_uint8( uint8_t ** p )
{
    uint8_t value = **p;
    ++(*p);
    return value;
}

uint16_t reliable_read_uint16( uint8_t ** p )
{
    uint16_t value;
    value = (*p)[0];
    value |= ( ( (uint16_t)( (*p)[1] ) ) << 8 );
    *p += 2;
    return value;
}

uint32_t reliable_read_uint32( uint8_t ** p )
{
    uint32_t value;
    value  = (*p)[0];
    value |= ( ( (uint32_t)( (*p)[1] ) ) << 8 );
    value |= ( ( (uint32_t)( (*p)[2] ) ) << 16 );
    value |= ( ( (uint32_t)( (*p)[3] ) ) << 24 );
    *p += 4;
    return value;
}

uint64_t reliable_read_uint64( uint8_t ** p )
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

void reliable_read_bytes( uint8_t ** p, uint8_t * byte_array, int num_bytes )
{
    int i;
    for ( i = 0; i < num_bytes; ++i )
    {
        byte_array[i] = reliable_read_uint8( p );
    }
}

// ---------------------------------------------------------------

struct reliable_fragment_reassembly_data_t
{
    uint16_t sequence;
    uint16_t ack;
    uint32_t ack_bits;
    int num_fragments_received;
    int num_fragments_total;
    uint8_t * packet_data;
    int packet_bytes;
    int packet_header_bytes;
    uint8_t fragment_received[256];
};

void reliable_fragment_reassembly_data_cleanup( void * data, void * allocator_context, void (*free_function)(void*,void*) )

{
    reliable_assert( free_function );
    struct reliable_fragment_reassembly_data_t * reassembly_data = (struct reliable_fragment_reassembly_data_t*) data;
    if ( reassembly_data->packet_data )
    {
        free_function( allocator_context, reassembly_data->packet_data );
        reassembly_data->packet_data = NULL;
    }
}

// ---------------------------------------------------------------

struct reliable_endpoint_t
{
    void * allocator_context;
    void * (*allocate_function)(void*,size_t);
    void (*free_function)(void*,void*);
    struct reliable_config_t config;
    double time;
    float rtt;
    float packet_loss;
    float sent_bandwidth_kbps;
    float received_bandwidth_kbps;
    float acked_bandwidth_kbps;
    int num_acks;
    uint16_t * acks;
    uint16_t sequence;
    struct reliable_sequence_buffer_t * sent_packets;
    struct reliable_sequence_buffer_t * received_packets;
    struct reliable_sequence_buffer_t * fragment_reassembly;
    uint64_t counters[RELIABLE_ENDPOINT_NUM_COUNTERS];
};

struct reliable_sent_packet_data_t
{
    double time;
    uint32_t acked : 1;
    uint32_t packet_bytes : 31;
};

struct reliable_received_packet_data_t
{
    double time;
    uint32_t packet_bytes;
};

void reliable_default_config( struct reliable_config_t * config )
{
    reliable_assert( config );
    memset( config, 0, sizeof( struct reliable_config_t ) );
    config->name[0] = 'e';
    config->name[1] = 'n';
    config->name[2] = 'd';
    config->name[3] = 'p';
    config->name[4] = 'o';
    config->name[5] = 'i';
    config->name[6] = 'n';
    config->name[7] = 't';
    config->name[8] = '\0';
    config->max_packet_size = 16 * 1024;
    config->fragment_above = 1024;
    config->max_fragments = 16;
    config->fragment_size = 1024;
    config->ack_buffer_size = 256;
    config->sent_packets_buffer_size = 256;
    config->received_packets_buffer_size = 256;
    config->fragment_reassembly_buffer_size = 64;
    config->rtt_smoothing_factor = 0.0025f;
    config->packet_loss_smoothing_factor = 0.1f;
    config->bandwidth_smoothing_factor = 0.1f;
    config->packet_header_size = 28;        // note: UDP over IPv4 = 20 + 8 bytes, UDP over IPv6 = 40 + 8 bytes
}

struct reliable_endpoint_t * reliable_endpoint_create( struct reliable_config_t * config, double time )
{
    reliable_assert( config );
    reliable_assert( config->max_packet_size > 0 );
    reliable_assert( config->fragment_above > 0 );
    reliable_assert( config->max_fragments > 0 );
    reliable_assert( config->max_fragments <= 256 );
    reliable_assert( config->fragment_size > 0 );
    reliable_assert( config->ack_buffer_size > 0 );
    reliable_assert( config->sent_packets_buffer_size > 0 );
    reliable_assert( config->received_packets_buffer_size > 0 );
    reliable_assert( config->transmit_packet_function != NULL );
    reliable_assert( config->process_packet_function != NULL );

    void * allocator_context = config->allocator_context;
    void * (*allocate_function)(void*,size_t) = config->allocate_function;
    void (*free_function)(void*,void*) = config->free_function;

    if ( allocate_function == NULL )
    {
        allocate_function = reliable_default_allocate_function;
    }

    if ( free_function == NULL )
    {
        free_function = reliable_default_free_function;
    }

    struct reliable_endpoint_t * endpoint = (struct reliable_endpoint_t*) allocate_function( allocator_context, sizeof( struct reliable_endpoint_t ) );

    reliable_assert( endpoint );

    memset( endpoint, 0, sizeof( struct reliable_endpoint_t ) );

    endpoint->allocator_context = allocator_context;
    endpoint->allocate_function = allocate_function;
    endpoint->free_function = free_function;
    endpoint->config = *config;
    endpoint->time = time;

    endpoint->acks = (uint16_t*) allocate_function( allocator_context, config->ack_buffer_size * sizeof( uint16_t ) );
    
    endpoint->sent_packets = reliable_sequence_buffer_create( config->sent_packets_buffer_size, 
                                                              sizeof( struct reliable_sent_packet_data_t ), 
                                                              allocator_context, 
                                                              allocate_function, 
                                                              free_function );

    endpoint->received_packets = reliable_sequence_buffer_create( config->received_packets_buffer_size, 
                                                                  sizeof( struct reliable_received_packet_data_t ), 
                                                                  allocator_context, 
                                                                  allocate_function, 
                                                                  free_function );
    
    endpoint->fragment_reassembly = reliable_sequence_buffer_create( config->fragment_reassembly_buffer_size, 
                                                                     sizeof( struct reliable_fragment_reassembly_data_t ), 
                                                                     allocator_context, 
                                                                     allocate_function, 
                                                                     free_function );

    memset( endpoint->acks, 0, config->ack_buffer_size * sizeof( uint16_t ) );

    return endpoint;
}

void reliable_endpoint_destroy( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    reliable_assert( endpoint->acks );
    reliable_assert( endpoint->sent_packets );
    reliable_assert( endpoint->received_packets );

    int i;
    for ( i = 0; i < endpoint->config.fragment_reassembly_buffer_size; ++i )
    {
        struct reliable_fragment_reassembly_data_t * reassembly_data = (struct reliable_fragment_reassembly_data_t*) 
            reliable_sequence_buffer_at_index( endpoint->fragment_reassembly, i );

        if ( reassembly_data && reassembly_data->packet_data )
        {
            endpoint->free_function( endpoint->allocator_context, reassembly_data->packet_data );
            reassembly_data->packet_data = NULL;
        }
    }

    endpoint->free_function( endpoint->allocator_context, endpoint->acks );

    reliable_sequence_buffer_destroy( endpoint->sent_packets );
    reliable_sequence_buffer_destroy( endpoint->received_packets );
    reliable_sequence_buffer_destroy( endpoint->fragment_reassembly );

    endpoint->free_function( endpoint->allocator_context, endpoint );
}

uint16_t reliable_endpoint_next_packet_sequence( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->sequence;
}

int reliable_write_packet_header( uint8_t * packet_data, uint16_t sequence, uint16_t ack, uint32_t ack_bits )
{
    uint8_t * p = packet_data;

    uint8_t prefix_byte = 0;

    if ( ( ack_bits & 0x000000FF ) != 0x000000FF )
    {
        prefix_byte |= (1<<1);
    }

    if ( ( ack_bits & 0x0000FF00 ) != 0x0000FF00 )
    {
        prefix_byte |= (1<<2);
    }

    if ( ( ack_bits & 0x00FF0000 ) != 0x00FF0000 )
    {
        prefix_byte |= (1<<3);
    }

    if ( ( ack_bits & 0xFF000000 ) != 0xFF000000 )
    {
        prefix_byte |= (1<<4);
    }

    int sequence_difference = sequence - ack;
    if ( sequence_difference < 0 )
        sequence_difference += 65536;
    if ( sequence_difference <= 255 )
        prefix_byte |= (1<<5);

    reliable_write_uint8( &p, prefix_byte );

    reliable_write_uint16( &p, sequence );

    if ( sequence_difference <= 255 )
    {
        reliable_write_uint8( &p, (uint8_t) sequence_difference );
    }
    else
    {
        reliable_write_uint16( &p, ack );
    }

    if ( ( ack_bits & 0x000000FF ) != 0x000000FF )
    {
        reliable_write_uint8( &p, (uint8_t) ( ack_bits & 0x000000FF ) );
    }

    if ( ( ack_bits & 0x0000FF00 ) != 0x0000FF00 )
    {
        reliable_write_uint8( &p, (uint8_t) ( ( ack_bits & 0x0000FF00 ) >> 8 ) );
    }

    if ( ( ack_bits & 0x00FF0000 ) != 0x00FF0000 )
    {
        reliable_write_uint8( &p, (uint8_t) ( ( ack_bits & 0x00FF0000 ) >> 16 ) );
    }

    if ( ( ack_bits & 0xFF000000 ) != 0xFF000000 )
    {
        reliable_write_uint8( &p, (uint8_t) ( ( ack_bits & 0xFF000000 ) >> 24 ) );
    }

    reliable_assert( p - packet_data <= RELIABLE_MAX_PACKET_HEADER_BYTES );

    return (int) ( p - packet_data );
}

void reliable_endpoint_send_packet( struct reliable_endpoint_t * endpoint, uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( endpoint );
    reliable_assert( packet_data );
    reliable_assert( packet_bytes > 0 );

    if ( packet_bytes > endpoint->config.max_packet_size )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] packet too large to send. packet is %d bytes, maximum is %d\n", 
            endpoint->config.name, packet_bytes, endpoint->config.max_packet_size );
        endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_SEND]++;
        return;
    }

    uint16_t sequence = endpoint->sequence++;
    uint16_t ack;
    uint32_t ack_bits;

    reliable_sequence_buffer_generate_ack_bits( endpoint->received_packets, &ack, &ack_bits );

    reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] sending packet %d\n", endpoint->config.name, sequence );

    struct reliable_sent_packet_data_t * sent_packet_data = (struct reliable_sent_packet_data_t*) reliable_sequence_buffer_insert( endpoint->sent_packets, sequence );

    reliable_assert( sent_packet_data );

    sent_packet_data->time = endpoint->time;
    sent_packet_data->packet_bytes = endpoint->config.packet_header_size + packet_bytes;
    sent_packet_data->acked = 0;

    if ( packet_bytes <= endpoint->config.fragment_above )
    {
        // regular packet

        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] sending packet %d without fragmentation\n", endpoint->config.name, sequence );

        uint8_t * transmit_packet_data = (uint8_t*) endpoint->allocate_function( endpoint->allocator_context, packet_bytes + RELIABLE_MAX_PACKET_HEADER_BYTES );

        int packet_header_bytes = reliable_write_packet_header( transmit_packet_data, sequence, ack, ack_bits );

        memcpy( transmit_packet_data + packet_header_bytes, packet_data, packet_bytes );

        endpoint->config.transmit_packet_function( endpoint->config.context, endpoint->config.id, sequence, transmit_packet_data, packet_header_bytes + packet_bytes );

        endpoint->free_function( endpoint->allocator_context, transmit_packet_data );
    }
    else
    {
        // fragmented packet

        uint8_t packet_header[RELIABLE_MAX_PACKET_HEADER_BYTES];

        memset( packet_header, 0, RELIABLE_MAX_PACKET_HEADER_BYTES );

        int packet_header_bytes = reliable_write_packet_header( packet_header, sequence, ack, ack_bits );        

        int num_fragments = ( packet_bytes / endpoint->config.fragment_size ) + ( ( packet_bytes % endpoint->config.fragment_size ) != 0 ? 1 : 0 );

        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] sending packet %d as %d fragments\n", endpoint->config.name, sequence, num_fragments );

        reliable_assert( num_fragments >= 1 );
        reliable_assert( num_fragments <= endpoint->config.max_fragments );

        int fragment_buffer_size = RELIABLE_FRAGMENT_HEADER_BYTES + RELIABLE_MAX_PACKET_HEADER_BYTES + endpoint->config.fragment_size;

        uint8_t * fragment_packet_data = (uint8_t*) endpoint->allocate_function( endpoint->allocator_context, fragment_buffer_size );

        uint8_t * q = packet_data;

        uint8_t * end = q + packet_bytes;

        int fragment_id;
        for ( fragment_id = 0; fragment_id < num_fragments; ++fragment_id )
        {
            uint8_t * p = fragment_packet_data;

            reliable_write_uint8( &p, 1 );
            reliable_write_uint16( &p, sequence );
            reliable_write_uint8( &p, (uint8_t) fragment_id );
            reliable_write_uint8( &p, (uint8_t) ( num_fragments - 1 ) );

            if ( fragment_id == 0 )
            {
                memcpy( p, packet_header, packet_header_bytes );
                p += packet_header_bytes;
            }

            int bytes_to_copy = endpoint->config.fragment_size;
            if ( q + bytes_to_copy > end )
            {
                bytes_to_copy = (int) ( end - q );
            }

            memcpy( p, q, bytes_to_copy );

            p += bytes_to_copy;
            q += bytes_to_copy;

            int fragment_packet_bytes = (int) ( p - fragment_packet_data );

            endpoint->config.transmit_packet_function( endpoint->config.context, endpoint->config.id, sequence, fragment_packet_data, fragment_packet_bytes );

            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_SENT]++;
        }

        endpoint->free_function( endpoint->allocator_context, fragment_packet_data );
    }

    endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_SENT]++;
}

int reliable_read_packet_header( RELIABLE_CONST char * name, uint8_t * packet_data, int packet_bytes, uint16_t * sequence, uint16_t * ack, uint32_t * ack_bits )
{
    if ( packet_bytes < 3 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet too small for packet header (1)\n", name );
        return -1;
    }

    uint8_t * p = packet_data;

    uint8_t prefix_byte = reliable_read_uint8( &p );

    if ( ( prefix_byte & 1 ) != 0 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] prefix byte does not indicate a regular packet\n", name );
        return -1;
    }

    *sequence = reliable_read_uint16( &p );

    if ( prefix_byte & (1<<5) )
    {
        if ( packet_bytes < 3 + 1 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet too small for packet header (2)\n", name );
            return -1;
        }
        uint8_t sequence_difference = reliable_read_uint8( &p );
        *ack = *sequence - sequence_difference;
    }
    else
    {
        if ( packet_bytes < 3 + 2 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet too small for packet header (3)\n", name );
            return -1;
        }
        *ack = reliable_read_uint16( &p );
    }

    int expected_bytes = 0;
    int i;
    for ( i = 1; i <= 4; ++i )
    {
        if ( prefix_byte & (1<<i) )
        {
            expected_bytes++;
        }
    }
    if ( packet_bytes < ( p - packet_data ) + expected_bytes )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet too small for packet header (4)\n", name );
        return -1;
    }

    *ack_bits = 0xFFFFFFFF;

    if ( prefix_byte & (1<<1) )
    {
        *ack_bits &= 0xFFFFFF00;
        *ack_bits |= (uint32_t) ( reliable_read_uint8( &p ) );
    }

    if ( prefix_byte & (1<<2) )
    {
        *ack_bits &= 0xFFFF00FF;
        *ack_bits |= (uint32_t) ( reliable_read_uint8( &p ) ) << 8;
    }

    if ( prefix_byte & (1<<3) )
    {
        *ack_bits &= 0xFF00FFFF;
        *ack_bits |= (uint32_t) ( reliable_read_uint8( &p ) ) << 16;
    }

    if ( prefix_byte & (1<<4) )
    {
        *ack_bits &= 0x00FFFFFF;
        *ack_bits |= (uint32_t) ( reliable_read_uint8( &p ) ) << 24;
    }

    return (int) ( p - packet_data );
}

int reliable_read_fragment_header( char * name, 
                                   uint8_t * packet_data, 
                                   int packet_bytes, 
                                   int max_fragments, 
                                   int fragment_size, 
                                   int * fragment_id, 
                                   int * num_fragments, 
                                   int * fragment_bytes, 
                                   uint16_t * sequence, 
                                   uint16_t * ack, 
                                   uint32_t * ack_bits )
{
    if ( packet_bytes < RELIABLE_FRAGMENT_HEADER_BYTES )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet is too small to read fragment header\n", name );
        return -1;
    }

    uint8_t * p = packet_data;

    uint8_t prefix_byte = reliable_read_uint8( &p );
    if ( prefix_byte != 1 )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] prefix byte is not a fragment\n", name );
        return -1;
    }
    
    *sequence = reliable_read_uint16( &p );
    *fragment_id = (int) reliable_read_uint8( &p );
    *num_fragments = ( (int) reliable_read_uint8( &p ) ) + 1;

    if ( *num_fragments > max_fragments )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] num fragments %d outside of range of max fragments %d\n", name, *num_fragments, max_fragments );
        return -1;
    }

    if ( *fragment_id >= *num_fragments )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] fragment id %d outside of range of num fragments %d\n", name, *fragment_id, *num_fragments );
        return -1;
    }

    *fragment_bytes = packet_bytes - RELIABLE_FRAGMENT_HEADER_BYTES;

    uint16_t packet_sequence = 0;
    uint16_t packet_ack = 0;
    uint32_t packet_ack_bits = 0;

    if ( *fragment_id == 0 )
    {
        int packet_header_bytes = reliable_read_packet_header( name, 
                                                               packet_data + RELIABLE_FRAGMENT_HEADER_BYTES, 
                                                               packet_bytes, 
                                                               &packet_sequence, 
                                                               &packet_ack, 
                                                               &packet_ack_bits );

        if ( packet_header_bytes < 0 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] bad packet header in fragment\n", name );
            return -1;
        }

        if ( packet_sequence != *sequence )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] bad packet sequence in fragment. expected %d, got %d\n", name, *sequence, packet_sequence );
            return -1;
        }

        *fragment_bytes = packet_bytes - packet_header_bytes - RELIABLE_FRAGMENT_HEADER_BYTES;
    }

    *ack = packet_ack;
    *ack_bits = packet_ack_bits;

    if ( *fragment_bytes > fragment_size )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] fragment bytes %d > fragment size %d\n", name, *fragment_bytes, fragment_size );
        return - 1;
    }

    if ( *fragment_id != *num_fragments - 1 && *fragment_bytes != fragment_size )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] fragment %d is %d bytes, which is not the expected fragment size %d\n", 
            name, *fragment_id, *fragment_bytes, fragment_size );
        return -1;
    }

    return (int) ( p - packet_data );
}

void reliable_store_fragment_data( struct reliable_fragment_reassembly_data_t * reassembly_data, 
                                   uint16_t sequence, 
                                   uint16_t ack, 
                                   uint32_t ack_bits, 
                                   int fragment_id, 
                                   int fragment_size, 
                                   uint8_t * fragment_data, 
                                   int fragment_bytes )
{
    if ( fragment_id == 0 )
    {
        uint8_t packet_header[RELIABLE_MAX_PACKET_HEADER_BYTES];

        memset( packet_header, 0, RELIABLE_MAX_PACKET_HEADER_BYTES );

        reassembly_data->packet_header_bytes = reliable_write_packet_header( packet_header, sequence, ack, ack_bits );

        memcpy( reassembly_data->packet_data + RELIABLE_MAX_PACKET_HEADER_BYTES - reassembly_data->packet_header_bytes, 
                packet_header, 
                reassembly_data->packet_header_bytes );

        fragment_data += reassembly_data->packet_header_bytes;
        fragment_bytes -= reassembly_data->packet_header_bytes;
    }

    if ( fragment_id == reassembly_data->num_fragments_total - 1 )
    {
        reassembly_data->packet_bytes = ( reassembly_data->num_fragments_total - 1 ) * fragment_size + fragment_bytes;
    }

    memcpy( reassembly_data->packet_data + RELIABLE_MAX_PACKET_HEADER_BYTES + fragment_id * fragment_size, fragment_data, fragment_bytes );
}

void reliable_endpoint_receive_packet( struct reliable_endpoint_t * endpoint, uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( endpoint );
    reliable_assert( packet_data );
    reliable_assert( packet_bytes > 0 );

    if ( packet_bytes > endpoint->config.max_packet_size + RELIABLE_MAX_PACKET_HEADER_BYTES + RELIABLE_FRAGMENT_HEADER_BYTES )
    {
        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] packet too large to receive. packet is at least %d bytes, maximum is %d\n",
            endpoint->config.name, packet_bytes - ( RELIABLE_MAX_PACKET_HEADER_BYTES + RELIABLE_FRAGMENT_HEADER_BYTES ), endpoint->config.max_packet_size );
        endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_RECEIVE]++;
        return;
    }

    uint8_t prefix_byte = packet_data[0];

    if ( ( prefix_byte & 1 ) == 0 )
    {
        // regular packet

        endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED]++;

        uint16_t sequence;
        uint16_t ack;
        uint32_t ack_bits;

        int packet_header_bytes = reliable_read_packet_header( endpoint->config.name, packet_data, packet_bytes, &sequence, &ack, &ack_bits );
        if ( packet_header_bytes < 0 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] ignoring invalid packet. could not read packet header\n", endpoint->config.name );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_INVALID]++;
            return;
        }

        reliable_assert( packet_header_bytes <= packet_bytes );

        int packet_payload_bytes = packet_bytes - packet_header_bytes;

        if ( packet_payload_bytes > endpoint->config.max_packet_size )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] packet too large to receive. packet is at %d bytes, maximum is %d\n",
                endpoint->config.name, packet_payload_bytes, endpoint->config.max_packet_size );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_RECEIVE]++;
            return;
        }

        if ( !reliable_sequence_buffer_test_insert( endpoint->received_packets, sequence ) )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] ignoring stale packet %d\n", endpoint->config.name, sequence );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_STALE]++;
            return;
        }

        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] processing packet %d\n", endpoint->config.name, sequence );

        if ( endpoint->config.process_packet_function( endpoint->config.context, 
                                                       endpoint->config.id, 
                                                       sequence, 
                                                       packet_data + packet_header_bytes, 
                                                       packet_bytes - packet_header_bytes ) )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] process packet %d successful\n", endpoint->config.name, sequence );

            struct reliable_received_packet_data_t * received_packet_data = (struct reliable_received_packet_data_t*) 
                reliable_sequence_buffer_insert( endpoint->received_packets, sequence );

            reliable_sequence_buffer_advance_with_cleanup( endpoint->fragment_reassembly, sequence, reliable_fragment_reassembly_data_cleanup );

            reliable_assert( received_packet_data );

            received_packet_data->time = endpoint->time;
            received_packet_data->packet_bytes = endpoint->config.packet_header_size + packet_bytes;

            int i;
            for ( i = 0; i < 32; ++i )
            {
                if ( ack_bits & 1 )
                {                    
                    uint16_t ack_sequence = ack - ((uint16_t)i);
                    
                    struct reliable_sent_packet_data_t * sent_packet_data = (struct reliable_sent_packet_data_t*) 
                        reliable_sequence_buffer_find( endpoint->sent_packets, ack_sequence );

                    if ( sent_packet_data && !sent_packet_data->acked && endpoint->num_acks < endpoint->config.ack_buffer_size )
                    {
                        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] acked packet %d\n", endpoint->config.name, ack_sequence );
                        endpoint->acks[endpoint->num_acks++] = ack_sequence;
                        endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_ACKED]++;
                        sent_packet_data->acked = 1;

                        float rtt = (float) ( endpoint->time - sent_packet_data->time ) * 1000.0f;
                        reliable_assert( rtt >= 0.0 );
                        if ( ( endpoint->rtt == 0.0f && rtt > 0.0f ) || fabs( endpoint->rtt - rtt ) < 0.00001 )
                        {
                            endpoint->rtt = rtt;
                        }
                        else
                        {
                            endpoint->rtt += ( rtt - endpoint->rtt ) * endpoint->config.rtt_smoothing_factor;
                        }
                    }
                }
                ack_bits >>= 1;
            }
        }
        else
        {
            reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] process packet failed\n", endpoint->config.name );
        }
    }
    else
    {
        // fragment packet

        int fragment_id;
        int num_fragments;
        int fragment_bytes;

        uint16_t sequence;
        uint16_t ack;
        uint32_t ack_bits;

        int fragment_header_bytes = reliable_read_fragment_header( endpoint->config.name, 
                                                                   packet_data, 
                                                                   packet_bytes, 
                                                                   endpoint->config.max_fragments, 
                                                                   endpoint->config.fragment_size,
                                                                   &fragment_id, 
                                                                   &num_fragments, 
                                                                   &fragment_bytes, 
                                                                   &sequence, 
                                                                   &ack, 
                                                                   &ack_bits );

        if ( fragment_header_bytes < 0 )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] ignoring invalid fragment. could not read fragment header\n", endpoint->config.name );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID]++;
            return;
        }

        struct reliable_fragment_reassembly_data_t * reassembly_data = (struct reliable_fragment_reassembly_data_t*) 
            reliable_sequence_buffer_find( endpoint->fragment_reassembly, sequence );

        if ( !reassembly_data )
        {
            reassembly_data = (struct reliable_fragment_reassembly_data_t*) 
                reliable_sequence_buffer_insert_with_cleanup( endpoint->fragment_reassembly, sequence, reliable_fragment_reassembly_data_cleanup );

            if ( !reassembly_data )
            {
                reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] ignoring invalid fragment. could not insert in reassembly buffer (stale)\n", endpoint->config.name );
                endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID]++;
                return;
            }

            reliable_sequence_buffer_advance( endpoint->received_packets, sequence );

            int packet_buffer_size = RELIABLE_MAX_PACKET_HEADER_BYTES + num_fragments * endpoint->config.fragment_size;

            reassembly_data->sequence = sequence;
            reassembly_data->ack = 0;
            reassembly_data->ack_bits = 0;
            reassembly_data->num_fragments_received = 0;
            reassembly_data->num_fragments_total = num_fragments;
            reassembly_data->packet_data = (uint8_t*) endpoint->allocate_function( endpoint->allocator_context, packet_buffer_size );
            reassembly_data->packet_bytes = 0;
            memset( reassembly_data->fragment_received, 0, sizeof( reassembly_data->fragment_received ) );
        }

        if ( num_fragments != (int) reassembly_data->num_fragments_total )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] ignoring invalid fragment. fragment count mismatch. expected %d, got %d\n", 
                endpoint->config.name, (int) reassembly_data->num_fragments_total, num_fragments );
            endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID]++;
            return;
        }

        if ( reassembly_data->fragment_received[fragment_id] )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_ERROR, "[%s] ignoring fragment %d of packet %d. fragment already received\n", 
                endpoint->config.name, fragment_id, sequence );
            return;
        }

        reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] received fragment %d of packet %d (%d/%d)\n", 
            endpoint->config.name, fragment_id, sequence, reassembly_data->num_fragments_received+1, num_fragments );

        reassembly_data->num_fragments_received++;
        reassembly_data->fragment_received[fragment_id] = 1;

        reliable_store_fragment_data( reassembly_data, 
                                      sequence, 
                                      ack, 
                                      ack_bits, 
                                      fragment_id, 
                                      endpoint->config.fragment_size, 
                                      packet_data + fragment_header_bytes, 
                                      packet_bytes - fragment_header_bytes );

        if ( reassembly_data->num_fragments_received == reassembly_data->num_fragments_total )
        {
            reliable_printf( RELIABLE_LOG_LEVEL_DEBUG, "[%s] completed reassembly of packet %d\n", endpoint->config.name, sequence );

            reliable_endpoint_receive_packet( endpoint, 
                                              reassembly_data->packet_data + RELIABLE_MAX_PACKET_HEADER_BYTES - reassembly_data->packet_header_bytes, 
                                              reassembly_data->packet_header_bytes + reassembly_data->packet_bytes );

            reliable_sequence_buffer_remove_with_cleanup( endpoint->fragment_reassembly, sequence, reliable_fragment_reassembly_data_cleanup );
        }

        endpoint->counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_RECEIVED]++;
    }
}

void reliable_endpoint_free_packet( struct reliable_endpoint_t * endpoint, void * packet )
{
    reliable_assert( endpoint );
    reliable_assert( packet );
    endpoint->free_function( endpoint->allocator_context, packet );
}

uint16_t * reliable_endpoint_get_acks( struct reliable_endpoint_t * endpoint, int * num_acks )
{
    reliable_assert( endpoint );
    reliable_assert( num_acks );
    *num_acks = endpoint->num_acks;
    return endpoint->acks;
}

void reliable_endpoint_clear_acks( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    endpoint->num_acks = 0;
}

void reliable_endpoint_reset( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );

    endpoint->num_acks = 0;
    endpoint->sequence = 0;

    memset( endpoint->acks, 0, endpoint->config.ack_buffer_size * sizeof( uint16_t ) );
    memset( endpoint->counters, 0, RELIABLE_ENDPOINT_NUM_COUNTERS * sizeof( uint64_t ) );

    int i;
    for ( i = 0; i < endpoint->config.fragment_reassembly_buffer_size; ++i )
    {
        struct reliable_fragment_reassembly_data_t * reassembly_data = (struct reliable_fragment_reassembly_data_t*) 
            reliable_sequence_buffer_at_index( endpoint->fragment_reassembly, i );

        if ( reassembly_data && reassembly_data->packet_data )
        {
            endpoint->free_function( endpoint->allocator_context, reassembly_data->packet_data );
            reassembly_data->packet_data = NULL;
        }
    }

    reliable_sequence_buffer_reset( endpoint->sent_packets );
    reliable_sequence_buffer_reset( endpoint->received_packets );
    reliable_sequence_buffer_reset( endpoint->fragment_reassembly );
}

void reliable_endpoint_update( struct reliable_endpoint_t * endpoint, double time )
{
    reliable_assert( endpoint );

    endpoint->time = time;
    
    // calculate packet loss
    {
        uint32_t base_sequence = ( endpoint->sent_packets->sequence - endpoint->config.sent_packets_buffer_size + 1 ) + 0xFFFF;
        int i;
        int num_dropped = 0;
        int num_samples = endpoint->config.sent_packets_buffer_size / 2;
        for ( i = 0; i < num_samples; ++i )
        {
            uint16_t sequence = (uint16_t) ( base_sequence + i );
            struct reliable_sent_packet_data_t * sent_packet_data = (struct reliable_sent_packet_data_t*) 
                reliable_sequence_buffer_find( endpoint->sent_packets, sequence );
            if ( sent_packet_data && !sent_packet_data->acked )
            {
                num_dropped++;
            }
        }
        float packet_loss = ( (float) num_dropped ) / ( (float) num_samples ) * 100.0f;
        if ( fabs( endpoint->packet_loss - packet_loss ) > 0.00001 )
        {
            endpoint->packet_loss += ( packet_loss - endpoint->packet_loss ) * endpoint->config.packet_loss_smoothing_factor;
        }
        else
        {
            endpoint->packet_loss = packet_loss;
        }
    }

    // calculate sent bandwidth
    {
        uint32_t base_sequence = ( endpoint->sent_packets->sequence - endpoint->config.sent_packets_buffer_size + 1 ) + 0xFFFF;
        int i;
        int bytes_sent = 0;
        double start_time = FLT_MAX;
        double finish_time = 0.0;
        int num_samples = endpoint->config.sent_packets_buffer_size / 2;
        for ( i = 0; i < num_samples; ++i )
        {
            uint16_t sequence = (uint16_t) ( base_sequence + i );
            struct reliable_sent_packet_data_t * sent_packet_data = (struct reliable_sent_packet_data_t*) 
                reliable_sequence_buffer_find( endpoint->sent_packets, sequence );
            if ( !sent_packet_data )
            {
                continue;
            }
            bytes_sent += sent_packet_data->packet_bytes;
            if ( sent_packet_data->time < start_time )
            {
                start_time = sent_packet_data->time;
            }
            if ( sent_packet_data->time > finish_time )
            {
                finish_time = sent_packet_data->time;
            }
        }
        if ( start_time != FLT_MAX && finish_time != 0.0 )
        {
            float sent_bandwidth_kbps = (float) ( ( (double) bytes_sent ) / ( finish_time - start_time ) * 8.0f / 1000.0f );
            if ( fabs( endpoint->sent_bandwidth_kbps - sent_bandwidth_kbps ) > 0.00001 )
            {
                endpoint->sent_bandwidth_kbps += ( sent_bandwidth_kbps - endpoint->sent_bandwidth_kbps ) * endpoint->config.bandwidth_smoothing_factor;
            }
            else
            {
                endpoint->sent_bandwidth_kbps = sent_bandwidth_kbps;
            }
        }
    }

    // calculate received bandwidth
    {
        uint32_t base_sequence = ( endpoint->received_packets->sequence - endpoint->config.received_packets_buffer_size + 1 ) + 0xFFFF;
        int i;
        int bytes_sent = 0;
        double start_time = FLT_MAX;
        double finish_time = 0.0;
        int num_samples = endpoint->config.received_packets_buffer_size / 2;
        for ( i = 0; i < num_samples; ++i )
        {
            uint16_t sequence = (uint16_t) ( base_sequence + i );
            struct reliable_received_packet_data_t * received_packet_data = (struct reliable_received_packet_data_t*) 
                reliable_sequence_buffer_find( endpoint->received_packets, sequence );
            if ( !received_packet_data )
            {
                continue;
            }
            bytes_sent += received_packet_data->packet_bytes;
            if ( received_packet_data->time < start_time )
            {
                start_time = received_packet_data->time;
            }
            if ( received_packet_data->time > finish_time )
            {
                finish_time = received_packet_data->time;
            }
        }
        if ( start_time != FLT_MAX && finish_time != 0.0 )
        {
            float received_bandwidth_kbps = (float) ( ( (double) bytes_sent ) / ( finish_time - start_time ) * 8.0f / 1000.0f );
            if ( fabs( endpoint->received_bandwidth_kbps - received_bandwidth_kbps ) > 0.00001 )
            {
                endpoint->received_bandwidth_kbps += ( received_bandwidth_kbps - endpoint->received_bandwidth_kbps ) * endpoint->config.bandwidth_smoothing_factor;
            }
            else
            {
                endpoint->received_bandwidth_kbps = received_bandwidth_kbps;
            }
        }
    }

    // calculate acked bandwidth
    {
        uint32_t base_sequence = ( endpoint->sent_packets->sequence - endpoint->config.sent_packets_buffer_size + 1 ) + 0xFFFF;
        int i;
        int bytes_sent = 0;
        double start_time = FLT_MAX;
        double finish_time = 0.0;
        int num_samples = endpoint->config.sent_packets_buffer_size / 2;
        for ( i = 0; i < num_samples; ++i )
        {
            uint16_t sequence = (uint16_t) ( base_sequence + i );
            struct reliable_sent_packet_data_t * sent_packet_data = (struct reliable_sent_packet_data_t*) 
                reliable_sequence_buffer_find( endpoint->sent_packets, sequence );
            if ( !sent_packet_data || !sent_packet_data->acked )
            {
                continue;
            }
            bytes_sent += sent_packet_data->packet_bytes;
            if ( sent_packet_data->time < start_time )
            {
                start_time = sent_packet_data->time;
            }
            if ( sent_packet_data->time > finish_time )
            {
                finish_time = sent_packet_data->time;
            }
        }
        if ( start_time != FLT_MAX && finish_time != 0.0 )
        {
            float acked_bandwidth_kbps = (float) ( ( (double) bytes_sent ) / ( finish_time - start_time ) * 8.0f / 1000.0f );
            if ( fabs( endpoint->acked_bandwidth_kbps - acked_bandwidth_kbps ) > 0.00001 )
            {
                endpoint->acked_bandwidth_kbps += ( acked_bandwidth_kbps - endpoint->acked_bandwidth_kbps ) * endpoint->config.bandwidth_smoothing_factor;
            }
            else
            {
                endpoint->acked_bandwidth_kbps = acked_bandwidth_kbps;
            }
        }
    }
}

float reliable_endpoint_rtt( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->rtt;
}

float reliable_endpoint_packet_loss( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->packet_loss;
}

void reliable_endpoint_bandwidth( struct reliable_endpoint_t * endpoint, float * sent_bandwidth_kbps, float * received_bandwidth_kbps, float * acked_bandwidth_kbps )
{
    reliable_assert( endpoint );
    reliable_assert( sent_bandwidth_kbps );
    reliable_assert( acked_bandwidth_kbps );
    reliable_assert( received_bandwidth_kbps );
    *sent_bandwidth_kbps = endpoint->sent_bandwidth_kbps;
    *received_bandwidth_kbps = endpoint->received_bandwidth_kbps;
    *acked_bandwidth_kbps = endpoint->acked_bandwidth_kbps;
}

RELIABLE_CONST uint64_t * reliable_endpoint_counters( struct reliable_endpoint_t * endpoint )
{
    reliable_assert( endpoint );
    return endpoint->counters;
}

void reliable_copy_string( char * dest, RELIABLE_CONST char * source, size_t dest_size )
{
    reliable_assert( dest );
    reliable_assert( source );
    reliable_assert( dest_size >= 1 );
    memset( dest, 0, dest_size );
    for ( size_t i = 0; i < dest_size - 1; i++ )
    {
        if ( source[i] == '\0' )
            break;
        dest[i] = source[i];
    }
}

// ---------------------------------------------------------------

#if RELIABLE_ENABLE_TESTS

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

static void check_handler( RELIABLE_CONST char * condition, 
                           RELIABLE_CONST char * function,
                           RELIABLE_CONST char * file,
                           int line )
{
    printf( "check failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
#ifdef RELIABLE_DEBUG
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
        check_handler( #condition, (RELIABLE_CONST char*) __FUNCTION__, __FILE__, __LINE__ );                   \
    }                                                                                                           \
} while(0)

static void test_endian()
{
    uint32_t value = 0x11223344;

    char * bytes = (char*) &value;

#if RELIABLE_LITTLE_ENDIAN

    check( bytes[0] == 0x44 );
    check( bytes[1] == 0x33 );
    check( bytes[2] == 0x22 );
    check( bytes[3] == 0x11 );

#else // #if RELIABLE_LITTLE_ENDIAN

    check( bytes[3] == 0x44 );
    check( bytes[2] == 0x33 );
    check( bytes[1] == 0x22 );
    check( bytes[0] == 0x11 );

#endif // #if RELIABLE_LITTLE_ENDIAN
}

struct test_sequence_data_t
{
    uint16_t sequence;
};

#define TEST_SEQUENCE_BUFFER_SIZE 256

static void test_sequence_buffer()
{
    struct reliable_sequence_buffer_t * sequence_buffer = reliable_sequence_buffer_create( TEST_SEQUENCE_BUFFER_SIZE, 
                                                                                           sizeof( struct test_sequence_data_t ), 
                                                                                           NULL, 
                                                                                           NULL, 
                                                                                           NULL );

    check( sequence_buffer );
    check( sequence_buffer->sequence == 0 );
    check( sequence_buffer->num_entries == TEST_SEQUENCE_BUFFER_SIZE );
    check( sequence_buffer->entry_stride == sizeof( struct test_sequence_data_t ) );

    int i;
    for ( i = 0; i < TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        check( reliable_sequence_buffer_find( sequence_buffer, ((uint16_t)i) ) == NULL );
    }                                                                      

    for ( i = 0; i <= TEST_SEQUENCE_BUFFER_SIZE*4; ++i )
    {
        struct test_sequence_data_t * entry = (struct test_sequence_data_t*) reliable_sequence_buffer_insert( sequence_buffer, ((uint16_t)i) );
        check( entry );
        entry->sequence = (uint16_t) i;
        check( sequence_buffer->sequence == i + 1 );
    }

    for ( i = 0; i <= TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        struct test_sequence_data_t * entry = (struct test_sequence_data_t*) reliable_sequence_buffer_insert( sequence_buffer, ((uint16_t)i) );
        check( entry == NULL );
    }    

    int index = TEST_SEQUENCE_BUFFER_SIZE * 4;
    for ( i = 0; i < TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        struct test_sequence_data_t * entry = (struct test_sequence_data_t*) reliable_sequence_buffer_find( sequence_buffer, (uint16_t) index );
        check( entry );
        check( entry->sequence == (uint32_t) index );
        index--;
    }

    reliable_sequence_buffer_reset( sequence_buffer );

    check( sequence_buffer );
    check( sequence_buffer->sequence == 0 );
    check( sequence_buffer->num_entries == TEST_SEQUENCE_BUFFER_SIZE );
    check( sequence_buffer->entry_stride == sizeof( struct test_sequence_data_t ) );

    for ( i = 0; i < TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        check( reliable_sequence_buffer_find( sequence_buffer, (uint16_t) i ) == NULL );
    }

    reliable_sequence_buffer_destroy( sequence_buffer );
}

static void test_generate_ack_bits()
{
    struct reliable_sequence_buffer_t * sequence_buffer = reliable_sequence_buffer_create( TEST_SEQUENCE_BUFFER_SIZE, 
                                                                                           sizeof( struct test_sequence_data_t ), 
                                                                                           NULL, 
                                                                                           NULL, 
                                                                                           NULL );

    uint16_t ack = 0;
    uint32_t ack_bits = 0xFFFFFFFF;

    reliable_sequence_buffer_generate_ack_bits( sequence_buffer, &ack, &ack_bits );
    check( ack == 0xFFFF );
    check( ack_bits == 0 );

    int i;
    for ( i = 0; i <= TEST_SEQUENCE_BUFFER_SIZE; ++i )
    {
        reliable_sequence_buffer_insert( sequence_buffer, (uint16_t) i );
    }

    reliable_sequence_buffer_generate_ack_bits( sequence_buffer, &ack, &ack_bits );
    check( ack == TEST_SEQUENCE_BUFFER_SIZE );
    check( ack_bits == 0xFFFFFFFF );

    reliable_sequence_buffer_reset( sequence_buffer );

    uint16_t input_acks[] = { 1, 5, 9, 11 };
    int input_num_acks = sizeof( input_acks ) / sizeof( uint16_t );
    for ( i = 0; i < input_num_acks; ++i )
    {
        reliable_sequence_buffer_insert( sequence_buffer, input_acks[i] );
    }

    reliable_sequence_buffer_generate_ack_bits( sequence_buffer, &ack, &ack_bits );

    check( ack == 11 );
    check( ack_bits == ( 1 | (1<<(11-9)) | (1<<(11-5)) | (1<<(11-1)) ) );

    reliable_sequence_buffer_destroy( sequence_buffer );
}

static void test_packet_header()
{
    uint16_t write_sequence;
    uint16_t write_ack;
    uint32_t write_ack_bits;

    uint16_t read_sequence;
    uint16_t read_ack;
    uint32_t read_ack_bits;

    uint8_t packet_data[RELIABLE_MAX_PACKET_HEADER_BYTES];

    // worst case, sequence and ack are far apart, no packets acked.

    write_sequence = 10000;
    write_ack = 100;
    write_ack_bits = 0;

    int bytes_written = reliable_write_packet_header( packet_data, write_sequence, write_ack, write_ack_bits );

    check( bytes_written == RELIABLE_MAX_PACKET_HEADER_BYTES );

    int bytes_read = reliable_read_packet_header( "test_packet_header", packet_data, bytes_written, &read_sequence, &read_ack, &read_ack_bits );

    check( bytes_read == bytes_written );

    check( read_sequence == write_sequence );
    check( read_ack == write_ack );
    check( read_ack_bits == write_ack_bits );

    // rare case. sequence and ack are far apart, significant # of acks are missing

    write_sequence = 10000;
    write_ack = 100;
    write_ack_bits = 0xFEFEFFFE;

    bytes_written = reliable_write_packet_header( packet_data, write_sequence, write_ack, write_ack_bits );

    check( bytes_written == 1 + 2 + 2 + 3 );

    bytes_read = reliable_read_packet_header( "test_packet_header", packet_data, bytes_written, &read_sequence, &read_ack, &read_ack_bits );

    check( bytes_read == bytes_written );

    check( read_sequence == write_sequence );
    check( read_ack == write_ack );
    check( read_ack_bits == write_ack_bits );

    // common case under packet loss. sequence and ack are close together, some acks are missing

    write_sequence = 200;
    write_ack = 100;
    write_ack_bits = 0xFFFEFFFF;

    bytes_written = reliable_write_packet_header( packet_data, write_sequence, write_ack, write_ack_bits );

    check( bytes_written == 1 + 2 + 1 + 1 );

    bytes_read = reliable_read_packet_header( "test_packet_header", packet_data, bytes_written, &read_sequence, &read_ack, &read_ack_bits );

    check( bytes_read == bytes_written );

    check( read_sequence == write_sequence );
    check( read_ack == write_ack );
    check( read_ack_bits == write_ack_bits );

    // ideal case. no packet loss.

    write_sequence = 200;
    write_ack = 100;
    write_ack_bits = 0xFFFFFFFF;

    bytes_written = reliable_write_packet_header( packet_data, write_sequence, write_ack, write_ack_bits );

    check( bytes_written == 1 + 2 + 1 );

    bytes_read = reliable_read_packet_header( "test_packet_header", packet_data, bytes_written, &read_sequence, &read_ack, &read_ack_bits );

    check( bytes_read == bytes_written );

    check( read_sequence == write_sequence );
    check( read_ack == write_ack );
    check( read_ack_bits == write_ack_bits );
}

struct test_context_t
{
    int drop;
    int allow_packets;
    struct reliable_endpoint_t * sender;
    struct reliable_endpoint_t * receiver;
};

void test_default_context( struct test_context_t * context )
{
    memset( context, 0, sizeof( *context ) );
    context->allow_packets = -1;
}

static void test_transmit_packet_function( void * _context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    (void) sequence;

    struct test_context_t * context = (struct test_context_t*) _context;

    if ( context->drop )
    {
        return;
    }

    if ( context->allow_packets >= 0 )
    {
        if ( context->allow_packets == 0 )
        {
            return;
        }

        context->allow_packets--;
    }

    if ( id == 0 )
    {
        reliable_endpoint_receive_packet( context->receiver, packet_data, packet_bytes );
    }
    else if ( id == 1 )
    {
        reliable_endpoint_receive_packet( context->sender, packet_data, packet_bytes );
    }
}

static int test_process_packet_function( void * _context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    struct test_context_t * context = (struct test_context_t*) _context;

    (void) context;
    (void) id;
    (void) sequence;
    (void) packet_data;
    (void) packet_bytes;

    return 1;
}

#define TEST_ACKS_NUM_ITERATIONS 256

static void test_acks()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    double delta_time = 0.01;

    int i;
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS; ++i )
    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );

        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.receiver, dummy_packet, sizeof( dummy_packet ) );

        reliable_endpoint_update( context.sender, time );
        reliable_endpoint_update( context.receiver, time );

        time += delta_time;
    }

    uint8_t sender_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( sender_acked_packet, 0, sizeof( sender_acked_packet ) );
    int sender_num_acks;
    uint16_t * sender_acks = reliable_endpoint_get_acks( context.sender, &sender_num_acks );
    for ( i = 0; i < sender_num_acks; ++i )
    {
        if ( sender_acks[i] < TEST_ACKS_NUM_ITERATIONS )
        {
            sender_acked_packet[sender_acks[i]] = 1;
        }
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( sender_acked_packet[i] == 1 );
    }

    uint8_t receiver_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( receiver_acked_packet, 0, sizeof( receiver_acked_packet ) );
    int receiver_num_acks;
    uint16_t * receiver_acks = reliable_endpoint_get_acks( context.receiver, &receiver_num_acks );
    for ( i = 0; i < receiver_num_acks; ++i )
    {
        if ( receiver_acks[i] < TEST_ACKS_NUM_ITERATIONS )
            receiver_acked_packet[receiver_acks[i]] = 1;
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( receiver_acked_packet[i] == 1 );
    }

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

static void test_acks_packet_loss()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    const double delta_time = 0.1f;

    int i;
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS; ++i )
    {
        uint8_t dummy_packet[8];
        memset( dummy_packet, 0, sizeof( dummy_packet ) );

        context.drop = ( i % 2 );

        reliable_endpoint_send_packet( context.sender, dummy_packet, sizeof( dummy_packet ) );
        reliable_endpoint_send_packet( context.receiver, dummy_packet, sizeof( dummy_packet ) );

        reliable_endpoint_update( context.sender, time );
        reliable_endpoint_update( context.receiver, time );

        time += delta_time;
    }

    uint8_t sender_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( sender_acked_packet, 0, sizeof( sender_acked_packet ) );
    int sender_num_acks;
    uint16_t * sender_acks = reliable_endpoint_get_acks( context.sender, &sender_num_acks );
    for ( i = 0; i < sender_num_acks; ++i )
    {
        if ( sender_acks[i] < TEST_ACKS_NUM_ITERATIONS )
        {
            sender_acked_packet[sender_acks[i]] = 1;
        }
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( sender_acked_packet[i] == (i+1) % 2 );
    }

    uint8_t receiver_acked_packet[TEST_ACKS_NUM_ITERATIONS];
    memset( receiver_acked_packet, 0, sizeof( receiver_acked_packet ) );
    int receiver_num_acks;
    uint16_t * receiver_acks = reliable_endpoint_get_acks( context.sender, &receiver_num_acks );
    for ( i = 0; i < receiver_num_acks; ++i )
    {
        if ( receiver_acks[i] < TEST_ACKS_NUM_ITERATIONS )
        {
            receiver_acked_packet[receiver_acks[i]] = 1;
        }
    }
    for ( i = 0; i < TEST_ACKS_NUM_ITERATIONS / 2; ++i )
    {
        check( receiver_acked_packet[i] == (i+1) % 2 );
    }

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

#define TEST_MAX_PACKET_BYTES (4*1024)

static void generate_packet_data_with_size( uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( packet_bytes >= 2 );
    reliable_assert( packet_bytes <= TEST_MAX_PACKET_BYTES );

    packet_data[0] = (uint8_t) ( sequence & 0xFF );
    packet_data[1] = (uint8_t) ( (sequence>>8) & 0xFF );
    int i;
    for ( i = 2; i < packet_bytes; ++i )
    {
        packet_data[i] = (uint8_t) ( ( (int)i + sequence ) % 256 );
    }
}

static int generate_packet_data( uint16_t sequence, uint8_t * packet_data )
{
    int packet_bytes = ( ( (int)sequence * 1023 ) % ( TEST_MAX_PACKET_BYTES - 2 ) ) + 2;
    generate_packet_data_with_size( sequence, packet_data, packet_bytes );
    return packet_bytes;
}

static void validate_packet_data( uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( packet_bytes >= 2 );
    reliable_assert( packet_bytes <= TEST_MAX_PACKET_BYTES );
    uint16_t sequence = 0;
    sequence |= (uint16_t) packet_data[0];
    sequence |= ( (uint16_t) packet_data[1] ) << 8;
    check( packet_bytes == ( ( (int)sequence * 1023 ) % ( TEST_MAX_PACKET_BYTES - 2 ) ) + 2 );
    int i;
    for ( i = 2; i < packet_bytes; ++i )
    {
        check( packet_data[i] == (uint8_t) ( ( (int)i + sequence ) % 256 ) );
    }
}

static int test_process_packet_function_validate( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( packet_data );
    reliable_assert( packet_bytes > 0 );
    reliable_assert( packet_bytes <= TEST_MAX_PACKET_BYTES );

    (void) context;
    (void) id;
    (void) sequence;

    validate_packet_data( packet_data, packet_bytes );

    return 1;
}

static int generate_packet_data_large( uint8_t* packet_data )
{
    int data_bytes = TEST_MAX_PACKET_BYTES - 2;
    reliable_assert( data_bytes >= 2) ;
    reliable_assert( data_bytes <= (1 << 16) );

    packet_data[0] = (uint8_t) (data_bytes & 0xFF);
    packet_data[1] = (uint8_t) ( (data_bytes >> 8) & 0xFF );
    int i;
    for ( i = 2; i < data_bytes; ++i )
    {
        packet_data[i] = (uint8_t) ( i % 256 );
    }
    return data_bytes + 2;
}

static int test_process_packet_function_validate_large( void * context, uint64_t id, uint16_t sequence, uint8_t * packet_data, int packet_bytes )
{
    reliable_assert( packet_data );
    reliable_assert( packet_bytes >= 2 );
    reliable_assert( packet_bytes <= TEST_MAX_PACKET_BYTES );

    (void) context;
    (void) id;
    (void) sequence;

    uint16_t data_bytes = 0;
    data_bytes |= (uint16_t) packet_data[0];
    data_bytes |= ( (uint16_t) packet_data[1] ) << 8;
    check( packet_bytes == data_bytes + 2 );
    int i;
    for ( i = 2; i < data_bytes; ++i )
    {
        check( packet_data[i] == (uint8_t) ( i  % 256 ) );
    }

    return 1;
}

void test_packets()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.fragment_above = 500;
    receiver_config.fragment_above = 500;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function_validate;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function_validate;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    double delta_time = 0.1;

    int i;
    for ( i = 0; i < 16; ++i )
    {
        {
            uint8_t packet_data[TEST_MAX_PACKET_BYTES];
            uint16_t sequence = reliable_endpoint_next_packet_sequence( context.sender );
            int packet_bytes = generate_packet_data( sequence, packet_data );
            reliable_endpoint_send_packet( context.sender, packet_data, packet_bytes );
        }

        {
            uint8_t packet_data[TEST_MAX_PACKET_BYTES];
            uint16_t sequence = reliable_endpoint_next_packet_sequence( context.sender );
            int packet_bytes = generate_packet_data( sequence, packet_data );
            reliable_endpoint_send_packet( context.sender, packet_data, packet_bytes );
        }

        reliable_endpoint_update( context.sender, time );
        reliable_endpoint_update( context.receiver, time );

        reliable_endpoint_clear_acks( context.sender );
        reliable_endpoint_clear_acks( context.receiver );

        time += delta_time;
    }

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

void test_large_packets()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );

    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.max_packet_size = TEST_MAX_PACKET_BYTES;
    receiver_config.max_packet_size = TEST_MAX_PACKET_BYTES;

    sender_config.fragment_above = TEST_MAX_PACKET_BYTES;
    receiver_config.fragment_above = TEST_MAX_PACKET_BYTES;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function_validate_large;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function_validate_large;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    {
        uint8_t packet_data[TEST_MAX_PACKET_BYTES];
        int packet_bytes = generate_packet_data_large( packet_data );
        check( packet_bytes == TEST_MAX_PACKET_BYTES );
        reliable_endpoint_send_packet( context.sender, packet_data, packet_bytes );
    }

    reliable_endpoint_update( context.sender, time );
    reliable_endpoint_update( context.receiver, time );

    reliable_endpoint_clear_acks( context.sender );
    reliable_endpoint_clear_acks( context.receiver );

    RELIABLE_CONST uint64_t * receiver_counters = reliable_endpoint_counters( context.receiver );
    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_TOO_LARGE_TO_RECEIVE] == 0 );
    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED] == 1 );

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

void test_sequence_buffer_rollover()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    sender_config.fragment_above = 500;
    receiver_config.fragment_above = 500;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    int num_packets_sent = 0;
    int i;
    for (i = 0; i <= 32767; ++i)
    {
        uint8_t packet_data[16];
        int packet_bytes = sizeof( packet_data ) / sizeof( uint8_t );
        reliable_endpoint_next_packet_sequence( context.sender );
        reliable_endpoint_send_packet( context.sender, packet_data, packet_bytes );

        ++num_packets_sent;
    }

    uint8_t packet_data[TEST_MAX_PACKET_BYTES];
    int packet_bytes = sizeof( packet_data ) / sizeof( uint8_t );
    reliable_endpoint_next_packet_sequence( context.sender );
    reliable_endpoint_send_packet( context.sender, packet_data, packet_bytes );
    ++num_packets_sent;

    RELIABLE_CONST uint64_t * receiver_counters = reliable_endpoint_counters( context.receiver );

    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_PACKETS_RECEIVED] == (uint16_t) num_packets_sent );
    check( receiver_counters[RELIABLE_ENDPOINT_COUNTER_NUM_FRAGMENTS_INVALID] == 0 );

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
}

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

struct test_tracking_allocate_context_t
{
    void* active_allocations[1024];
};

void * test_tracking_allocate_function( void * context, size_t bytes )
{
    struct test_tracking_allocate_context_t* tracking_context = (struct test_tracking_allocate_context_t*)context;
    void * allocation = malloc( bytes );
    int tracking_index;
    for ( tracking_index = 0; tracking_index < (int) ARRAY_LENGTH(tracking_context->active_allocations); ++tracking_index )
    {
        if ( tracking_context->active_allocations[tracking_index] == NULL )
        {
            break;
        }
    }

    reliable_assert(tracking_index < (int) ARRAY_LENGTH(tracking_context->active_allocations));
    tracking_context->active_allocations[tracking_index] = allocation;
    return allocation;
}

void test_tracking_free_function( void * context, void * pointer )
{
    struct test_tracking_allocate_context_t* tracking_context = (struct test_tracking_allocate_context_t*)context;
    int tracking_index;
    for ( tracking_index = 0; tracking_index < (int) ARRAY_LENGTH(tracking_context->active_allocations); ++tracking_index )
    {
        if ( tracking_context->active_allocations[tracking_index] == pointer )
        {
            break;
        }
    }

    reliable_assert( tracking_index < (int) ARRAY_LENGTH(tracking_context->active_allocations) );
    tracking_context->active_allocations[tracking_index] = NULL;
    free( pointer );
}

void test_fragment_cleanup()
{
    double time = 100.0;

    struct test_context_t context;
    test_default_context( &context );

    struct test_tracking_allocate_context_t tracking_alloc_context;
    memset( &tracking_alloc_context, 0, sizeof( tracking_alloc_context ) );
    
    struct reliable_config_t sender_config;
    struct reliable_config_t receiver_config;

    reliable_default_config( &sender_config );
    reliable_default_config( &receiver_config );

    receiver_config.allocator_context = &tracking_alloc_context;
    receiver_config.allocate_function = &test_tracking_allocate_function;
    receiver_config.free_function = &test_tracking_free_function;
    receiver_config.fragment_reassembly_buffer_size = 4;

    reliable_copy_string( sender_config.name, "sender", sizeof( sender_config.name ) );
    sender_config.context = &context;
    sender_config.id = 0;
    sender_config.transmit_packet_function = &test_transmit_packet_function;
    sender_config.process_packet_function = &test_process_packet_function;

    reliable_copy_string( receiver_config.name, "receiver", sizeof( receiver_config.name ) );
    receiver_config.context = &context;
    receiver_config.id = 1;
    receiver_config.transmit_packet_function = &test_transmit_packet_function;
    receiver_config.process_packet_function = &test_process_packet_function;

    context.sender = reliable_endpoint_create( &sender_config, time );
    context.receiver = reliable_endpoint_create( &receiver_config, time );

    double delta_time = 0.1;

    int packet_sizes[] = {
        sender_config.fragment_size + sender_config.fragment_size/2,
        10,
        10,
        10,
        10,
    };

    // Make sure we're sending more than receiver_config.fragment_reassembly_buffer_size packets, so the buffer wraps around.
    reliable_assert( (int) ARRAY_LENGTH( packet_sizes ) > receiver_config.fragment_reassembly_buffer_size );

    int i;
    for ( i = 0; i < (int) ARRAY_LENGTH( packet_sizes ); ++i )
    {
        // Only allow one packet per transmit, so that our fragmented packets are only partially
        // delivered.
        context.allow_packets = 1;
        {
            uint8_t packet_data[TEST_MAX_PACKET_BYTES];
            uint16_t sequence = reliable_endpoint_next_packet_sequence( context.sender );
            generate_packet_data_with_size( sequence, packet_data, packet_sizes[i] );
            reliable_endpoint_send_packet( context.sender, packet_data, packet_sizes[i]);
        }

        reliable_endpoint_update( context.sender, time );
        reliable_endpoint_update( context.receiver, time );

        reliable_endpoint_clear_acks( context.sender );
        reliable_endpoint_clear_acks( context.receiver );

        time += delta_time;
    }

    reliable_endpoint_destroy( context.sender );
    reliable_endpoint_destroy( context.receiver );
    
    // Make sure that there is no memory that hasn't been freed.
    int tracking_index;
    for ( tracking_index = 0; tracking_index < (int) ARRAY_LENGTH(tracking_alloc_context.active_allocations); ++tracking_index )
    {
        check( tracking_alloc_context.active_allocations[tracking_index] == NULL );
    }
}

#define RUN_TEST( test_function )                                           \
    do                                                                      \
    {                                                                       \
        printf( #test_function "\n" );                                      \
        test_function();                                                    \
    }                                                                       \
    while (0)

void reliable_test()
{
    //while ( 1 )
    {
        RUN_TEST( test_endian );
        RUN_TEST( test_sequence_buffer );
        RUN_TEST( test_generate_ack_bits );
        RUN_TEST( test_packet_header );
        RUN_TEST( test_acks );
        RUN_TEST( test_acks_packet_loss );
        RUN_TEST( test_packets );
        RUN_TEST( test_large_packets );
        RUN_TEST( test_sequence_buffer_rollover );
        RUN_TEST( test_fragment_cleanup );
    }
}

#endif // #if RELIABLE_ENABLE_TESTS
