// src/evaluate.c

#include "evaluate.h"
#include "defs.h"
#include "bitboard.h"
#include "movegen.h"

/*
================================================================================
 Stockfish 11 Tuned Values
================================================================================
The following values are translated directly from the Stockfish 11 source code.
They are the result of countless hours of testing and tuning by the Stockfish team.
The evaluation is split into a middlegame (mg) and endgame (eg) score, which
are then blended together based on the game phase.
*/

// --- Material Values ---
// Note: King value is not used for material, but is important for other calculations.
const Score material_score[12] = {
    { 128, 213 }, { 781, 854 }, { 825, 915 }, { 1276, 1380 }, { 2538, 2682 }, { 0, 0 },
    { 128, 213 }, { 781, 854 }, { 825, 915 }, { 1276, 1380 }, { 2538, 2682 }, { 0, 0 }
};

// --- Mobility Bonus Tables ---
const Score knight_mobility[9] = {
    {-62,-79},{-53,-53},{-12,-31},{-4,-12},{3,8},{12,23},{21,34},{28,45},{39,55}
};
const Score bishop_mobility[14] = {
    {-48,-59},{-20,-24},{16,-11},{40,1},{62,17},{78,33},{91,45},{100,56},{110,66},{122,76},{126,84},{133,90},{144,96},{150,100}
};
const Score rook_mobility[15] = {
    {-58,-76},{-27,-18},{1,20},{22,53},{41,80},{54,103},{63,119},{72,133},{82,148},{88,159},{98,168},{108,177},{113,184},{122,191},{128,196}
};
const Score queen_mobility[28] = {
    {-39,-53},{-21,-27},{3, -1},{19,20},{40,43},{55,64},{68,80},{82,96},{93,109},{104,121},{116,133},{125,145},{133,156},{140,166},{150,175},
    {159,185},{168,194},{176,203},{185,211},{194,219},{202,226},{210,233},{218,240},{225,246},{232,252},{239,258},{246,264},{252,269}
};

// --- Piece-Square Tables (PSTs) ---
// These tables give a score to each piece based on its position on the board.
// Scores are from White's perspective. Black's scores are the mirror image.

// To make the tables easier to read, we use a macro to mirror the square for Black.
#define S(sq) (56 ^ (sq))

