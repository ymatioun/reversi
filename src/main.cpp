#include <iostream>
#include "rev.h"
#include "hash.h"
#include <pthread.h>
#include <semaphore.h>

// for GGS: 7/15 helpers, large hash table **********************
#define helper_count 15 // count of helper threads, in addition to the main one *********************

int ggs_score; // score to pass to GGS
int depth0; // search depth, to pass to helpers

static int ggs_game;
static board b_g; // global poard to pass starting pos to helpers
static sem_t sem; // semaphore to start helpers
static volatile int helper_status[1+helper_count]; // helper status
static volatile UINT64 helper_nc[1+helper_count]; // helper nc; here 'volatile' does make a differnce!
static volatile int helper_history_count[1+helper_count][2][64]; // helper history count
static int alp1, be1; // search window to share with helpers

TimePoint timeGetTime(void){// return current time in msec
  return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now().time_since_epoch()).count();
}

static unsigned int str_comp(char *input,const char * command){
	for(unsigned int i=0; i<strlen(command); ++i)
		if( input[i] != command[i] )
			return(0);
	return(1);
}

void pass_message_to_GUI(const char *sss){// pass message to GUI
    std::cout << sss << std::flush; // here i need 'flush'!
}

void pos_from_FEN(const char *sss){// -/. is empty, O/o/B/b is opponent, anything else is player. Use side to move if present
    b_m.pos[0]=0; b_m.pos[1]=0;
    for(int i=0; i<64; ++i){
        b_m.pos[0] = b_m.pos[0]<<1; b_m.pos[1] = b_m.pos[1]<<1;
        if(sss[63-i] != '-' && sss[63-i] != '.'){
            if(sss[63-i] == 'O' || sss[63-i] == 'o' || sss[63-i] == 'B' || sss[63-i] == 'b')
                b_m.pos[1] ++;
            else
                b_m.pos[0] ++;
        }
    }
    if(sss[64] && sss[65] && (sss[65] == 'O' || sss[65] == 'o' || sss[65] == 'B' || sss[65] == 'b')){// swap if nedded
        UINT64 t = b_m.pos[0]; b_m.pos[0] = b_m.pos[1]; b_m.pos[1] = t;
    }
    // save position globally
    b_g.pos[0] = b_m.pos[0]; b_g.pos[1] = b_m.pos[1];
}

int print_pos(board *b, char *sss){
    UINT64 p1 = b->pos[0], p2 = b->pos[1];
    unsigned int j = 0;
    j+= sprintf(sss+j,"\n");
    for(unsigned int i=0; i<64; ++i){
        if(p1&1)
            j+= sprintf(sss+j,"*");
        else if(p2&1)
            j+= sprintf(sss+j,"O");
        else
            j+= sprintf(sss+j,"-");
        //if((i&7)==7)
        //    j+= sprintf(sss+j,"\n");
        p1 = p1>>1; p2 = p2>>1;
    }
    //j+= sprintf(sss+j,"\n*\n\n"); // side to move - always *
    j+= sprintf(sss+j," *\n"); // side to move - always *
    return(j);
}

void* threadFunction(void* args){
    __attribute__ ((aligned (32))) board b_l; // local board
    TimePoint t1_prev;
    int scoret, index = ((int*)args)[0]; // 1+
    char sss[200];

    b_l.master = index;
    helper_status[index] = 0; // 0 = waiting

    while(1){
        // wait for 'start' signal: decrements (locks) the semaphore.
        // If the semaphore's value is greater than zero, then the decrement proceeds, and the function returns, immediately.
        // If the semaphore currently has the value zero, then the call blocks until it becomes possible to perform the decrement
        sem_wait(&sem);
        helper_status[index] = 1; // 1 = running

        b_l.pos[0] = b_g.pos[0];b_l.pos[1] = b_g.pos[1];
        b_l.player = 0; b_l.t1 = b_m.t1; // init
        b_l.m = 64; // init to invalid
        memcpy(&b_l.history_count[0][0], &b_m.history_count[0][0], 2 * 64 * sizeof(int)); // copy current hist from master

        // reset count for new search
        if(b_l.t1 >  t1_prev)
            b_l.n_c = 0; // reset count
        t1_prev = b_l.t1;

        scoret = Msearch(depth0, alp1, be1, &b_l, bit_mob_mask(b_l.pos[0], b_l.pos[1]), 1);// call move predictor ******************************************************************************
        helper_nc[index] = b_l.n_c; // share nodes searched with master
        memcpy(&helper_history_count[index][0][0], &b_l.history_count[0][0], 2 * 64 * sizeof(int)); // share history count with master

        // this code ensures that node count/hist is passed before status change
        if(depth0 >= 200){
            sprintf(sss,"    %d info depth %d/%d, currmove %d, score %d, nc %u, time %d %.1f\n", index, depth0, MPCmult, b_l.m, scoret, b_l.n_c, int(timeGetTime() - b_l.t1), .001 * b_l.n_c / (timeGetTime() - b_l.t1));
            pass_message_to_GUI(sss);
        }

        // clear flag only after printing status and sharing node count
        helper_status[index] = 0; // 0 = waiting
	};
}

