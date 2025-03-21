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

typedef std::chrono::milliseconds::rep TimePoint; // A value in milliseconds

#define UINT64 uint64_t

#define MAX_SCORE 126
#define MIN_SCORE -126

#define EVAL_WIDTH 32           // number of EVAL OUTPUTS.
#define eval_type int16_t       // type of evaluation params, to match width

typedef struct{// board.
	UINT64 pos[2];				// 16 bytes. Position.
	UINT64 n_c;                 // node count
	unsigned int m;             // best move
	unsigned int cut_d;			// Cut diff
	unsigned int player;        // 0/1. Only used for move history sorting
	int history_count[2][64];   // count by square and player
	int master;                 // 0 = master, 1+ - helpers
	int depth0;                 // starting depth, to determine ply
	TimePoint t1;               // starting time
} board;

void make_move(unsigned int i, board *b);
int Msearch(const int depth, int alp, int be, board *b, UINT64 move_mask, unsigned int node_type);
void Ginit(void);
UINT64 bit_mob_mask(UINT64 p1,UINT64 p0);
unsigned long long flip(int, unsigned long long P, unsigned long long O);
int print_pos(board *b, char *sss);
void pass_message_to_GUI(const char *sss);
void pass_message_to_GGS0(char *s);
void pass_message_to_game(const char *s);
int eval(board *b, UINT64 move_mask);
TimePoint timeGetTime(void);
int true_score(UINT64 p0, UINT64 p1);
int top_solve(int rem_time);
unsigned int bit_pot_mob(UINT64 P, UINT64 O);
void pos_from_FEN(char *sss);
unsigned int move_list_f(unsigned int* l, UINT64 mask);
void start_GGS(void);
float sigma_mult(board *b, UINT64 move_mask, unsigned int d1);

extern board b_m;
extern unsigned int MPCmult;
extern TimePoint t_break;
extern unsigned int TTage;
extern unsigned int sigmas[][64];
extern int cut_depth[];
extern unsigned int MIN_ENDCUT_DEPTH;
extern unsigned int break_ind;

extern unsigned int *c2to3;     // converts power of 2 to power of 3. Up to 10 bits
extern unsigned int *tr_ar_c9;  // converts 3 c9 patterns. Up to 9 bits each.
extern unsigned int *tr_ar_c10; // converts 7 c10 patterns. Up to 10 bits each.

extern eval_type *mob1_c, *mob2_c, *s8a_c, *s8b_c, *s8c_c, *s8d_c, *d8_c, *d7_c, *d6_c, *d5_c, *d4_c, *c8_c, *c9_c;
extern float n_c[][EVAL_WIDTH];

extern eval_type n_c2[][16], mob1_c2[][16], mob2_c2[][16], s8a_c2[][16], s8b_c2[][16], s8c_c2[][16], s8d_c2[][16], c9_c2[][16], c10_c2[][16];
extern float d_c2[][16];
