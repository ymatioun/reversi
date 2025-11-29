#include "rev.h"
#include "hash.h"

#define MIN_SORT_DEPTH1 6		// min depth for endgame sorting. use 6.
#define MIN_SORT_DEPTH2 0		// min depth for midgame sorting. use 0.
#define MIN_SORT_COUNT1 2		// endgame: sort only if more than X moves. use 2. Use the same for midgame.
#define MIN_HASH_DEPTH1	9		// min hash depth - endgame. use 9. For midgame, use 0 = always use hash
#define DEPTH_BREAK	9			// depth where time limit is checked.

int true_score(const UINT64 p0, const UINT64 p1){// called from endgame. Return exact score
	int r = 0, s1 = (int)_popcnt64(p0), s2 = (int)_popcnt64(p1);
	if(s1 > s2)
		r = 64 - 2 * s2;
	else if(s1 < s2)
		r = 2 * s1 - 64;
	return(r);
}

#define save_e p0[0] = b->pos[0]; p0[1] = b->pos[1]; // save current position
#define restore_e b->pos[0] = p0[0]; b->pos[1] = p0[1]; // restore position after a move

static void make_move_e(const unsigned int i, board *b){ // make a move; do not change 'player'
    // make a move for player 0. Assume move is legal.
    UINT64 mask = flip(i, b->pos[0], b->pos[1]); // this excludes the piece being placed
    assert(mask!=0); // confirm that move is legal
    mask = mask | (UINT64(1)<<i);
	b->pos[0] |= mask;
	b->pos[1] &= (~mask);

	// flip position to indicate change of players.
	mask = b->pos[0]; b->pos[0] = b->pos[1]; b->pos[1] = mask;
}

void __attribute__ ((always_inline)) make_move(const unsigned int i, board *b){ // make a move
    // make a move for player 0. Assume move is legal.
    UINT64 mask = flip(i, b->pos[0], b->pos[1]); // this excludes the piece being placed
    assert(mask!=0); // confirm that move is legal
    mask = mask | (UINT64(1)<<i);
	b->pos[0] |= mask;
	b->pos[1] &= (~mask);

	// flip position to indicate change of players.
	b->player = 1 - b->player;
	mask = b->pos[0]; b->pos[0] = b->pos[1]; b->pos[1] = mask;
}

static void sort_e(const unsigned int mc, unsigned int* move_list, board *b){// sort, for endgame
	UINT64 p0[2], mask;
	unsigned int i, j, k;
	int mob_l, mobility[32];

	// calculate mobility for each possible move
	for(j=0; j<mc; ++j){
        i = move_list[j];

        mask = flip(i, b->pos[0], b->pos[1]);
        mask = mask | (UINT64(1)<<i);
        p0[0] = b->pos[0] | mask; p0[1] = b->pos[1] & (~mask); // put position in p0 to avoid save/restore

        mob_l = (int)_popcnt64(bit_mob_mask(p0[1], p0[0]));

		// what to do for passes?
		if(mob_l == 0)
            mob_l = 0; // this seems better than using opp mob

        k = j;
        while(k && mobility[k-1] > mob_l){
            mobility[k] = mobility[k-1];
            move_list[k] = move_list[k-1];
            --k;
        }
        move_list[k] = i;
        mobility[k] = mob_l;
	}
}

static void __attribute__ ((always_inline)) sort_m(const unsigned int mc, unsigned int* move_list, board *b){// sort, for endgame
	unsigned int i, j, k;
	int mob_l, mobility[32];

	// calculate value for each possible move
	for(j=0; j<mc; ++j){
        i = move_list[j];
        mob_l = b->history_count[b->player][i];
        static const int ha[64] = {
            1000000,-100000,0,0,0,0,-100000,1000000,
            -100000,-100000,0,0,0,0,-100000,-100000,
            0,0,0,0,0,0,0,0,        0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,        0,0,0,0,0,0,0,0,
            -100000,-100000,0,0,0,0,-100000,-100000,
            1000000,-100000,0,0,0,0,-100000,1000000};
        mob_l += ha[i]; // corner = great, all 3 next to corner = not good

        k = j;
        while(k && mobility[k-1] < mob_l){
            mobility[k] = mobility[k-1];
            move_list[k] = move_list[k-1];
            --k;
        }
        move_list[k] = i;
        mobility[k] = mob_l;
	}
}