int top_solve(int rem_time){// arg: remaining time for this game, msec
    static UINT64 pr_pos[2];// previous position
    UINT64 move_mask = bit_mob_mask(b_m.pos[0], b_m.pos[1]), ncl;
	unsigned int i, j, best_move = 64, timeout;
    int depth, scoret, n0, best_score = -64, bd1 = 0, bd2 = 3, first = 0;
    #define verbose 1 // a way to remove messages for testing
    TimePoint t1, t2;
    char sss[300];

    // if there is only 1 move left, make it immeditely
    if(_popcnt64(move_mask) == 1){
        best_move = __builtin_ctzl(move_mask);
        sprintf(sss,"bestmove %d score0 %d time %d\n", best_move, true_score(b_m.pos[0], b_m.pos[1]), 0); // return true score, call is score0
        if(verbose) pass_message_to_GUI(sss);
        return(best_move);
    }


    // switch history / TT for different sync games
    static int hist_l[2][2][64], diff_pos_count;
	if(pr_pos[0] && (pr_pos[0] != b_m.pos[0] || pr_pos[1] != b_m.pos[1])){ // prior position exists, and is different - switch!
        // save current history
        memcpy(&hist_l[n0 % 2][0][0], b_m.history_count, 2 * 64 * sizeof(int));

        // restore history, if not empty
        if(diff_pos_count > 0) // > 0 means second time, so was saved before
            memcpy(b_m.history_count, &hist_l[1 - n0 % 2][0][0], 2 * 64 * sizeof(int));

        // copy current hash over to h2, if first time
        if(diff_pos_count == 0)
            memcpy(h2, h, (HASH_SIZE + 10) * sizeof(hashtype));

        // switch TT
        hashtype *ht = h; h = h2; h2 = ht;

        diff_pos_count++; // increment count of different positions
    }


    TTage = (TTage + 1) & 3; // increment TT age before the solve.
    b_m.n_c = 0; // reset count
    for(j = 0; j < helper_count; ++j)
        helper_nc[j + 1] = 0;
    b_m.player = 0; b_m.master = 0; // init
    for(unsigned int i = 0; i < 64; ++i){ // reduce history by a factor of X for each new search
        b_m.history_count[0][i] /= 8; b_m.history_count[1][i] /= 8;
    }
    n0 = _popcnt64(b_m.pos[0]+b_m.pos[1]);
    t1 = timeGetTime(); // starting time
    b_m.t1 = t1;
    b_g.pos[0] = b_m.pos[0]; b_g.pos[1] = b_m.pos[1]; // pass pos to helpers

    // timing
    float mult = 1.0f;
    if(pr_pos[0]==0 && n0<40)// increase time for first search for synch games, excluding endgame
        mult = 1.2f; // by 20%
    t_break = t1 + mult * std::max(20.f, 1.8f * rem_time / std::max(4.8f, 37.0f - n0)); // assume solved at 37[27]. Max of 5. Mult of 1.8
    float t_min_mult = 0.5f; // break if over X% of max time has been spent. So, always spend 0.9 to 1.8 of Y

    // set starting depth/MPC/window
    // use even/odd depth based on n
    depth = 6 + n0 % 2; // starting depth = 6/7
    depth = std::min(64 - n0, depth);
    if(depth < 64 - n0)
        MPCmult = 3;
    else
        MPCmult = std::max(3, (int)MPCmult);
    alp1 = MIN_SCORE; be1 = MAX_SCORE; // first search is always full width

    // look in TT, and if it has exact score entry, start search from there: depth, window
    hash_data hd;
	if(lookup_hash(0, &hd, &b_m, depth) && hd.alp_hash == hd.be_hash){ // lookup hash table and exact score
        hashtype *h1 = &h[(hd.index_hash % HASH_SIZE) & 0xfffffffffffffffe];
        unsigned int lock = hd.index_hash>>32;
        if(h1[0].lock == lock || (h1++ && h1[0].lock)){ // find the matching entry
            if(((UINT64(1) << hd.move_hash) & move_mask)){// legit move, use it!
                // set depth/MPC
                MPCmult = h1[0].MPCmult;
                depth = h1[0].depth;
                if(depth == 0)
                    depth = 64 - n0;

                // set window
                int window = 2;
                alp1 = h1[0].score - window; be1 = h1[0].score + window;

                // set 'best' values for display only: depth/MPC, move, score; in case search terminates before best move is found.
                bd1 = depth; bd2 = MPCmult;
                best_move = h1[0].move; best_score = h1[0].score;
            }
        }
	}

    if(ggs_game){ // display GGS board on the screen
        j = print_pos(&b_m, sss);
        pass_message_to_GUI(sss);
    }

    do{ // start loop over depth **************************************************************************************************************************************
        int window = 2, asp_try = 1, result = 0;
        do{ // start loop over window *********************************************************************************************************************************
            if(asp_try++ > 1){ // widen the window on second try
                alp1 = std::min(alp1, scoret - window);
                be1 = std::max(be1, scoret + window);
            }

            b_m.m = 64; // init to invalid to catch uninitialized values
            depth0 = depth; // pass to helpers
            break_ind = 0; // reset break indicator

             // allow helpers to start ***********************************************************************
            // for deep enough search only!
            if(depth >= 12)
                for(j=0; j<helper_count; ++j)
                    sem_post(&sem);


            scoret = Msearch(depth, alp1, be1, &b_m, move_mask, 1);// call move predictor ********************


            timeout = break_ind; // save
            break_ind = 1; // set break indicator to stop all the helpers

            // wait for helpers to finish - before prining status.............................................
            do{
                for(i=0, j=0; j<helper_count; ++j)
                    i += helper_status[j + 1];
            }while(i > 0);

            // get total nc
            ncl = b_m.n_c;
            for(j = 0; j < helper_count; ++j)
                ncl += helper_nc[j + 1];

            // get total history
            for(unsigned int k = 0; k < 2; ++k)
                for(i = 0; i < 64; ++i){
                    b_m.history_count[k][i] /= 1 + helper_count; // average
                    for(j = 0; j < helper_count; ++j)
                        b_m.history_count[k][i] += helper_history_count[j][k][i] / (1 + helper_count); // average
                }

            t2 = timeGetTime();
            j = sprintf(sss, "info depth %d/%d, move %d, score %d/%d/%d, nc %u, time %d/%d, nps %.1f\n", depth, MPCmult, b_m.m, scoret, alp1, be1, ncl, int(t2 - t1), int(t_break - t1), .001f * ncl / (t2 - t1));
            if(scoret >= be1){
                j += sprintf(sss+j-1, " high\n") - 1;
                result = 1; // 1= high
            }else if(scoret <= alp1){
                j += sprintf(sss+j-1, " low\n") - 1;
                result = 2; // 2= low
            }
            if(timeout)
                j += sprintf(sss+j-1, " timeout\n");
            if(verbose) pass_message_to_GUI(sss);
        }while(!timeout && (scoret >= be1 || scoret <= alp1)); // end loop over window ********************************************************************************
        alp1 = scoret - window; be1 = scoret + window; // set window for next depth

        int move_change = 0;
        if(result != 2 && b_m.m < 64){ // save new best move - ignore fail-low move
            if((move_mask & (UINT64(1)<<b_m.m)) == 0 ){ // illegal move!
                sprintf(sss, "info illegal move %d!!!!!!!", b_m.m);
                if(verbose) pass_message_to_GUI(sss);
            }else{
                if(best_move != b_m.m) // best move changed - no early stopping!
                    move_change = 1;
                best_move = b_m.m; best_score = scoret;
                bd1 = depth; bd2 = MPCmult;
            }
        }

		if(ggs_game && depth > 22 && n0 <= 44){ // GGS display logic *******************************************************************
            if(!first)
                pass_message_to_game("\n");
            first++;
            j = sprintf(sss, "info depth %d/%d move %c%c score %d time %.1f nps %.1f\n", depth, MPCmult, b_m.m%8 + 'A', b_m.m/8 + '1', scoret, .001f * (t2 - t1), .001f * ncl / (t2 - t1));
            if(timeout)
                j += sprintf(sss+j-1, " TO\n");
            pass_message_to_game(sss);
        }

		if(t2 >= t_break || MPCmult >= 10) // break when full search is completed, or timeout
            break;
        if(move_change == 0 && result != 2 && t2 - t1 > int((t_break - t1) * t_min_mult)) // break if over X% of max time has been spent, except for fail low and best move change
            break;

        // determine next depth
        if(depth + 2 < 59 - n0) // new search will not go past 59 pieces.
            depth += 2;
        else{ // full depth search
            if(depth >= 64 - n0){ // already did fully depth at 3, increase mult
                if(MPCmult <= 3)
                    MPCmult = 4;
                else if(MPCmult == 4)
                    MPCmult = 5;
                else if(MPCmult == 5)
                    MPCmult = 6;
                else if(MPCmult == 6)
                    MPCmult = 8;
                else if(MPCmult >= 8)
                    MPCmult = 10; // 10 = inf
            }
            depth = 64 - n0; // do full depth at 3
        }
	}while(1); // end loop over window ********************************************************************************************************************************
	j = sprintf(sss,"bestmove %d score %d time %d depth %d.%d\n", best_move, best_score, int(t2-t1), bd1, bd2);
	if(timeout)
        j += sprintf(sss+j-1, " timeout\n");
	assert(best_move<64);
    if(verbose) pass_message_to_GUI(sss);
    ggs_score = best_score;

    // record position resulting from the move
    board b_l; // do not change the b_m, make a copy
    b_l.pos[0] = b_m.pos[0];b_l.pos[1] = b_m.pos[1];
    make_move(best_move, &b_l);
    if(n0 <= 50){
        pr_pos[0] = b_l.pos[0]; pr_pos[1] = b_l.pos[1]; // store position
    }else{
        pr_pos[0] = pr_pos[1] = 0;// reset pr_pos to zero for endgame - next game will start fresh.
        memset(&hist_l, 0, 2 * 2 * 64 * sizeof(int)); // clear history
        diff_pos_count = 0; // reset count
    }

    return(best_move);
}

