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

#ifndef YOJIMBO_SEQUENCE_BUFFER_H
#define YOJIMBO_SEQUENCE_BUFFER_H

#include "yojimbo_config.h"
#include "yojimbo_serialize.h"
#include "yojimbo_utils.h"

namespace yojimbo
{
    /**
        Data structure that stores data indexed by sequence number.
        Entries may or may not exist. If they don't exist the sequence value for the entry at that index is set to 0xFFFFFFFF.
        This provides a constant time lookup for an entry by sequence number. If the entry at sequence modulo buffer size doesn't have the same sequence number, that sequence number is not stored.
        This is incredibly useful and is used as the foundation of the packet level ack system and the reliable message send and receive queues.
        @see Connection
     */

    template <typename T> class SequenceBuffer
    {
    public:

        /**
            Sequence buffer constructor.
            @param allocator The allocator to use.
            @param size The size of the sequence buffer.
         */

        SequenceBuffer( Allocator & allocator, int size )
        {
            yojimbo_assert( size > 0 );
            m_size = size;
            m_sequence = 0;
            m_allocator = &allocator;
            m_entry_sequence = (uint32_t*) YOJIMBO_ALLOCATE( allocator, sizeof( uint32_t ) * size );
            m_entries = (T*) YOJIMBO_ALLOCATE( allocator, sizeof(T) * size );
            Reset();
        }

        /**
            Sequence buffer destructor.
         */

        ~SequenceBuffer()
        {
            yojimbo_assert( m_allocator );
            YOJIMBO_FREE( *m_allocator, m_entries );
            YOJIMBO_FREE( *m_allocator, m_entry_sequence );
            m_allocator = NULL;
        }

        /**
            Reset the sequence buffer.
            Removes all entries from the sequence buffer and restores it to initial state.
         */

        void Reset()
        {
            m_sequence = 0;
            memset( m_entry_sequence, 0xFF, sizeof( uint32_t ) * m_size );
        }

        /**
            Insert an entry in the sequence buffer.
            IMPORTANT: If another entry exists at the sequence modulo buffer size, it is overwritten.
            @param sequence The sequence number.
            @param guaranteed_order Whether sequence is always the newest value (when sending) or can be out of order (when receiving).
            @returns The sequence buffer entry, which you must fill with your data. NULL if a sequence buffer entry could not be added for your sequence number (if the sequence number is too old for example).
         */

        T * Insert( uint16_t sequence, bool guaranteed_order = false )
        {
            if ( yojimbo_sequence_greater_than( sequence + 1, m_sequence ) || guaranteed_order )
            {
                RemoveEntries( m_sequence, sequence );
                m_sequence = sequence + 1;
            }
            else if ( yojimbo_sequence_less_than( sequence, m_sequence - m_size ) )
            {
                return NULL;
            }
            const int index = sequence % m_size;
            m_entry_sequence[index] = sequence;
            return &m_entries[index];
        }

        /**
            Remove an entry from the sequence buffer.
            @param sequence The sequence number of the entry to remove.
         */

        void Remove( uint16_t sequence )
        {
            m_entry_sequence[ sequence % m_size ] = 0xFFFFFFFF;
        }

        /**
            Is the entry corresponding to the sequence number available? eg. Currently unoccupied.
            This works because older entries are automatically set back to unoccupied state as the sequence buffer advances forward.
            @param sequence The sequence number.
            @returns True if the sequence buffer entry is available, false if it is already occupied.
         */

        bool Available( uint16_t sequence ) const
        {
            return m_entry_sequence[ sequence % m_size ] == 0xFFFFFFFF;
        }

        /**
            Does an entry exist for a sequence number?
            @param sequence The sequence number.
            @returns True if an entry exists for this sequence number.
         */

        bool Exists( uint16_t sequence ) const
        {
            return m_entry_sequence[ sequence % m_size ] == uint32_t( sequence );
        }

        /**
            Get the entry corresponding to a sequence number.
            @param sequence The sequence number.
            @returns The entry if it exists. NULL if no entry is in the buffer for this sequence number.
         */

        T * Find( uint16_t sequence )
        {
            const int index = sequence % m_size;
            if ( m_entry_sequence[index] == uint32_t( sequence ) )
                return &m_entries[index];
            else
                return NULL;
        }

        /**
            Get the entry corresponding to a sequence number (const version).
            @param sequence The sequence number.
            @returns The entry if it exists. NULL if no entry is in the buffer for this sequence number.
         */

        const T * Find( uint16_t sequence ) const
        {
            const int index = sequence % m_size;
            if ( m_entry_sequence[index] == uint32_t( sequence ) )
                return &m_entries[index];
            else
                return NULL;
        }

        /**
            Get the entry at the specified index.
            Use this to iterate across entries in the sequence buffer.
            @param index The entry index in [0,GetSize()-1].
            @returns The entry if it exists. NULL if no entry is in the buffer at the specified index.
         */

        T * GetAtIndex( int index )
        {
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_size );
            return m_entry_sequence[index] != 0xFFFFFFFF ? &m_entries[index] : NULL;
        }

