//#define HASH_SIZE (16 * 1024 * 1024) // 16M * 8 bytes = 128 Mb
//#define HASH_SIZE (16 * 16 * 1024 * 1024) // 256M * 8 bytes = 2 Gb
#define HASH_SIZE (16 * 16 * 4 * 1024 * 1024) // 1B * 8 bytes = 8 Gb

typedef struct{// transposition table entry
	unsigned int lock;		                    // lock: 4 bytes
	uint16_t type:2, endg:1, move:6, MPCmult:4; // 13 bits = 2 bytes, 3 bits free ***
	int8_t score;	                            // score: 1 byte
	uint8_t depth:6, age:2;                     // 8 bits  = 1 byte
        // type;			// type of score: 0-exact, 1-lower bound, 2-upper bound. Need only 2 bits
        // endg;			// endg: endgame. Need only 1 bit
        // move;			// best move chosen. Need only 6 bits (0 to 63)
        // depth;		// depth of stored search. Need only 6 bits (0 to 63)
        // MPCmult;		// MPCmult. Need only 4 bits (0 to 15) - inf is set to 10
        // age;		    // TT age. Need only 2 bits
} hashtype; // 8 bytes a pop.

typedef struct{// return of hash lookup
	UINT64 index_hash;
	int alp_hash, be_hash;
	unsigned int move_hash;
} hash_data;

unsigned int lookup_hash(const unsigned int endg,hash_data *hd,board *b, int depth);
void add_hash(int alp, int be, int score, const unsigned int move, const unsigned int endg,const UINT64 index_hl,board *b, int depth);
void clear_hash(void);

extern hashtype *h;
