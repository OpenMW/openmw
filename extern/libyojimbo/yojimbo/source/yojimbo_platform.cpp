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

#include "yojimbo_platform.h"
#include "netcode.h"
#include "reliable.h"
#include <stdarg.h>
#include <stdio.h>

static void default_assert_handler( const char * condition, const char * function, const char * file, int line )
{
    // We use YOJIMBO_LOG_LEVEL_NONE because it's lower than YOJIMBO_LOG_LEVEL_ERROR, so even if you suppress errors (by setting
    // yojimbo_log_level(YOJIMBO_LOG_LEVEL_NONE)), this will still be logged.
    yojimbo_printf( YOJIMBO_LOG_LEVEL_NONE, "assert failed: ( %s ), function %s, file %s, line %d\n", condition, function, file, line );
    #if defined( __GNUC__ )
    __builtin_trap();
    #elif defined( _MSC_VER )
    __debugbreak();
    #endif
}

static int log_level = 0;

static int (*printf_function)( const char *, ... ) = printf;

void (*yojimbo_assert_function)( const char *, const char *, const char * file, int line ) = default_assert_handler;

void yojimbo_log_level( int level )
{
    log_level = level;
    netcode_log_level( level );
    reliable_log_level( level );
}

void yojimbo_set_printf_function( int (*function)( const char *, ... ) )
{
    yojimbo_assert( function );
    printf_function = function;
    netcode_set_printf_function( function );
    reliable_set_printf_function( function );
}

void yojimbo_set_assert_function( void (*function)( const char *, const char *, const char * file, int line ) )
{
    yojimbo_assert_function = function;
    netcode_set_assert_function( function );
    reliable_set_assert_function( function );
}

#if YOJIMBO_ENABLE_LOGGING

void yojimbo_printf( int level, const char * format, ... ) 
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

#else // #if YOJIMBO_ENABLE_LOGGING

void yojimbo_printf( int level, const char * format, ... ) 
{
    (void) level;
    (void) format;
}

#endif // #if YOJIMBO_ENABLE_LOGGING

#if __APPLE__

// ===============================
//              MacOS
// ===============================

#include <unistd.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

void yojimbo_sleep( double time )
{
    usleep( (int) ( time * 1000000 ) );
}

double yojimbo_time()
{
    static uint64_t start = 0;

    static mach_timebase_info_data_t timebase_info;

    if ( start == 0 )
    {
        mach_timebase_info( &timebase_info );
        start = mach_absolute_time();
        return 0.0;
    }

    uint64_t current = mach_absolute_time();

    if ( current < start )
        current = start;

    return ( double( current - start ) * double( timebase_info.numer ) / double( timebase_info.denom ) ) / 1000000000.0;
}

#elif __linux

// ===============================
//              Linux
// ===============================

#include <unistd.h>
#include <time.h>

void yojimbo_sleep( double time )
{
    usleep( (int) ( time * 1000000 ) );
}

double yojimbo_time()
{
    static double start = -1;

    if ( start == -1 )
    {
        timespec ts;
        clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
        start = ts.tv_sec + double( ts.tv_nsec ) / 1000000000.0;
        return 0.0;
    }

    timespec ts;
    clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
    double current = ts.tv_sec + double( ts.tv_nsec ) / 1000000000.0;
    if ( current < start )
        current = start;
    return current - start;
}

#elif defined(_WIN32)

// ===============================
//             Windows
// ===============================

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

void yojimbo_sleep( double time )
{
    const int milliseconds = time * 1000;
    Sleep( milliseconds );
}

static bool timer_initialized = false;
static LARGE_INTEGER timer_frequency;
static LARGE_INTEGER timer_start;

double yojimbo_time()
{
    if ( !timer_initialized )
    {
        QueryPerformanceFrequency( &timer_frequency );
        QueryPerformanceCounter( &timer_start );
        timer_initialized = true;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter( &now );
    if ( now.QuadPart < timer_start.QuadPart )
        now.QuadPart = timer_start.QuadPart;
    return double( now.QuadPart - timer_start.QuadPart ) / double( timer_frequency.QuadPart );
}

#else

#error unsupported platform!

#endif
