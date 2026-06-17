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

#ifndef YOJIMBO_SERIALIZE_H
#define YOJIMBO_SERIALIZE_H

#include "yojimbo_config.h"
#include "yojimbo_platform.h"

namespace yojimbo
{
    template <typename Stream> bool serialize_ack_relative_internal( Stream & stream, uint16_t sequence, uint16_t & ack )
    {
        int ack_delta = 0;
        bool ack_in_range = false;
        if ( Stream::IsWriting )
        {
            if ( ack < sequence )
            {
                ack_delta = sequence - ack;
            }
            else
            {
                ack_delta = (int)sequence + 65536 - ack;
            }
            yojimbo_assert( ack_delta > 0 );
            yojimbo_assert( uint16_t( sequence - ack_delta ) == ack );
            ack_in_range = ack_delta <= 64;
        }
        serialize_bool( stream, ack_in_range );
        if ( ack_in_range )
        {
            serialize_int( stream, ack_delta, 1, 64 );
            if ( Stream::IsReading )
            {
                ack = sequence - ack_delta;
            }
        }
        else
        {
            serialize_bits( stream, ack, 16 );
        }
        return true;
    }

    /**
        Serialize an ack relative to the current sequence number (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param sequence The current sequence number.
        @param ack The ack sequence number, which is typically near the current sequence number.
     */

    #define serialize_ack_relative( stream, sequence, ack  )                                        \
        do                                                                                          \
        {                                                                                           \
            if ( !yojimbo::serialize_ack_relative_internal( stream, sequence, ack ) )               \
            {                                                                                       \
                return false;                                                                       \
            }                                                                                       \
        } while (0)

    template <typename Stream> bool serialize_sequence_relative_internal( Stream & stream, uint16_t sequence1, uint16_t & sequence2 )
    {
        if ( Stream::IsWriting )
        {
            uint32_t a = sequence1;
            uint32_t b = sequence2 + ( ( sequence1 > sequence2 ) ? 65536 : 0 );
            serialize_int_relative( stream, a, b );
        }
        else
        {
            uint32_t a = sequence1;
            uint32_t b = 0;
            serialize_int_relative( stream, a, b );
            if ( b >= 65536 )
            {
                b -= 65536;
            }
            sequence2 = uint16_t( b );
        }

        return true;
    }

    /**
        Serialize a sequence number relative to another (read/write/measure).
        This is a helper macro to make writing unified serialize functions easier.
        Serialize macros returns false on error so we don't need to use exceptions for error handling on read. This is an important safety measure because packet data comes from the network and may be malicious.
        IMPORTANT: This macro must be called inside a templated serialize function with template \<typename Stream\>. The serialize method must have a bool return value.
        @param stream The stream object. May be a read, write or measure stream.
        @param sequence1 The first sequence number to serialize relative to.
        @param sequence2 The second sequence number to be encoded relative to the first.
     */

    #define serialize_sequence_relative( stream, sequence1, sequence2 )                             \
        do                                                                                          \
        {                                                                                           \
            if ( !yojimbo::serialize_sequence_relative_internal( stream, sequence1, sequence2 ) )   \
            {                                                                                       \
                return false;                                                                       \
            }                                                                                       \
        } while (0)

    /**
        Interface for an object that knows how to read, write and measure how many bits it would take up in a bit stream.
        IMPORTANT: Instead of overriding the serialize virtual methods method directly, use the YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS macro in your derived class to override and redirect them to your templated serialize method.
        This way you can implement read and write for your messages in a single method and the C++ compiler takes care of generating specialized read, write and measure implementations for you.
        See tests/shared.h for some examples of this.
        @see ReadStream
        @see WriteStream
        @see MeasureStream
     */

    class Serializable
    {
    public:

        virtual ~Serializable() {}

        /**
            Virtual serialize function (read).
            Reads the object in from a bitstream.
            @param stream The stream to read from.
         */

        virtual bool SerializeInternal( class ReadStream & stream ) = 0;

        /**
            Virtual serialize function (write).
            Writes the object to a bitstream.
            @param stream The stream to write to.
         */

        virtual bool SerializeInternal( class WriteStream & stream ) = 0;

        /**
            Virtual serialize function (measure).
            Quickly measures how many bits the object would take if it were written to a bit stream.
            @param stream The read stream.
         */

        virtual bool SerializeInternal( class MeasureStream & stream ) = 0;
    };

    /**
        Helper macro to define virtual serialize functions for read, write and measure that call into the templated serialize function.
        This helps avoid writing boilerplate code, which is nice when you have lots of hand coded message types.
        See tests/shared.h for examples of usage.
     */

    #define YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS()                                                               \
        bool SerializeInternal( class yojimbo::ReadStream & stream ) { return Serialize( stream ); };           \
        bool SerializeInternal( class yojimbo::WriteStream & stream ) { return Serialize( stream ); };          \
        bool SerializeInternal( class yojimbo::MeasureStream & stream ) { return Serialize( stream ); };
}

#endif // #ifndef YOJIMBO_SERIALIZE_H