static unsigned int __attribute__ ((always_inline)) move_list_f(unsigned int* l, UINT64 mask){ // return count of moves. List is placed in 'l'.
    unsigned int i=0;

    assert(mask!=0); // confirm that there are moves - this function is only called with non-empty mask
    do{
        l[i++] = __builtin_ctzl(mask);
        mask = _blsr_u64(mask);
    }while(mask);

    return(i);
}

static const uint64_t NEIGHBOUR[] = {
	0x0000000000000302ULL, 0x0000000000000604ULL, 0x0000000000000e0aULL, 0x0000000000001c14ULL,
	0x0000000000003828ULL, 0x0000000000007050ULL, 0x0000000000006020ULL, 0x000000000000c040ULL,
	0x0000000000030200ULL, 0x0000000000060400ULL, 0x00000000000e0a00ULL, 0x00000000001c1400ULL,
	0x0000000000382800ULL, 0x0000000000705000ULL, 0x0000000000602000ULL, 0x0000000000c04000ULL,
	0x0000000003020300ULL, 0x0000000006040600ULL, 0x000000000e0a0e00ULL, 0x000000001c141c00ULL,
	0x0000000038283800ULL, 0x0000000070507000ULL, 0x0000000060206000ULL, 0x00000000c040c000ULL,
	0x0000000302030000ULL, 0x0000000604060000ULL, 0x0000000e0a0e0000ULL, 0x0000001c141c0000ULL,
	0x0000003828380000ULL, 0x0000007050700000ULL, 0x0000006020600000ULL, 0x000000c040c00000ULL,
	0x0000030203000000ULL, 0x0000060406000000ULL, 0x00000e0a0e000000ULL, 0x00001c141c000000ULL,
	0x0000382838000000ULL, 0x0000705070000000ULL, 0x0000602060000000ULL, 0x0000c040c0000000ULL,
	0x0003020300000000ULL, 0x0006040600000000ULL, 0x000e0a0e00000000ULL, 0x001c141c00000000ULL,
	0x0038283800000000ULL, 0x0070507000000000ULL, 0x0060206000000000ULL, 0x00c040c000000000ULL,
	0x0002030000000000ULL, 0x0004060000000000ULL, 0x000a0e0000000000ULL, 0x00141c0000000000ULL,
	0x0028380000000000ULL, 0x0050700000000000ULL, 0x0020600000000000ULL, 0x0040c00000000000ULL,
	0x0203000000000000ULL, 0x0406000000000000ULL, 0x0a0e000000000000ULL, 0x141c000000000000ULL,
	0x2838000000000000ULL, 0x5070000000000000ULL, 0x2060000000000000ULL, 0x40c0000000000000ULL
};

