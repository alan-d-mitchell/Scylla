#ifndef TRANSPOSE_H
#define TRANSPOSE_H

#include "defs.h"
#include "board.h"

#define NO_HASH_ENTRY 100000

enum { HASH_FLAG_EXACT, HASH_FLAG_ALPHA, HASH_FLAG_BETA };

typedef struct {
    u64 key;
    int depth;
    int flags;
    int score;
} HashEntry;

extern u64 piece_keys[12][64];
extern u64 castle_keys[16];
extern u64 side_key;
extern u64 enpassant_keys[64];

void init_zobrist_keys();
u64 generate_hash_key(const Board* board);
void init_transposition_table();
int probe_hash(u64 hash_key, int depth, int alpha, int beta);
void record_hash(u64 hash_key, int depth, int score, int hash_flag);

#endif