static void test(){
    int m, j, n0;
    char sss[300];

	pass_message_to_GUI("Testing speed. Please wait.\n");
    //pos_from_FEN("---XXXX-X-XXXO--XXOXOO--XXXOXO--XXOXXO---OXXXOO-O-OOOO------OO-- X"); // f45 - 40 pieces:  42.2 sec / 3,065,734,985
    pos_from_FEN("------------O---*-**O*--OOOOO----O*OO-----**O---OO*--O----*----- *"); // n0 = 23
    //int score = eval(&b_m, bit_mob_mask(b_m.pos[0], b_m.pos[1])); // test

    // sigma NN v1                              d=26 13/21,  64,615,099 nodes, 7.0 sec, ELO=+172 [+16] - better, accept *******************************************************************************************************************
    // sigma_v2                                 d=26 13/21,  55,823,245 nodes, 5.8 sec, ELO=+167 [-5] - a wash???
    // sigma mult: add s8b-d, c10: sigma_v3:    d=26 13/21,  64,711,787 nodes, 7.3 sec, ELO=+177 - better, keep *******************************************************************************************************************
    // eval from 2-depth search positions: eval_v14: +143 - worse, undo
    // reduce hist by 2, not 4:                 +168
    // reduce hist by 8, not 4:                 +183 - better, keep *******************************************************************************************************************
    // rerun baseline:                          +175
    // eval_v14, E1=64:                         d=26 13/21,  61,847,924 nodes, 8.8 sec, ELO=+181 - slightly better? Basically, a wash
    // eval_v15                                 d=26 13/21,  15,552,261 nodes, 2.2 sec, ELO=+186 - accept
    // eval_v16+parity                          d=26 29/20,  30,682,459 nodes, 4.4 sec, ELO=+185 - a wash
    // v17(Na)                                  d=26 13/23,  21,038,677 nodes, 2.9 sec, ELO=+183 - a wash
    // PV extension: +2                             ELO=+155 - bad, undo
    // clean eg, no 4/2 cut                     ELO=+176 - a wash, accept
    // no sigma mult:                               ELO=+137, =-40.
    // revert to eval_!5                        d=26 13/21,  15,552,261 nodes, 2.2 sec, ELO=+186
    // sigmas capped at 6                           d=26 13/21,  14,962,462 nodes, 2.1 sec, ELO=+172=-14, worse, undo...
    // cuts to 9/10                             d=26 13/21,  15,644,232 nodes, 2.1 sec, ELO=+188 - a wash, keep
    // timing:1.5, 4, 37                        ELO=+184 - a wash, keep
    // v18      13 epochs:                      189
    // v19 210 epochs                           d=26 21/21,  36,132,806 nodes, 5.4 sec, ELO=+199 - accept!
    // full window for j=0 all node types       d=26 29/21,  36,611,013 nodes, 5.5 sec +17 ELO
    // no alp0                                  d=26 29/21,  34,938,238 nodes, 5.2 sec a wash, accept
    // TT cuts on first part of PV              d=26 29/21,  28,975,964 nodes, 4.2 sec a wash, accept
    // no ETC                                   d=26 29/21,  29,086,760 nodes, 4.2 sec a wash -25 ELO
    // sort: smaller values, next to c=neg      d=26 29/21,  26,496,660 nodes, 3.8 sec -36 ELO, accept
    // TT before MPC                            d=26 29/21,  26,125,491 nodes, 3.8 sec -41 ELO, accept
    // longer condition for re-search           d=26 29/21,  26,126,005 nodes, 3.7 sec -42 ELO, accept
    // MIN_SORT_DEPTH2 = 1                          d=26 29/21,             nodes, 4.2 sec -38 ELO - undo!
    // make different MPC best move second, but sort the MPC move away (for d>MIN_SORT_DEPTH2)
    //                                              d=26 29/21,  24,767,793 nodes, 3.6 sec -38 ELO - undo!
    // do not sort it away:                         d=26 29/21,  25,874,607 nodes, 3.8 sec -43 ELO - undo!
    // w = d^1.5                                    d=26 29/21,  24,301,038 nodes, 3.5 sec -40 ELO - undo!
    // window = 4, not 2                        d=26 29/21,  26,324,234 nodes, 3.9 sec -43 ELO, accept
    // SF hist bonus                                d=26 29/21,  25,661,285 nodes, 3.8 sec -41 ELO - undo!
    // hist /=4, not 8                              -36 ELO - undo!
    // remove long EFP condition                d=26 29/21,  26,316,177 nodes, 3.8 sec; true sync games: -34+-5
    // hist /=16, not 8                             -37 ELO - undo!
    // levelized timing v1                          true sync games: -16 ELO - undo!
    // use odd depths                               d=27 29/20,  44,863,466 nodes, 6.8 sec; true sync games: -38+-8
    // alp0, eval hash                          d=26 29/21,  25,285,635 nodes, 3.4 sec; true sync games: -46+-7 - some improvement!
    // starting depth = 6/7                     87,321,892 11.3 sec -52 ELO - better!
    // starting depth = 7/6                         -41 ELO - worse, undo
    // as window center, use TT score           -50 - a wash, keep
    // 2 sets of TT for sync games              -66 ELO - better!
    // faster bitmob, always inline             10.7 sec: ELO=-78 - better!
    // 1.5x time for first search for synch games: -75 - not better, undo
    // crc hash, AVX2 flips                     d=29 29/20,  83,262,622 9.8 sec; ELO=-80 - better!
    // timing v2 - narrow the range                 ELO=-70 - worse, undo!
    // timing v3 - incr initial time by 1.1     -86 - better!
    // no MPC for depth0-depth < 2                  d=29 29/20,  73,834,411 8.9 sec; ELO=-82 -a wash, undo
    // MPC: always try eval first, d=[1:2]: -77 - worse, undo
    // MPC: always try eval first, d=[2:5]: -72 - worse, undo
    // ignore fail-low move: seems to reduce bad score tail!!!!!!!!!!!!!!!! ELO=-87 - better!
    // eval hash per thread, parallel search:               -261
    // infinite window                          d=29 29/20,  85,212,767 10.0 sec; ELO=-58 - worse, undo
    // window = 3:                              ELO=-94 - better!
    // window = 2:                              d=29 29/20,  90,960,026 11.0 sec; ELO=-102 - better! ************************* best so far **************************
    // window = 1:                                  ELO=-98 - worse, undo
    // more agressive timing                    ELO=-101 - a wash, accept
    // switch to endgame at different N:
    // if(depth + 2 < 60 - n0) vs 59:               ELO=-98 - worse, undo
    // 58:                                          ELO=-86 - worse, undo
    // no early stop if fail low:               ELO=-100 - a wash,accept
    // no early stopping if move changed        ELO=-99 - a wash,accept



    int time_per_game = 150000; // time limit - inf for test, low for self-play
    TimePoint t1 = timeGetTime();
    for(unsigned int mmm=0; mmm<64; ++mmm){ // loop over moves
        TimePoint t2 = timeGetTime();
        n0 = _popcnt64(b_m.pos[0]+b_m.pos[1]);
        j = print_pos(&b_m, sss);
        j += sprintf(sss+j, "remaining time %d, pieces %d\n", int(time_per_game - t2 + t1), n0);
        pass_message_to_GUI(sss);
        m = top_solve(time_per_game - t2 + t1);
        // write history
        /*FILE *f=fopen("h.csv","w");
        for(unsigned i=0;i<64;++i)
            fprintf(f, "%d,%d,%d\n", i, b_m.history_count[0][i], b_m.history_count[1][i]);
        fclose(f);*/

        //break; // test - stop after 1 move********************************************************
        if(m >= 64){
            pass_message_to_GUI("search returned illegal move!!! **********************************************************\n");
            break;
        }
        make_move(m, &b_m);
        if(!bit_mob_mask(b_m.pos[0],b_m.pos[1])){ // pass - flip players
            UINT64 a = b_m.pos[0];
            b_m.pos[0] = b_m.pos[1];
            b_m.pos[1] = a;
            pass_message_to_GUI("pass ********\n");
        }
        if(!bit_mob_mask(b_m.pos[0],b_m.pos[1])){ // second pass - end of game
            pass_message_to_GUI("no moves left\n");
            sprintf(sss, "final score is %d *****************\n", true_score(b_m.pos[0], b_m.pos[1])); // this score is really true**
            pass_message_to_GUI(sss);
            break;
        }
        if(_popcnt64(b_m.pos[0]+b_m.pos[1]) >= 64)
            break;
    }
	pass_message_to_GUI("finished\n");
}