static int Esearch_shallow(const int depth, int alp, int be, board *b){// this function needs to handle passes and endgames!!!
    UINT64 p0[2], mask, mask2;
    int st, score;
    unsigned int m, i;

	// last 0/1/2 plys
	if(depth == 0)// no moves left
        return(true_score(b->pos[0], b->pos[1]));
    else if(depth == 1){// 1 (possible) move left
        i = __builtin_ctzl(~(b->pos[0] | b->pos[1])); // the one remaining empty square
        if((NEIGHBOUR[i] & b->pos[1]) && (mask = flip(i, b->pos[0], b->pos[1]))){
            b->n_c++; // count this node
            return(2 * (int)_popcnt64(b->pos[0] | mask) - 62);
        }else if((NEIGHBOUR[i] & b->pos[0]) && (mask = flip(i, b->pos[1], b->pos[0]))){ // pass
            b->n_c++; // count this node
            return(62 - 2 * (int)_popcnt64(b->pos[1] | mask));
        }else
            return(true_score(b->pos[0], b->pos[1]));
    }else if(depth == 2){// 2 moves left; improvement: do not do 'bit mob' before this, try each move instead - to do ...
        // here i need the 2 empty cells, not 2 possible moves (there can be only 1 move)
        mask = ~(b->pos[0] | b->pos[1]);
        int m1 = __builtin_ctzl(mask); // first move
        int m2 = 63 - __builtin_clzl(mask); // second move

        // make the first move (m1)
        if((NEIGHBOUR[m1] & b->pos[1]) && (mask = flip(m1, b->pos[0], b->pos[1]))){
            b->m = m1;
            b->n_c++; // count this node
            mask = mask | (UINT64(1)<<m1);
            p0[0] = b->pos[0] | mask; p0[1] = b->pos[1] & (~mask); // put position in p0 to avoid save/restore
            if((NEIGHBOUR[m2] & p0[0]) && (mask2 = flip(m2, p0[1], p0[0]))){ // make the last move - normal
                b->n_c++; // count this node
                st = 62 - 2 * (int)_popcnt64(p0[1] | mask2);
            }else if((NEIGHBOUR[m2] & p0[1]) &&  (mask2 = flip(m2, p0[0], p0[1]))){ // make the last move - pass
                b->n_c++; // count this node
                st = 2 * (int)_popcnt64(p0[0] | mask2) - 62;
            }else
                st = true_score(p0[0], p0[1]); // pass
            // beta cut-off?
            if(st >= be)
                return(st);
        }else
            st = -126; // first move is impossible

        // make the second move (m2)
        int st2;
        if((NEIGHBOUR[m2] & b->pos[1]) && (mask = flip(m2, b->pos[0], b->pos[1]))){
            b->n_c++; // count this node
            mask = mask | (UINT64(1)<<m2);
            p0[0] = b->pos[0] | mask; p0[1] = b->pos[1] & (~mask); // put position in p0 to avoid save/restore
            if((NEIGHBOUR[m1] & p0[0]) &&  (mask2 = flip(m1, p0[1], p0[0]))){ // make the last move - normal
                b->n_c++; // count this node
                st2 = 62 - 2 * (int)_popcnt64(p0[1] | mask2);
            }else if((NEIGHBOUR[m1] & p0[1]) &&  (mask2 = flip(m1, p0[0], p0[1]))){ // make the last move - pass
                b->n_c++; // count this node
                st2 = 2 * (int)_popcnt64(p0[0] | mask2) - 62;
            }else
                st2 = true_score(p0[0], p0[1]); // pass
            // best score
            if(st2 > st){
                b->m = m2;
                st = st2;
            }
        }

        if(st == -126){// no moves- pass. Here exchange the players in all places
            // make the first move (m1)
            if((NEIGHBOUR[m1] & b->pos[0]) && (mask = flip(m1, b->pos[1], b->pos[0]))){
                b->m = m1;
                b->n_c++; // count this node
                mask = mask | (UINT64(1)<<m1);
                p0[1] = b->pos[1] | mask; p0[0] = b->pos[0] & (~mask); // put position in p0 to avoid save/restore
                if((NEIGHBOUR[m2] & p0[1]) &&   (mask2 = flip(m2, p0[0], p0[1]))){ // make the last move - normal
                    b->n_c++; // count this node
                    st = 2 * (int)_popcnt64(p0[0] | mask2) - 62; // flip sign
                }else if((NEIGHBOUR[m2] & p0[0]) && (mask2 = flip(m2, p0[1], p0[0]))){ // make the last move - pass
                    b->n_c++; // count this node
                    st = 62 - 2 * (int)_popcnt64(p0[1] | mask2); // flip sign
                }else
                    st = true_score(p0[0], p0[1]); // pass
                // beta cut-off?
                if(st <= alp) // Since here the player is oposite, min instead of max!!!
                    return(st);
            }else
                st = 126; // first move is impossible

            // make the second move (m2)
            if((NEIGHBOUR[m2] & b->pos[0]) && (mask = flip(m2, b->pos[1], b->pos[0]))){
                b->n_c++; // count this node
                mask = mask | (UINT64(1)<<m2);
                p0[1] = b->pos[1] | mask; p0[0] = b->pos[0] & (~mask); // put position in p0 to avoid save/restore
                if((NEIGHBOUR[m1] & p0[1]) &&  (mask2 = flip(m1, p0[0], p0[1]))){ // make the last move - normal
                    b->n_c++; // count this node
                    st2 = 2 * (int)_popcnt64(p0[0] | mask2) - 62; // flip sign
                }else if((NEIGHBOUR[m1] & p0[0]) &&  (mask2 = flip(m1, p0[1], p0[0]))){ // make the last move - pass
                    b->n_c++; // count this node
                    st2 = 62 - 2 * (int)_popcnt64(p0[1] | mask2); // flip sign
                }else
                    st2 = true_score(p0[0], p0[1]); // pass
                // best score. Since here the player is oposite, min instead of max!!!
                if(st2 < st){
                    b->m = m2;
                    st = st2;
                }
            }

            if(st == 126)// no moves for either player - endgame
                return(true_score(b->pos[0], b->pos[1]));
        }
        return(st);
    } // end of "2 moves left"


    // get bitmask of empties
	mask = ~(b->pos[0] | b->pos[1]);

	// loop over moves
	save_e // save current position
	score = MIN_SCORE; m = 64;
	while(mask){
		i = __builtin_ctzl(mask);
		mask = _blsr_u64(mask);

		if((NEIGHBOUR[i] & b->pos[1]) && (mask2 = flip(i, b->pos[0], b->pos[1]))){ // legal move, proceed
            mask2 = mask2 | (UINT64(1)<<i); b->pos[0] |= mask2; b->pos[1] &= (~mask2);
            b->n_c++; // count this node
            // flip position to indicate change of players.
            mask2 = b->pos[0]; b->pos[0] = b->pos[1]; b->pos[1] = mask2;
            st = -Esearch_shallow(depth-1, -be, -alp, b);
            restore_e // restore
            if(st > score){
                if(st >= be){
                    b->m = i; // best move
                    return(st);
                }
                score = st; m = i; alp=std::max(alp, st);
            }
        }
	}

	if(score == MIN_SCORE){ // pass - flip sides
        // get bitmask of empties
        mask = ~(b->pos[0] | b->pos[1]);

        // loop over moves
        score = MAX_SCORE; m = 64;
        while(mask){
            i = __builtin_ctzl(mask);
            mask = _blsr_u64(mask);

            if((NEIGHBOUR[i] & b->pos[0]) && (mask2 = flip(i, b->pos[1], b->pos[0]))){ // legal move, proceed. Here players are switched!
                mask2 = mask2 | (UINT64(1)<<i); b->pos[1] |= mask2; b->pos[0] &= (~mask2); // Here players are switched!
                b->n_c++; // count this node
                st = Esearch_shallow(depth-1, alp, be, b);
                restore_e // restore
                if(st < score){ // opp player, so min not max
                    if(st <= alp){
                        b->m = i; // best move
                        return(st);
                    }
                    score = st; m = i; be=std::min(be, st);
                }
            }
        }

        if(score == MAX_SCORE){ // endgame
            b->m = 64; // best move
            return(true_score(b->pos[0], b->pos[1]));
        }
    }

	b->m = m; // best move
	return(score);
}

