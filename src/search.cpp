#include "rev.h"
#include "hash.h"

#define MIN_SORT_DEPTH1 6		// min depth for endgame fastest-first sorting. use 6.
#define MIN_SORT_DEPTH2 0		// min depth for midgame fastest-first sorting. use 1.
#define MIN_SORT_COUNT1 2		// endgame: sort fast_estfirst only if more than X moves. use 2. Use the same for midgame.
#define MIN_HASH_DEPTH1	9		// min hash depth - endgame. use 9. For midgame, use 0 = always use hash
#define DEPTH_BREAK	9			// depth where time limit is checked.
#define ETC_DEPTH2 8			// depth for midgame ETC.

int true_score(UINT64 p0, UINT64 p1){// called from endgame. Return exact score
	int r = 0, s1 = (int)_popcnt64(p0), s2 = (int)_popcnt64(p1);
	if(s1 > s2)
		r = 64 - 2 * s2;
	else if(s1 < s2)
		r = 2 * s1 - 64;
	return(r);
}

#define save p0[0] = b->pos[0]; p0[1] = b->pos[1];  player = b->player; // save current position/player
#define restore b->pos[0] = p0[0]; b->pos[1] = p0[1]; b->player = player; // restore position and player after a move

static void /*__attribute__ ((noinline))*/ sort_fastest_first_e(unsigned int mc, unsigned int* move_list, board *b){// sort fastest first, for endgame
	UINT64 p0[2];
	unsigned int i, j, k;
	int mob_l, mobility[32], player;

	save // save current position

	// calculate mobility for each possible move
	for(j=0; j<mc; ++j){
        i = move_list[j];
		make_move(i, b); // make a move. This flips players
		mob_l = (int)_popcnt64(bit_mob_mask(b->pos[0], b->pos[1]));

		// what to do for passes?
		if(mob_l == 0)
            mob_l = (int)_popcnt64(bit_mob_mask(b->pos[1], b->pos[0])); // use mob for the one who has moves

		restore // restore

        k = j;
        while(k && mobility[k-1] > mob_l){
            mobility[k] = mobility[k - 1];
            move_list[k] = move_list[k - 1];
            --k;
        }
        move_list[k] = i;
        mobility[k] = mob_l;
	}
}

static void /*__attribute__ ((noinline))*/ sort_fastest_first_m(unsigned int mc, unsigned int* move_list, board *b, unsigned int depth){// sort fastest first, for endgame
	unsigned int i, j, k;
	int mob_l, mobility[32];

	// calculate value for each possible move
	for(j=0; j<mc; ++j){
        i = move_list[j];
        mob_l = -b->history_count[b->player][i];
        static const int ha[64] = {100000000,0,0,0,0,0,0,100000000,
            0,-1000000,0,0,0,0,-1000000,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,-1000000,0,0,0,0,-1000000,0,
            100000000,0,0,0,0,0,0,100000000};
        mob_l -= ha[i]; // corner = great, X = not good

        k = j;
        while(k && mobility[k-1] > mob_l){
            mobility[k] = mobility[k - 1];
            move_list[k] = move_list[k - 1];
            --k;
        }
        move_list[k] = i;
        mobility[k] = mob_l;
	}
}

void make_move(unsigned int i, board *b){ // make a move
    UINT64 mt, one = 1;

    // make a move for player 0. Assume move is legal.
    UINT64 mask = flip(i, b->pos[0], b->pos[1]); // this excludes the piece being placed
    assert(mask!=0); // confirm that move is legal
    mask = mask | (one<<i);
	b->pos[0] |= mask;
	b->pos[1] &= (~mask);

	// flip position to indicate change of players.
	b->player = 1 - b->player; mt = b->pos[0];
	b->pos[0] = b->pos[1]; b->pos[1] = mt;
}

unsigned int move_list_f(unsigned int* l, UINT64 mask){ // return count of moves. List is placed in 'l'.
    unsigned int i=0;

    while(mask){
        l[i++] = __builtin_ctzl(mask);
        mask = mask&(mask-1);
    }

    return(i);
}

