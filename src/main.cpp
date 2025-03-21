#include <iostream>
#include "rev.h"
#include "hash.h"
#include <pthread.h>
#include <semaphore.h>

#define helper_count 7 // count of helper threads, in addition to the main one *********************

int ggs_game, ggs_score;
board b_g; // global poard to pass starting pos to helpers
sem_t sem; // semaphore to start helpers
volatile int helper_status[1+helper_count]; // helper status
volatile UINT64 helper_nc[1+helper_count]; // helper nc; here 'volatile' does make a differnce!
int alp1, be1; // search window to share with helpers

TimePoint timeGetTime(void){// return current time in msec
  return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now().time_since_epoch()).count();
}

static unsigned int str_comp(char *input,char * command){
	for(unsigned int i=0; i<strlen(command); ++i)
		if( input[i] != command[i] )
			return(0);
	return(1);
}

void pass_message_to_GUI(const char *sss){// pass message to GUI
    std::cout << sss << std::flush; // here i need 'flush'!
}

void pos_from_FEN(char *sss){// -/. is empty, O/o/B/b is opponent, anything else is player. Use side to move if present
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
        if((i&7)==7)
            j+= sprintf(sss+j,"\n");
        p1 = p1>>1; p2 = p2>>1;
    }
    j+= sprintf(sss+j,"\n*\n\n"); // side to move
    return(j);
}

void* threadFunction(void* args){
    board b_l; // local board
    TimePoint t1_prev, t2;
    int depth, scoret, index = ((int*)args)[0]; // 1+
    char sss[200];

    b_l.master = index;
    helper_status[index] = 0; // 0 = waiting
    while(1){
        // wait for 'start' signal:
        // decrements (locks) the semaphore.
        // If the semaphore's value is greater than zero, then the decrement proceeds, and the function returns, immediately.
        // If the semaphore currently has the value zero, then the call blocks until it becomes possible to perform the decrement
        sem_wait(&sem);
        helper_status[index] = 1; // 1 = running

        b_l.pos[0] = b_g.pos[0];b_l.pos[1] = b_g.pos[1];
        b_l.player = 0; b_l.cut_d = 0; b_l.t1 = b_m.t1; // init
        b_l.m = 64; // init to invalid
        for(unsigned int i=0; i<64; ++i){
            b_l.history_count[0][i] = b_m.history_count[0][i]; b_l.history_count[1][i] = b_m.history_count[1][i];
        }

        // reset count for new search
        if(b_l.t1 >  t1_prev)
            b_l.n_c = 0; // reset count
        t1_prev = b_l.t1;

        depth = b_m.depth0;
        b_l.depth0 = depth;

        int alp2 = alp1, be2 = be1;
        int n0 = _popcnt64(b_l.pos[0] | b_l.pos[1]);
        UINT64 move_mask = bit_mob_mask(b_l.pos[0], b_l.pos[1]);
        helper_nc[index] = b_l.n_c; // share nodes searched with master
        scoret = Msearch(depth, alp2, be2, &b_l, move_mask, 1);// call move predictor ******************************************************************************
        helper_nc[index] = b_l.n_c; // share nodes searched with master
        int window = 2;
        if(depth >= 64 - n0)
            window = 7; // wider window for endgame
        if(!break_ind && (scoret >= be2 || scoret <= alp2)){ // wider search - if no timeout and out of window
            do{
                if(depth >= 20){
                    t2 = timeGetTime();
                    sprintf(sss, "    %d info depth %d/%d, move %d, score %d, nc %d, time %d, nps %.1f\n", depth, MPCmult, b_m.m, scoret, b_m.n_c, t2 - b_l.t1, .001f * b_m.n_c / (t2 - b_l.t1));
                    pass_message_to_GUI(sss);
                }
                alp2 = std::min(alp2, scoret - window); be2 = std::max(be2, scoret + window);
                scoret = Msearch(depth, alp2, be2, &b_l, move_mask, 1);// call move predictor ******************************************************************************
                helper_nc[index] = b_l.n_c; // share nodes searched with master
            }while(!break_ind && (scoret >= be2 || scoret <= alp2));
        }
        // end of window logic *****************

        if(depth >= 20){
            sprintf(sss,"    %d info depth %d/%d, currmove %d, score %d, n_co %d, time %d %.1f\n", index, depth, MPCmult, b_l.m, scoret, b_l.n_c, timeGetTime() - b_l.t1, .001 * b_l.n_c / (timeGetTime() - b_l.t1));
            pass_message_to_GUI(sss);
        }

        // clear flag only after printing status and sharing node count
        helper_status[index] = 0; // 0 = waiting
	};
}