static int Esearch(const int depth, int alp, int be, board *b, UINT64 move_mask){
    UINT64 p0[2], mask;
    int st, score, alp0;
    unsigned int m, mc, move_list[32], i, j;

	// last 0/1/2 plys
	if(depth <= 0)// no moves left
        return(true_score(b->pos[0], b->pos[1]));
    else if(depth == 1){// 1 move left
        i = __builtin_ctzl(move_mask); // the one remaining empty square
        mask = flip(i, b->pos[0], b->pos[1]); // this excludes the piece being placed
        assert(mask);// i check for pass before calling the child, so here there should always be moves.
        b->n_c++; // count this node
        st = (int)_popcnt64(b->pos[0] | mask);
        return(2 * st - 62);
    }else if(depth == 2){// 2 moves left; improvement: do not do 'bit mob' before this, try each move instead - to do ...
        UINT64 mask2;
        // here i need the 2 empty cells, not 2 possible moves (there can be only 1 move)
        mask = ~(b->pos[0] | b->pos[1]);
        int m1 = __builtin_ctzl(mask); // first move
        int m2 = 63 - __builtin_clzl(mask); // second move

        // make the first move (m1)
        if((move_mask>>m1) & 1){
            mask = flip(m1, b->pos[0], b->pos[1]);
            b->m = m1;
            b->n_c++; // count this node
            mask = mask | (UINT64(1)<<m1);
            p0[0] = b->pos[0] | mask; p0[1] = b->pos[1] & (~mask); // put position in p0 to avoid save/restore
            if((mask2 = flip(m2, p0[1], p0[0]))){ // make the last move - normal
                b->n_c++; // count this node
                st = 62 - 2 * (int)_popcnt64(p0[1] | mask2);
            }else if((mask2 = flip(m2, p0[0], p0[1]))){ // make the last move - pass
                b->n_c++; // count this node
                st = 2 * (int)_popcnt64(p0[0] | mask2) - 62;
            }else
                st = true_score(p0[0] , p0[1]); // pass
            // beta cut-off?
            if(st >= be)
                return(st);
        }else
            st = -126; // first move is impossible

        // make the second move (m2)
        int st2;
        if((move_mask>>m2) & 1){
            mask = flip(m2, b->pos[0], b->pos[1]);
            b->n_c++; // count this node
            mask = mask | (UINT64(1)<<m2);
            p0[0] = b->pos[0] | mask; p0[1] = b->pos[1] & (~mask); // put position in p0 to avoid save/restore
            if((mask2 = flip(m1, p0[1], p0[0]))){ // make the last move - normal
                b->n_c++; // count this node
                st2 = 62 - 2 * (int)_popcnt64(p0[1] | mask2);
            }else if((mask2 = flip(m1, p0[0], p0[1]))){ // make the last move - pass
                b->n_c++; // count this node
                st2 = 2 * (int)_popcnt64(p0[0] | mask2) - 62;
            }else
                st2 = true_score(p0[0] , p0[1]); // pass
            // best score
            if(st2 > st){
                b->m = m2;
                st = st2;
            }
        }

        assert(st>-126); // there are always moves - confirm that.
        return(st);
    } // end of "2 moves left"
    else if(depth <= 6){// few moves left; use shallow search, without 'bit mob' ***********
        mc = move_list_f(move_list, move_mask);
        save_e // save current position
        score = MIN_SCORE; m = 64;
        for(j = 0; j < mc; ++j){
            i = move_list[j];
            make_move_e(i, b); // make a move.
            st = -Esearch_shallow(depth-1, -be, -alp, b); // this function needs to handle passes and endgames!!!
            restore_e // restore
            if(break_ind){
                b->m = m; // best move
                return(score);
            }
            if(st > score){
                if(st >= be){
                    b->m = i; // best move
                    b->n_c += i + 1; // count all moves
                    return(st);
                }
                score = st; m = i; alp=std::max(alp, st);
            }
        }
        b->m = m; // best move
        b->n_c += mc; // count all moves
        return(score);
    }


    // check for timeout
    if(depth > DEPTH_BREAK + 2 && b->master == 0 && timeGetTime() > t_break){// emergency break ***************************
		break_ind++; b->m = 64;
		return(MIN_SCORE);
	}


    // hash logic
    hash_data hd;
	if(depth >= MIN_HASH_DEPTH1 && lookup_hash(1, &hd, b, depth)){// lookup hash table.
        b->m = hd.move_hash;
		if(be > hd.be_hash){
			be = hd.be_hash;
			if(be <= alp)
				return(be);
		}
		if(alp < hd.alp_hash){
			alp = hd.alp_hash;
			if(alp >= be)
				return(alp);
		}
	}else
        b->m = 64;
    alp0 = alp; // use alp after TT – better bound for when score does not improve


    // get move list
	assert(move_mask); // i check for pass before calling the child, so here there should always be moves.
	mc = move_list_f(move_list, move_mask);

	if(depth > MIN_SORT_DEPTH1 && mc > MIN_SORT_COUNT1) // sort ALL(3) nodes less. But they have lower average number of moves.
		sort_e(mc, move_list, b);  // sort the moves

	if(b->m < 64 && ((UINT64(1)<<b->m) & move_mask)){ // legal TT move - make it first!
        for(i=0; i<mc; ++i)
            if(move_list[i] == b->m){
                move_list[i] = move_list[0];
                move_list[0] = b->m;
                break;
            }
	}


	// loop over moves
	save_e // save current position
	score = MIN_SCORE; m = 64;
	for(j=0; j<mc; ++j){
		i = move_list[j];
		make_move_e(i, b); // make a move.
		b->n_c++; // count this node
		mask = bit_mob_mask(b->pos[0], b->pos[1]);
		if(mask) // normal - there are moves available
            st = -Esearch(depth-1, -be, -alp, b, mask);
		else if((mask = bit_mob_mask(b->pos[1], b->pos[0]))){// pass. make a move for opponent.
            UINT64 mt = b->pos[0]; b->pos[0] = b->pos[1]; b->pos[1] = mt; // flip position to indicate change of players.
            st = Esearch(depth-1, alp, be, b, mask);
		}else// the end. True score
			st = true_score(b->pos[1], b->pos[0]); // reverse the players here!
		restore_e // restore
		if(break_ind){
            b->m = m; // best move
            return(score);
        }
		if(st > score){
			if(st >= be){
                if(depth >= MIN_HASH_DEPTH1)
                    add_hash(alp0, be, st, i, 1, hd.index_hash, depth); // add hash table entry.
                b->m = i; // best move
                return(st);
            }
            score = st; m = i; alp=std::max(alp, st);
        }
	}
	if(depth >= MIN_HASH_DEPTH1)
        add_hash(alp0, be, score, m, 1, hd.index_hash, depth); // add hash table entry.
    b->m = m; // best move
	return(score);
}

