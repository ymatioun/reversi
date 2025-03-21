#include "rev.h"
#include "hash.h"
#include <malloc.h>
#include <stdlib.h>
#include <sys/mman.h>

extern int ggs_score;

void init_hash(void), init_arrays(void), init_eval(void), init_sigmas(void);

void Ginit(void){
    srand(123435);	// init random number generator, JIC
    init_hash();
    init_arrays();
    init_eval();
    init_sigmas();





    // test: load training data *************************************************************************************************************
    /*typedef struct{// positions of training set of games, used for fitting coefficients
        unsigned int pos32[4];	//16
        unsigned char mob1, mob2;//2, mobilities
        char sc;				//1, true score [-64, 64]
        char tmp2;	            //1, placeholder
    } posit; // 20 bytes a pop.
    posit *p0;
	unsigned int pos_count = 22000000;	// 22M positions; used to allocate memory.
	FILE *f1 = fopen("pos_bin0.bin", "rb");	// input file
	p0 = (posit*)malloc(pos_count*sizeof(posit));		// 20Mb per 1M positions.
	pos_count = (unsigned int)fread(p0,sizeof(posit),pos_count,f1);	// set "pos_count" to actual count of positions.
	fclose(f1);*/





	// test: run training set through search, save best position after [1] move *************************************************************************************************************
	/*for(unsigned int i=0; i<pos_count; ++i){
        UINT64 *t = (UINT64*)&p0[i], move_mask = bit_mob_mask(t[0], t[1]), pl[2];
        unsigned int move_list[32], mc = move_list_f(move_list, move_mask);
        int s0 = -128;
        pl[0] = t[0]; pl[1] = t[1]; // save
        for(unsigned int j=0; j<mc; ++j){ // loop over moves
            b_m.pos[0] = pl[0]; b_m.pos[1] = pl[1]; // restore
            make_move(move_list[j], &b_m); // make a move. This flips players
            move_mask = bit_mob_mask(b_m.pos[0], b_m.pos[1]);
            int s = -eval(&b_m, move_mask);
            if(s > s0){
                s0 = s; t[0] = b_m.pos[0]; t[1] = b_m.pos[1];
            }
        }
        if(mc) // flip score, only if there are moves
            p0[i].sc = -p0[i].sc; // flip score
        p0[i].mob1 = _popcnt64(bit_mob_mask(t[0], t[1])); p0[i].mob2 = _popcnt64(bit_mob_mask(t[1], t[0])); // refresh mobs
	}
	// save new file
	f1 = fopen("pos_bin1.bin", "wb");
	fwrite(p0, sizeof(posit), pos_count, f1);
	fclose(f1); exit(0);*/





	// test: run training set through search, save best position after [2] moves *************************************************************************************************************
	/*for(unsigned int i=0; i<pos_count; ++i){
        UINT64 *t = (UINT64*)&p0[i], t2[2], move_mask = bit_mob_mask(t[0], t[1]), pl[2];
        unsigned int move_list[32], mc = move_list_f(move_list, move_mask);
        pl[0] = t[0]; pl[1] = t[1]; // save
        int s0 = -128;

        b_m.pos[0] = pl[0]; b_m.pos[1] = pl[1]; // restore
        for(unsigned int j=0; j<mc; ++j){ // loop over moves
            b_m.pos[0] = pl[0]; b_m.pos[1] = pl[1]; // restore
            make_move(move_list[j], &b_m); // make a move. This flips players
            move_mask = bit_mob_mask(b_m.pos[0], b_m.pos[1]);
            unsigned int move_list2[32], mc2 = move_list_f(move_list2, move_mask);
            int s02 = 128;
            UINT64 pl2[2];
            pl2[0] = b_m.pos[0]; pl2[1] = b_m.pos[1]; // save
            for(unsigned int j2=0; j2<mc; ++j2){ // loop over moves2
                b_m.pos[0] = pl2[0]; b_m.pos[1] = pl2[1]; // restore
                make_move(move_list2[j2], &b_m); // make a move. This flips players
                move_mask = bit_mob_mask(b_m.pos[0], b_m.pos[1]);
                int s = eval(&b_m, move_mask);
                if(s > s02){
                    s02 = s; t2[0] = b_m.pos[0]; t2[1] = b_m.pos[1]; // keep the sign here
                }
            } // loop 2
            if(s02 < s0){
                s0 = s02; t[0] = t2[0]; t[1] = t2[1];
            }
        } // loop 1
        p0[i].mob1 = _popcnt64(bit_mob_mask(t[0], t[1])); p0[i].mob2 = _popcnt64(bit_mob_mask(t[1], t[0])); // refresh mobs
	}
	// save new file
	f1 = fopen("pos_bin2.bin", "wb");
	fwrite(p0, sizeof(posit), pos_count, f1);
	fclose(f1); exit(0);*/





    // test: run training set through sigma_mult, save sm1 - used to validate sigma_mult implementation
    /*f1=fopen("pos_sigma.csv", "w"); // output file
	fprintf(f1, "n,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12,s13,s14,s15,s16,s17,s18,s19\n");
	for(unsigned int i=0; i<pos_count; ++i){
        UINT64 *t = (UINT64*)&p0[i];
        b_m.pos[0] = t[0]; b_m.pos[1] = t[1];
        int n = (int)_popcnt64(b_m.pos[0] | b_m.pos[1]);
        // only keep n 20 to 58
        if(n > 58 || n < 20)
            continue;
        fprintf(f1, "%d", n);
        for(unsigned int depth=2; depth<=19; ++depth){
            float sm1 = sigma_mult(&b_m, bit_mob_mask(b_m.pos[0], b_m.pos[1]), depth);
            fprintf(f1, ",%.2f", sm1);
        }
        fprintf(f1, "\n");
	}
	fclose(f1); exit(0);*/





    // test: run training set through eval, save evaluation scores - used to validate evaluation function implementation
    /*f1=fopen("pos_eval.csv", "w"); // output file
	fprintf(f1, "n,sc,ev\n");
	for(unsigned int i=0; i<pos_count; ++i){
        UINT64 *t = (UINT64*)&p0[i];
        b_m.pos[0] = t[0]; b_m.pos[1] = t[1];
        int score = eval(&b_m, bit_mob_mask(b_m.pos[0], b_m.pos[1]));
        int n = (int)_popcnt64(b_m.pos[0] | b_m.pos[1]);
        fprintf(f1, "%d,%d,%d\n", n, (int)p0[i].sc, score);
	}
	fclose(f1); exit(0);*/




	// compute sigmas for each position in training set.
	/*
	// loop over all positions
	f1=fopen("sigmas_even_2.csv", "w"); // output file *********************************
	TimePoint t1, t2;
	t1 = timeGetTime(); // starting time
	t_break = t1 + 1000000000; // way in the future
	int s;
	for(unsigned int i=0; i<pos_count; ++i){ // 19.45
        UINT64 *t = (UINT64*)&p0[i];
        b_m.pos[0] = t[0]; b_m.pos[1] = t[1];
        int s0 = eval(&b_m, bit_mob_mask(b_m.pos[0], b_m.pos[1]));
        int n = _popcnt64(b_m.pos[0] | b_m.pos[1]);
        if(n < 10 || n > 60)// only keep what i need for sigmas
            continue;
        fprintf(f1, "%d, %d, %d, %d, %d", n, p0[i].sc, s0, _popcnt64(bit_mob_mask(b_m.pos[0], b_m.pos[1])), _popcnt64(bit_mob_mask(b_m.pos[1], b_m.pos[0]))); // n, score, eval, mob1, mob2
        UINT64 move_mask = bit_mob_mask(b_m.pos[0], b_m.pos[1]);
        for(unsigned int j=2; j<=2; j+=2){ // 16: 4.7 h, 17: 8.6 h, 18: 12.0 h
            if(n+j <= 60)
                s = Msearch(std::min((int)j, 64 - n), MIN_SCORE, MAX_SCORE, &b_m, move_mask, 1);// call move predictor ******************************************************************************
            else
                s = p0[i].sc; // skip search past n=60 - it gets extended to infinity and is very slow.
            fprintf(f1, ",%d", s); // here writing results immediately has 0 overhead (per perf)
        }
        fprintf(f1, "\n");
        // time
        if((i % 10000) == 0){
            t2 = timeGetTime();
            clear_hash();
            char sss[300];
            sprintf(sss, "i %.2f, time %.1f\n", i / 1000000.f, (t2 - t1) / 1000.f / 60.f); // minutes
            pass_message_to_GUI(sss);
        }
	}
	fclose(f1);*/
}

