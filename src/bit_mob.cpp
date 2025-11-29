#include "rev.h"

/*#define	bswap_int(x)	__builtin_bswap32(x)
UINT64 bit_mob_mask(const UINT64 P, const UINT64 O){
	unsigned int	mO, movesL, movesH, flip1, pre1;
	__m128i	OP, rOP, PP, OO, MM, flip, pre;

    // vertical_mirror in PP[1], OO[1]
	OP  = _mm_unpacklo_epi64(_mm_cvtsi64_si128(P), _mm_cvtsi64_si128(O));   	mO = (unsigned int) O & 0x7e7e7e7eU;
	rOP = _mm_shufflelo_epi16(OP, 0x1B);                                    	flip1  = mO & ((unsigned int) P << 1);
	rOP = _mm_shufflehi_epi16(rOP, 0x1B);                                   	movesL = mO + flip1;
	rOP = _mm_or_si128(_mm_srli_epi16(rOP, 8), _mm_slli_epi16(rOP, 8));
	PP  = _mm_unpacklo_epi64(OP, rOP);
	OO  = _mm_unpackhi_epi64(OP, rOP);

	flip = _mm_and_si128(OO, _mm_slli_epi64(PP, 8));                        	flip1  = mO & ((unsigned int) P >> 1);
	flip = _mm_or_si128(flip, _mm_and_si128(OO, _mm_slli_epi64(flip, 8)));  	flip1 |= mO & (flip1 >> 1);
	pre = _mm_and_si128(OO, _mm_slli_epi64(OO, 8));                         	pre1   = mO & (mO >> 1);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 16)));	flip1 |= pre1 & (flip1 >> 2);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 16)));	flip1 |= pre1 & (flip1 >> 2);
	MM = _mm_slli_epi64(flip, 8);                                           	movesL |= flip1 >> 1;

	OO = _mm_and_si128(OO, _mm_set1_epi8(0x7e));                            	mO = (unsigned int) (O >> 32) & 0x7e7e7e7eU;
	flip = _mm_and_si128(OO, _mm_slli_epi64(PP, 7));                        	flip1  = mO & ((unsigned int) (P >> 32) << 1);
	flip = _mm_or_si128(flip, _mm_and_si128(OO, _mm_slli_epi64(flip, 7)));  	movesH = mO + flip1;
	pre = _mm_and_si128(OO, _mm_slli_epi64(OO, 7));
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 14)));
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_slli_epi64(flip, 14)));
	MM = _mm_or_si128(MM, _mm_slli_epi64(flip, 7));

	flip = _mm_and_si128(OO, _mm_srli_epi64(PP, 7));                        	flip1  = mO & ((unsigned int) (P >> 32) >> 1);
	flip = _mm_or_si128(flip, _mm_and_si128(OO, _mm_srli_epi64(flip, 7)));  	flip1 |= mO & (flip1 >> 1);
	pre = _mm_srli_epi64(pre, 7);                                           	pre1   = mO & (mO >> 1);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_srli_epi64(flip, 14)));	flip1 |= pre1 & (flip1 >> 2);
	flip = _mm_or_si128(flip, _mm_and_si128(pre, _mm_srli_epi64(flip, 14)));	flip1 |= pre1 & (flip1 >> 2);
	MM = _mm_or_si128(MM, _mm_srli_epi64(flip, 7));                         	movesH |= flip1 >> 1;

	movesL |= _mm_cvtsi128_si32(MM);	MM = _mm_srli_si128(MM, 4);
	movesH |= _mm_cvtsi128_si32(MM);	MM = _mm_srli_si128(MM, 4);
	movesH |= bswap_int(_mm_cvtsi128_si32(MM));
	movesL |= bswap_int(_mm_cvtsi128_si32(_mm_srli_si128(MM, 4)));
	return (movesL | ((unsigned long long) movesH << 32)) & ~(P|O);	// mask with empties
}*/