        /**
            Get the entry at the specified index (const version).
            Use this to iterate across entries in the sequence buffer.
            @param index The entry index in [0,GetSize()-1].
            @returns The entry if it exists. NULL if no entry is in the buffer at the specified index.
         */

        const T * GetAtIndex( int index ) const
        {
            yojimbo_assert( index >= 0 );
            yojimbo_assert( index < m_size );
            return m_entry_sequence[index] != 0xFFFFFFFF ? &m_entries[index] : NULL;
        }

        /**
            Get the most recent sequence number added to the buffer.
            This sequence number can wrap around, so if you are at 65535 and add an entry for sequence 0, then 0 becomes the new "most recent" sequence number.
            @returns The most recent sequence number.
            @see yojimbo::sequence_greater_than
            @see yojimbo::sequence_less_than
         */

        uint16_t GetSequence() const
        {
            return m_sequence;
        }

        /**
            Get the entry index for a sequence number.
            This is simply the sequence number modulo the sequence buffer size.
            @param sequence The sequence number.
            @returns The sequence buffer index corresponding of the sequence number.
         */

        int GetIndex( uint16_t sequence ) const
        {
            return sequence % m_size;
        }

        /**
            Get the size of the sequence buffer.
            @returns The size of the sequence buffer (number of entries).
         */

        int GetSize() const
        {
            return m_size;
        }

    protected:

        /**
            Helper function to remove entries.
            This is used to remove old entries as we advance the sequence buffer forward.
            Otherwise, if when entries are added with holes (eg. receive buffer for packets or messages, where not all sequence numbers are added to the buffer because we have high packet loss),
            and we are extremely unlucky, we can have old sequence buffer entries from the previous sequence # wrap around still in the buffer, which corrupts our internal connection state.
            This actually happened in the soak test at high packet loss levels (>90%). It took me days to track it down :)
         */

        void RemoveEntries( int start_sequence, int finish_sequence )
        {
            if ( finish_sequence < start_sequence )
                finish_sequence += 65535;
            yojimbo_assert( finish_sequence >= start_sequence );
            if ( finish_sequence - start_sequence < m_size )
            {
                for ( int sequence = start_sequence; sequence <= finish_sequence; ++sequence )
                    m_entry_sequence[sequence % m_size] = 0xFFFFFFFF;
            }
            else
            {
                for ( int i = 0; i < m_size; ++i )
                    m_entry_sequence[i] = 0xFFFFFFFF;
            }
        }

    private:

        Allocator * m_allocator;                   ///< The allocator passed in to the constructor.
        int m_size;                                ///< The size of the sequence buffer.
        uint16_t m_sequence;                       ///< The most recent sequence number added to the buffer.
        uint32_t * m_entry_sequence;               ///< Array of sequence numbers corresponding to each sequence buffer entry for fast lookup. Set to 0xFFFFFFFF if no entry exists at that index.
        T * m_entries;                             ///< The sequence buffer entries. This is where the data is stored per-entry. Separate from the sequence numbers for fast lookup (hot/cold split) when the data per-sequence number is relatively large.

        SequenceBuffer( const SequenceBuffer<T> & other );

        SequenceBuffer<T> & operator = ( const SequenceBuffer<T> & other );
    };
}

#endif // #ifndef YOJIMBO_SEQUENCE_BUFFER_H
