#include "rev.h"
#include "hash.h"

// modified from http://www.burtleburtle.net/bob/c/lookup8.c
// suggested usage:a=data1+level, b=data2+level, c=data3+gold, c contains the result. level - the previous hash, or an arbitrary value, gold -the golden ratio; an arbitrary value
static inline UINT64 key_hash(board *bo){
	UINT64 a=0x9e3779b97f4a7c13, b = bo->pos[0], c = bo->pos[1];
	a -= b; a -= c; a ^= (c>>43); //1
	b -= c; b -= a; b ^= (a<<9);  //2
	c -= a; c -= b; c ^= (b>>8);  //3
	a -= b; a -= c; a ^= (c>>38); //4
	b -= c; b -= a; b ^= (a<<23); //5
	c -= a; c -= b; c ^= (b>>5);  //6
	a -= b; a -= c; a ^= (c>>35); //7
	b -= c; b -= a; b ^= (a<<49); //8
	c -= a; c -= b; c ^= (b>>11); //9
	a -= b; a -= c; a ^= (c>>12); //10
	b -= c; b -= a; b ^= (a<<18); //11
	c -= a; c -= b; c ^= (b>>22); //12
	return(c); // return full hash
}

static hashtype __attribute__ ((noinline)) atomic_read(hashtype *h1){return(*h1);}

static void __attribute__ ((noinline)) atomic_write(hashtype *h1, hashtype h_read){*h1 = h_read;}

unsigned int lookup_hash(const unsigned int endg, hash_data *hd, board *b, int depth){// Returns indicator only.
    UINT64 index_hl = key_hash(b);
	hashtype *h1 = &h[(index_hl % HASH_SIZE) & 0xfffffffffffffffe];
	hashtype h_read;
	unsigned int lock = index_hl>>32;

    hd->index_hash = index_hl; // save lock and key
    h_read = atomic_read(h1);  // atomic read
    if(h_read.lock != lock){
        h1 ++;
        h_read = atomic_read(h1); // atomic read
        if(h_read.lock != lock)
            return(0);
    }

    if(h_read.age != TTage){         // update age only if needed!
        h_read.age = TTage;          // mark "current"
        atomic_write(h1, h_read);    // atomic write, to update age
    }

    hd->move_hash = h_read.move;// get move
    hd->be_hash = MAX_SCORE;    // init scores
    hd->alp_hash = MIN_SCORE;   // init scores

    if( (h_read.endg >= endg) && (endg || ( (depth <= h_read.depth) && (MPCmult <= h_read.MPCmult) )) ){ // use bounds for search of the same or higher depth. endg always has the same depth.
        if(h_read.type == 0) // exact score
            hd->be_hash = hd->alp_hash = h_read.score; // assign both bounds
        else if(h1->type == 1) // lower bound
            hd->alp_hash = h_read.score; // assign lower bound
        else // upper bound
            hd->be_hash = h_read.score; // assign upper bound
    }

    return(1);
}

#define replace \
	h_read.score = score;\
	if(score < be){\
		if(score > alp)\
			h_read.type = 0;\
		else\
			h_read.type = 2;\
	}else\
		h_read.type = 1;\
	h_read.move = move;\
	h_read.endg = endg;\
	h_read.depth = depth;\
	h_read.MPCmult = MPCmult;\

void add_hash(int alp, int be, int score, const unsigned int move, const unsigned int endg, const UINT64 index_hl, board *b, int depth){
    hashtype *h1 = &h[(index_hl % HASH_SIZE) & 0xfffffffffffffffe];
    hashtype h_read, h_read1, h_read2;
    unsigned int lock = index_hl>>32;

    // check first one
    h_read1 = h_read = atomic_read(h1); // atomic read
    if(h_read.lock == lock){
		h_read.age = TTage; // mark "current"
        if(depth >= h_read.depth){ // replace old values
            replace
        } // else, current depth is larger - retain the values.
        atomic_write(h1, h_read); // atomic write
        return;
	}

	// check second one
	h1++;
    h_read2 = h_read = atomic_read(h1); // atomic read
    if(h_read.lock == lock){
		h_read.age = TTage; // mark "current"
        if(depth >= h_read.depth){ // replace old values
            replace
        } // else, current depth is larger - retain the values.
        atomic_write(h1, h_read); // atomic write
        return;
	}

	h1--; // return to the first entry
    h_read = h_read1;
    if( TTage != h_read.age											// stale
        || depth >= h_read.depth					                    // or new depth is greater than or equal to old depth
        || MPCmult > h_read.MPCmult					                // or new MPC is higher
        ){
        h_read.lock = lock;				                        // store lock
        h_read.age = TTage;                                     // mark "current"
        replace			                                        // replace score. This also sets move, etc

        atomic_write(h1, h_read); // atomic write
        return;
    }

    h1++; // the second entry
    h_read = h_read2;
    if( TTage != h_read.age											// stale
        || depth >= h_read.depth					                    // or new depth is greater than or equal to old depth
        || MPCmult > h_read.MPCmult					                // or new MPC is higher
        ){
        h_read.lock = lock;				                        // store lock
        h_read.age = TTage;                                     // mark "current"
        replace			                                        // replace score. This also sets move, etc

        atomic_write(h1, h_read); // atomic write
        return;
    }

	return;
}

void clear_hash(void){
    // set hash to 0
    memset(h, 0, (HASH_SIZE + 10) * sizeof(hashtype));

    // also clear history count
    memset(b_m.history_count, 0, 2 * 64 * sizeof(int));
}
