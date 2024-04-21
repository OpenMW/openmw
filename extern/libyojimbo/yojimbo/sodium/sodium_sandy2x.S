
#ifdef NETCODE_AVX

#define IN_SANDY2X

#include "sodium_sandy2x_consts.S"
#include "sodium_sandy2x_fe51_mul.S"
#include "sodium_sandy2x_fe51_nsquare.S"
#include "sodium_sandy2x_fe51_pack.S"
#include "sodium_sandy2x_ladder.S"
#include "sodium_sandy2x_ladder_base.S"

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

#endif

.globl consts_S_dummy
