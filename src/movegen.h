#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include "defs.h"
#include "board.h"

// --- "Fancy" Magic Bitboard Struct ---
typedef struct {
    u64* ptr;      // Pointer to the attack table for this square
    u64 mask;     // Mask for relevant blocker squares
    u64 magic;    // The 64-bit magic number
    int shift;    // The right-shift value
} SMagic;

// --- Pre-computed Attack Tables ---
extern u64 pawn_attacks[2][64];
extern u64 knight_attacks[64];
extern u64 king_attacks[64];

// Declaration for the bishop magic table
extern SMagic mBishopTbl[64];
extern SMagic mRookTbl[64];

// PRNG
void seed_prng(unsigned int seed);
u64 rand64_prng();

// Initialization function
void init_attack_tables();

// Get bishop attacks using the "fancy" magic bitboard approach
u64 bishopAttacks(u64 occ, int sq);
u64 rookAttacks(u64 occ, int sq);

// Real move generation functions
int is_square_attacked(int square, int side, const Board* board);
void generate_all_pawn_moves(const Board* board, MoveList* move_list);
void generate_all_knight_moves(const Board* board, MoveList* move_list);
void generate_all_bishop_moves(const Board* board, MoveList* move_list);
void generate_all_rook_moves(const Board* board, MoveList* move_list);
void generate_all_queen_moves(const Board* board, MoveList* move_list);
void generate_all_king_moves(const Board* board, MoveList* move_list);
void generate_all_moves(const Board* board, MoveList* move_list);

#endif // MOVEGEN_H