static void init_hash(){ // allocate hash
	UINT64 sz = sizeof(hashtype) * (HASH_SIZE + 10);
	h = (hashtype*)aligned_alloc(2 * 1024 * 1024, sz); // large memory pages (2 Mb)
	madvise((void*)h, sz, MADV_HUGEPAGE);
	clear_hash(); // this also inits history
}

static void init_arrays(){// allocate params and other vars
    const unsigned int sizes[] = {7 * 1024 * sizeof(unsigned int) / sizeof(eval_type) / EVAL_WIDTH, 3 * 512 * sizeof(unsigned int) / sizeof(eval_type) / EVAL_WIDTH, 1024 * sizeof(unsigned int) / sizeof(eval_type) / EVAL_WIDTH, 24, 24, 81*81, 81*81, 81*81, 81*81, 81*81, 81*27, 81*9, 81*3, 81, 81*81*9, 81*81*3};
    unsigned int st = 0, i;
    for(i=0; i<sizeof(sizes)/sizeof(sizes[0]); ++i)
        st += sizes[i];
	UINT64 sz = sizeof(eval_type) * st * EVAL_WIDTH;
	eval_type * t = (eval_type*)aligned_alloc(2 * 1024 * 1024, sz); // large memory pages (2 Mb)
	madvise((void*)t, sz, MADV_HUGEPAGE);

	tr_ar_c10 = (unsigned int *)t;
	t += 1024 * 7 * sizeof(unsigned int) / sizeof(eval_type); // 7 patterns of up to 10 bits;

	tr_ar_c9 = (unsigned int *)t;
	t += 512 * 3 * sizeof(unsigned int) / sizeof(eval_type); // 3 patterns of up to 9 bits;

	c2to3 = (unsigned int *)t;
	t += 1024 * sizeof(unsigned int) / sizeof(eval_type); // up to 10 bits;

	i = 3; // update this value as needed
	mob1_c = t;
	t += sizes[i] * EVAL_WIDTH;
	mob2_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	s8a_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	s8b_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	s8c_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	s8d_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	d8_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	d7_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	d6_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	d5_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	d4_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	c8_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;
	c9_c = t; i++;
	t += sizes[i] * EVAL_WIDTH;


	// prepare conversion arrays - 9 bits
	for(unsigned int i1=0; i1<2; ++i1)
    for(unsigned int i2=0; i2<2; ++i2)
    for(unsigned int i3=0; i3<2; ++i3)
    for(unsigned int i4=0; i4<2; ++i4)
    for(unsigned int i5=0; i5<2; ++i5)
	for(unsigned int i6=0; i6<2; ++i6)
	for(unsigned int i7=0; i7<2; ++i7)
	for(unsigned int i8=0; i8<2; ++i8)
	for(unsigned int i9=0; i9<2; ++i9){
        unsigned int ii = i1 + i2 * 2 + i3 * 4 + i4 * 8 + i5 * 16 + i6 * 32 + i7 * 64 + i8 * 128 + i9 * 256;
        // tr_ar_c9
        tr_ar_c9[512 * 0 + ii] = i3 + i2 * 2 + i1 * 4 + i6 * 8 + i5 * 16 + i4 * 32 + i9 * 64 + i8 * 128 + i7 * 256;
        tr_ar_c9[512 * 1 + ii] = i7 + i8 * 2 + i9 * 4 + i4 * 8 + i5 * 16 + i6 * 32 + i1 * 64 + i2 * 128 + i3 * 256;
        tr_ar_c9[512 * 2 + ii] = i9 + i8 * 2 + i7 * 4 + i6 * 8 + i5 * 16 + i4 * 32 + i3 * 64 + i2 * 128 + i1 * 256;
    }


    // prepare conversion arrays - 10 bits
	for(unsigned int i1=0; i1<2; ++i1)
    for(unsigned int i2=0; i2<2; ++i2)
    for(unsigned int i3=0; i3<2; ++i3)
    for(unsigned int i4=0; i4<2; ++i4)
    for(unsigned int i5=0; i5<2; ++i5)
	for(unsigned int i6=0; i6<2; ++i6)
	for(unsigned int i7=0; i7<2; ++i7)
	for(unsigned int i8=0; i8<2; ++i8)
	for(unsigned int i9=0; i9<2; ++i9)
	for(unsigned int i10=0; i10<2; ++i10){
        unsigned int ii = i1 + i2 * 2 + i3 * 4 + i4 * 8 + i5 * 16 + i6 * 32 + i7 * 64 + i8 * 128 + i9 * 256 + i10 * 512;
        // c2to3
        c2to3[ii] = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81 + i6 * 243 + i7 * 729 + i8 * 2187 + i9 * 6561 + i10 * 19683;

        // need to translate from 7 c10 patterns
        tr_ar_c10[1024 * 0 + ii] = i5 + i4 * 2 + i3 * 4 + i2 * 8 + i1 * 16 + i10 * 32 + i9 * 64 + i8 * 128 + i7 * 256 + i6 * 512;
        tr_ar_c10[1024 * 1 + ii] = i6 + i7 * 2 + i8 * 4 + i9 * 8 + i10 * 16 + i1 * 32 + i2 * 64 + i3 * 128 + i4 * 256 + i5 * 512;
        tr_ar_c10[1024 * 2 + ii] = i10 + i9 * 2 + i8 * 4 + i7 * 8 + i6 * 16 + i5 * 32 + i4 * 64 + i3 * 128 + i2 * 256 + i1 * 512;
        tr_ar_c10[1024 * 3 + ii] = i1 + i3 * 2 + i5 * 4 + i7 * 8 + i9 * 16 + i2 * 32 + i4 * 64 + i6 * 128 + i8 * 256 + i10 * 512;
        tr_ar_c10[1024 * 4 + ii] = i2 + i4 * 2 + i6 * 4 + i8 * 8 + i10 * 16 + i1 * 32 + i3 * 64 + i5 * 128 + i7 * 256 + i9 * 512;
        tr_ar_c10[1024 * 5 + ii] = i9 + i7 * 2 + i5 * 4 + i3 * 8 + i1 * 16 + i10 * 32 + i8 * 64 + i6 * 128 + i4 * 256 + i2 * 512;
        tr_ar_c10[1024 * 6 + ii] = i10 + i8 * 2 + i6 * 4 + i4 * 8 + i2 * 16 + i9 * 32 + i7 * 64 + i5 * 128 + i3 * 256 + i1 * 512;
    }
}