// from http://www.tckerrigan.com/Chess/Parallel_Search/Simplified_ABDADA/simplified_abdada.html *********************************************************
#define DEFER_DEPTH    5        // do not defer for d < this
#define CS_SIZE        32768    // CS = "currently_searching"
#define CS_WAYS        4        // N-way

static volatile UINT64 currently_searching[CS_SIZE][CS_WAYS];

static bool __attribute__ ((always_inline)) defer_move(UINT64 move_hash){
	unsigned int n, i;

	n = move_hash & (CS_SIZE - 1);
	for(i = 0; i < CS_WAYS; i++)
		if(currently_searching[n][i] == move_hash)
			return true;
	return false;
}

static void __attribute__ ((always_inline)) starting_search(UINT64 move_hash){
	unsigned int n, i;

	n = move_hash & (CS_SIZE - 1);
	for(i = 0; i < CS_WAYS; i++){
		if(currently_searching[n][i] == 0){
			currently_searching[n][i] = move_hash;
			return;
        }
		if(currently_searching[n][i] == move_hash)
			return;
    }
	currently_searching[n][0] = move_hash;
}

static void __attribute__ ((always_inline)) finished_search(UINT64 move_hash){
	unsigned int n, i;

	n = move_hash & (CS_SIZE - 1);
	for(i = 0; i < CS_WAYS; i++)
		if(currently_searching[n][i] == move_hash)
			currently_searching[n][i] = 0;
}

