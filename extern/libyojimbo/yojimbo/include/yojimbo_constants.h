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

#ifndef YOJIMBO_CONSTANTS_H
#define YOJIMBO_CONSTANTS_H

#include "serialize.h"

namespace yojimbo
{
    const int MaxClients = 64;                                      ///< The maximum number of clients supported by this library. You can increase this if you want, but this library is designed around patterns that work best for [2,64] player games. If your game has less than 64 clients, reducing this will save memory.

    const int MaxChannels = 64;                                     ///< The maximum number of message channels supported by this library. If you need less than 64 channels per-packet, reducing this will save memory.

    const int KeyBytes = 32;                                        ///< Size of encryption key for dedicated client/server in bytes. Must be equal to key size for libsodium encryption primitive. Do not change.

    const int ConnectTokenBytes = 2048;                             ///< Size of the encrypted connect token data return from the matchmaker. Must equal size of NETCODE_CONNECT_TOKEN_BYTE (2048).

    const int ConservativeMessageHeaderBits = 32;                   ///< Conservative number of bits per-message header.
    
    const int ConservativeFragmentHeaderBits = 64;                  ///< Conservative number of bits per-fragment header.
    
    const int ConservativeChannelHeaderBits = 32;                   ///< Conservative number of bits per-channel header.
    
    const int ConservativePacketHeaderBits = 16;                    ///< Conservative number of bits per-packet header.
    
    const int MaxAddressLength = 256;                               ///< The maximum length of an address when converted to a string (includes terminating NULL). @see Address::ToString
}

#endif // #ifndef YOJIMBO_CONSTANTS_H