static int compare(const void* a, const void* b){ // Comparison function for bsearch()
    return (*(int*)a - *(int*)b);
}

static void init_eval(){ // load eval coeffs
    // l_s8a - 3005 integer values
    unsigned int l_s8a_s = 0;
    FILE *f = fopen("eval_v13//l_s8a.txt", "r");
    int l_s8a[6561];
    while(1){
        if( fscanf(f, "%d\n", l_s8a + l_s8a_s) < 1 )
            break;
        l_s8a_s++;
    }
    assert(l_s8a_s < sizeof(l_s8a) / sizeof(l_s8a[0])); // make sure there is no overflow on read
    fclose(f);

    // l_s8b - 3321 integer values
    unsigned int l_s8b_s = 0;
    f = fopen("eval_v13//l_s8b.txt", "r");
    int l_s8b[6561];
    while(1){
        if( fscanf(f, "%d\n", l_s8b + l_s8b_s) < 1 )
            break;
        l_s8b_s++;
    }
    assert(l_s8b_s < sizeof(l_s8b) / sizeof(l_s8b[0])); // make sure there is no overflow on read
    fclose(f);

    // l_s8c - 3321 integer values
    unsigned int l_s8c_s = 0;
    f = fopen("eval_v13//l_s8c.txt", "r");
    int l_s8c[6561];
    while(1){
        if( fscanf(f, "%d\n", l_s8c + l_s8c_s) < 1 )
            break;
        l_s8c_s++;
    }
    assert(l_s8c_s < sizeof(l_s8c) / sizeof(l_s8c[0])); // make sure there is no overflow on read
    fclose(f);

    // l_s8d - 1485 integer values
    unsigned int l_s8d_s = 0;
    f = fopen("eval_v13//l_s8d.txt", "r");
    int l_s8d[6561];
    while(1){
        if( fscanf(f, "%d\n", l_s8d + l_s8d_s) < 1 )
            break;
        l_s8d_s++;
    }
    assert(l_s8d_s < sizeof(l_s8d) / sizeof(l_s8d[0])); // make sure there is no overflow on read
    fclose(f);

    // l_d8 - 1405 integer values
    unsigned int l_d8_s = 0;
    f = fopen("eval_v13//l_d8.txt", "r");
    int l_d8[6561];
    while(1){
        if( fscanf(f, "%d\n", l_d8 + l_d8_s) < 1 )
            break;
        l_d8_s++;
    }
    assert(l_d8_s < sizeof(l_d8) / sizeof(l_d8[0])); // make sure there is no overflow on read
    fclose(f);

    // l_d7 - 729 integer values
    unsigned int l_d7_s = 0;
    f = fopen("eval_v13//l_d7.txt", "r");
    int l_d7[2187];
    while(1){
        if( fscanf(f, "%d\n", l_d7 + l_d7_s) < 1 )
            break;
        l_d7_s++;
    }
    assert(l_d7_s < sizeof(l_d7) / sizeof(l_d7[0])); // make sure there is no overflow on read
    fclose(f);

    // l_d6 - 378 integer values
    unsigned int l_d6_s = 0;
    f = fopen("eval_v13//l_d6.txt", "r");
    int l_d6[729];
    while(1){
        if( fscanf(f, "%d\n", l_d6 + l_d6_s) < 1 )
            break;
        l_d6_s++;
    }
    assert(l_d7_s < sizeof(l_d7) / sizeof(l_d7[0])); // make sure there is no overflow on read
    fclose(f);

    // l_d5 - 135 integer values
    unsigned int l_d5_s = 0;
    f = fopen("eval_v13//l_d5.txt", "r");
    int l_d5[243];
    while(1){
        if( fscanf(f, "%d\n", l_d5 + l_d5_s) < 1 )
            break;
        l_d5_s++;
    }
    assert(l_d5_s < sizeof(l_d5) / sizeof(l_d5[0])); // make sure there is no overflow on read
    fclose(f);

    // l_d4 - 45 integer values
    unsigned int l_d4_s = 0;
    f = fopen("eval_v13//l_d4.txt", "r");
    int l_d4[81];
    while(1){
        if( fscanf(f, "%d\n", l_d4 + l_d4_s) < 1 )
            break;
        l_d4_s++;
    }
    assert(l_d4_s < sizeof(l_d4) / sizeof(l_d4[0])); // make sure there is no overflow on read
    fclose(f);

    // l_c8 - integer values
    unsigned int l_c8_s = 0;
    f = fopen("eval_v13//l_c8.txt", "r");
    int l_c8[33000];
     while(1){
        if( fscanf(f, "%d\n", l_c8 + l_c8_s) < 1 )
            break;
        l_c8_s++;
    }
    assert(l_c8_s < sizeof(l_c8) / sizeof(l_c8[0])); // make sure there is no overflow on read
    fclose(f);

    // l_c9 - 5154 integer values
    unsigned int l_c9_s = 0;
    f = fopen("eval_v13//l_c9.txt", "r");
    int l_c9[5000];
     while(1){
        if( fscanf(f, "%d\n", l_c9 + l_c9_s) < 1 )
            break;
        l_c9_s++;
    }
    assert(l_c9_s < sizeof(l_c9) / sizeof(l_c9[0])); // make sure there is no overflow on read
    fclose(f);

    // c_s8a - x EVAL_WIDTH short int values ******************************************************************
    f = fopen("eval_v13//c_s8a.txt", "r");
    short int c_s8a[6561 * EVAL_WIDTH];
    for(unsigned int i=0; i<l_s8a_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_s8a + i);
    fclose(f);

    // c_s8b - x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_s8b.txt", "r");
    short int c_s8b[6561 * EVAL_WIDTH];
    for(unsigned int i=0; i<l_s8b_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_s8b + i);
    fclose(f);

    // c_s8c - x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_s8c.txt", "r");
    short int c_s8c[6561 * EVAL_WIDTH];
    for(unsigned int i=0; i<l_s8c_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_s8c + i);
    fclose(f);

    // c_s8d - x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_s8d.txt", "r");
    short int c_s8d[6561 * EVAL_WIDTH];
    for(unsigned int i=0; i<l_s8d_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_s8d + i);
    fclose(f);

    // c_d8 - x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_d8.txt", "r");
    short int c_d8[6561 * EVAL_WIDTH];
    for(unsigned int i=0; i<l_d8_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_d8 + i);
    fclose(f);

    // c_d7 - x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_d7.txt", "r");
    short int c_d7[2187 * EVAL_WIDTH];
    for(unsigned int i=0; i<l_d7_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_d7 + i);
    fclose(f);

    // c_d6 - x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_d6.txt", "r");
    short int c_d6[729 * EVAL_WIDTH];
    for(unsigned int i=0; i<l_d6_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_d6 + i);
    fclose(f);

    // c_d5 - x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_d5.txt", "r");
    short int c_d5[243 * EVAL_WIDTH];
    for(unsigned int i=0; i<l_d5_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_d5 + i);
    fclose(f);

    // c_d4 - x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_d4.txt", "r");
    short int c_d4[81 * EVAL_WIDTH];
    for(unsigned int i=0; i<l_d4_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_d4 + i);
    fclose(f);

    // c_c8 - x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_c8.txt", "r");
    short int c_c8[sizeof(l_c8) / sizeof(l_c8[0]) * EVAL_WIDTH];
    for(unsigned int i=0; i<l_c8_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_c8 + i);
    fclose(f);

    // c_c9 - x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_c9.txt", "r");
    short int c_c9[sizeof(l_c9) / sizeof(l_c9[0]) * EVAL_WIDTH];
    for(unsigned int i=0; i<l_c9_s * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", c_c9 + i);
    fclose(f);

    // mob1 - 24 x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_mob1.txt", "r");
    for(unsigned int i=0; i<24 * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", &mob1_c[i]);
    fclose(f);

    // mob2 - 24 x EVAL_WIDTH short int values
    f = fopen("eval_v13//c_mob2.txt", "r");
    for(unsigned int i=0; i<24 * EVAL_WIDTH; ++i)
        fscanf(f, "%hi\n", &mob2_c[i]);
    fclose(f);

    // c_n - 49 x EVAL_WIDTH float values
    f = fopen("eval_v13//c_n.txt", "r");
    for(unsigned int i=0; i<49; ++i){
        unsigned int j;
        for(j=0; j<EVAL_WIDTH-1; ++j)
            fscanf(f, "%f ", &n_c[i][j]);
        fscanf(f, "%f\n", &n_c[i][j]);
    }
    fclose(f);

    // now load reduced s8abcd coeffs into full coeff array
    int k, j1, j2, j12;
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4)
    for(int i5=-1; i5<2; ++i5)
	for(int i6=-1; i6<2; ++i6)
	for(int i7=-1; i7<2; ++i7)
	for(int i8=-1; i8<2; ++i8){
        j1 = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81 + i6 * 81 * 3 + i7 * 81 * 9 + i8 * 81 * 27;
	    j2 = i8 + i7 * 3 + i6 * 9 + i5 * 27 + i4 * 81 + i3 * 81 * 3 + i2 * 81 * 9 + i1 * 81 * 27; // symmetry
	    j12 = std::min(j1, j2);

	    int* item = (int*)bsearch(&j12, l_s8a, l_s8a_s, sizeof(int), compare);
	    if (item != NULL){
            k = item - l_s8a; // index of found key
            for(unsigned int l=0; l<EVAL_WIDTH; ++l){// copy coeffs into both symmetries
                s8a_c[(3280 + j1) * EVAL_WIDTH + l] = c_s8a[k * EVAL_WIDTH + l];
                s8a_c[(3280 + j2) * EVAL_WIDTH + l] = c_s8a[k * EVAL_WIDTH + l];
            }
        }


        item = (int*)bsearch(&j12, l_s8b, l_s8b_s, sizeof(int), compare);
	    if (item != NULL){
            k = item - l_s8b; // index of found key
            for(unsigned int l=0; l<EVAL_WIDTH; ++l){// copy coeffs into both symmetries
                s8b_c[(3280 + j1) * EVAL_WIDTH + l] = c_s8b[k * EVAL_WIDTH + l];
                s8b_c[(3280 + j2) * EVAL_WIDTH + l] = c_s8b[k * EVAL_WIDTH + l];
            }
        }


        item = (int*)bsearch(&j12, l_s8c, l_s8c_s, sizeof(int), compare);
	    if (item != NULL){
            k = item - l_s8c; // index of found key
            for(unsigned int l=0; l<EVAL_WIDTH; ++l){// copy coeffs into both symmetries
                s8c_c[(3280 + j1) * EVAL_WIDTH + l] = c_s8c[k * EVAL_WIDTH + l];
                s8c_c[(3280 + j2) * EVAL_WIDTH + l] = c_s8c[k * EVAL_WIDTH + l];
            }
        }


        item = (int*)bsearch(&j12, l_s8d, l_s8d_s, sizeof(int), compare);
	    if (item != NULL){
            k = item - l_s8d; // index of found key
            for(unsigned int l=0; l<EVAL_WIDTH; ++l){// copy coeffs into both symmetries
                s8d_c[(3280 + j1) * EVAL_WIDTH + l] = c_s8d[k * EVAL_WIDTH + l];
                s8d_c[(3280 + j2) * EVAL_WIDTH + l] = c_s8d[k * EVAL_WIDTH + l];
            }
        }
    }

    // now load reduced d8 coeffs into full coeff array
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4)
    for(int i5=-1; i5<2; ++i5)
	for(int i6=-1; i6<2; ++i6)
	for(int i7=-1; i7<2; ++i7)
	for(int i8=-1; i8<2; ++i8){
        j1 = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81 + i6 * 81 * 3 + i7 * 81 * 9 + i8 * 81 * 27;
	    j2 = i8 + i7 * 3 + i6 * 9 + i5 * 27 + i4 * 81 + i3 * 81 * 3 + i2 * 81 * 9 + i1 * 81 * 27; // symmetry
	    j12 = std::min(j1, j2);

	    int* item = (int*)bsearch(&j12, l_d8, l_d8_s, sizeof(int), compare);
	    if (item == NULL) // not found - skip
            continue;
	    k = item - l_d8; // index of found key

        for(unsigned int l=0; l<EVAL_WIDTH; ++l){// copy coeffs into both symmetries
            d8_c[(3280 + j1) * EVAL_WIDTH + l] = c_d8[k * EVAL_WIDTH + l];
            d8_c[(3280 + j2) * EVAL_WIDTH + l] = c_d8[k * EVAL_WIDTH + l];
        }
    }

    // now load reduced d7 coeffs into full coeff array
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4)
    for(int i5=-1; i5<2; ++i5)
	for(int i6=-1; i6<2; ++i6)
	for(int i7=-1; i7<2; ++i7){
	    j1 = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81 + i6 * 81 * 3 + i7 * 81 * 9;
	    j2 = i7 + i6 * 3 + i5 * 9 + i4 * 27 + i3 * 81 + i2 * 81 * 3 + i1 * 81 * 9; // symmetry
	    j12 = std::min(j1, j2);

	    int* item = (int*)bsearch(&j12, l_d7, l_d7_s, sizeof(int), compare);
	    if (item == NULL) // not found - skip
            continue;
	    k = item - l_d7; // index of found key

        for(unsigned int l=0; l<EVAL_WIDTH; ++l){// copy coeffs into both symmetries
            d7_c[(1093 + j1) * EVAL_WIDTH + l] = c_d7[k * EVAL_WIDTH + l];
            d7_c[(1093 + j2) * EVAL_WIDTH + l] = c_d7[k * EVAL_WIDTH + l];
        }
    }

    // now load reduced d6 coeffs into full coeff array
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4)
    for(int i5=-1; i5<2; ++i5)
	for(int i6=-1; i6<2; ++i6){
	    j1 = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81 + i6 * 81 * 3;
	    j2 = i6 + i5 * 3 + i4 * 9 + i3 * 27 + i2 * 81 + i1 * 81 * 3; // symmetry
	    j12 = std::min(j1, j2);

	    int* item = (int*)bsearch(&j12, l_d6, l_d6_s, sizeof(int), compare);
	    if (item == NULL) // not found - skip
            continue;
	    k = item - l_d6; // index of found key

        for(unsigned int l=0; l<EVAL_WIDTH; ++l){// copy coeffs into both symmetries
            d6_c[(364 + j1) * EVAL_WIDTH + l] = c_d6[k * EVAL_WIDTH + l];
            d6_c[(364 + j2) * EVAL_WIDTH + l] = c_d6[k * EVAL_WIDTH + l];
        }
    }

    // now load reduced d5 coeffs into full coeff array
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4)
    for(int i5=-1; i5<2; ++i5){
	    j1 = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81;
	    j2 = i5 + i4 * 3 + i3 * 9 + i2 * 27 + i1 * 81; // symmetry
	    j12 = std::min(j1, j2);

	    int* item = (int*)bsearch(&j12, l_d5, l_d5_s, sizeof(int), compare);
	    if (item == NULL) // not found - skip
            continue;
	    k = item - l_d5; // index of found key

        for(unsigned int l=0; l<EVAL_WIDTH; ++l){// copy coeffs into both symmetries
            d5_c[(121 + j1) * EVAL_WIDTH + l] = c_d5[k * EVAL_WIDTH + l];
            d5_c[(121 + j2) * EVAL_WIDTH + l] = c_d5[k * EVAL_WIDTH + l];
        }
    }

    // now load reduced d4 coeffs into full coeff array
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4){
	    j1 = i1 + i2 * 3 + i3 * 9 + i4 * 27;
	    j2 = i4 + i3 * 3 + i2 * 9 + i1 * 27; // symmetry
	    j12 = std::min(j1, j2);

	    int* item = (int*)bsearch(&j12, l_d4, l_d4_s, sizeof(int), compare);
	    if (item == NULL) // not found - skip
            continue;
	    k = item - l_d4; // index of found key

        for(unsigned int l=0; l<EVAL_WIDTH; ++l){// copy coeffs into both symmetries
            d4_c[(40 + j1) * EVAL_WIDTH + l] = c_d4[k * EVAL_WIDTH + l];
            d4_c[(40 + j2) * EVAL_WIDTH + l] = c_d4[k * EVAL_WIDTH + l];
        }
    }

    // now load reduced c8 coeffs into full coeff array
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4)
    for(int i5=-1; i5<2; ++i5)
	for(int i6=-1; i6<2; ++i6)
	for(int i7=-1; i7<2; ++i7)
	for(int i8=-1; i8<2; ++i8)
	for(int i9=-1; i9<2; ++i9)
	for(int i10=-1; i10<2; ++i10){
	    j12 = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81 + i6 * 81 * 3 + i7 * 81 * 9 + i8 * 81 * 27 + i9 * 81 * 81 + i10 * 81 * 81 * 3; // no symms here!

	    int* item = (int*)bsearch(&j12, l_c8, l_c8_s, sizeof(int), compare);
	    if (item == NULL) // not found - skip
            continue;
	    k = item - l_c8; // index of found key

        for(unsigned int l=0; l<EVAL_WIDTH; ++l)// copy coeffs - no symms here!
            c8_c[(29524 + j12) * EVAL_WIDTH + l] = c_c8[k * EVAL_WIDTH + l];
    }

    // now load reduced c9 coeffs into full coeff array
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4)
    for(int i5=-1; i5<2; ++i5)
	for(int i6=-1; i6<2; ++i6)
	for(int i7=-1; i7<2; ++i7)
	for(int i8=-1; i8<2; ++i8)
	for(int i9=-1; i9<2; ++i9){
	    j1 = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81 + i6 * 81 * 3 + i7 * 81 * 9 + i8 * 81 * 27 + i9 * 81 * 81;
	    j2 = i1 + i4 * 3 + i7 * 9 + i2 * 27 + i5 * 81 + i8 * 81 * 3 + i3 * 81 * 9 + i6 * 81 * 27 + i9 * 81 * 81; // symmetry
	    j12 = std::min(j1, j2);

	    int* item = (int*)bsearch(&j12, l_c9, l_c9_s, sizeof(int), compare);
	    if (item == NULL) // not found - skip
            continue;
	    k = item - l_c9; // index of found key

        for(unsigned int l=0; l<EVAL_WIDTH; ++l){// copy coeffs into both symmetries
            c9_c[(9841 + j1) * EVAL_WIDTH + l] = c_c9[k * EVAL_WIDTH + l];
            c9_c[(9841 + j2) * EVAL_WIDTH + l] = c_c9[k * EVAL_WIDTH + l];
        }
    }
}