int main(){
	char *input = (char *)malloc(1024 * 8);	// input buffer
	unsigned int offset, read_counter;

	// init everything
	Ginit();

	// prep parallel search
	#if helper_count > 0
	sem_init(&sem, 0, 0);
	pthread_t id[helper_count];
	int args[helper_count];
	for(unsigned int j=0; j<helper_count; ++j){
        args[j] = j + 1; // start helpers from 1, not from 0
        pthread_create(&id[j], NULL, &threadFunction, (void *)&args[j]);
    }
    #endif

	while(1){// start of main - infinite - loop ***************************************************************************
        fgets(input, 1024*8, stdin);

        read_counter = strlen(input);
		if( input[read_counter] == 13 || input[read_counter] == 10 ) // skip one new line
			read_counter--;
		input[read_counter] = 0; // terminate the string

		// parse input string
		offset=0;// start parsing from the start
		while( input[offset] == 10 || input[offset] == 13 || input[offset] == ' ' )// skip blanks
			offset++;
		if(read_counter<=offset)// skip if only input is "\n"
			continue;

        do{ // loop over input commands
            if( str_comp(input + offset, "quit") ){// ******************************************************************************************************** quit
				exit(0);
            }else if( str_comp(input + offset, "newgame") ){// *********************************************************************************************** newgame
     			offset += 7;
				clear_hash();
            }else if( str_comp(input + offset, "test") ){// ************************************************************************************************** test
                offset += 4;
				test();
            }else if( str_comp(input + offset, "ggs") ){// ************************************************************************************************** GGS
                offset += 3;
				ggs_game = 1;
				start_GGS();
            }else if( str_comp(input + offset, "position") ){// ********************************************************************************************** position fen ...
                offset += 9;// skip blank
				// assume command is always "position fen ..."
                offset += 4;// skip "fen "
                pos_from_FEN(input + offset);
                offset += 64; // skip min FEN
                while( input[offset] != 10 && input[offset] != 13 && input[offset] != 0 ) offset++;// skip FEN: it does not end on space, go to the new line! Or to "moves"
            }else if( str_comp(input+offset, "go time") ){// ****************************************************************************************************** go time
                offset += 8;// skip blank

                // get time
                int rem_time = input[offset] - '0';
                // skip '-'
                if(input[offset] == '-'){
                    offset++;
                    rem_time = input[offset] - '0';
                }
                offset++;
                while( input[offset] >= '0' && input[offset] <= '9'){
                    rem_time = rem_time * 10 + input[offset] - '0';
                    offset++;
                }
                offset++;

				// call the solver
				top_solve(rem_time);
            }else{// ************************************************************************************************************************************ skip unrecognized command
				while( input[offset] != 0 ) // skip unrecognized command - go to the end of the string
					offset++;
			}
        }while(read_counter>offset); // end of loop over input commands
	}// end of infinite loop
	return(0);
}
