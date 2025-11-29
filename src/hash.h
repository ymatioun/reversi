//#define HASH_SIZE (16 * 1024 * 1024) // 16M * 8 bytes = 128 Mb; for local single-threaded games
//#define HASH_SIZE (16 * 16 * 1024 * 1024) // 16 * 16M * 8 bytes = 2 Gb; for local 16-threaded games
#define HASH_SIZE (16 * 16 * 8 * UINT64(1024 * 1024)) // 2B * 8 bytes = 16 Gb; for GGS games

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
	unsigned int move_hash, depth_hash;
} hash_data;

unsigned int lookup_hash(const unsigned int, hash_data*, board*, int);
void add_hash(int alp, int be, int score, const unsigned int, const unsigned int, const UINT64, int);
UINT64 key_hash(board*);
void clear_hash(void);

extern hashtype *h, *h2;
