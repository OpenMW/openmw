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

#ifndef YOJIMBO_ALLOCATOR_H
#define YOJIMBO_ALLOCATOR_H

#include "yojimbo_config.h"
#include "yojimbo_platform.h"

#include <stdint.h>
#include <new>
#if YOJIMBO_DEBUG_MEMORY_LEAKS
#include <map>
#endif // YOJIMBO_DEBUG_MEMORY_LEAKS

typedef void* tlsf_t;

namespace yojimbo
{
    /**
        Get the default allocator.
        Use this allocator when you just want to use malloc/free, but in the form of a yojimbo allocator.
        This allocator instance is created inside InitializeYojimbo and destroyed in ShutdownYojimbo.
        In debug build, it will automatically check for memory leaks and print them out for you when you shutdown the library.
        @returns The default allocator instances backed by malloc and free.
     */

    class Allocator & GetDefaultAllocator();

    /// Macro for creating a new object instance with a yojimbo allocator.
    #define YOJIMBO_NEW( a, T, ... ) ( new ( (a).Allocate( sizeof(T), __FILE__, __LINE__ ) ) T(__VA_ARGS__) )

    /// Macro for deleting an object created with a yojimbo allocator.
    #define YOJIMBO_DELETE( a, T, p ) do { if (p) { (p)->~T(); (a).Free( p, __FILE__, __LINE__ ); p = NULL; } } while (0)

    /// Macro for allocating a block of memory with a yojimbo allocator.
    #define YOJIMBO_ALLOCATE( a, bytes ) (a).Allocate( (bytes), __FILE__, __LINE__ )

    /// Macro for freeing a block of memory created with a yojimbo allocator.
    #define YOJIMBO_FREE( a, p ) do { if ( p ) { (a).Free( p, __FILE__, __LINE__ ); p = NULL; } } while(0)

    /// Allocator error level.
    enum AllocatorErrorLevel
    {
        ALLOCATOR_ERROR_NONE = 0,                               ///< No error. All is well.
        ALLOCATOR_ERROR_OUT_OF_MEMORY                           ///< The allocator is out of memory!
    };

    /// Helper function to convert an allocator error to a user friendly string.
    inline const char * GetAllocatorErrorString( AllocatorErrorLevel error )
    {
        switch ( error )
        {
            case ALLOCATOR_ERROR_NONE:                  return "none";
            case ALLOCATOR_ERROR_OUT_OF_MEMORY:         return "out of memory";
            default:
                yojimbo_assert( false );
                return "(unknown)";
        }
    }

#if YOJIMBO_DEBUG_MEMORY_LEAKS

    /**
        Debug structure used to track allocations and find memory leaks.
        Active in debug build only. Disabled in release builds for performance reasons.
     */

    struct AllocatorEntry
    {
        size_t size;                        ///< The size of the allocation in bytes.
        const char * file;                  ///< Filename of the source code file that made the allocation.
        int line;                           ///< Line number in the source code where the allocation was made.
    };

#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

    /**
        Functionality common to all allocators.
        Extend this class to hook up your own allocator to yojimbo.
        IMPORTANT: This allocator is not yet thread safe. Only call it from one thread!
     */

    class Allocator
    {
    public:

        /**
            Allocator constructor.
            Sets the error level to ALLOCATOR_ERROR_NONE.
         */

        Allocator();

        /**
            Allocator destructor.
            Make sure all allocations made from this allocator are freed before you destroy this allocator.
            In debug build, validates this is true walks the map of allocator entries. Any outstanding entries are considered memory leaks and printed to stdout.
         */

        virtual ~Allocator();

        /**
            Allocate a block of memory.
            IMPORTANT: Don't call this directly. Use the YOJIMBO_NEW or YOJIMBO_ALLOCATE macros instead, because they automatically pass in the source filename and line number for you.
            @param size The size of the block of memory to allocate (bytes).
            @param file The source code filename that is performing the allocation. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the allocation.
            @returns A block of memory of the requested size, or NULL if the allocation could not be performed. If NULL is returned, the error level is set to ALLOCATION_ERROR_FAILED_TO_ALLOCATE.
            @see Allocator::Free
            @see Allocator::GetErrorLevel
         */

        virtual void * Allocate( size_t size, const char * file, int line ) = 0;

        /**
            Free a block of memory.
            IMPORTANT: Don't call this directly. Use the YOJIMBO_DELETE or YOJIMBO_FREE macros instead, because they automatically pass in the source filename and line number for you.
            @param p Pointer to the block of memory to free. Must be non-NULL block of memory that was allocated with this allocator. Will assert otherwise.
            @param file The source code filename that is performing the free. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the free.
            @see Allocator::Allocate
            @see Allocator::GetErrorLevel
         */

        virtual void Free( void * p, const char * file, int line ) = 0;

        /**
            Get the allocator error level.
            Use this function to check if an allocation has failed. This is used in the client/server to disconnect a client with a failed allocation.
            @returns The allocator error level.
         */

        AllocatorErrorLevel GetErrorLevel() const { return m_errorLevel; }

        /**
            Clear the allocator error level back to default.
         */

        void ClearError() { m_errorLevel = ALLOCATOR_ERROR_NONE; }

    protected:

        /**
            Set the error level.
            For correct client/server behavior when an allocation fails, please make sure you call this method to set the error level to ALLOCATOR_ERROR_FAILED_TO_ALLOCATE.
            @param error The allocator error level to set.
         */

