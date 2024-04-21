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

#ifndef YOJIMBO_QUEUE_H
#define YOJIMBO_QUEUE_H

#include "yojimbo_config.h"

namespace yojimbo
{
    /**
        A simple templated queue.
        This is a FIFO queue. First entry in, first entry out.
     */

    template <typename T> class Queue
    {
    public:

        /**
            Queue constructor.
            @param allocator The allocator to use.
            @param size The maximum number of entries in the queue.
         */

        Queue( Allocator & allocator, int size )
        {
            yojimbo_assert( size > 0 );
            m_arraySize = size;
            m_startIndex = 0;
            m_numEntries = 0;
            m_allocator = &allocator;
            m_entries = (T*) YOJIMBO_ALLOCATE( allocator, sizeof(T) * size );
            memset( m_entries, 0, sizeof(T) * size );
        }

        /**
            Queue destructor.
         */

        ~Queue()
        {
            yojimbo_assert( m_allocator );

            YOJIMBO_FREE( *m_allocator, m_entries );

            m_arraySize = 0;
            m_startIndex = 0;
            m_numEntries = 0;

            m_allocator = NULL;
        }

        /**
            Clear all entries in the queue and reset back to default state.
         */

        void Clear()
        {
            m_numEntries = 0;
            m_startIndex = 0;
        }

        /**
            Pop a value off the queue.
            IMPORTANT: This will assert if the queue is empty. Check Queue::IsEmpty or Queue::GetNumEntries first!
            @returns The value popped off the queue.
         */

        T Pop()
        {
            yojimbo_assert( !IsEmpty() );
            const T & entry = m_entries[m_startIndex];
            m_startIndex = ( m_startIndex + 1 ) % m_arraySize;
            m_numEntries--;
            return entry;
        }

        /**
            Push a value on to the queue.
            @param value The value to push onto the queue.
            IMPORTANT: Will assert if the queue is already full. Check Queue::IsFull before calling this!
         */

        void Push( const T & value )
        {
            yojimbo_assert( !IsFull() );
            const int index = ( m_startIndex + m_numEntries ) % m_arraySize;
            m_entries[index] = value;
            m_numEntries++;
        }

        /**
            Random access for entries in the queue.
            @param index The index into the queue. 0 is the oldest entry, Queue::GetNumEntries() - 1 is the newest.
            @returns The value in the queue at the index.
         */

        T & operator [] ( int index )
        {
            yojimbo_assert( !IsEmpty() );
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_numEntries );
            return m_entries[ ( m_startIndex + index ) % m_arraySize ];
        }

        /**
            Random access for entries in the queue (const version).
            @param index The index into the queue. 0 is the oldest entry, Queue::GetNumEntries() - 1 is the newest.
            @returns The value in the queue at the index.
         */

        const T & operator [] ( int index ) const
        {
            yojimbo_assert( !IsEmpty() );
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_numEntries );
            return m_entries[ ( m_startIndex + index ) % m_arraySize ];
        }

        /**
            Get the size of the queue.
            This is the maximum number of values that can be pushed on the queue.
            @returns The size of the queue.
         */

        int GetSize() const
        {
            return m_arraySize;
        }

        /**
            Is the queue currently full?
            @returns True if the queue is full. False otherwise.
         */

        bool IsFull() const
        {
            return m_numEntries == m_arraySize;
        }

        /**
            Is the queue currently empty?
            @returns True if there are no entries in the queue.
         */

        bool IsEmpty() const
        {
            return m_numEntries == 0;
        }

        /**
            Get the number of entries in the queue.
            @returns The number of entries in the queue in [0,GetSize()].
         */

        int GetNumEntries() const
        {
            return m_numEntries;
        }

    private:


        Allocator * m_allocator;                        ///< The allocator passed in to the constructor.
        T * m_entries;                                  ///< Array of entries backing the queue (circular buffer).
        int m_arraySize;                                ///< The size of the array, in number of entries. This is the "size" of the queue.
        int m_startIndex;                               ///< The start index for the queue. This is the next value that gets popped off.
        int m_numEntries;                               ///< The number of entries currently stored in the queue.
    };
}

#endif // #ifndef YOJIMBO_QUEUE_H
