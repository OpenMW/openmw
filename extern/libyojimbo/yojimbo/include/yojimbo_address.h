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

#ifndef YOJIMBO_ADDRESS_H
#define YOJIMBO_ADDRESS_H

#include "yojimbo_config.h"
#include <stdint.h>

namespace yojimbo
{
    /**
        Address type.
        @see Address::GetType.
     */

    enum AddressType
    {
        ADDRESS_NONE,                                                       ///< Not an address. Set by the default constructor.
        ADDRESS_IPV4,                                                       ///< An IPv4 address, eg: "146.95.129.237"
        ADDRESS_IPV6                                                        ///< An IPv6 address, eg: "48d9:4a08:b543:ae31:89d8:3226:b92c:cbba"
    };

    /**
        An IP address and port number.
        Supports both IPv4 and IPv6 addresses.
        Identifies where a packet came from, and where a packet should be sent.
     */

    class Address
    {
        AddressType m_type;                                                 ///< The address type: IPv4 or IPv6.
        union
        {
            uint8_t ipv4[4];                                                ///< IPv4 address data. Valid if type is ADDRESS_IPV4.
            uint16_t ipv6[8];                                               ///< IPv6 address data. Valid if type is ADDRESS_IPV6.
        } m_address;
        uint16_t m_port;                                                    ///< The IP port. Valid for IPv4 and IPv6 address types.

   public:

        /**
            Address default constructor.
            Designed for convenience so you can have address members of classes and initialize them via assignment.
            An address created by the default constructor will have address type set to ADDRESS_NONE. Address::IsValid will return false.
            @see IsValid
         */

        Address();

        /**
            Create an IPv4 address.
            IMPORTANT: Pass in port in local byte order. The address class handles the conversion to network order for you.
            @param a The first field of the IPv4 address.
            @param b The second field of the IPv4 address.
            @param c The third field of the IPv4 address.
            @param d The fourth field of the IPv4 address.
            @param port The IPv4 port (local byte order).
         */

        Address( uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port = 0 );

        /**
            Create an IPv4 address.
            @param address Array of four address fields for the IPv4 address.
            @param port The port number (local byte order).
         */

        Address( const uint8_t address[], uint16_t port = 0 );

        /**
            Create an IPv6 address.
            IMPORTANT: Pass in address fields and the port in local byte order. The address class handles the conversion to network order for you.
            @param a First field of the IPv6 address (local byte order).
            @param b Second field of the IPv6 address (local byte order).
            @param c Third field of the IPv6 address (local byte order).
            @param d Fourth field of the IPv6 address (local byte order).
            @param e Fifth field of the IPv6 address (local byte order).
            @param f Sixth field of the IPv6 address (local byte order).
            @param g Seventh field of the IPv6 address (local byte order).
            @param h Eighth field of the IPv6 address (local byte order).
            @param port The port number (local byte order).
         */

        Address( uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f, uint16_t g, uint16_t h, uint16_t port = 0 );

        /**
            Create an IPv6 address.
            IMPORTANT: Pass in address fields and the port in local byte order. The address class handles the conversion to network order for you.
            @param address Array of 8 16 bit address fields for the IPv6 address (local byte order).
            @param port The IPv6 port (local byte order).
         */

        Address( const uint16_t address[], uint16_t port = 0 );

        /**
            Parse a string to an address.
            This versions supports parsing a port included in the address string. For example, "127.0.0.1:4000" and "[::1]:40000".
            Parsing is performed via inet_pton once the port # has been extracted from the string, so you may specify any IPv4 or IPv6 address formatted in any valid way, and it should work as you expect.
            Depending on the type of data in the string the address will become ADDRESS_TYPE_IPV4 or ADDRESS_TYPE_IPV6.
            If the string is not recognized as a valid address, the address type is set to ADDRESS_TYPE_NONE, causing Address::IsValid to return false. Please check that after creating an address from a string.
            @param address The string to parse to create the address.
            @see Address::IsValid
            @see Address::GetType
         */

        explicit Address( const char * address );

        /**
            Parse a string to an address.
            This versions overrides any port read in the address with the port parameter. This lets you parse "127.0.0.1" and "[::1]" and pass in the port you want programmatically.
            Parsing is performed via inet_pton once the port # has been extracted from the string, so you may specify any IPv4 or IPv6 address formatted in any valid way, and it should work as you expect.
            Depending on the type of data in the string the address will become ADDRESS_TYPE_IPV4 or ADDRESS_TYPE_IPV6.
            If the string is not recognized as a valid address, the address type is set to ADDRESS_TYPE_NONE, causing Address::IsValid to return false. Please check that after creating an address from a string.
            @param address The string to parse to create the address.
            @param port Overrides the port number read from the string (if any).
            @see Address::IsValid
            @see Address::GetType
         */

        explicit Address( const char * address, uint16_t port );

        /**
            Clear the address.
            The address type is set to ADDRESS_TYPE_NONE.
            After this function is called Address::IsValid will return false.
         */

        void Clear();

        /**
            Get the IPv4 address data.
            @returns The IPv4 address as an array of bytes.
         */

        const uint8_t * GetAddress4() const;

        /**
            Get the IPv6 address data.
            @returns the IPv6 address data as an array of uint16_t (local byte order).
         */

        const uint16_t * GetAddress6() const;

        /**
            Set the port.
            This is useful when you want to programmatically set a server port, eg. try to open a server on ports 40000, 40001, etc...
            @param port The port number (local byte order). Works for both IPv4 and IPv6 addresses.
         */

        void SetPort( uint16_t port );

        /**
            Get the port number.
            @returns The port number (local byte order).
         */

        uint16_t GetPort() const;

        /**
            Get the address type.
            @returns The address type: ADDRESS_NONE, ADDRESS_IPV4 or ADDRESS_IPV6.
         */

        AddressType GetType() const;

        /**
            Convert the address to a string.
            @param buffer The buffer the address will be written to.
            @param bufferSize The size of the buffer in bytes. Must be at least MaxAddressLength.
         */

        const char * ToString( char buffer[], int bufferSize ) const;

        /**
            True if the address is valid.
            A valid address is any address with a type other than ADDRESS_TYPE_NONE.
            @returns True if the address is valid, false otherwise.
         */

        bool IsValid() const;

        /**
            Is this a loopback address?
            Corresponds to an IPv4 address of "127.0.0.1", or an IPv6 address of "::1".
            @returns True if this is the loopback address.
         */

        bool IsLoopback() const;

        /**
            Is this an IPv6 link local address?
            Corresponds to the first field of the address being 0xfe80
            @returns True if this address is a link local IPv6 address.
         */

        bool IsLinkLocal() const;

        /**
            Is this an IPv6 site local address?
            Corresponds to the first field of the address being 0xfec0
            @returns True if this address is a site local IPv6 address.
         */

        bool IsSiteLocal() const;

        /**
            Is this an IPv6 multicast address?
            Corresponds to the first field of the IPv6 address being 0xff00
            @returns True if this address is a multicast IPv6 address.
         */

        bool IsMulticast() const;

        /**
            Is this in IPv6 global unicast address?
            Corresponds to any IPv6 address that is not any of the following: Link Local, Site Local, Multicast or Loopback.
            @returns True if this is a global unicast IPv6 address.
         */

        bool IsGlobalUnicast() const;

        bool operator ==( const Address & other ) const;

        bool operator !=( const Address & other ) const;

    protected:

        /**
            Helper function to parse an address string.
            Used by the constructors that take a string parameter.
            @param address The string to parse.
         */

        void Parse( const char * address );
    };
}

#endif // #ifndef YOJIMBO_ADDRESS_H
