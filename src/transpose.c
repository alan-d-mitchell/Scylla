#include <stdlib.h>
#include "transpose.h"
#include "defs.h"
#include "movegen.h" // For rand64_prng()

// --- Zobrist Keys ---
// Used for incrementally updating the hash key of a board position
u64 piece_keys[12][64];
u64 castle_keys[16];
u64 side_key;
u64 enpassant_keys[64];

// --- Transposition Table ---
#define HASH_SIZE 0x100000 // Size of the TT, must be a power of 2
HashEntry* transposition_table = NULL;

void init_zobrist_keys() {
    seed_prng(1070372); // Seed your PRNG

    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 64; j++) {
            piece_keys[i][j] = rand64_prng();
        }
    }
    for (int i = 0; i < 16; i++) {
        castle_keys[i] = rand64_prng();
    }
    for (int i = 0; i < 64; i++) {
        enpassant_keys[i] = rand64_prng();
    }
    side_key = rand64_prng();
}

u64 generate_hash_key(const Board* board) {
    u64 final_key = 0ULL;
    u64 bitboard;

    for (int piece = P; piece <= k; piece++) {
        bitboard = board->piece_bitboards[piece];
        while (bitboard) {
            int square = __builtin_ctzll(bitboard);
            final_key ^= piece_keys[piece][square];
            bitboard &= bitboard - 1;
        }
    }

    if (board->enpassant_square != -1) {
        final_key ^= enpassant_keys[board->enpassant_square];
    }

    final_key ^= castle_keys[board->castling_rights];

    if (board->side_to_move == BLACK) {
        final_key ^= side_key;
    }

    return final_key;
}

void init_transposition_table() {
    if (transposition_table != NULL) {
        free(transposition_table);
    }
    // Allocate memory for the transposition table and initialize it to zero
    transposition_table = (HashEntry*)calloc(HASH_SIZE, sizeof(HashEntry));
}

int probe_hash(u64 hash_key, int depth, int alpha, int beta) {
    HashEntry* entry = &transposition_table[hash_key & (HASH_SIZE - 1)];

    if (entry->key == hash_key) {
        if (entry->depth >= depth) {
            if (entry->flags == HASH_FLAG_EXACT) {
                return entry->score;
            }
            if ((entry->flags == HASH_FLAG_ALPHA) && (entry->score <= alpha)) {
                return alpha;
            }
            if ((entry->flags == HASH_FLAG_BETA) && (entry->score >= beta)) {
                return beta;
            }
        }
    }
    return NO_HASH_ENTRY;
}

void record_hash(u64 hash_key, int depth, int score, int hash_flag) {
    HashEntry* entry = &transposition_table[hash_key & (HASH_SIZE - 1)];

    entry->key = hash_key;
    entry->score = score;
    entry->flags = hash_flag;
    entry->depth = depth;
}