static void init_sigmas(){// load sigma coeffs
    // l_s8a - 3005 integer values
    unsigned int l_s8a_s = 0;
    FILE *f = fopen("sigma_v3//l_s8a.txt", "r");
    int l_s8a[6561];
    while(1){
        if( fscanf(f, "%d\n", l_s8a + l_s8a_s) < 1 )
            break;
        l_s8a_s++;
    }
    assert(l_s8a_s < sizeof(l_s8a) / sizeof(l_s8a[0])); // make sure there is no overflow on read
    fclose(f);

    // l_s8b - 3005 integer values
    unsigned int l_s8b_s = 0;
    f = fopen("sigma_v3//l_s8b.txt", "r");
    int l_s8b[6561];
    while(1){
        if( fscanf(f, "%d\n", l_s8b + l_s8b_s) < 1 )
            break;
        l_s8b_s++;
    }
    assert(l_s8b_s < sizeof(l_s8b) / sizeof(l_s8b[0])); // make sure there is no overflow on read
    fclose(f);

    // l_s8c - 3005 integer values
    unsigned int l_s8c_s = 0;
    f = fopen("sigma_v3//l_s8c.txt", "r");
    int l_s8c[6561];
    while(1){
        if( fscanf(f, "%d\n", l_s8c + l_s8c_s) < 1 )
            break;
        l_s8c_s++;
    }
    assert(l_s8c_s < sizeof(l_s8c) / sizeof(l_s8c[0])); // make sure there is no overflow on read
    fclose(f);

    // l_s8d - 3005 integer values
    unsigned int l_s8d_s = 0;
    f = fopen("sigma_v3//l_s8d.txt", "r");
    int l_s8d[6561];
    while(1){
        if( fscanf(f, "%d\n", l_s8d + l_s8d_s) < 1 )
            break;
        l_s8d_s++;
    }
    assert(l_s8d_s < sizeof(l_s8d) / sizeof(l_s8d[0])); // make sure there is no overflow on read
    fclose(f);

    // l_c9 - 5154 integer values
    unsigned int l_c9_s = 0;
    f = fopen("sigma_v3//l_c9.txt", "r");
    int l_c9[5000];
     while(1){
        if( fscanf(f, "%d\n", l_c9 + l_c9_s) < 1 )
            break;
        l_c9_s++;
    }
    assert(l_c9_s < sizeof(l_c9) / sizeof(l_c9[0])); // make sure there is no overflow on read
    fclose(f);

    // l_c10 - integer values
    unsigned int l_c10_s = 0;
    f = fopen("sigma_v3//l_c10.txt", "r");
    int l_c10[33000];
     while(1){
        if( fscanf(f, "%d\n", l_c10 + l_c10_s) < 1 )
            break;
        l_c10_s++;
    }
    assert(l_c10_s < sizeof(l_c10) / sizeof(l_c10[0])); // make sure there is no overflow on read
    fclose(f);

    // c_s8a - x EVAL_WIDTH short int values ******************************************************************
    f = fopen("sigma_v3//c_s8a.txt", "r");
    eval_type c_s8a[6561 * 16];
    for(unsigned int i=0; i<l_s8a_s * 16; ++i)
        fscanf(f, "%hi\n", c_s8a + i);
    fclose(f);

    // c_s8b - x EVAL_WIDTH short int values
    f = fopen("sigma_v3//c_s8b.txt", "r");
    eval_type c_s8b[6561 * 16];
    for(unsigned int i=0; i<l_s8b_s * 16; ++i)
        fscanf(f, "%hi\n", c_s8b + i);
    fclose(f);

    // c_s8c - x EVAL_WIDTH short int values
    f = fopen("sigma_v3//c_s8c.txt", "r");
    eval_type c_s8c[6561 * 16];
    for(unsigned int i=0; i<l_s8c_s * 16; ++i)
        fscanf(f, "%hi\n", c_s8c + i);
    fclose(f);

    // c_s8d - x EVAL_WIDTH short int values
    f = fopen("sigma_v3//c_s8d.txt", "r");
    eval_type c_s8d[6561 * 16];
    for(unsigned int i=0; i<l_s8d_s * 16; ++i)
        fscanf(f, "%hi\n", c_s8d + i);
    fclose(f);

    // c_c9 - x EVAL_WIDTH short int values
    f = fopen("sigma_v3//c_c9.txt", "r");
    eval_type c_c9[sizeof(l_c9) / sizeof(l_c9[0]) * 16];
    for(unsigned int i=0; i<l_c9_s * 16; ++i)
        fscanf(f, "%hi\n", c_c9 + i);
    fclose(f);

    // c_c10 - x EVAL_WIDTH short int values
    f = fopen("sigma_v3//c_c10.txt", "r");
    eval_type c_c10[sizeof(l_c10) / sizeof(l_c10[0]) * 16];
    for(unsigned int i=0; i<l_c10_s * 16; ++i)
        fscanf(f, "%hi\n", c_c10 + i);
    fclose(f);

    // mob1 - 22 x EVAL_WIDTH short int values
    f = fopen("sigma_v3//c_mob1.txt", "r");
    for(unsigned int i=0; i<22 * 16; ++i)
        fscanf(f, "%hi\n", &mob1_c2[0][i]);
    fclose(f);

    // mob2 - 22 x EVAL_WIDTH short int values
    f = fopen("sigma_v3//c_mob2.txt", "r");
    for(unsigned int i=0; i<22 * 16; ++i)
        fscanf(f, "%hi\n", &mob2_c2[0][i]);
    fclose(f);

    // c_n - 41 x EVAL_WIDTH short int values
    f = fopen("sigma_v3//c_n.txt", "r");
    for(unsigned int i=0; i<41 * 16; ++i)
        fscanf(f, "%hi\n", &n_c2[0][i]);
    fclose(f);

    // d_c2 - 16 x 18 float values
    f = fopen("sigma_v3//c_dd.txt", "r");
    for(unsigned int i=0; i<16; ++i){
        unsigned int j;
        for(j=0; j<18-1; ++j)
            fscanf(f, "%f ", &d_c2[j][i]);
        fscanf(f, "%f\n", &d_c2[j][i]);
    }
    fclose(f);

    // now load reduced s8abcd coeffs into full coeff array
    int k, j1, j2, j12;
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4)
    for(int i5=-1; i5<2; ++i5)
	for(int i6=-1; i6<2; ++i6)
	for(int i7=-1; i7<2; ++i7)
	for(int i8=-1; i8<2; ++i8){
        j1 = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81 + i6 * 81 * 3 + i7 * 81 * 9 + i8 * 81 * 27;
	    j2 = i8 + i7 * 3 + i6 * 9 + i5 * 27 + i4 * 81 + i3 * 81 * 3 + i2 * 81 * 9 + i1 * 81 * 27; // symmetry
	    j12 = std::min(j1, j2);

	    int* item = (int*)bsearch(&j12, l_s8a, l_s8a_s, sizeof(int), compare);
	    if (item != NULL){
            k = item - l_s8a; // index of found key
            for(unsigned int l=0; l<16; ++l){// copy coeffs into both symmetries
                s8a_c2[0][(3280 + j1) * 16 + l] = c_s8a[k * 16 + l];
                s8a_c2[0][(3280 + j2) * 16 + l] = c_s8a[k * 16 + l];
            }
        }


        item = (int*)bsearch(&j12, l_s8b, l_s8b_s, sizeof(int), compare);
	    if (item != NULL){
            k = item - l_s8b; // index of found key
            for(unsigned int l=0; l<16; ++l){// copy coeffs into both symmetries
                s8b_c2[0][(3280 + j1) * 16 + l] = c_s8b[k * 16 + l];
                s8b_c2[0][(3280 + j2) * 16 + l] = c_s8b[k * 16 + l];
            }
        }


        item = (int*)bsearch(&j12, l_s8c, l_s8c_s, sizeof(int), compare);
	    if (item != NULL){
            k = item - l_s8c; // index of found key
            for(unsigned int l=0; l<16; ++l){// copy coeffs into both symmetries
                s8c_c2[0][(3280 + j1) * 16 + l] = c_s8c[k * 16 + l];
                s8c_c2[0][(3280 + j2) * 16 + l] = c_s8c[k * 16 + l];
            }
        }


        item = (int*)bsearch(&j12, l_s8d, l_s8d_s, sizeof(int), compare);
	    if (item != NULL){
            k = item - l_s8d; // index of found key
            for(unsigned int l=0; l<16; ++l){// copy coeffs into both symmetries
                s8d_c2[0][(3280 + j1) * 16 + l] = c_s8d[k * 16 + l];
                s8d_c2[0][(3280 + j2) * 16 + l] = c_s8d[k * 16 + l];
            }
        }
    }

    // now load reduced c9 coeffs into full coeff array
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4)
    for(int i5=-1; i5<2; ++i5)
	for(int i6=-1; i6<2; ++i6)
	for(int i7=-1; i7<2; ++i7)
	for(int i8=-1; i8<2; ++i8)
	for(int i9=-1; i9<2; ++i9){
	    j1 = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81 + i6 * 81 * 3 + i7 * 81 * 9 + i8 * 81 * 27 + i9 * 81 * 81;
	    j2 = i1 + i4 * 3 + i7 * 9 + i2 * 27 + i5 * 81 + i8 * 81 * 3 + i3 * 81 * 9 + i6 * 81 * 27 + i9 * 81 * 81; // symmetry
	    j12 = std::min(j1, j2);

	    int* item = (int*)bsearch(&j12, l_c9, l_c9_s, sizeof(int), compare);
	    if (item == NULL) // not found - skip
            continue;
	    k = item - l_c9; // index of found key

        for(unsigned int l=0; l<16; ++l){// copy coeffs into both symmetries
            c9_c2[0][(9841 + j1) * 16 + l] = c_c9[k * 16 + l];
            c9_c2[0][(9841 + j2) * 16 + l] = c_c9[k * 16 + l];
        }
    }


    // now load reduced c10 coeffs into full coeff array
    for(int i1=-1; i1<2; ++i1)
    for(int i2=-1; i2<2; ++i2)
    for(int i3=-1; i3<2; ++i3)
    for(int i4=-1; i4<2; ++i4)
    for(int i5=-1; i5<2; ++i5)
	for(int i6=-1; i6<2; ++i6)
	for(int i7=-1; i7<2; ++i7)
	for(int i8=-1; i8<2; ++i8)
	for(int i9=-1; i9<2; ++i9)
	for(int i10=-1; i10<2; ++i10){
        j12 = i1 + i2 * 3 + i3 * 9 + i4 * 27 + i5 * 81 + i6 * 81 * 3 + i7 * 81 * 9 + i8 * 81 * 27 + i9 * 81 * 81 + i10 * 81 * 81 * 3; // no symms here!

	    int* item = (int*)bsearch(&j12, l_c10, l_c10_s, sizeof(int), compare);
	    if (item == NULL) // not found - skip
            continue;
	    k = item - l_c10; // index of found key

        for(unsigned int l=0; l<16; ++l)// copy coeffs - no symms here!
            c10_c2[0][(29524 + j12) * 16 + l] = c_c10[k * 16 + l];
    }
}