#define save p0[0] = b->pos[0]; p0[1] = b->pos[1]; player = b->player; // save current position/player
#define restore b->pos[0] = p0[0]; b->pos[1] = p0[1]; b->player = player; // restore position and player after a move

typedef struct{// eval hash entry
    unsigned int lock;
    int score;
} ehash; // 8 bytes

static ehash __attribute__ ((noinline)) atomic_read(ehash *h1){return(*h1);}

static void __attribute__ ((noinline)) atomic_write(ehash *h1, ehash h_read){*h1 = h_read;}

#include <sys/mman.h>
static int __attribute__ ((always_inline)) Msearch0(board *b, const UINT64 move_mask){ // check eval hash and call eval if needed
    #define EHASH_SIZE (256 * 1024) // 256 K entries, 2 Mb. Larger size does not help, but makes lookup slower.
    static __thread ehash* eh; // make this per thread, to greatly reduce memory contention.

    // allocate eval hash on first use
    if(eh == 0){
        UINT64 sz = EHASH_SIZE * sizeof(ehash);
        eh = (ehash*)aligned_alloc(2 * 1024 * 1024, sz); // large memory pages (2 Mb)
        madvise((void*)eh, sz, MADV_HUGEPAGE);
    }

    UINT64 ih = key_hash(b);
    ehash *h1 = &eh[(ih % EHASH_SIZE)];
    ehash h_read = atomic_read(h1); // atomic read
    unsigned int lock = ih>>32;
    if(h_read.lock == lock)
        return(h_read.score);

    int st = eval(b, move_mask); // calc

    h_read.lock = lock;
    h_read.score = st;
    atomic_write(h1, h_read); // atomic write
    return(st);
}

