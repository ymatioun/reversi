#include "rev.h"

#define	bswap_int(x)	__builtin_bswap32(x)
UINT64 bit_mob_mask(UINT64 P,UINT64 O){
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
}