/*UINT64 bit_mob_mask(const UINT64 P, const UINT64 O){
    UINT64 moves, mO;
    UINT64 flip1, flip7, flip9, flip8, pre1, pre7, pre9, pre8;
    mO = O & 0x7e7e7e7e7e7e7e7eULL;
    flip1 = mO & (P << 1);         flip7  = mO & (P << 7);        flip9  = mO & (P << 9);        flip8  = O & (P << 8);
    flip1 |= mO & (flip1 << 1);    flip7 |= mO & (flip7 << 7);    flip9 |= mO & (flip9 << 9);    flip8 |= O & (flip8 << 8);
    pre1 = mO & (mO << 1);         pre7 = mO & (mO << 7);         pre9 = mO & (mO << 9);         pre8 = O & (O << 8);
    flip1 |= pre1 & (flip1 << 2);  flip7 |= pre7 & (flip7 << 14); flip9 |= pre9 & (flip9 << 18); flip8 |= pre8 & (flip8 << 16);
    flip1 |= pre1 & (flip1 << 2);  flip7 |= pre7 & (flip7 << 14); flip9 |= pre9 & (flip9 << 18); flip8 |= pre8 & (flip8 << 16);
    moves = flip1 << 1;            moves |= flip7 << 7;           moves |= flip9 << 9;           moves |= flip8 << 8;
    flip1 = mO & (P >> 1);         flip7  = mO & (P >> 7);        flip9  = mO & (P >> 9);        flip8  = O & (P >> 8);
    flip1 |= mO & (flip1 >> 1);    flip7 |= mO & (flip7 >> 7);    flip9 |= mO & (flip9 >> 9);    flip8 |= O & (flip8 >> 8);
    pre1 >>= 1;                    pre7 >>= 7;                    pre9 >>= 9;                    pre8 >>= 8;
    flip1 |= pre1 & (flip1 >> 2);  flip7 |= pre7 & (flip7 >> 14); flip9 |= pre9 & (flip9 >> 18); flip8 |= pre8 & (flip8 >> 16);
    flip1 |= pre1 & (flip1 >> 2);  flip7 |= pre7 & (flip7 >> 14); flip9 |= pre9 & (flip9 >> 18); flip8 |= pre8 & (flip8 >> 16);
    moves |= flip1 >> 1;           moves |= flip7 >> 7;           moves |= flip9 >> 9;           moves |= flip8 >> 8;
    return moves & ~(P | O);
}*/


static const __m256i shift1897 = {7, 9, 8, 1}, mflipH = {0x7E7E7E7E7E7E7E7E, 0x7E7E7E7E7E7E7E7E, -1, 0x7E7E7E7E7E7E7E7E};
UINT64 __attribute__ ((always_inline)) bit_mob_mask(const UINT64 P, const UINT64 O){
    __m256i	PP, mOO, MM, flip_l, flip_r, pre_l, pre_r, shift2;
    __m128i	M;
    PP = _mm256_broadcastq_epi64(_mm_cvtsi64_si128(P));
    mOO = _mm256_and_si256(_mm256_broadcastq_epi64(_mm_cvtsi64_si128(O)), mflipH);
    flip_l = _mm256_and_si256(mOO, _mm256_sllv_epi64(PP, shift1897));
    flip_r = _mm256_and_si256(mOO, _mm256_srlv_epi64(PP, shift1897));
    flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(mOO, _mm256_sllv_epi64(flip_l, shift1897)));
    flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(mOO, _mm256_srlv_epi64(flip_r, shift1897)));
    pre_l = _mm256_and_si256(mOO, _mm256_sllv_epi64(mOO, shift1897));
    pre_r = _mm256_srlv_epi64(pre_l, shift1897);
    shift2 = _mm256_add_epi64(shift1897, shift1897);
    flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
    flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
    flip_l = _mm256_or_si256(flip_l, _mm256_and_si256(pre_l, _mm256_sllv_epi64(flip_l, shift2)));
    flip_r = _mm256_or_si256(flip_r, _mm256_and_si256(pre_r, _mm256_srlv_epi64(flip_r, shift2)));
    MM = _mm256_sllv_epi64(flip_l, shift1897);
    MM = _mm256_or_si256(MM, _mm256_srlv_epi64(flip_r, shift1897));
    M = _mm_or_si128(_mm256_castsi256_si128(MM), _mm256_extracti128_si256(MM, 1));
    M = _mm_or_si128(M, _mm_unpackhi_epi64(M, M));
    return _mm_cvtsi128_si64(M) & ~(P | O);
}
