#include <stdio.h>
#include <assert.h>
#include <sys/param.h>
#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <cctype>
#include <climits>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <immintrin.h>
#include <math.h>

typedef std::chrono::milliseconds::rep TimePoint; // A value in milliseconds

#define UINT64 uint64_t

#define MAX_SCORE 126
#define MIN_SCORE -126

#define EVAL_WIDTH 64           // number of EVAL OUTPUTS.
#define eval_type int16_t       // type of evaluation params, to match width

typedef struct{// board.
	UINT64 pos[2];				// 16 bytes. Position.
	unsigned int n_c;           // node count
	unsigned int m;             // best move
	unsigned int player;        // 0/1. Only used for move history sorting
	int history_count[2][64];   // count by square and player
	int master;                 // 0 = master, 1+ - helpers
	TimePoint t1;               // starting time
} board;

UINT64 bit_mob_mask(UINT64, UINT64);
UINT64 flip(const uint_fast8_t, UINT64, UINT64);
int eval(board*, UINT64);
float sigma_mult(board*, UINT64, unsigned int);
void make_move(unsigned int, board*);
int Msearch(const int, int, int, board*, UINT64, unsigned int);
void Ginit(void);
int print_pos(board*, char*);
void pass_message_to_GUI(const char*);
void pass_message_to_GGS0(char*);
void pass_message_to_game(const char*);
TimePoint timeGetTime(void);
int true_score(UINT64, UINT64);
int top_solve(int);
void pos_from_FEN(const char*);
void start_GGS(void);

extern board b_m;
extern unsigned int MPCmult;
extern TimePoint t_break;
extern unsigned int TTage;
extern unsigned int sigmas[][64];
extern int cut_depth[];
extern unsigned int break_ind;

extern unsigned int *c2to3;     // converts power of 2 to power of 3. Up to 10 bits
extern unsigned int *tr_ar_c9;  // converts 3 c9 patterns. Up to 9 bits each.
extern unsigned int *tr_ar_c10; // converts 7 c10 patterns. Up to 10 bits each.

extern eval_type *mob1_c, *mob2_c, *s8a_c, *s8b_c, *s8c_c, *s8d_c, *d8_c, *d7_c, *d6_c, *d5_c, *d4_c, *c8_c, *c9_c, *c10a_c;
extern float n_c[][EVAL_WIDTH];
extern eval_type n_ci[][EVAL_WIDTH];

extern eval_type n_c2[][16], mob1_c2[][16], mob2_c2[][16], s8a_c2[][16], s8b_c2[][16], s8c_c2[][16], s8d_c2[][16], c9_c2[][16], c10_c2[][16];
extern float d_c2[][16];
