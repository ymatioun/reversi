#include "rev.h"

float __attribute__ ((noinline)) sigma_mult(board *b, UINT64 move_mask, unsigned int d1){
    __m256i w1, w2;
    unsigned int v1, v2, i, j;

    // mob1 *****************************************************************************************
    unsigned int mob1 = std::min(21,(int)_popcnt64(move_mask)); // my mob, capped at 21
    w1 = _mm256_load_si256((__m256i*)&mob1_c2[mob1][0]);

    // mob2 *****************************************************************************************
    unsigned int mob2 = std::min(21,(int)_popcnt64(bit_mob_mask(b->pos[1], b->pos[0]))); // opp mob, capped at 21
    w2 = _mm256_load_si256((__m256i*)&mob2_c2[mob2][0]);
    w1 = _mm256_add_epi16(w1, w2);

    // n *****************************************************************************************
    unsigned int nn = std::min(41, std::max(20, (int)_popcnt64(b->pos[1] | b->pos[0])) - 20); // n, 20 to 61
    w2 = _mm256_load_si256((__m256i*)&n_c2[nn][0]);
    w1 = _mm256_add_epi16(w1, w2);

    // s8a *****************************************************************************************
    unsigned int s8a[4];
    static const UINT64 s8a_masks[] = {0x00000000000000ff, 0xff00000000000000, 0x8080808080808080, 0x0101010101010101}; // T, B, L, R
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], s8a_masks[i]);
        v2 = _pext_u64(b->pos[1], s8a_masks[i]);
        s8a[i] = 3280 + c2to3[v1] - c2to3[v2];
        w2 = _mm256_load_si256((__m256i*)&s8a_c2[s8a[i]][0]);
        w1 = _mm256_add_epi16(w1, w2);
    }

    // s8b
    unsigned int s8b[4];
    static const UINT64 s8b_masks[] = {0x000000000000ff00, 0x00ff000000000000, 0x4040404040404040, 0x0202020202020202}; // T, B, L, R
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], s8b_masks[i]);
        v2 = _pext_u64(b->pos[1], s8b_masks[i]);
        s8b[i] = 3280 + c2to3[v1] - c2to3[v2];
        w2 = _mm256_load_si256((__m256i*)&s8b_c2[s8b[i]][0]);
        w1 = _mm256_add_epi16(w1, w2);
    }

    // s8c
    unsigned int s8c[4];
    static const UINT64 s8c_masks[] = {0x0000000000ff0000, 0x0000ff0000000000, 0x2020202020202020, 0x0404040404040404}; // T, B, L, R
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], s8c_masks[i]);
        v2 = _pext_u64(b->pos[1], s8c_masks[i]);
        s8c[i] = 3280 + c2to3[v1] - c2to3[v2];
        w2 = _mm256_load_si256((__m256i*)&s8c_c2[s8c[i]][0]);
        w1 = _mm256_add_epi16(w1, w2);
    }

    // s8d
    unsigned int s8d[4];
    static const UINT64 s8d_masks[] = {0x00000000ff000000, 0x000000ff00000000, 0x1010101010101010, 0x0808080808080808}; // T, B, L, R
    for(i=0; i<4; ++i){
        v1 = _pext_u64(b->pos[0], s8d_masks[i]);
        v2 = _pext_u64(b->pos[1], s8d_masks[i]);
        s8d[i] = 3280 + c2to3[v1] - c2to3[v2];
        w2 = _mm256_load_si256((__m256i*)&s8d_c2[s8d[i]][0]);
        w1 = _mm256_add_epi16(w1, w2);
    }

    // c9
    unsigned int c9[4];
    static const UINT64 c9_masks[] = {0x0000000000070707, 0x0000000000e0e0e0, 0x0707070000000000, 0xe0e0e00000000000}; // TL, TR, BL, BR
    v1 = _pext_u64(b->pos[0], c9_masks[0]); // TL - as is
    v2 = _pext_u64(b->pos[1], c9_masks[0]);
    c9[0] = 9841 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c9_c2[c9[0]][0]);
    w1 = _mm256_add_epi16(w1, w2);

    v1 = _pext_u64(b->pos[0], c9_masks[1]); // TR - need to translate from 012345678 to 210543876
    v2 = _pext_u64(b->pos[1], c9_masks[1]);
    v1 = tr_ar_c9[512 * 0 + v1];
    v2 = tr_ar_c9[512 * 0 + v2];
    c9[1] = 9841 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c9_c2[c9[1]][0]);
    w1 = _mm256_add_epi16(w1, w2);

    v1 = _pext_u64(b->pos[0], c9_masks[2]); // BL - need to translate from 012345678 to 678345012
    v2 = _pext_u64(b->pos[1], c9_masks[2]);
    v1 = tr_ar_c9[512 * 1 + v1];
    v2 = tr_ar_c9[512 * 1 + v2];
    c9[2] = 9841 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c9_c2[c9[2]][0]);
    w1 = _mm256_add_epi16(w1, w2);

    v1 = _pext_u64(b->pos[0], c9_masks[3]); // BR - need to translate from 012345678 to 876543210
    v2 = _pext_u64(b->pos[1], c9_masks[3]);
    v1 = tr_ar_c9[512 * 2 + v1];
    v2 = tr_ar_c9[512 * 2 + v2];
    c9[3] = 9841 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c9_c2[c9[3]][0]);
    w1 = _mm256_add_epi16(w1, w2);


    // c10
    unsigned int c10[8];
    static const UINT64 c10_masks[] = {0x0000000000001f1f, 0x000000000000f8f8, 0x1f1f000000000000, 0xf8f8000000000000, 0x0000000303030303, 0x000000c0c0c0c0c0, 0x0303030303000000, 0xc0c0c0c0c0000000};
    v1 = _pext_u64(b->pos[0], c10_masks[0]); // as is: bits 0 1 2 3 4 8 9 10 11 12
    v2 = _pext_u64(b->pos[1], c10_masks[0]);
    c10[0] = 29524 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c10_c2[c10[0]][0]);
    w1 = _mm256_add_epi16(w1, w2);

    v1 = _pext_u64(b->pos[0], c10_masks[1]); // need to translate from 01234567 to 32107654
    v2 = _pext_u64(b->pos[1], c10_masks[1]);
    v1 = tr_ar_c10[1024 * 0 + v1];
    v2 = tr_ar_c10[1024 * 0 + v2];
    c10[1] = 29524 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c10_c2[c10[1]][0]);
    w1 = _mm256_add_epi16(w1, w2);

    v1 = _pext_u64(b->pos[0], c10_masks[2]); // need to translate from 01234567 to 45670123
    v2 = _pext_u64(b->pos[1], c10_masks[2]);
    v1 = tr_ar_c10[1024 * 1 + v1];
    v2 = tr_ar_c10[1024 * 1 + v2];
    c10[2] = 29524 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c10_c2[c10[2]][0]);
    w1 = _mm256_add_epi16(w1, w2);

    v1 = _pext_u64(b->pos[0], c10_masks[3]); // need to translate from 01234567 to 76543210
    v2 = _pext_u64(b->pos[1], c10_masks[3]);
    v1 = tr_ar_c10[1024 * 2 + v1];
    v2 = tr_ar_c10[1024 * 2 + v2];
    c10[3] = 29524 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c10_c2[c10[3]][0]);
    w1 = _mm256_add_epi16(w1, w2);

    v1 = _pext_u64(b->pos[0], c10_masks[4]); // need to translate from 01234567 to 04152637
    v2 = _pext_u64(b->pos[1], c10_masks[4]);
    v1 = tr_ar_c10[1024 * 3 + v1];
    v2 = tr_ar_c10[1024 * 3 + v2];
    c10[4] = 29524 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c10_c2[c10[4]][0]);
    w1 = _mm256_add_epi16(w1, w2);

    v1 = _pext_u64(b->pos[0], c10_masks[5]); // need to translate from 01234567 to 40516273
    v2 = _pext_u64(b->pos[1], c10_masks[5]);
    v1 = tr_ar_c10[1024 * 4 + v1];
    v2 = tr_ar_c10[1024 * 4 + v2];
    c10[5] = 29524 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c10_c2[c10[5]][0]);
    w1 = _mm256_add_epi16(w1, w2);

    v1 = _pext_u64(b->pos[0], c10_masks[6]); // need to translate from 01234567 to 37261504
    v2 = _pext_u64(b->pos[1], c10_masks[6]);
    v1 = tr_ar_c10[1024 * 5 + v1];
    v2 = tr_ar_c10[1024 * 5 + v2];
    c10[6] = 29524 + c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c10_c2[c10[6]][0]);
    w1 = _mm256_add_epi16(w1, w2);

    v1 = _pext_u64(b->pos[0], c10_masks[7]); // need to translate from 01234567 to 73625140
    v2 = _pext_u64(b->pos[1], c10_masks[7]);
    v1 = tr_ar_c10[1024 * 6 + v1];
    v2 = tr_ar_c10[1024 * 6 + v2];
    c10[7] = 29524+ c2to3[v1] - c2to3[v2];
    w2 = _mm256_load_si256((__m256i*)&c10_c2[c10[7]][0]);
    w1 = _mm256_add_epi16(w1, w2);





    // relu
    __m256i z = _mm256_set1_epi16(0);
    w1 = _mm256_max_epi16(w1, z);


    // convert epi16 into epi32
    // a. save epi16
    int16_t t1[16];
    _mm256_store_si256((__m256i*)t1, w1);
    // b. load as epi32
    __m128i w3a[2];
    __m256i w3[2];
    w3a[0] = _mm_load_si128((__m128i*)&t1[0]);
    w3a[1] = _mm_load_si128((__m128i*)&t1[8]);
    // c. convert 128 to 256
    w3[0] = _mm256_cvtepi16_epi32(w3a[0]);
    w3[1] = _mm256_cvtepi16_epi32(w3a[1]);

    // epi32 into ps
    __m256 w4[2];
    w4[0] = _mm256_cvtepi32_ps(w3[0]);
    w4[1] = _mm256_cvtepi32_ps(w3[1]);

    // load ps multiples
    __m256 w5[2];
    if(d1 > 17) // 0 to 17 only
        d1 = d1 % 2 + std::min((int)(d1 - 2) / 2, 8) * 2;
    w5[0] = _mm256_load_ps(&d_c2[d1][0]);
    w5[1] = _mm256_load_ps(&d_c2[d1][8]);

    // multiply
    w5[0] = _mm256_mul_ps(w4[0], w5[0]);
    w5[1] = _mm256_mul_ps(w4[1], w5[1]);

    // + into a single ps
    w5[0] = _mm256_add_ps(w5[0], w5[1]);

    // store and sum-up remaining 8 floats
    float s1 = 0.0f, s2[8];
    _mm256_store_ps(s2, w5[0]);
    for(i=0; i<8; ++i)
        s1 += s2[i];

    return(1.0f + s1);
}
