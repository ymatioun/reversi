#include "rev.h"

union V8DI {
    UINT64 ull[8];
    __m256i v4[2];
};
V8DI lrmask[64];


UINT64 __attribute__ ((always_inline)) flip(const uint_fast8_t place, UINT64 P, UINT64 O){
    __m128i OP = _mm_set_epi64x(O, P);
    __m256i PP = _mm256_broadcastq_epi64(OP);
    __m256i OO = _mm256_broadcastq_epi64(_mm_unpackhi_epi64(OP, OP)); // fast with AMD
    __m256i mask = lrmask[place].v4[1];

    // right: shadow mask lower than leftmost P
    __m256i rP = _mm256_and_si256(PP, mask);
    __m256i rS = _mm256_or_si256(rP, _mm256_srlv_epi64(rP, _mm256_set_epi64x(7, 9, 8, 1)));
    rS = _mm256_or_si256(rS, _mm256_srlv_epi64(rS, _mm256_set_epi64x(14, 18, 16, 2)));
    rS = _mm256_or_si256(rS, _mm256_srlv_epi64(rS, _mm256_set_epi64x(28, 36, 32, 4)));

    // erase if non-opponent MS1B is not P
    __m256i rE = _mm256_xor_si256(_mm256_andnot_si256(OO, mask), rP);	// masked Empty
    __m256i F4 = _mm256_and_si256(_mm256_andnot_si256(rS, mask), _mm256_cmpgt_epi64(rP, rE));

    mask = lrmask[place].v4[0];
    __m256i lO = _mm256_andnot_si256(OO, mask);
    // left: non-opponent BLSMSK
    lO = _mm256_and_si256(_mm256_xor_si256(_mm256_add_epi64(lO, _mm256_set1_epi64x(-1)), lO), mask);
    // clear MSB of BLSMSK if it is P
    __m256i lF = _mm256_andnot_si256(PP, lO);
    // erase lF if lO = lF (i.e. MSB is not P)
    F4 = _mm256_or_si256(F4, _mm256_andnot_si256(_mm256_cmpeq_epi64(lF, lO), lF));

    __m128i F2 = _mm_or_si128(_mm256_castsi256_si128(F4), _mm256_extracti128_si256(F4, 1));
    return _mm_cvtsi128_si64(_mm_or_si128(F2, _mm_shuffle_epi32(F2, 0x4e)));	// SWAP64
}


void flip_init(void){
    for(int x = 0; x < 8; ++x) {
        __m256i lmask = _mm256_set_epi64x(
            (0x0102040810204080ULL >> ((7 - x) * 8)) & 0xffffffffffffff00ULL,
            (0x8040201008040201ULL >> (x * 8)) & 0xffffffffffffff00ULL,
            (0x0101010101010101ULL << x) & 0xffffffffffffff00ULL,
            (0xfe << x) & 0xff
        );
        __m256i rmask = _mm256_set_epi64x(
            (0x0102040810204080ULL << (x * 8)) & 0x00ffffffffffffffULL,
            (0x8040201008040201ULL << ((7 - x) * 8)) & 0x00ffffffffffffffULL,
            (0x0101010101010101ULL << x) & 0x00ffffffffffffffULL,
            (uint64_t)(0x7f >> (7 - x)) << 56
        );

        for(int y = 0; y < 8; ++y) {
            lrmask[y * 8 + x].v4[0] = lmask;
            lrmask[(7 - y) * 8 + x].v4[1] = rmask;
            lmask = _mm256_slli_epi64(lmask, 8);
            rmask = _mm256_srli_epi64(rmask, 8);
        }
    }
}