const Score pawn_pst[64] = {
    {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
    {9,15},{13,15},{13,15},{13,15},{13,15},{13,15},{13,15},{9,15},
    {-2,5},{-5,5},{-5,5},{-5,5},{-5,5},{-5,5},{-5,5},{-2,5},
    {-7,-5},{-9,-5},{-9,-5},{-9,-5},{-9,-5},{-9,-5},{-9,-5},{-7,-5},
    {-7,-10},{-9,-10},{-9,-10},{-9,-10},{-9,-10},{-9,-10},{-9,-10},{-7,-10},
    {13,-14},{10,-14},{10,-14},{10,-14},{10,-14},{10,-14},{10,-14},{13,-14},
    {29,25},{34,25},{34,25},{34,25},{34,25},{34,25},{34,25},{29,25},
    {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}
};

const Score knight_pst[64] = {
    {-204,-100},{-111,-80},{-88,-60},{-77,-50},{-77,-50},{-88,-60},{-111,-80},{-204,-100},
    {-98,-80},{-48,-60},{-34,-40},{-15,-30},{-15,-30},{-34,-40},{-48,-60},{-98,-80},
    {-72,-60},{-17,-40},{-4,-20},{10,-10},{10,-10},{-4,-20},{-17,-40},{-72,-60},
    {-55,-50},{-1,-30},{22,-10},{38,0},{38,0},{22,-10},{-1,-30},{-55,-50},
    {-55,-50},{11,-30},{38,-10},{55,0},{55,0},{38,-10},{11,-30},{-55,-50},
    {-72,-60},{1,-40},{18,-20},{30,-10},{30,-10},{18,-20},{1,-40},{-72,-60},
    {-98,-80},{-40,-60},{-27,-40},{-15,-30},{-15,-30},{-27,-40},{-40,-60},{-98,-80},
    {-204,-100},{-111,-80},{-88,-60},{-77,-50},{-77,-50},{-88,-60},{-111,-80},{-204,-100}
};

const Score bishop_pst[64] = {
    {-52,-50},{-15,-40},{-20,-30},{-13,-20},{-13,-20},{-20,-30},{-15,-40},{-52,-50},
    {-15,-40},{1, -20},{8, -10},{10,0},{10,0},{8, -10},{1, -20},{-15,-40},
    {-20,-30},{8, -10},{18,0},{24,10},{24,10},{18,0},{8, -10},{-20,-30},
    {-13,-20},{10,0},{24,10},{33,20},{33,20},{24,10},{10,0},{-13,-20},
    {-13,-20},{10,0},{24,10},{33,20},{33,20},{24,10},{10,0},{-13,-20},
    {-20,-30},{8, -10},{18,0},{24,10},{24,10},{18,0},{8, -10},{-20,-30},
    {-15,-40},{1, -20},{8, -10},{10,0},{10,0},{8, -10},{1, -20},{-15,-40},
    {-52,-50},{-15,-40},{-20,-30},{-13,-20},{-13,-20},{-20,-30},{-15,-40},{-52,-50}
};

const Score rook_pst[64] = {
    {-31,-10},{-21,0},{-18,5},{-12,10},{-12,10},{-18,5},{-21,0},{-31,-10},
    {-21, -10},{-13,0},{-10,5},{-1, 10},{-1, 10},{-10,5},{-13,0},{-21,-10},
    {-21, -10},{-13,0},{-10,5},{-1, 10},{-1, 10},{-10,5},{-13,0},{-21,-10},
    {-21, -10},{-13,0},{-10,5},{-1, 10},{-1, 10},{-10,5},{-13,0},{-21,-10},
    {-21, -10},{-13,0},{-10,5},{-1, 10},{-1, 10},{-10,5},{-13,0},{-21,-10},
    {-21, -10},{-13,0},{-10,5},{-1, 10},{-1, 10},{-10,5},{-13,0},{-21,-10},
    {1, -10},{10,0},{13,5},{18,10},{18,10},{13,5},{10,0},{1, -10},
    {-2, -10},{-2,0},{-2,5},{5,10},{5,10},{-2,5},{-2,0},{-2,-10}
};

const Score queen_pst[64] = {
    {3,-50},{-2,-40},{-1,-30},{0,-20},{0,-20},{-1,-30},{-2,-40},{3,-50},
    {-2,-40},{4,-20},{5,-10},{6,0},{6,0},{5,-10},{4,-20},{-2,-40},
    {-1,-30},{5,-10},{7,0},{8,10},{8,10},{7,0},{5,-10},{-1,-30},
    {0,-20},{6,0},{8,10},{10,20},{10,20},{8,10},{6,0},{0,-20},
    {0,-20},{6,0},{8,10},{10,20},{10,20},{8,10},{6,0},{0,-20},
    {-1,-30},{5,-10},{7,0},{8,10},{8,10},{7,0},{5,-10},{-1,-30},
    {-2,-40},{4,-20},{5,-10},{6,0},{6,0},{5,-10},{4,-20},{-2,-40},
    {3,-50},{-2,-40},{-1,-30},{0,-20},{0,-20},{-1,-30},{-2,-40},{3,-50}
};

const Score king_pst[64] = {
    {271,0},{327,50},{271,80},{198,100},{198,100},{271,80},{327,50},{271,0},
    {278,50},{303,100},{256,130},{195,150},{195,150},{256,130},{303,100},{278,50},
    {195,80},{252,130},{169,160},{120,180},{120,180},{169,160},{252,130},{195,80},
    {169,100},{190,150},{131,180},{78,200},{78,200},{131,180},{190,150},{169,100},
    {169,100},{190,150},{131,180},{78,200},{78,200},{131,180},{190,150},{169,100},
    {195,80},{252,130},{169,160},{120,180},{120,180},{169,160},{252,130},{195,80},
    {278,50},{303,100},{256,130},{195,150},{195,150},{256,130},{303,100},{278,50},
    {271,0},{327,50},{271,80},{198,100},{198,100},{271,80},{327,50},{271,0}
};

const Score* psts[12] = {
    pawn_pst, knight_pst, bishop_pst, rook_pst, queen_pst, king_pst,
    pawn_pst, knight_pst, bishop_pst, rook_pst, queen_pst, king_pst
};

const int game_phase_inc[] = {0, 1, 1, 2, 4, 0};

// These help us quickly identify passed, isolated, and doubled pawns.
u64 file_masks[8];
u64 adjacent_file_masks[8];
u64 passed_pawn_masks[2][64]; // [color][square]


void init_evaluation_masks() {
    for (int f = 0; f < 8; ++f) {
        file_masks[f] = 0x0101010101010101ULL << f;
        adjacent_file_masks[f] = 0;

        if (f > 0) adjacent_file_masks[f] |= file_masks[f - 1];
        if (f < 7) adjacent_file_masks[f] |= file_masks[f + 1];
    }

    for (int sq = 0; sq < 64; ++sq) {
        int rank = sq / 8;
        int file = sq % 8;

        u64 ahead_mask_white = 0;
        u64 ahead_mask_black = 0;
        for (int r = rank + 1; r < 8; ++r) ahead_mask_white |= (0xFFULL << (r * 8));
        for (int r = rank - 1; r >= 0; --r) ahead_mask_black |= (0xFFULL << (r * 8));


        passed_pawn_masks[WHITE][sq] = adjacent_file_masks[file] | file_masks[file];
        passed_pawn_masks[BLACK][sq] = adjacent_file_masks[file] | file_masks[file];
    }
}

int evaluate(Board* board) {
    int mg_score = 0;
    int eg_score = 0;
    int game_phase = 0;
    u64 bitboard;
    int square;

    // --- Material and PST Evaluation (Unchanged) ---
    for (int piece = P; piece <= k; ++piece) {
        bitboard = board->piece_bitboards[piece];
        int piece_type = (piece < 6) ? piece : piece - 6;
        game_phase += popcount(bitboard) * game_phase_inc[piece_type];

        while (bitboard) {
            square = __builtin_ctzll(bitboard);
            if (piece < 6) { // White
                mg_score += material_score[piece].mg + psts[piece][square].mg;
                eg_score += material_score[piece].eg + psts[piece][square].eg;
            } else { // Black
                mg_score -= material_score[piece].mg + psts[piece][S(square)].mg;
                eg_score -= material_score[piece].eg + psts[piece][S(square)].eg;
            }
            bitboard &= bitboard - 1;
        }
    }

    u64 all_pieces = board->occupancies[BOTH];
    // White Mobility
    bitboard = board->piece_bitboards[N]; while(bitboard) { square = __builtin_ctzll(bitboard); int moves = popcount(knight_attacks[square] & ~board->occupancies[WHITE]); mg_score += knight_mobility[moves].mg; eg_score += knight_mobility[moves].eg; bitboard &= bitboard-1; }
    bitboard = board->piece_bitboards[B]; while(bitboard) { square = __builtin_ctzll(bitboard); int moves = popcount(bishopAttacks(all_pieces, square) & ~board->occupancies[WHITE]); mg_score += bishop_mobility[moves].mg; eg_score += bishop_mobility[moves].eg; bitboard &= bitboard-1; }
    bitboard = board->piece_bitboards[R]; while(bitboard) { square = __builtin_ctzll(bitboard); int moves = popcount(rookAttacks(all_pieces, square) & ~board->occupancies[WHITE]); mg_score += rook_mobility[moves].mg; eg_score += rook_mobility[moves].eg; bitboard &= bitboard-1; }
    bitboard = board->piece_bitboards[Q]; while(bitboard) { square = __builtin_ctzll(bitboard); int moves = popcount((bishopAttacks(all_pieces, square) | rookAttacks(all_pieces, square)) & ~board->occupancies[WHITE]); mg_score += queen_mobility[moves].mg; eg_score += queen_mobility[moves].eg; bitboard &= bitboard-1; }
    // Black Mobility
    bitboard = board->piece_bitboards[n]; while(bitboard) { square = __builtin_ctzll(bitboard); int moves = popcount(knight_attacks[square] & ~board->occupancies[BLACK]); mg_score -= knight_mobility[moves].mg; eg_score -= knight_mobility[moves].eg; bitboard &= bitboard-1; }
    bitboard = board->piece_bitboards[b]; while(bitboard) { square = __builtin_ctzll(bitboard); int moves = popcount(bishopAttacks(all_pieces, square) & ~board->occupancies[BLACK]); mg_score -= bishop_mobility[moves].mg; eg_score -= bishop_mobility[moves].eg; bitboard &= bitboard-1; }
    bitboard = board->piece_bitboards[r]; while(bitboard) { square = __builtin_ctzll(bitboard); int moves = popcount(rookAttacks(all_pieces, square) & ~board->occupancies[BLACK]); mg_score -= rook_mobility[moves].mg; eg_score -= rook_mobility[moves].eg; bitboard &= bitboard-1; }
    bitboard = board->piece_bitboards[q]; while(bitboard) { square = __builtin_ctzll(bitboard); int moves = popcount((bishopAttacks(all_pieces, square) | rookAttacks(all_pieces, square)) & ~board->occupancies[BLACK]); mg_score -= queen_mobility[moves].mg; eg_score -= queen_mobility[moves].eg; bitboard &= bitboard-1; }

    // --- NEW: Pawn Structure Evaluation ---
    // (A simplified version for clarity)
    u64 white_pawns = board->piece_bitboards[P];
    u64 black_pawns = board->piece_bitboards[p];
    bitboard = white_pawns;
    
    while (bitboard) {
        square = __builtin_ctzll(bitboard);
        int file = square % 8;
        // Passed pawn check
        if ((passed_pawn_masks[WHITE][square] & black_pawns) == 0) {
            mg_score += 10; eg_score += 20; // Bonus for passed pawn
        }
        // Doubled pawn check
        if (popcount(white_pawns & file_masks[file]) > 1) {
            mg_score -= 10; eg_score -= 10; // Penalty for doubled pawn
        }
        // Isolated pawn check
        if ((adjacent_file_masks[file] & white_pawns) == 0) {
            mg_score -= 10; eg_score -= 10; // Penalty for isolated pawn
        }
        bitboard &= bitboard - 1;
    }
    // (Repeat for black pawns)
    bitboard = black_pawns;

    while(bitboard) {
        square = __builtin_ctzll(bitboard);
        int file = square % 8;

        if ((passed_pawn_masks[BLACK][square] & white_pawns) == 0) { mg_score -= 10; eg_score -= 20; }
        if (popcount(black_pawns & file_masks[file]) > 1) { mg_score += 10; eg_score += 10; }
        if ((adjacent_file_masks[file] & white_pawns) == 0) { mg_score += 10; eg_score += 10; }

        bitboard &= bitboard - 1;
    }

    // --- Final Tapered Score Calculation ---
    if (game_phase > 24) game_phase = 24;
    int final_score = (mg_score * game_phase + eg_score * (24 - game_phase)) / 24;
    return (board->side_to_move == WHITE) ? final_score : -final_score;
}