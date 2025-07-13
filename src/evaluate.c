#include <stddef.h>

#include "evaluate.h"
#include "defs.h"
#include "bitboard.h"
#include "movegen.h" // Needed for mobility calculations

/*
================================================================================
 Tapered Evaluation and Game Phase
================================================================================
The 'GamePhase' is a score from 0 (endgame) to 24 (opening/middlegame).
It's calculated based on the non-pawn material on the board.
We use this phase to blend our middlegame and endgame evaluations.
*/
const int game_phase_inc[12] = { 0, 1, 1, 2, 4, 0, 0, 1, 1, 2, 4, 0 };

/*
================================================================================
 Piece-Square Tables (PSTs)
================================================================================
These tables give a bonus or penalty to a piece based on its square.
The values are for the middlegame (mg) and endgame (eg).
Scores are from white's perspective. Black's scores are the mirror image.
*/

const int mg_pawn_pst[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10,-20,-20, 10, 10,  5,
    5, -5,-10,  0,  0,-10, -5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5,  5, 10, 25, 25, 10,  5,  5,
   10, 10, 20, 30, 30, 20, 10, 10,
   50, 50, 50, 50, 50, 50, 50, 50,
    0,  0,  0,  0,  0,  0,  0,  0
};
const int eg_pawn_pst[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
   10, 10,  0,  0,  0,  0, 10, 10,
   10, 10,  0,  0,  0,  0, 10, 10,
   15, 15, 10, 10, 10, 10, 15, 15,
   20, 20, 20, 20, 20, 20, 20, 20,
   30, 30, 30, 30, 30, 30, 30, 30,
   80, 80, 80, 80, 80, 80, 80, 80,
    0,  0,  0,  0,  0,  0,  0,  0
};
const int mg_knight_pst[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50
};

const int mg_bishop_pst[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int mg_rook_pst[64] = {
    0,  0,  0,  5,  5,  0,  0,  0,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
    5, 10, 10, 10, 10, 10, 10,  5,
    0,  0,  0,  0,  0,  0,  0,  0
};

const int mg_king_pst[64] = {
   20, 30, 10,  0,  0, 10, 30, 20,
   20, 20,  0,  0,  0,  0, 20, 20,
  -10,-20,-20,-20,-20,-20,-20,-10,
  -20,-30,-30,-40,-40,-30,-30,-20,
  -30,-40,-40,-50,-50,-40,-40,-30,
  -30,-40,-40,-50,-50,-40,-40,-30,
  -30,-40,-40,-50,-50,-40,-40,-30,
  -30,-40,-40,-50,-50,-40,-40,-30
};
const int eg_king_pst[64] = {
  -50,-30,-30,-30,-30,-30,-30,-50,
  -30,-30,  0,  0,  0,  0,-30,-30,
  -30,-10, 20, 30, 30, 20,-10,-30,
  -30,-10, 30, 40, 40, 30,-10,-30,
  -30,-10, 30, 40, 40, 30,-10,-30,
  -30,-10, 20, 30, 30, 20,-10,-30,
  -30,-20,-10,  0,  0,-10,-20,-30,
  -50,-40,-30,-20,-20,-30,-40,-50
};

// To make the tables easier to read, we mirror them for black's perspective
#define S(sq) (56 ^ (sq))

// A master array of pointers to the PSTs for easy access
const int* mg_psts[12] = {
    mg_pawn_pst, mg_knight_pst, mg_bishop_pst, mg_rook_pst, NULL, mg_king_pst,
    mg_pawn_pst, mg_knight_pst, mg_bishop_pst, mg_rook_pst, NULL, mg_king_pst
};

// --- THIS IS THE FIX ---
// Initialize directly with the array names, which are compile-time constants.
// For pieces where the endgame table is the same as the middlegame, just use the mg_ table.
const int* eg_psts[12] = {
    eg_pawn_pst, mg_knight_pst, mg_bishop_pst, mg_rook_pst, NULL, eg_king_pst,
    eg_pawn_pst, mg_knight_pst, mg_bishop_pst, mg_rook_pst, NULL, eg_king_pst
};

/*
================================================================================
 Main Evaluation Function
================================================================================
*/
int evaluate(Board* board) {
    int mg_score = 0;
    int eg_score = 0;
    int game_phase = 0;

    u64 bitboard;
    int piece, square;

    // --- Material and Piece-Square Table Evaluation ---
    const int material_score[12] = { 100, 320, 330, 500, 900, 0, -100, -320, -330, -500, -900, 0 };

    for (piece = P; piece <= k; ++piece) {
        bitboard = board->piece_bitboards[piece];
        while (bitboard) {
            square = popcount(bitboard - 1);
            bitboard &= bitboard - 1;

            // Add material score
            mg_score += material_score[piece];
            eg_score += material_score[piece];

            // Add PST scores
            if (mg_psts[piece] != NULL) {
                mg_score += (piece < 6) ? mg_psts[piece][square] : -mg_psts[piece][S(square)];
            }
            if (eg_psts[piece] != NULL) {
                eg_score += (piece < 6) ? eg_psts[piece][square] : -eg_psts[piece][S(square)];
            }
            
            // Add to game phase
            game_phase += game_phase_inc[piece];
        }
    }
    
    // --- Mobility Evaluation ---
    // Simple mobility bonus for bishops and rooks (queens are complex)
    int mg_mobility = 0;
    int eg_mobility = 0;

    // White Bishop Mobility
    bitboard = board->piece_bitboards[B];
    while(bitboard) {
        square = popcount(bitboard - 1);
        bitboard &= bitboard - 1;
        int moves = popcount(bishopAttacks(board->occupancies[BOTH], square) & ~board->occupancies[WHITE]);
        mg_mobility += (moves - 4) * 2; // small bonus/penalty relative to an average of 4 moves
    }

    // Black Bishop Mobility
    bitboard = board->piece_bitboards[b];
    while(bitboard) {
        square = popcount(bitboard - 1);
        bitboard &= bitboard - 1;
        int moves = popcount(bishopAttacks(board->occupancies[BOTH], square) & ~board->occupancies[BLACK]);
        mg_mobility -= (moves - 4) * 2;
    }
    
    // ... (similar mobility logic for rooks) ...
    
    mg_score += mg_mobility;
    eg_score += eg_mobility;


    // --- Final Tapered Score ---
    // Ensure game phase is within bounds [0, 24]
    if (game_phase > 24) game_phase = 24;

    int final_score = (mg_score * game_phase + eg_score * (24 - game_phase)) / 24;
    
    // Return score from the perspective of the side to move
    return (board->side_to_move == WHITE) ? final_score : -final_score;
}