#define print_top 0 // print all scores at top search
extern int depth0;
int Msearch(const int depth, int alp, int be, board *b, const UINT64 move_mask, const unsigned int node_type){ // main search (midgame)
    UINT64 p0[2], mask;
    hash_data hd;
    int st, score, sorted, player, alp0;
    unsigned int i, j, m, mc, deferred_moves, move_list[32], deferred_move[32];
    static const unsigned int tto[] = {0, 3, 3, 2};	// node type of other children: PV->CUT, ALL->CUT, CUT->ALL
    static const unsigned int tt1[] = {0, 1, 3, 2};	// node type of first child: PV->PV, ALL->CUT, CUT->ALL

    // last ply - call eval
	if(depth <= 0)// use eval hash. It does not change the node count, but reduces runtime
        return(Msearch0(b, move_mask));

    // check for timeout
    if(depth > DEPTH_BREAK && b->master == 0 && timeGetTime() > t_break){// emergency break ***************************
		break_ind++; b->m = 64;
		return(MIN_SCORE);
	}

    // new search will not go past 60 pieces
    if((i = _popcnt64(b->pos[0] | b->pos[1])) > (MPCmult>=10?50:60)) // PV extension - not better
        return(Esearch(64 - i, alp, be, b, move_mask));


    // TT logic: before MPC = +5 ELO
    if(lookup_hash(0, &hd, b, depth)){// lookup hash table.
        b->m = hd.move_hash;
		if(be > hd.be_hash){ // here cuts on PV are OK.
			be = hd.be_hash;
			if(be <= alp)
				return(be);
		}
		if(alp < hd.alp_hash && node_type > 1){// no cuts on PV = +30 ELO. Part of EFP logic
			alp = hd.alp_hash;
			if(alp >= be)
                return(alp);
		}
	}else
        b->m = hd.move_hash = 64; // init to invalid. Here changing b->m has an impact - do it, to avoid using uninitialized value.


    // MPC cut ************************************************************************ +400 ELO - huge, as expected
	if(depth >= 2 && node_type > 1 && MPCmult < 10){// && depth0 > 1 + depth){// no cuts on PV. Compound MPC: +20 ELO
        int d1 = cut_depth[depth];
		if(d1 >= 0){
			int s = sigmas[depth - 2][_popcnt64(b->pos[0] | b->pos[1]) - 4]; // this is std(s1, s2) * 20
			if(s){
                float sm1 = std::min(std::max(sigma_mult(b, move_mask, depth), 0.15f), 5.0f);
                s = (sm1 * s * MPCmult + 20) / 40;// round to nearest
                st = Msearch(d1, alp - s, be + s, b, move_mask, node_type); // same node_type. Here 0-window search is way slower than full window; why???
                if(st >= be + s) // big cut
                    return(be);
				if(st <= alp - s) // small cut
                    return(alp);
                if(hd.move_hash == 64 || d1 >= hd.depth_hash)
                    hd.move_hash = b->m; // use best move from MPC as TT move - only if better.
			}
		}
	}// end of MPC cut *****************************************************************

    // get move list
	assert(move_mask); // i check for pass before calling the child, so here there should always be moves.
	mc = move_list_f(move_list, move_mask);

    sorted = 99; // these many [all] first moves are in correct order; set to 99.
	if(hd.move_hash < 64 && ((UINT64(1) << hd.move_hash) & move_mask)){ // legal TT move - make it first!
        for(i=0; i<mc; ++i)
            if(move_list[i] == hd.move_hash){
                move_list[i] = move_list[0];
                move_list[0] = hd.move_hash;
                sorted = 1; // these many [1] first moves are in correct order
                break;
            }
	}else if(depth > MIN_SORT_DEPTH2 && mc > MIN_SORT_COUNT1) // only sort if no TT move
		sort_m(mc, move_list, b);  // sort the moves

    save // save current position
    score = MIN_SCORE; m = 64; alp0 = alp; // use alp after TT – better bound for when score does not improve
    for(deferred_moves = j = 0; j < mc; ++j){// loop over moves - part 1
        if(j == sorted && depth > MIN_SORT_DEPTH2 && mc > MIN_SORT_COUNT1 + j) // only sort after both moves(if they are present)
            sort_m(mc - j, move_list + j, b);  // sort the remaining moves, after TT move has been played and caused no cut-off
		i = move_list[j];
		make_move(i, b); // make a move. This flips players
		mask = bit_mob_mask(b->pos[0], b->pos[1]);
		if(mask){ // normal - there are moves available
            if(j == 0){// first move - use full window and type opp of P/A/C. Using full window for all node types is +17 ELO
                st = -Msearch(depth-1, -be, -alp, b, mask, tt1[node_type]);
                #if print_top
                if(depth == depth0){
                    char sss[200];
                    sprintf(sss,"   top search first move %d score %d alp %d be %d nc %u\n", i, st, alp, be, b->n_c);
                    pass_message_to_GUI(sss);
                }
                #endif
            }else{
                if(depth < DEFER_DEPTH) // shallow depth - skip 'defer' logic
                    st = -Msearch(depth-1, -alp-1, -alp, b, mask, tto[node_type]);
                else{ // moves 2+: only search if not already in progress
                    // ABDADA logic **************************************************************************
                    UINT64 move_hash = hd.index_hash; move_hash ^= (i * 1664525) + 1013904223; // random number generator
                    if(defer_move(move_hash)){
                        deferred_move[deferred_moves++] = i;
                        restore // restore
                        continue;
                    }
                    starting_search(move_hash);
                    st = -Msearch(depth-1, -alp-1, -alp, b, mask, tto[node_type]);
                    finished_search(move_hash);
                    #if print_top
                    if(depth == depth0){
                        char sss[200];
                        sprintf(sss,"   top search second_zero move %d score %d alp %d be %d nc %u\n", i, st, alp, alp + 1, b->n_c);
                        pass_message_to_GUI(sss);
                    }
                    #endif
                }
                if(st > alp && st < be && !break_ind){ // better move - call as PV with full window. Only happens on PV nodes. Because all other nodes always have be=alp+1. Addition 2
                    if(st == alp + 1)
                        st = alp; // per Enhanced Forward Pruning.
                    st = -Msearch(depth-1, -be, -st, b, mask, 1); // type = PV
                    #if print_top
                    if(depth == depth0){
                        char sss[200];
                        sprintf(sss,"   top search second_research move %d score %d alp* %d be %d nc %u\n", i, st, alp, be, b->n_c);
                        pass_message_to_GUI(sss);
                    }
                    #endif
                }
            }
		}else if((mask = bit_mob_mask(b->pos[1], b->pos[0]))){// pass. make a move for opponent.
            // flip position to indicate change of players.
            UINT64 mt = b->pos[0]; b->pos[0] = b->pos[1]; b->pos[1] = mt;
            b->player = 1 - b->player;
            if(j == 0)// first move - use full window and type opp of P/A/C.
                st = Msearch(depth-1, alp, be, b, mask, node_type);
            else{
                st = Msearch(depth-1, alp, alp+1, b, mask, node_type);
                if(st > alp && st < be && !break_ind){ // better move - call as PV with full window. Only happens on PV nodes. Because all other nodes always have be=alp+1. Addition 2
                    if(st == alp + 1)
                        st = alp; // per Enhanced Forward Pruning.
                    st = Msearch(depth-1, st, be, b, mask, 1); // type = PV
                }
            }
		}else// the end. True score
			st = true_score(b->pos[1], b->pos[0]); // reverse the players here!
		restore // restore
		if(break_ind){
            b->m = m; // best move
            return(score);
        }

        if(st > score){
			if(st >= be){
                add_hash(alp0, be, st, i, 0, hd.index_hash, depth); // add hash table entry.
                b->history_count[player][i] += depth; // update history. Varying hist by player is +10 ELO
                for(unsigned int k=0; k<j; ++k) // reduce history for prior moves that did not lead to cut.
                    b->history_count[player][move_list[k]] -= depth; // update history.
                b->m = i; // best move
                b->n_c += j + 1 - deferred_moves; // count all nodes
                return(st);
            }
            score = st; m = i; alp=std::max(alp, st);
        }
	}
	b->n_c += mc - deferred_moves; // count all nodes, excl deferred ones

	for(j = 0; j < deferred_moves; ++j){// loop over moves - part 2 (ABDADA)
        i = deferred_move[j];
		make_move(i, b); // make a move. This flips players
		mask = bit_mob_mask(b->pos[0], b->pos[1]);
		if(mask){ // normal - there are moves available
            st = -Msearch(depth-1, -alp-1, -alp, b, mask, tto[node_type]);
            if(st > alp && st < be && !break_ind){ // better move - call as PV with full window. Only happens on PV nodes. Because all other nodes always have be=alp+1. Addition 2
                if(st == alp + 1)
                    st = alp; // per Enhanced Forward Pruning.
                st = -Msearch(depth-1, -be, -st, b, mask, 1); // type = PV
            }
		}else if((mask = bit_mob_mask(b->pos[1], b->pos[0]))){// pass. make a move for opponent.
            // flip position to indicate change of players.
            UINT64 mt = b->pos[0]; b->pos[0] = b->pos[1];
            b->pos[1] = mt; b->player = 1 - b->player;
            st = Msearch(depth-1, alp, alp+1, b, mask, node_type);
            if(st > alp && st < be && !break_ind){ // better move - call as PV with full window. Only happens on PV nodes. Because all other nodes always have be=alp+1. Addition 2
                if(st == alp + 1)
                    st = alp; // per Enhanced Forward Pruning.
                st = Msearch(depth-1, st, be, b, mask, 1); // type = PV
            }
		}else// the end. True score
			st = true_score(b->pos[1], b->pos[0]); // reverse the players here!
		restore // restore
		if(break_ind){
            b->m = m; // best move
            return(score);
        }

		if(st > score){
			if(st >= be){
                add_hash(alp0, be, st, i, 0, hd.index_hash, depth); // add hash table entry.
                b->history_count[player][i] += depth; // update history. Varying hist by player is +10 ELO
                for(unsigned int k=0; k<j; ++k) // reduce history for prior moves that did not lead to cut.
                    b->history_count[player][move_list[k]] -= depth; // update history.
                b->m = i; // best move
                b->n_c += j + 1; // count all nodes
                return(st);
            }
            score = st; m = i; alp=std::max(alp, st);
        }
	}


	add_hash(alp0, be, score, m, 0, hd.index_hash, depth); // add hash table entry.
    b->m = m; // best move
    b->n_c += deferred_moves; // count all nodes
	return(score);
}
