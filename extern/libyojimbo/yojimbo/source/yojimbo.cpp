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

#include "yojimbo.h"
#include "yojimbo_utils.h"

#ifdef _MSC_VER
#define SODIUM_STATIC
#endif // #ifdef _MSC_VER

#include <sodium.h>

static yojimbo::Allocator * g_defaultAllocator;

namespace yojimbo
{
    Allocator & GetDefaultAllocator()
    {
        yojimbo_assert( g_defaultAllocator );
        return *g_defaultAllocator;
    }
}

extern "C" int netcode_init();
extern "C" int reliable_init();

extern "C" void netcode_term();
extern "C" void reliable_term();

extern "C" int netcode_enable_packet_tagging();

#define NETCODE_OK 1
#define RELIABLE_OK 1

bool InitializeYojimbo()
{
    g_defaultAllocator = new yojimbo::DefaultAllocator();

    if ( netcode_init() != NETCODE_OK )
        return false;

    if ( reliable_init() != RELIABLE_OK )
        return false;

    return sodium_init() != -1;
}

void EnablePacketTagging()
{
    netcode_enable_packet_tagging();
}

void ShutdownYojimbo()
{
    reliable_term();

    netcode_term();

    yojimbo_assert( g_defaultAllocator );
    delete g_defaultAllocator;
    g_defaultAllocator = NULL;
}