int top_solve(int rem_time){// arg: remaining time for this game, msec
    UINT64 move_mask = bit_mob_mask(b_m.pos[0], b_m.pos[1]), one = 1;
	unsigned int i, j, best_move = 64, timeout;
    int depth, scoret, n0, best_score = -64, bd1 = 0, bd2 = 3, first = 0;
    const int verbose = 1; // a way to remove messages for testing
    TimePoint t1, t2;
    char sss[300];

    // if there is only 1 move left, make it immeditely
    if(_popcnt64(move_mask) == 1){
        best_move = __builtin_ctzl(move_mask);
        sprintf(sss,"bestmove %d score0 %d time %d\n", best_move, true_score(b_m.pos[0], b_m.pos[1]), 0); // return true score, call is score0
        if(verbose) pass_message_to_GUI(sss);
        return(best_move);
    }

    TTage=(TTage + 1) & 3; // increment TT age before the solve.
    b_m.n_c = 0; // reset count
    for(j=0; j<helper_count; ++j)
        helper_nc[j + 1] = 0;
    b_m.player = 0; // init
    b_m.cut_d = 0; b_m.master = 0;
    for(unsigned int i=0; i<64; ++i){ // reduce history by a factor of X for each new search
        b_m.history_count[0][i] /= 8; b_m.history_count[1][i] /= 8;
    }
    n0 = _popcnt64(b_m.pos[0]+b_m.pos[1]);
    t1 = timeGetTime(); // starting time
    b_m.t1 = t1;
    b_g.pos[0] = b_m.pos[0]; b_g.pos[1] = b_m.pos[1]; // pass pos to helpers

    // set max time for this search
    t_break = t1 + std::max(20.f, 1.3f * rem_time / std::max(3, 38 - n0)); // incr by .3
    float t_min_mult = 0.5f; // break if over X% of max time has been spent

    // set starting depth/MPC
    depth = std::min(64 - n0, 6); // starting depth = 6
    if(depth < 64 - n0)
        MPCmult = 3;
    else
        MPCmult = std::max(3, (int)MPCmult);

    // look in TT, and if it has exact score entry, start search from there
    hash_data hd;
	if(lookup_hash(0, &hd, &b_m, depth) && hd.alp_hash == hd.be_hash){ // lookup hash table and exact score
        hashtype *h1 = &h[(hd.index_hash % HASH_SIZE) & 0xfffffffffffffffe];
        unsigned int lock = hd.index_hash>>32;
        if(h1[0].lock == lock || (h1++ && h1[0].lock)){ // find the matching entry
            if(((one << hd.move_hash) & move_mask)){// legit move, use it!
                depth = h1[0].depth;
                if(depth == 0)
                    depth = 64 - n0;
                MPCmult = h1[0].MPCmult;
                bd1 = depth;
                bd2 = MPCmult;
                best_move = h1[0].move;
                best_score = h1[0].score;
            }
        }
	}

    static const unsigned char med[] = {11, 11, 11, 11, 11, 13, 14, 14};
    //                                   -   -   -   3   4   5   6  max
    MIN_ENDCUT_DEPTH = med[std::min(7, (int)MPCmult)];

    if(ggs_game){ // display GGS board on the screen
        j = print_pos(&b_m, sss);
        pass_message_to_GUI(sss);
    }

    alp1 = MIN_SCORE; be1 = MAX_SCORE;
    do{ b_m.m = 64; // init to invalid to catch uninitialized values
        b_m.depth0 = depth; // store globally, so 'ply' can be caclulated
        break_ind = 0; // reset break indicator

        // allow helpers to start ***********************************************************************
        // for deep enough search only!
        if(depth >= 12)
            for(j=0; j<helper_count; ++j)
                sem_post(&sem);

        scoret = Msearch(depth, alp1, be1, &b_m, move_mask, 1);// call move predictor ******************************************************************************
        int window = 2;
        if(depth >= 64 - n0)
            window = 7; // wider window for endgame
        if(!break_ind && (scoret >= be1 || scoret <= alp1)){ // wider search - if no timeout and out of window
            do{
                t2 = timeGetTime();
                sprintf(sss, "info depth %d/%d, move %d, score %d, nc %d, time %d, nps %.1f\n", depth, MPCmult, b_m.m, scoret, b_m.n_c, t2 - t1, .001f * b_m.n_c / (t2 - t1));
                if(verbose) pass_message_to_GUI(sss);

                alp1 = std::min(alp1, scoret - window);
                be1 = std::max(be1, scoret + window);
                scoret = Msearch(depth, alp1, be1, &b_m, move_mask, 1);// call move predictor ******************************************************************************
            }while(!break_ind && (scoret >= be1 || scoret <= alp1));
        }
        if(!break_ind){ // set window for next search - if not timeout
            alp1 = scoret - window;
            be1 = scoret + window;
        }
        // end of window logic *******************
        timeout = break_ind; // save
        break_ind = 1; // set break indicator to stop all the helpers

         // wait for helpers to finish - before prining status...................................
        do{
            for(i=0, j=0; j<helper_count; ++j)
                i += helper_status[j + 1];
        }while(i > 0);

        if( b_m.m < 64){ // save new best move
            // check if move is legal
            if((move_mask & (one<<b_m.m)) == 0 ){ // illegal move!
                sprintf(sss, "info illegal move %d!!!!!!!", b_m.m);
                if(verbose) pass_message_to_GUI(sss);
            }else{
                best_move = b_m.m;
                best_score = scoret;
                bd1 = depth;
                bd2 = MPCmult;
            }
        }
		t2 = timeGetTime();

		// get total nc
        UINT64 ncl = b_m.n_c;
        for(j=0; j<helper_count; ++j)
            ncl += helper_nc[j + 1];

		j = sprintf(sss, "info depth %d/%d, move %d, score %d, nc %d, time %d, nps %.1f\n", depth, MPCmult, b_m.m, scoret, b_m.n_c, t2 - t1, .001f * ncl / (t2 - t1));
		if(timeout)
            j += sprintf(sss+j-1, " timeout\n");
		if(verbose) pass_message_to_GUI(sss);
		if(ggs_game && depth > 17 && n0 <= 44){ // GGS display logic *******************************************************************
            j = 0;
            if(!first)
                j = sprintf(sss, "\n");
            first ++;
            j = sprintf(sss+j, "info depth %d/%d, move %c%c, score %d, nc %.1f, time %.1f, nps %.1f\n", depth, MPCmult, b_m.m/8 + 'A', b_m.m%8 + 1, scoret, b_m.n_c * .000001f, .001f * (t2 - t1), .001f * ncl / (t2 - t1));
            if(timeout)
                j += sprintf(sss+j-1, " to\n");
            pass_message_to_game(sss);
        }

		if(depth >= 64 - n0 && MPCmult >= 10) // break when full search is completed
            break;
        if(t2 - t1 > int((t_break - t1) * t_min_mult)) // break if over X% of max time has been spent
            break;

        // determine next depth
        int depth_inc = 2;
        if(depth + depth_inc < 59 - n0){ // new search will not go past 58 pieces. Better ELO than +-1.
            depth += depth_inc;
        }else{ // full depth search
            if(depth >= 64 - n0){
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
            depth = 64 - n0;
        }
	}while(1);
	j = sprintf(sss,"bestmove %d score %d time %d depth %d.%d\n", best_move, best_score, t2-t1, bd1, bd2);
	if(timeout)
        j += sprintf(sss+j-1, " timeout\n");
	assert(best_move<64);
    if(verbose) pass_message_to_GUI(sss);
    ggs_score = best_score;
    return(best_move);
}

static void test(){
    int m, j, n0;
    char sss[300];

	pass_message_to_GUI("Testing speed. Please wait.\n");
    pos_from_FEN("------------O---*-**O*--OOOOO----O*OO-----**O---OO*--O----*----- *"); // n0 = 23; 9 possible moves

    int time_per_game = 300000; // time limit - inf for test, low for self-play
    TimePoint t1 = timeGetTime();
    for(unsigned int mmm=0; mmm<64; ++mmm){ // loop over moves
        TimePoint t2 = timeGetTime();
        n0 = _popcnt64(b_m.pos[0]+b_m.pos[1]);
        j = print_pos(&b_m, sss);
        j += sprintf(sss+j, "remaining time %d, pieces %d\n", time_per_game - t2 + t1, n0);
        pass_message_to_GUI(sss);
        m = top_solve(time_per_game - t2 + t1);
        break; // test - stop after 1 move********************************************************
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

int main(int argc, char* argv[]){
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