static int Esearch(const int depth, int alp, int be, board *b, UINT64 move_mask){
    UINT64 p0[2], mask, one = 1;
    int st, score = MIN_SCORE, alp0, player;
    unsigned int m = 64, mc, move_list[32], i, j;


	// check for timeout
    if( depth > DEPTH_BREAK && b->master == 0 && timeGetTime() > t_break ){// emergency break ***************************
		break_ind++;
		return(MIN_SCORE);
	}


	// last 0/1/2 plys
	if(depth == 0)// no moves left
        return(true_score(b->pos[0], b->pos[1]));
    else if(depth == 1){// 1 move left
        i = __builtin_ctzl(move_mask); // the one remaining empty square
        mask = flip(i, b->pos[0], b->pos[1]); // this excludes the piece being placed
        assert(mask);// i check for pass before calling the child, so here there should always be moves.
        b->n_c++; // count this node
        st = (int)_popcnt64(b->pos[0] | mask);
        return(2 * st - 62);
    }else if(depth == 2){// 2 moves left
        UINT64 mask2;
        // here i need the 2 empty cells, not 2 possible moves (there can be only 1 move)
        mask = ~(b->pos[0] | b->pos[1]);
        int m1 = __builtin_ctzl(mask); // first move
        int m2 = 63 - __builtin_clzl(mask); // second move

        // make the first move (m1)
        if( (mask = flip(m1, b->pos[0], b->pos[1])) ){
            b->m = m1;
            b->n_c++; // count this node
            mask = mask | (one<<m1);
            p0[0] = b->pos[0] | mask; p0[1] = b->pos[1] & (~mask); // put position in p0 to avoid save/restore
            if( (mask2 = flip(m2, p0[1], p0[0])) ){ // make the last move - normal
                b->n_c++; // count this node
                st = 62 - 2 * (int)_popcnt64(p0[1] | mask2);
            }else if( (mask2 = flip(m2, p0[0], p0[1])) ){ // make the last move - pass
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
        if( (mask = flip(m2, b->pos[0], b->pos[1])) ){
            b->n_c++; // count this node
            mask = mask | (one<<m2);
            p0[0] = b->pos[0] | mask; p0[1] = b->pos[1] & (~mask); // put position in p0 to avoid save/restore
            if( (mask2 = flip(m1, p0[1], p0[0])) ){ // make the last move - normal
                b->n_c++; // count this node
                st2 = 62 - 2 * (int)_popcnt64(p0[1] | mask2);
            }else if( (mask2 = flip(m1, p0[0], p0[1])) ){ // make the last move - pass
                b->n_c++; // count this node
                st2 = 2 * (int)_popcnt64(p0[0] | mask2) - 62;
            }else
                st2 = true_score(p0[0] , p0[1]); // pass
            // best score
            if( st2 > st){
                b->m = m2;
                st = st2;
            }
        }

        assert(st>-126); // there are always moves - confirm that.
        return(st);
    } // end of "2 moves left"


    // hash logic
    alp0 = alp;
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


    save // save current position


    // get move list
	assert(move_mask); // i check for pass before calling the child, so here there should always be moves.
	mc = move_list_f(move_list, move_mask);

	if(depth > MIN_SORT_DEPTH1 && mc > MIN_SORT_COUNT1) // sort ALL(3) nodes less. But they have lower average number of moves.
		sort_fastest_first_e(mc, move_list, b);  // sort the moves

	if(b->m < 64 && ((one<<b->m)&move_mask)){ // legal TT move - make it first!
        for(i=0; i<mc; ++i)
            if(move_list[i] == b->m){
                move_list[i] = move_list[0];
                move_list[0] = b->m;
                break;
            }
	}


	// loop over moves
	for(j=0; j<mc; ++j){
		i = move_list[j];
		make_move(i, b); // make a move. This flips players
		b->n_c++; // count this node
		mask = bit_mob_mask(b->pos[0], b->pos[1]);
		if(mask){ // normal - there are moves available
            st = -Esearch(depth-1, -be, -alp, b, mask);
            if( break_ind ){
                b->m = m; // best move
                restore // restore
                return(score);
            }
		}else if((mask = bit_mob_mask(b->pos[1], b->pos[0]))){// pass. make a move for opponent.
            // flip position to indicate change of players.
            UINT64 mt = b->pos[0]; b->pos[0] = b->pos[1];
            b->pos[1] = mt; b->player = 1 - b->player;
            st = Esearch(depth-1, alp, be, b, mask);
			if( break_ind ){
                b->m = m; // best move
                restore // restore
                return(score);
            }
		}else// the end. True score
			st = true_score(b->pos[1], b->pos[0]); // reverse the players here!
		restore // restore

		if(st > score){
			score = st; m = i; alp=std::max(alp, st);
			if(score >= be){
                b->m = m; // best move
                if(!break_ind && depth >= MIN_HASH_DEPTH1)
                    add_hash(alp0, be, score, m, 1, hd.index_hash, b,depth); // add hash table entry. Use unmodified alp.
                return(score);
            }
        }
	}
	b->m = m; // best move
	if(!break_ind && depth >= MIN_HASH_DEPTH1)
        add_hash(alp0, be, score, m, 1, hd.index_hash, b, depth); // add hash table entry. Use unmodified alp.
	return(score);
}

// from http://www.tckerrigan.com/Chess/Parallel_Search/Simplified_ABDADA/simplified_abdada.html *********************************************************
#define DEFER_DEPTH    3        // do not defer for d < this
#define CS_SIZE        32768    // CS = "currently_searching"
#define CS_WAYS        4        // 4-way

static volatile UINT64 currently_searching[CS_SIZE][CS_WAYS];

// return true if a move is being searched
static bool defer_move(UINT64 move_hash, int depth){
	int n, i;

	if (depth < DEFER_DEPTH)
		return false;

	n = move_hash & (CS_SIZE - 1);
	for (i = 0; i < CS_WAYS; i++){
		if (currently_searching[n][i] == move_hash)
			return true;

		}
	return false;
}

static void starting_search(UINT64 move_hash, int depth){
	int n, i;

	if (depth < DEFER_DEPTH)
		return;

	n = move_hash & (CS_SIZE - 1);
	for (i = 0; i < CS_WAYS; i++){
		if (currently_searching[n][i] == 0){
			currently_searching[n][i] = move_hash;
			return;
        }
		if (currently_searching[n][i] == move_hash)
			return;
    }
	currently_searching[n][0] = move_hash;
}

static void finished_search(UINT64 move_hash, int depth){
	int n, i;

	if (depth < DEFER_DEPTH)
		return;

	n = move_hash & (CS_SIZE - 1);
	for (i = 0; i < CS_WAYS; i++){
		if (currently_searching[n][i] == move_hash)
			currently_searching[n][i] = 0;
    }
}

int Msearch(int depth, int alp, int be, board *b, UINT64 move_mask, unsigned int node_type){ // main search (midgame)
    UINT64 p0[2], mask, one = 1;
    int st, score = MIN_SCORE, alp0, sorted, player;
    unsigned int i, j, m = 64, mc, move_list[32], deferred_move[32], deferred_moves;
    static const unsigned int tto[] = {0, 3, 3, 2};	// node type of other children: PV->CUT, ALL->CUT, CUT->ALL

    // last ply - call eval
	if(depth == 0)
        return(eval(b, move_mask));

    // check for timeout
    if( depth > DEPTH_BREAK && b->master == 0 && timeGetTime() > t_break ){// emergency break ***************************
		break_ind++;
		return(score);
	}

    // switch to endgame, if: search is to > 60 pieces, and depth < X
    if(depth < MIN_ENDCUT_DEPTH && _popcnt64(b->pos[0] | b->pos[1]) > 60)
        return(Esearch(64 - st, alp, be, b, move_mask));

    // MPC cut ************************************************************************ +400 ELO - huge, as expected
	if(MPCmult < 10 && node_type > 1){// no cuts on PV. Compound MPC: +20 ELO
		int d1 = cut_depth[depth];
		if(d1 >= 0){
			int s = sigmas[depth - 2][_popcnt64(b->pos[0] | b->pos[1]) - 4]; // this is std(s1, s2) * 20
			if(s){
                float sm1 = sigma_mult(b, move_mask, depth);
                sm1 = std::max(sm1, 0.15f);
                sm1 = std::min(sm1, 5.0f);
                s = (sm1 * s * MPCmult + 20) / 40;// round to nearest
                b->cut_d = depth - d1;
				st = Msearch(d1, alp - s, be + s, b, move_mask, node_type); // same node_type
				b->cut_d = 0;
				if(st >= be + s) // big cut
                    return(be);
				if(st <= alp - s) // small cut
                    return(alp);
			}
		}
	}// end of MPC cut *****************************************************************

    // hash logic. Why is this after MPC? +36 ELO
    alp0 = alp;
    hash_data hd;
	if(lookup_hash(0, &hd, b, depth)){// lookup hash table.
        b->m = hd.move_hash;
		if(be > hd.be_hash && node_type > 1){// no cuts on PV Why?
			be = hd.be_hash;
			if(be <= alp)
				return(be);
		}
		if(alp < hd.alp_hash && node_type > 1){// no cuts on PV. Why?
			alp = hd.alp_hash;
			if(alp >= be)
				return(alp);
		}
	}else
        b->m = hd.move_hash = 64; // init to invalid

    save // save current position

    // get move list
	assert(move_mask); // i check for pass before calling the child, so here there should always be moves.
	mc = move_list_f(move_list, move_mask);

	// enhanced transposition cutoff
	if(depth > ETC_DEPTH2 && node_type > 1){// no cuts on PV
	    for(j=0; j<mc; ++j){
            make_move(move_list[j], b); // make a move. This flips players
            hash_data hd2;
			if(lookup_hash(0, &hd2, b, depth - 1) && -hd2.be_hash > be){// lookup hash table.
                restore // restore
                return(-hd2.be_hash);
			}
			restore // restore
        }
	}

    sorted = 0;
	if(hd.move_hash < 64 && ((one << hd.move_hash) & move_mask)){ // legal TT move - make it first!
        for(i=0; i<mc; ++i)
            if(move_list[i] == hd.move_hash){
                move_list[i] = move_list[0];
                move_list[0] = hd.move_hash;
                break;
            }
	}else if(depth > MIN_SORT_DEPTH2 && mc > MIN_SORT_COUNT1){ // only sort if no TT move
		sort_fastest_first_m(mc, move_list, b, depth);  // sort the moves
		sorted = 1;
    }

	for(deferred_moves=0, j=0; j<mc; ++j){// loop over moves - part 1
        if(j == 1 && sorted == 0 && depth > MIN_SORT_DEPTH2 && mc - 1 > MIN_SORT_COUNT1)
            sort_fastest_first_m(mc - 1, move_list + 1, b, depth);  // sort the remaining moves, after TT move has been played and caused no cut-off
		i = move_list[j];
		make_move(i, b); // make a move. This flips players
		b->n_c++; // count this node
		mask = bit_mob_mask(b->pos[0], b->pos[1]);
		if(mask){ // normal - there are moves available
            if( node_type == 1 && j == 0 )// first move on PV - use full window and type PV. Here type = 1 is important, so that only children of PV are PV.
                st = -Msearch(depth-1, -be, -alp, b, mask, 1);
            else{

                // ABDADA logic ******************************************
                if(j == 0) // first move - always search
                    st = -Msearch(depth-1, -alp-1, -alp, b, mask, tto[node_type]);
                else{ // moves 2+: only search if not already in progress
                    UINT64 move_hash = hd.index_hash; move_hash ^= (i * 1664525) + 1013904223; // random number generator
                    if(defer_move(move_hash, depth)){
                        deferred_move[deferred_moves++] = i;
                        restore // restore
                        continue;
                    }
                    starting_search(move_hash, depth);
                    st = -Msearch(depth-1, -alp-1, -alp, b, mask, tto[node_type]);
                    finished_search(move_hash, depth);
                }

                if( st > alp && st < be && !break_ind ){ // better move - call as PV with full window. Only happens on PV nodes. Because all other nodes always have be=alp+1.
                    if( st == alp + 1 )
                        st = alp; // per Enhanced Forward Pruning.
                    st = -Msearch(depth-1, -be, -st, b, mask, 1); // type = PV
                }
            }
            if( break_ind ){
                b->m = m; // best move
                restore // restore
                return(score);
            }
		}else if((mask = bit_mob_mask(b->pos[1], b->pos[0]))){// pass. make a move for opponent.
            // flip position to indicate change of players.
            UINT64 mt = b->pos[0]; b->pos[0] = b->pos[1];
            b->pos[1] = mt; b->player = 1 - b->player;
            if( node_type == 1 && j == 0 )// first move on PV - use full window and type PV. Here type=1 is important, so that only children of PV are PV.
                st = Msearch(depth-1, alp, be, b, mask, 1);
            else{
                st = Msearch(depth-1, alp, alp+1, b, mask, tto[node_type]);
                if( st > alp && st < be && !break_ind ){ // better move - call as PV with full window. Only happens on PV nodes. Because all other nodes always have be=alp+1.
                    if( st == alp + 1 )
                        st = alp; // per Enhanced Forward Pruning.
                    st = Msearch(depth-1, st, be, b, mask, 1); // type = PV
                    }
            }
			if( break_ind ){
                b->m = m; // best move
                restore // restore
                return(score);
            }
		}else// the end. True score
			st = true_score(b->pos[1], b->pos[0]); // reverse the players here!
		restore // restore

		if(st > score){
			score = st; m = i; alp=std::max(alp, st);
			if(score >= be){
                b->history_count[player][i] += depth; // update history.
                // reduce history for prior moves that did not lead to cut.
                for(unsigned int k=0; k<j; ++k)
                    b->history_count[player][move_list[k]] -= depth; // update history.
                b->m = m; // best move
                if(!break_ind)
                    add_hash(alp0, be, score, m, 0, hd.index_hash, b,depth); // add hash table entry. Use unmodified alp.
                return(score);
            }
        }
	}

	for(j=0; j<deferred_moves; ++j){// loop over moves - part 2 (ABDADA)
        i = deferred_move[j];
		make_move(i, b); // make a move. This flips players
		b->n_c++; // count this node
		mask = bit_mob_mask(b->pos[0], b->pos[1]);
		if(mask){ // normal - there are moves available
            st = -Msearch(depth-1, -alp-1, -alp, b, mask, tto[node_type]);
            if( st > alp && st < be && !break_ind ){ // better move - call as PV with full window. Only happens on PV nodes. Because all other nodes always have be=alp+1.
                if( st == alp + 1 )
                    st = alp; // per Enhanced Forward Pruning.
                st = -Msearch(depth-1, -be, -st, b, mask, 1); // type = PV
            }
            if( break_ind ){
                b->m = m; // best move
                restore // restore
                return(score);
            }
		}else if((mask = bit_mob_mask(b->pos[1], b->pos[0]))){// pass. make a move for opponent.
            // flip position to indicate change of players.
            UINT64 mt = b->pos[0]; b->pos[0] = b->pos[1];
            b->pos[1] = mt; b->player = 1 - b->player;
            st = Msearch(depth-1, alp, alp+1, b, mask, tto[node_type]);
            if( st > alp && st < be && !break_ind ){ // better move - call as PV with full window. Only happens on PV nodes. Because all other nodes always have be=alp+1.
                if( st == alp + 1 )
                    st = alp; // per Enhanced Forward Pruning.
                st = Msearch(depth-1, st, be, b, mask, 1); // type = PV
                }
			if( break_ind ){
                b->m = m; // best move
                restore // restore
                return(score);
            }
		}else// the end. True score
			st = true_score(b->pos[1], b->pos[0]); // reverse the players here!
		restore // restore

		if(st > score){
			score = st; m = i; alp=std::max(alp, st);
			if(score >= be){
                b->history_count[player][i] += depth; // update history.
                // reduce history for prior moves that did not lead to cut.
                for(unsigned int k=0; k<j; ++k)
                    b->history_count[player][move_list[k]] -= depth; // update history.
                b->m = m; // best move
                if(!break_ind)
                    add_hash(alp0, be, score, m, 0, hd.index_hash, b,depth); // add hash table entry. Use unmodified alp.
                return(score);
            }
        }
	}


	b->m = m; // best move
	if(!break_ind)
        add_hash(alp0, be, score, m, 0, hd.index_hash, b, depth); // add hash table entry. Use unmodified alp.
	return(score);
}
