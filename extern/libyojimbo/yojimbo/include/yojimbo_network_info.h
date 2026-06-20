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

#ifndef YOJIMBO_NETWORK_INFO_H
#define YOJIMBO_NETWORK_INFO_H

#include "yojimbo_config.h"

namespace yojimbo
{
    /**
        Network information for a connection.
        Contains statistics like round trip time (RTT), packet loss %, bandwidth estimates, number of packets sent, received and acked.
     */

    struct NetworkInfo
    {
        float RTT;                                  ///< Round trip time estimate (milliseconds). Exponentially smoothed average tracking most recent RTT value.
        float minRTT;                               ///< Minimum RTT seen over the last n samples (see rtt_history_size in reliable config). This is a more stable and accurate RTT value under typical Wi-Fi jitter.
        float maxRTT;                               ///< Maximum RTT seen over the last n samples. 
        float averageRTT;                           ///< Average RTT seen over the last n samples.
        float averageJitter;                        ///< Average jitter relative to min RTT over the last n samples.
        float maxJitter;                            ///< Max jitter relative to min RTT seen over the last n samples.
        float stddevJitter;                         ///< One standard deviation of jitter relative to average RTT over the last n samples.
        float packetLoss;                           ///< Packet loss percent.
        float sentBandwidth;                        ///< Sent bandwidth (kbps).
        float receivedBandwidth;                    ///< Received bandwidth (kbps).
        float ackedBandwidth;                       ///< Acked bandwidth (kbps).
        uint64_t numPacketsSent;                    ///< Number of packets sent.
        uint64_t numPacketsReceived;                ///< Number of packets received.
        uint64_t numPacketsAcked;                   ///< Number of packets acked.
    };
}

#endif // #ifndef YOJIMBO_NETWORK_INFO_H
