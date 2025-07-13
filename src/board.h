#ifndef BOARD_H
#define BOARD_H

#include "defs.h"
#include "bitboard.h"

// --- Structs ---
// Holds the information needed to undo a move
typedef struct {
    int captured_piece;
    int enpassant_square;
    int castling_rights;
} UndoInfo;

// The main board struct
typedef struct {
    u64 piece_bitboards[12];
    u64 occupancies[3];
    int side_to_move;
    int enpassant_square;
    int castling_rights;
    int ply;
    UndoInfo history[256];
} Board;

// --- Function Prototypes ---
void make_move(Board* board, Move move);
void unmake_move(Board* board, Move move);
void parse_fen(Board* board, const char* fen);

#endif // BOARD_H