        void SetErrorLevel( AllocatorErrorLevel errorLevel );

        /**
            Call this function to track an allocation made by your derived allocator class.
            In debug build, tracked allocations are automatically checked for leaks when the allocator is destroyed.
            @param p Pointer to the memory that was allocated.
            @param size The size of the allocation in bytes.
            @param file The source code file that performed the allocation.
            @param line The line number in the source file where the allocation was performed.
         */

        void TrackAlloc( void * p, size_t size, const char * file, int line );

        /**
            Call this function to track a free made by your derived allocator class.
            In debug build, any allocation tracked without a corresponding free is considered a memory leak when the allocator is destroyed.
            @param p Pointer to the memory that was allocated.
            @param file The source code file that is calling in to free the memory.
            @param line The line number in the source file where the free is being called from.
         */

        void TrackFree( void * p, const char * file, int line );

        AllocatorErrorLevel m_errorLevel;                                       ///< The allocator error level.

#if YOJIMBO_DEBUG_MEMORY_LEAKS
        std::map<void*,AllocatorEntry> m_alloc_map;                             ///< Debug only data structure used to find and report memory leaks.
#endif // #if YOJIMBO_DEBUG_MEMORY_LEAKS

    private:

        Allocator( const Allocator & other );

        Allocator & operator = ( const Allocator & other );
    };

    /**
        The default allocator implementation based around malloc and free.
     */

    class DefaultAllocator : public Allocator
    {
    public:

        /**
            Default constructor.
         */

        DefaultAllocator() {}

        /**
            Allocates a block of memory using "malloc".
            IMPORTANT: Don't call this directly. Use the YOJIMBO_NEW or YOJIMBO_ALLOCATE macros instead, because they automatically pass in the source filename and line number for you.
            @param size The size of the block of memory to allocate (bytes).
            @param file The source code filename that is performing the allocation. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the allocation.
            @returns A block of memory of the requested size, or NULL if the allocation could not be performed. If NULL is returned, the error level is set to ALLOCATION_ERROR_FAILED_TO_ALLOCATE.
         */

        void * Allocate( size_t size, const char * file, int line );

        /**
            Free a block of memory by calling "free".
            IMPORTANT: Don't call this directly. Use the YOJIMBO_DELETE or YOJIMBO_FREE macros instead, because they automatically pass in the source filename and line number for you.
            @param p Pointer to the block of memory to free. Must be non-NULL block of memory that was allocated with this allocator. Will assert otherwise.
            @param file The source code filename that is performing the free. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the free.
         */

        void Free( void * p, const char * file, int line );

    private:

        DefaultAllocator( const DefaultAllocator & other );

        DefaultAllocator & operator = ( const DefaultAllocator & other );
    };

    /**
        An allocator built on the TLSF allocator implementation by Matt Conte. Thanks Matt!
        This is a fast allocator that supports multiple heaps. It's used inside the yojimbo server to silo allocations for each client to their own heap.
        See https://github.com/mattconte/tlsf for details on this allocator implementation.
     */

    class TLSF_Allocator : public Allocator
    {
    public:

        /**
            TLSF allocator constructor.
            If you want to integrate your own allocator with yojimbo for use with the client and server, this class is a good template to start from.
            Make sure your constructor has the same signature as this one, and it will work with the YOJIMBO_SERVER_ALLOCATOR and YOJIMBO_CLIENT_ALLOCATOR helper macros.
            @param memory Block of memory in which the allocator will work. This block must remain valid while this allocator exists. The allocator does not assume ownership of it, you must free it elsewhere, if necessary.
            @param bytes The size of the block of memory (bytes). The maximum amount of memory you can allocate will be less, due to allocator overhead.
         */

        TLSF_Allocator( void * memory, size_t bytes );

        /**
            TLSF allocator destructor.
            Checks for memory leaks in debug build. Free all memory allocated by this allocator before destroying.
         */

        ~TLSF_Allocator();

        /**
            Allocates a block of memory using TLSF.
            IMPORTANT: Don't call this directly. Use the YOJIMBO_NEW or YOJIMBO_ALLOCATE macros instead, because they automatically pass in the source filename and line number for you.
            @param size The size of the block of memory to allocate (bytes).
            @param file The source code filename that is performing the allocation. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the allocation.
            @returns A block of memory of the requested size, or NULL if the allocation could not be performed. If NULL is returned, the error level is set to ALLOCATION_ERROR_FAILED_TO_ALLOCATE.
         */

        void * Allocate( size_t size, const char * file, int line );

        /**
            Free a block of memory using TLSF.
            IMPORTANT: Don't call this directly. Use the YOJIMBO_DELETE or YOJIMBO_FREE macros instead, because they automatically pass in the source filename and line number for you.
            @param p Pointer to the block of memory to free. Must be non-NULL block of memory that was allocated with this allocator. Will assert otherwise.
            @param file The source code filename that is performing the free. Used for tracking allocations and reporting on memory leaks.
            @param line The line number in the source code file that is performing the free.
            @see Allocator::Allocate
            @see Allocator::GetError
         */

        void Free( void * p, const char * file, int line );

    private:

        tlsf_t m_tlsf;              ///< The TLSF allocator instance backing this allocator.

        TLSF_Allocator( const TLSF_Allocator & other );
        TLSF_Allocator & operator = ( const TLSF_Allocator & other );
    };
}

#endif // # YOJIMBO_PLATFORM_H
