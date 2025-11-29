#include "rev.h"

# define S (EVAL_WIDTH/16) // number of mm256 vars in use
int __attribute__ ((always_inline)) eval(board *b, UINT64 move_mask){
    __m256i w1[S], w2[S];
    unsigned int v1, v2, i, j, n, mob1, mob2;

    n = _popcnt64(b->pos[0] | b->pos[1]);
    n = std::min(60, std::max((int)n, 12)) - 12; // now 0 to 48

    // mob1 *****************************************************************************************
    mob1 = std::min(23, (int)_popcnt64(move_mask)); // my mob, capped at 23
    for(j=0; j<S; ++j)
        w1[j] = _mm256_load_si256((__m256i*)&mob1_c[mob1 * EVAL_WIDTH + 16 * j]);

    // mob2 *****************************************************************************************
    mob2 = std::min(23, (int)_popcnt64(bit_mob_mask(b->pos[1], b->pos[0]))); // opp mob, capped at 23
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&mob2_c[mob2 * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    // s8a *****************************************************************************************
    unsigned int s8a[4];
    static const UINT64 s8a_masks[] = {0x00000000000000ff, 0xff00000000000000, 0x8080808080808080, 0x0101010101010101}; // T, B, L, R
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], s8a_masks[i]);
        v2 = _pext_u64(b->pos[1], s8a_masks[i]);
        s8a[i] = 3280 + c2to3[v1] - c2to3[v2];
        for(j=0; j<S; ++j){
            w2[j] = _mm256_load_si256((__m256i*)&s8a_c[s8a[i] * EVAL_WIDTH + 16 * j]);
            w1[j] = _mm256_add_epi16(w1[j], w2[j]);
        }
    }

    // s8b
    unsigned int s8b[4];
    static const UINT64 s8b_masks[] = {0x000000000000ff00, 0x00ff000000000000, 0x4040404040404040, 0x0202020202020202}; // T, B, L, R
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], s8b_masks[i]);
        v2 = _pext_u64(b->pos[1], s8b_masks[i]);
        s8b[i] = 3280 + c2to3[v1] - c2to3[v2];
        for(j=0; j<S; ++j){
            w2[j] = _mm256_load_si256((__m256i*)&s8b_c[s8b[i] * EVAL_WIDTH + 16 * j]);
            w1[j] = _mm256_add_epi16(w1[j], w2[j]);
        }
    }

    // s8c
    unsigned int s8c[4];
    static const UINT64 s8c_masks[] = {0x0000000000ff0000, 0x0000ff0000000000, 0x2020202020202020, 0x0404040404040404}; // T, B, L, R
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], s8c_masks[i]);
        v2 = _pext_u64(b->pos[1], s8c_masks[i]);
        s8c[i] = 3280 + c2to3[v1] - c2to3[v2];
        for(j=0; j<S; ++j){
            w2[j] = _mm256_load_si256((__m256i*)&s8c_c[s8c[i] * EVAL_WIDTH + 16 * j]);
            w1[j] = _mm256_add_epi16(w1[j], w2[j]);
        }
    }

    // s8d
    unsigned int s8d[4];
    static const UINT64 s8d_masks[] = {0x00000000ff000000, 0x000000ff00000000, 0x1010101010101010, 0x0808080808080808}; // T, B, L, R
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], s8d_masks[i]);
        v2 = _pext_u64(b->pos[1], s8d_masks[i]);
        s8d[i] = 3280 + c2to3[v1] - c2to3[v2];
        for(j=0; j<S; ++j){
            w2[j] = _mm256_load_si256((__m256i*)&s8d_c[s8d[i] * EVAL_WIDTH + 16 * j]);
            w1[j] = _mm256_add_epi16(w1[j], w2[j]);
        }
    }

    // d8
    unsigned int d8[2];
    static const UINT64 d8_masks[] = {0x8040201008040201, 0x0102040810204080};
    for(i=0; i<2; ++i){
        v1 = _pext_u64(b->pos[0], d8_masks[i]);
        v2 = _pext_u64(b->pos[1], d8_masks[i]);
        d8[i] = 3280 + c2to3[v1] - c2to3[v2];
        for(j=0; j<S; ++j){
            w2[j] = _mm256_load_si256((__m256i*)&d8_c[d8[i] * EVAL_WIDTH + 16 * j]);
            w1[j] = _mm256_add_epi16(w1[j], w2[j]);
        }
    }

    // d7
    unsigned int d7[4];
    static const UINT64 d7_masks[] = {0x0080402010080402, 0x4020100804020100, 0x0001020408102040, 0x0204081020408000};
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], d7_masks[i]);
        v2 = _pext_u64(b->pos[1], d7_masks[i]);
        d7[i] = 1093 + c2to3[v1] - c2to3[v2];
        for(j=0; j<S; ++j){
            w2[j] = _mm256_load_si256((__m256i*)&d7_c[d7[i] * EVAL_WIDTH + 16 * j]);
            w1[j] = _mm256_add_epi16(w1[j], w2[j]);
        }
    }

    // d6
    unsigned int d6[4];
    static const UINT64 d6_masks[] = {0x0000804020100804, 0x2010080402010000, 0x0000010204081020, 0x0408102040800000};
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], d6_masks[i]);
        v2 = _pext_u64(b->pos[1], d6_masks[i]);
        d6[i] = 364 + c2to3[v1] - c2to3[v2];
        for(j=0; j<S; ++j){
            w2[j] = _mm256_load_si256((__m256i*)&d6_c[d6[i] * EVAL_WIDTH + 16 * j]);
            w1[j] = _mm256_add_epi16(w1[j], w2[j]);
        }
    }

    // d5
    unsigned int d5[4];
    static const UINT64 d5_masks[] = {0x0000008040201008, 0x1008040201000000, 0x0000000102040810, 0x0810204080000000};
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], d5_masks[i]);
        v2 = _pext_u64(b->pos[1], d5_masks[i]);
        d5[i] = 121 + c2to3[v1] - c2to3[v2];
        for(j=0; j<S; ++j){
            w2[j] = _mm256_load_si256((__m256i*)&d5_c[d5[i] * EVAL_WIDTH + 16 * j]);
            w1[j] = _mm256_add_epi16(w1[j], w2[j]);
        }
    }

    // d4
    unsigned int d4[4];
    static const UINT64 d4_masks[] = {0x0804020100000000, 0x0000000080402010, 0x0000000001020408, 0x1020408000000000};
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], d4_masks[i]);
        v2 = _pext_u64(b->pos[1], d4_masks[i]);
        d4[i] = 40 + c2to3[v1] - c2to3[v2];
        for(j=0; j<S; ++j){
            w2[j] = _mm256_load_si256((__m256i*)&d4_c[d4[i] * EVAL_WIDTH + 16 * j]);
            w1[j] = _mm256_add_epi16(w1[j], w2[j]);
        }
    }

    // c9
    unsigned int c9[4];
    static const UINT64 c9_masks[] = {0x0000000000070707, 0x0000000000e0e0e0, 0x0707070000000000, 0xe0e0e00000000000}; // TL, TR, BL, BR
    v1 = _pext_u64(b->pos[0], c9_masks[0]); // TL - as is
    v2 = _pext_u64(b->pos[1], c9_masks[0]);
    c9[0] = 9841 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c9_c[c9[0] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c9_masks[1]); // TR - need to translate from 012345678 to 210543876
    v2 = _pext_u64(b->pos[1], c9_masks[1]);
    v1 = tr_ar_c9[512 * 0 + v1];
    v2 = tr_ar_c9[512 * 0 + v2];
    c9[1] = 9841 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c9_c[c9[1] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c9_masks[2]); // BL - need to translate from 012345678 to 678345012
    v2 = _pext_u64(b->pos[1], c9_masks[2]);
    v1 = tr_ar_c9[512 * 1 + v1];
    v2 = tr_ar_c9[512 * 1 + v2];
    c9[2] = 9841 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c9_c[c9[2] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c9_masks[3]); // BR - need to translate from 012345678 to 876543210
    v2 = _pext_u64(b->pos[1], c9_masks[3]);
    v1 = tr_ar_c9[512 * 2 + v1];
    v2 = tr_ar_c9[512 * 2 + v2];
    c9[3] = 9841 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c9_c[c9[3] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    // c8 (c10)
    unsigned int c8[8];
    static const UINT64 c8_masks[] = {0x0000000000001f1f, 0x000000000000f8f8, 0x1f1f000000000000, 0xf8f8000000000000, 0x0000000303030303, 0x000000c0c0c0c0c0, 0x0303030303000000, 0xc0c0c0c0c0000000};
    v1 = _pext_u64(b->pos[0], c8_masks[0]); // as is: bits 0 1 2 3 4 8 9 10 11 12
    v2 = _pext_u64(b->pos[1], c8_masks[0]);
    c8[0] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c8_c[c8[0] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c8_masks[1]); // need to translate from 01234567 to 32107654
    v2 = _pext_u64(b->pos[1], c8_masks[1]);
    v1 = tr_ar_c10[1024 * 0 + v1];
    v2 = tr_ar_c10[1024 * 0 + v2];
    c8[1] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c8_c[c8[1] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c8_masks[2]); // need to translate from 01234567 to 45670123
    v2 = _pext_u64(b->pos[1], c8_masks[2]);
    v1 = tr_ar_c10[1024 * 1 + v1];
    v2 = tr_ar_c10[1024 * 1 + v2];
    c8[2] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c8_c[c8[2] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c8_masks[3]); // need to translate from 01234567 to 76543210
    v2 = _pext_u64(b->pos[1], c8_masks[3]);
    v1 = tr_ar_c10[1024 * 2 + v1];
    v2 = tr_ar_c10[1024 * 2 + v2];
    c8[3] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c8_c[c8[3] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c8_masks[4]); // need to translate from 01234567 to 04152637
    v2 = _pext_u64(b->pos[1], c8_masks[4]);
    v1 = tr_ar_c10[1024 * 3 + v1];
    v2 = tr_ar_c10[1024 * 3 + v2];
    c8[4] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c8_c[c8[4] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c8_masks[5]); // need to translate from 01234567 to 40516273
    v2 = _pext_u64(b->pos[1], c8_masks[5]);
    v1 = tr_ar_c10[1024 * 4 + v1];
    v2 = tr_ar_c10[1024 * 4 + v2];
    c8[5] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c8_c[c8[5] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c8_masks[6]); // need to translate from 01234567 to 37261504
    v2 = _pext_u64(b->pos[1], c8_masks[6]);
    v1 = tr_ar_c10[1024 * 5 + v1];
    v2 = tr_ar_c10[1024 * 5 + v2];
    c8[6] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c8_c[c8[6] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c8_masks[7]); // need to translate from 01234567 to 73625140
    v2 = _pext_u64(b->pos[1], c8_masks[7]);
    v1 = tr_ar_c10[1024 * 6 + v1];
    v2 = tr_ar_c10[1024 * 6 + v2];
    c8[7] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c8_c[c8[7] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }




    // c10a
    unsigned int c10a[4];
    static const UINT64 c10a_masks[] = {0x00000000000066e7, 0x80c0c00000c0c080, 0x0103030000030301, 0xe766000000000000}; // T, R, L, B
    v1 = _pext_u64(b->pos[0], c10a_masks[0]); // T - as is
    v2 = _pext_u64(b->pos[1], c10a_masks[0]);
    c10a[0] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c10a_c[c10a[0] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c10a_masks[1]); // R - need to translate from T to R
    v2 = _pext_u64(b->pos[1], c10a_masks[1]);
    v1 = tr_ar_c10[1024 * 7 + v1];
    v2 = tr_ar_c10[1024 * 7 + v2];
    c10a[1] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c10a_c[c10a[1] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c10a_masks[2]); // L - need to translate from T to L
    v2 = _pext_u64(b->pos[1], c10a_masks[2]);
    v1 = tr_ar_c10[1024 * 8 + v1];
    v2 = tr_ar_c10[1024 * 8 + v2];
    c10a[2] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c10a_c[c10a[2] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }

    v1 = _pext_u64(b->pos[0], c10a_masks[3]); // B - need to translate from T to B
    v2 = _pext_u64(b->pos[1], c10a_masks[3]);
    v1 = tr_ar_c10[1024 * 9 + v1];
    v2 = tr_ar_c10[1024 * 9 + v2];
    c10a[3] = 29524 + c2to3[v1] - c2to3[v2];
    for(j=0; j<S; ++j){
        w2[j] = _mm256_load_si256((__m256i*)&c10a_c[c10a[3] * EVAL_WIDTH + 16 * j]);
        w1[j] = _mm256_add_epi16(w1[j], w2[j]);
    }






    // convert epi16 into epi32
    // a. save epi16
    int16_t t1[EVAL_WIDTH];
    for(j=0; j<S; ++j)
        _mm256_store_si256((__m256i*)(t1+16*j), w1[j]);
    // b. load as epi32
    __m128i w3a[S*2];
    __m256i w3[S*2];
    for(j=0; j<S*2; ++j)
        w3a[j] = _mm_load_si128((__m128i*)&t1[8*j]);
    // c. convert 128 to 256
    for(j=0; j<S*2; ++j)
        w3[j] = _mm256_cvtepi16_epi32(w3a[j]);

    // epi32 into ps
    __m256 w4[S*2];
    for(j=0; j<S*2; ++j)
        w4[j] = _mm256_cvtepi32_ps(w3[j]);

    // load ps multiples
    __m256 w5[S*2];
    for(j=0; j<S*2; ++j)
        w5[j] = _mm256_load_ps(&n_c[n][8*j]);

    // multiply
    for(j=0; j<S*2; ++j)
        w5[j] = _mm256_mul_ps(w4[j], w5[j]);

    // floor at 0
    __m256 z = _mm256_set1_ps(0.0f);
    for(j=0; j<S*2; ++j)
        w5[j] = _mm256_max_ps(w5[j], z);

    // +- into a single ps
    w5[0] = _mm256_sub_ps(w5[0], w5[S]);
    for(j=1; j<S; ++j){
        w5[0] = _mm256_add_ps(w5[0], w5[j]);
        w5[0] = _mm256_sub_ps(w5[0], w5[j + S]);
    }

    // store and sum-up remaining floats
    float s1 = 0.0f, s2[8];
    _mm256_store_ps(s2, w5[0]);
    for(i=0; i<8; ++i)
        s1 += s2[i];


    // turn into final score - divide by 128 and round
    int score;
    if(s1 > 0) // this helps
        score = s1 / 128.0f + 0.49f;
    else
        score = s1 / 128.0f - 0.49f;
    return( std::min(64, std::max(score, -64)) ); // clipping does not seem to help



    // integer post-processing - does not seem to work
    /*unsigned int n2 = _popcnt64(b->pos[0] | b->pos[1]);
    n2 = std::min(60, std::max((int)n2, 12)) - 12; // now 0 to 48
    __m256i z2 = _mm256_set1_epi64x(0);
    for(j=0; j<S; ++j){
        // load multiples
        w2[j] = _mm256_load_si256((__m256i*)&n_ci[n2][16*j]);
        // integer multiplication, low 16 bits only. But first, divide by c_ni multiple
        //w1[j] = _mm256_srai_epi16(w1[j], 2); // divide by ...
        w1[j] = _mm256_mullo_epi16(w1[j], w2[j]);
        // floor at 0
        w1[j] = _mm256_max_epi16(w1[j], z2);
    }

    // +- into a single int16; here size is hardcoded (64, so S=4)
    w1[0] = _mm256_add_epi16(w1[0], w1[1]);
    w1[2] = _mm256_add_epi16(w1[2], w1[3]);
    w1[0] = _mm256_sub_epi16(w1[0], w1[2]);

    // store and sum-up remaining ints
    eval_type s12 = 0, s22[16];
    _mm256_store_si256((__m256i*)s22, w1[0]);
    for(i=0; i<16; ++i)
        s12 += s22[i];


    // turn into final score - divide by 128
    int score2;
    if(s12 > 0)
        score2 = (s12 + 63) / 128 / 4;
    else
        score2 = (s12 - 63) / 128 / 4;

    return(score2);*/
}
