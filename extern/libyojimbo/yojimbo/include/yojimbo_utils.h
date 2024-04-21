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

#ifndef YOJIMBO_UTILS_H
#define YOJIMBO_UTILS_H

#include "yojimbo_config.h"

/**
    Template function to get the minimum of two values.
    @param a The first value.
    @param b The second value.
    @returns The minimum of a and b.
 */

template <typename T> const T & yojimbo_min( const T & a, const T & b )
{
    return ( a < b ) ? a : b;
}

/**
    Template function to get the maximum of two values.
    @param a The first value.
    @param b The second value.
    @returns The maximum of a and b.
 */

template <typename T> const T & yojimbo_max( const T & a, const T & b )
{
    return ( a > b ) ? a : b;
}

/**
    Template function to clamp a value.
    @param value The value to be clamped.
    @param a The minimum value.
    @param b The minimum value.
    @returns The clamped value in [a,b].
 */

template <typename T> T yojimbo_clamp( const T & value, const T & a, const T & b )
{
    if ( value < a )
        return a;
    else if ( value > b )
        return b;
    else
        return value;
}

/**
    Swap two values.
    @param a First value.
    @param b Second value.
 */

template <typename T> void yojimbo_swap( T & a, T & b )
{
    T tmp = a;
    a = b;
    b = tmp;
};

/**
    Get the absolute value.

    @param value The input value.

    @returns The absolute value.
 */

template <typename T> T yojimbo_abs( const T & value )
{
    return ( value < 0 ) ? -value : value;
}

/**
    Generate cryptographically secure random data.
    @param data The buffer to store the random data.
    @param bytes The number of bytes of random data to generate.
 */

void yojimbo_random_bytes( uint8_t * data, int bytes );

/**
    Generate a random integer between a and b (inclusive).
    IMPORTANT: This is not a cryptographically secure random. It's used only for test functions and in the network simulator.
    @param a The minimum integer value to generate.
    @param b The maximum integer value to generate.
    @returns A pseudo random integer value in [a,b].
 */

inline int yojimbo_random_int( int a, int b )
{
    yojimbo_assert( a < b );
    int result = a + rand() % ( b - a + 1 );
    yojimbo_assert( result >= a );
    yojimbo_assert( result <= b );
    return result;
}

/**
    Generate a random float between a and b.
    IMPORTANT: This is not a cryptographically secure random. It's used only for test functions and in the network simulator.
    @param a The minimum integer value to generate.
    @param b The maximum integer value to generate.
    @returns A pseudo random float value in [a,b].
 */

inline float yojimbo_random_float( float a, float b )
{
    yojimbo_assert( a < b );
    float random = ( (float) rand() ) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

/**
    Compares two 16 bit sequence numbers and returns true if the first one is greater than the second (considering wrapping).
    IMPORTANT: This is not the same as s1 > s2!
    Greater than is defined specially to handle wrapping sequence numbers.
    If the two sequence numbers are close together, it is as normal, but they are far apart, it is assumed that they have wrapped around.
    Thus, sequence_greater_than( 1, 0 ) returns true, and so does sequence_greater_than( 0, 65535 )!
    @param s1 The first sequence number.
    @param s2 The second sequence number.
    @returns True if the s1 is greater than s2, with sequence number wrapping considered.
 */

inline bool yojimbo_sequence_greater_than( uint16_t s1, uint16_t s2 )
{
    return ( ( s1 > s2 ) && ( s1 - s2 <= 32768 ) ) ||
           ( ( s1 < s2 ) && ( s2 - s1  > 32768 ) );
}

/**
    Compares two 16 bit sequence numbers and returns true if the first one is less than the second (considering wrapping).
    IMPORTANT: This is not the same as s1 < s2!
    Greater than is defined specially to handle wrapping sequence numbers.
    If the two sequence numbers are close together, it is as normal, but they are far apart, it is assumed that they have wrapped around.
    Thus, sequence_less_than( 0, 1 ) returns true, and so does sequence_greater_than( 65535, 0 )!
    @param s1 The first sequence number.
    @param s2 The second sequence number.
    @returns True if the s1 is less than s2, with sequence number wrapping considered.
 */

inline bool yojimbo_sequence_less_than( uint16_t s1, uint16_t s2 )
{
    return yojimbo_sequence_greater_than( s2, s1 );
}

/**
    Copy a string without being stupid
    Because apparently in 2023 it's really really hard...
 */

inline void yojimbo_copy_string( char * dest, const char * source, size_t dest_size )
{
    yojimbo_assert( dest );
    yojimbo_assert( source );
    yojimbo_assert( dest_size >= 1 );
    memset( dest, 0, dest_size );
    for ( size_t i = 0; i < dest_size - 1; i++ )
    {
        if ( source[i] == '\0' )
            break;
        dest[i] = source[i];
    }
}

#endif // # YOJIMBO_UTILS_H
