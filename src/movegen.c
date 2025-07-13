#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h> // Needed for the PRNG

#include "board.h"
#include "bitboard.h"
#include "movegen.h"

// --- Piece attack tables ---
u64 pawn_attacks[2][64];
u64 knight_attacks[64];
u64 king_attacks[64];

// "Fancy" Magic Bitboard Data for Bishops and Rooks
SMagic mBishopTbl[64];
u64 bishop_attack_table[5248];

SMagic mRookTbl[64];
u64 rook_attack_table[102400];

// --- Constant Masks ---
const u64 not_a_file = 18374403900871474942ULL;
const u64 not_h_file = 9187201950435737471ULL;
const u64 not_hg_file = 4557430888798830399ULL;
const u64 not_ab_file = 18229723555195321596ULL;
const u64 board_edges = 0xFF818181818181FFULL;

// --- Helper Functions for Initialization ---

// Pseudo-Random Number Generator (PRNG) state
unsigned int prng_state;

void seed_prng(unsigned int seed) {
    prng_state = seed;
}

// 32-bit generator
unsigned int rand_prng() {
    prng_state ^= prng_state << 13;
    prng_state ^= prng_state >> 17;
    prng_state ^= prng_state << 5;

    return prng_state;
}

// Creates a full 64-bit random number.
u64 rand64_prng() {
    u64 r1 = (u64)rand_prng();
    u64 r2 = (u64)rand_prng();

    return r1 | (r2 << 32);
}

// Generates a random 64-bit number with few set bits using the 64-bit generator.
u64 sparse_rand_u64() {
    return rand64_prng() & rand64_prng() & rand64_prng();
}

// --- Sliding Piece Helper Functions (used only during init) ---

// Generates bishop attacks for a given square and blocker configuration.
u64 generate_bishop_attacks(int square, u64 blockers) {
    u64 attacks = 0ULL;
    int r, f, tr = square / 8, tf = square % 8;

    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) { attacks |= (1ULL << (r * 8 + f)); if ((1ULL << (r * 8 + f)) & blockers) break; }
    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) { attacks |= (1ULL << (r * 8 + f)); if ((1ULL << (r * 8 + f)) & blockers) break; }
    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) { attacks |= (1ULL << (r * 8 + f)); if ((1ULL << (r * 8 + f)) & blockers) break; }
    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) { attacks |= (1ULL << (r * 8 + f)); if ((1ULL << (r * 8 + f)) & blockers) break; }

    return attacks;
}

// Generates rook attacks for a given square and blocker config
u64 generate_rook_attacks(int square, u64 blockers) {
    u64 attacks = 0ULL;
    int r, f, tr = square / 8, tf = square % 8;

    for (r = tr + 1; r <= 7; r++) { attacks |= (1ULL << (r * 8 + tf)); if ((1ULL << (r * 8 + tf)) & blockers) break; }
    for (r = tr - 1; r >= 0; r--) { attacks |= (1ULL << (r * 8 + tf)); if ((1ULL << (r * 8 + tf)) & blockers) break; }
    for (f = tf + 1; f <= 7; f++) { attacks |= (1ULL << (tr * 8 + f)); if ((1ULL << (tr * 8 + f)) & blockers) break; }
    for (f = tf - 1; f >= 0; f--) { attacks |= (1ULL << (tr * 8 + f)); if ((1ULL << (tr * 8 + f)) & blockers) break; }

    return attacks;
}

// Generates the blocker mask for a bishop on a given square.
u64 generate_bishop_blocker_mask(int square) {
    return generate_bishop_attacks(square, 0ULL) & ~board_edges;
}

// Generates the blocker mask for a rook on a given square
u64 generate_rook_blocker_mask(int square) {
    u64 attacks = 0ULL;
    int r, f, tr = square / 8, tf = square % 8;

    // Only include the "inner" squares, not the very edges of the board
    for (r = tr + 1; r < 7; r++) attacks |= (1ULL << (r * 8 + tf));
    for (r = tr - 1; r > 0; r--) attacks |= (1ULL << (r * 8 + tf));
    for (f = tf + 1; f < 7; f++) attacks |= (1ULL << (tr * 8 + f));
    for (f = tf - 1; f > 0; f--) attacks |= (1ULL << (tr * 8 + f));

    return attacks;
}

// Magic Finding Initialization for Bishops
void init_magic_bishop_attacks() {
    u64* attack_table_ptr = bishop_attack_table;
    int seeds[] = {8977, 44560, 54343, 38998, 5731, 95205, 104912, 17020};
    
    for (int s = 0; s < 64; s++) {
        // Reset state for each square
        int epoch[4096] = {0};
        int cnt = 0;

        mBishopTbl[s].mask = generate_bishop_blocker_mask(s);
        mBishopTbl[s].ptr = attack_table_ptr;
        
        int relevant_bits = popcount(mBishopTbl[s].mask);
        mBishopTbl[s].shift = 64 - relevant_bits;
        int size = 1 << relevant_bits;
        
        u64 occupancy[4096];
        u64 reference[4096];
        
        u64 b = 0;
        for(int i = 0; i < size; i++) {
            occupancy[i] = b;
            reference[i] = generate_bishop_attacks(s, b);
            b = (b - mBishopTbl[s].mask) & mBishopTbl[s].mask;
        }

        seed_prng(seeds[s % 8]);
        
        while (1) {
            // Use the corrected 64-bit sparse random number generator
            mBishopTbl[s].magic = sparse_rand_u64();
            
            // Optimization: quickly discard bad magic candidates
            if (popcount((mBishopTbl[s].magic * mBishopTbl[s].mask) >> 56) < 6) continue;
            
            ++cnt; // Mark this attempt
            int i;

            // Test the magic number against all occupancies
            for (i = 0; i < size; ++i) {
                unsigned idx = (occupancy[i] * mBishopTbl[s].magic) >> mBishopTbl[s].shift;
                
                if (epoch[idx] < cnt) {
                    epoch[idx] = cnt;
                    mBishopTbl[s].ptr[idx] = reference[i];
                } else if (mBishopTbl[s].ptr[idx] != reference[i]) {
                    break; // Collision
                }
            }

            if (i == size) {
                break; // Found a good magic number
            }
        }
        
        attack_table_ptr += size;
    }
}

void init_magic_rook_attacks() {
    u64* attack_table_ptr = rook_attack_table;
    int seeds[] = {72120, 44560, 54343, 38998, 5731, 95205, 104912, 17020}; // Changed first seed slightly
    
    for (int s = 0; s < 64; s++) {
        int epoch[4096] = {0};
        int cnt = 0;

        mRookTbl[s].mask = generate_rook_blocker_mask(s);
        mRookTbl[s].ptr = attack_table_ptr;
        
        int relevant_bits = popcount(mRookTbl[s].mask);
        mRookTbl[s].shift = 64 - relevant_bits;
        int size = 1 << relevant_bits;
        
        u64 occupancy[4096];
        u64 reference[4096];
        
        u64 b = 0;
        for(int i = 0; i < size; i++) {
            occupancy[i] = b;
            reference[i] = generate_rook_attacks(s, b);
            b = (b - mRookTbl[s].mask) & mRookTbl[s].mask;
        }

        seed_prng(seeds[s % 8]);
        
        while (1) {
            mRookTbl[s].magic = sparse_rand_u64();
            if (popcount((mRookTbl[s].magic * mRookTbl[s].mask) >> 56) < 6) continue;
            
            ++cnt;
            int i;
            for (i = 0; i < size; ++i) {
                unsigned idx = (occupancy[i] * mRookTbl[s].magic) >> mRookTbl[s].shift;
                if (epoch[idx] < cnt) {
                    epoch[idx] = cnt;
                    mRookTbl[s].ptr[idx] = reference[i];
                } else if (mRookTbl[s].ptr[idx] != reference[i]) {
                    break;
                }
            }
            if (i == size) break;
        }
        attack_table_ptr += size;
    }
}

// Attack generation functions
void generate_pawn_attacks() {
    for (int square = 0; square < 64; square++) {
        u64 start_bitboard = 1ULL << square;
        u64 white_attacks = 0ULL;
        u64 black_attacks = 0ULL;

        if (((start_bitboard << 9) & not_a_file) != 0) white_attacks |= (start_bitboard << 9);
        if (((start_bitboard << 7) & not_h_file) != 0) white_attacks |= (start_bitboard << 7);
        if (((start_bitboard >> 9) & not_h_file) != 0) black_attacks |= (start_bitboard >> 9);
        if (((start_bitboard >> 7) & not_a_file) != 0) black_attacks |= (start_bitboard >> 7);

        pawn_attacks[0][square] = white_attacks;
        pawn_attacks[1][square] = black_attacks;
    }
}

void generate_knight_attacks() {
    for (int square = 0; square < 64; square++) {
        u64 start_bitboard = 1ULL << square;
        u64 attacks = 0ULL;

        if (((start_bitboard << 17) & not_a_file) != 0) attacks |= (start_bitboard << 17);
        if (((start_bitboard << 15) & not_h_file) != 0) attacks |= (start_bitboard << 15);
        if (((start_bitboard << 10) & not_ab_file) != 0) attacks |= (start_bitboard << 10);
        if (((start_bitboard << 6) & not_hg_file) != 0) attacks |= (start_bitboard << 6);
        if (((start_bitboard >> 17) & not_h_file) != 0) attacks |= (start_bitboard >> 17);
        if (((start_bitboard >> 15) & not_a_file) != 0) attacks |= (start_bitboard >> 15);
        if (((start_bitboard >> 10) & not_hg_file) != 0) attacks |= (start_bitboard >> 10);
        if (((start_bitboard >> 6) & not_ab_file) != 0) attacks |= (start_bitboard >> 6);

        knight_attacks[square] = attacks;
    }
}

void generate_king_attacks() {
    for (int square = 0; square < 64; square++) {
        u64 start_bitboard = 1ULL << square;
        u64 attacks = 0ULL;

        if (((start_bitboard << 9) & not_a_file) != 0) attacks |= (start_bitboard << 9);
        if (((start_bitboard << 8)) != 0) attacks |= (start_bitboard << 8);
        if (((start_bitboard << 7) & not_h_file) != 0) attacks |= (start_bitboard << 7);
        if (((start_bitboard << 1) & not_a_file) != 0) attacks |= (start_bitboard << 1);
        if (((start_bitboard >> 9) & not_h_file) != 0) attacks |= (start_bitboard >> 9);
        if (((start_bitboard >> 8)) != 0) attacks |= (start_bitboard >> 8);
        if (((start_bitboard >> 7) & not_a_file) != 0) attacks |= (start_bitboard >> 7);
        if (((start_bitboard >> 1) & not_h_file) != 0) attacks |= (start_bitboard >> 1);

        king_attacks[square] = attacks;
    }
}

// --- Lookup and Move Generation ---
u64 bishopAttacks(u64 occ, int sq) {
   occ &= mBishopTbl[sq].mask;
   occ *= mBishopTbl[sq].magic;
   occ >>= mBishopTbl[sq].shift;

   return mBishopTbl[sq].ptr[occ];
}

u64 rookAttacks(u64 occ, int sq) {
   occ &= mRookTbl[sq].mask;
   occ *= mRookTbl[sq].magic;
   occ >>= mRookTbl[sq].shift;

   return mRookTbl[sq].ptr[occ];
}

// Generate move functions

int is_square_attacked(int square, int side, const Board* board) {
    // Pawn
    if ((pawn_attacks[!side][square] & board->piece_bitboards[side == WHITE ? P : p])) return 1;

    // Knight
    if ((knight_attacks[square] & board->piece_bitboards[side == WHITE ? N : n])) return 1;

    // Bishop
    if (bishopAttacks(board->occupancies[2], square) & (board->piece_bitboards[side == WHITE ? B : b] | board->piece_bitboards[side == WHITE ? Q : q])) return 1;

    // Rook
    if (rookAttacks(board->occupancies[2], square) & (board->piece_bitboards[side == WHITE ? R : r] | board->piece_bitboards[side == WHITE ? Q : q])) return 1;
    
    // King
    if ((king_attacks[square] & board->piece_bitboards[side == WHITE ? K : k])) return 1;

    return 0;
}

void generate_all_pawn_moves(const Board* board, MoveList* move_list) {
    int side = board->side_to_move;
    u64 my_pawns = board->piece_bitboards[side == WHITE ? P : p];
    u64 enemy_pieces = board->occupancies[!side];
    u64 all_pieces = board->occupancies[BOTH];

    int from_square, to_square;

    // White promotes on rank 8, Black promotes on rank 1.
    u64 promotion_rank = (side == WHITE) ? 0xFF00000000000000ULL : 0x00000000000000FFULL;

    // --- 1. Pawn Pushes and Promotions ---
    u64 single_pushes = (side == WHITE) ? (my_pawns << 8) & ~all_pieces : (my_pawns >> 8) & ~all_pieces;
    u64 rank_for_double_push = (side == WHITE) ? 0x000000000000FF00ULL : 0x00FF000000000000ULL;
    u64 pawns_on_start_rank = my_pawns & rank_for_double_push;
    u64 double_push_targets = (side == WHITE) ? (pawns_on_start_rank << 16) : (pawns_on_start_rank >> 16);
    u64 double_pushes = (side == WHITE) ? ((single_pushes & (rank_for_double_push << 8)) << 8) & ~all_pieces : ((single_pushes & (rank_for_double_push >> 8)) >> 8) & ~all_pieces;


    u64 quiet_pushes = single_pushes & ~promotion_rank;
    while (quiet_pushes) {
        to_square = __builtin_ctzll(quiet_pushes);
        from_square = (side == WHITE) ? to_square - 8 : to_square + 8;
        move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), 0, 0, 0, 0};
        quiet_pushes &= quiet_pushes - 1;
    }

    u64 promo_pushes = single_pushes & promotion_rank;
    while (promo_pushes) {
        to_square = __builtin_ctzll(promo_pushes);
        from_square = (side == WHITE) ? to_square - 8 : to_square + 8;
        move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), (side == WHITE ? Q : q), 0, 0, 0};
        move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), (side == WHITE ? R : r), 0, 0, 0};
        move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), (side == WHITE ? B : b), 0, 0, 0};
        move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), (side == WHITE ? N : n), 0, 0, 0};
        promo_pushes &= promo_pushes - 1;
    }

    while (double_pushes) {
        to_square = __builtin_ctzll(double_pushes);
        from_square = (side == WHITE) ? to_square - 16 : to_square + 16;
        move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), 0, 0, 0, 0};
        double_pushes &= double_pushes - 1;
    }

    // --- 2. Pawn Captures ---
    u64 pawns_to_capture_from = my_pawns;
    while(pawns_to_capture_from) {
        from_square = __builtin_ctzll(pawns_to_capture_from);
        u64 attacks = pawn_attacks[side][from_square] & enemy_pieces;

        u64 capture_promos = attacks & promotion_rank;
        while(capture_promos) {
            to_square = __builtin_ctzll(capture_promos);
            move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), (side == WHITE ? Q : q), 1, 0, 0};
            move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), (side == WHITE ? R : r), 1, 0, 0};
            move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), (side == WHITE ? B : b), 1, 0, 0};
            move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), (side == WHITE ? N : n), 1, 0, 0};
            capture_promos &= capture_promos - 1;
        }

        u64 normal_captures = attacks & ~promotion_rank;
        while(normal_captures) {
            to_square = __builtin_ctzll(normal_captures);
            move_list->moves[move_list->count++] = (Move){from_square, to_square, (side == WHITE ? P : p), 0, 1, 0, 0};
            normal_captures &= normal_captures - 1;
        }

        // --- 3. En Passant ---
        if (board->enpassant_square != -1) {
            u64 ep_attack = pawn_attacks[side][from_square] & (1ULL << board->enpassant_square);
            if (ep_attack) {
                move_list->moves[move_list->count++] = (Move){from_square, board->enpassant_square, (side == WHITE ? P : p), 0, 1, 1, 0};
            }
        }
        pawns_to_capture_from &= pawns_to_capture_from - 1;
    }
}


void generate_all_knight_moves(const Board* board, MoveList* move_list) {
    int side = board->side_to_move;
    u64 friendlies = board->occupancies[side];
    u64 enemies = board->occupancies[!side];
    u64 knights = board->piece_bitboards[side == WHITE ? N : n];

     while (knights) {
        int from_square = __builtin_ctzll(knights);
        u64 attacks = knight_attacks[from_square];
        u64 valid_moves = attacks & ~friendlies;
        
        while (valid_moves) {
            int to_square = __builtin_ctzll(valid_moves);
            move_list->moves[move_list->count++] = (Move){ .from = from_square, .to = to_square, .piece = (side == WHITE ? N : n), .is_capture = (enemies & (1ULL << to_square)) ? 1 : 0, 0, 0 };
            valid_moves &= valid_moves - 1;
        }

        knights &= knights - 1;
    }
}

void generate_all_bishop_moves(const Board* board, MoveList* move_list) {
    int side = board->side_to_move;
    u64 friendly_pieces = board->occupancies[side];
    u64 bishops = board->piece_bitboards[side == WHITE ? B : b];

    while (bishops) {
        int from_square = __builtin_ctzll(bishops);
        u64 attacks = bishopAttacks(board->occupancies[2], from_square);
        u64 valid_moves = attacks & ~friendly_pieces;

        while (valid_moves) {
            int to_square = __builtin_ctzll(valid_moves);
            move_list->moves[move_list->count++] = (Move){ .from = from_square, .to = to_square, .piece = (side == WHITE ? B : b), .is_capture = (board->occupancies[!side] & (1ULL << to_square)) ? 1 : 0, 0, 0 };
           
            valid_moves &= valid_moves - 1;
        }

        bishops &= bishops - 1;
    }
}

void generate_all_rook_moves(const Board* board, MoveList* move_list) {
    int side = board->side_to_move;
    u64 friendly_pieces = board->occupancies[side];
    // Get the bitboard for the rooks of the current side
    u64 rooks = board->piece_bitboards[side == WHITE ? R : r];

    // Loop through each rook
    while (rooks) {
        int from_square = __builtin_ctzll(rooks);
        // Get the attacks for that square using the rook magic lookup
        u64 attacks = rookAttacks(board->occupancies[2], from_square);
        // Filter out moves that land on friendly pieces
        u64 valid_moves = attacks & ~friendly_pieces;

        // Loop through the valid destination squares
        while (valid_moves) {
            int to_square = __builtin_ctzll(valid_moves);
            // Create and add the move to the move list
            move_list->moves[move_list->count++] = (Move){ .from = from_square, .to = to_square, .piece = (side == WHITE ? R : r), .is_capture = (board->occupancies[!side] & (1ULL << to_square)) ? 1 : 0, 0, 0 };
            // Clear the 'to' bit to continue the loop
            valid_moves &= valid_moves - 1;
        }
        // Clear the 'from' bit to continue the outer loop
        rooks &= rooks - 1;
    }
}

void generate_all_queen_moves(const Board* board, MoveList* move_list) {
    int side = board->side_to_move;
    u64 friendly_pieces = board->occupancies[side];
    // Get the bitboard for the queens of the current side
    u64 queens = board->piece_bitboards[side == WHITE ? Q : q];

    // Loop through each queen
    while (queens) {
        int from_square = __builtin_ctzll(queens);

        // Get bishop and rook attacks from the same square and combine them
        u64 bishop_attacks = bishopAttacks(board->occupancies[2], from_square);
        u64 rook_attacks = rookAttacks(board->occupancies[2], from_square);
        u64 attacks = bishop_attacks | rook_attacks;

        // Filter out moves that land on friendly pieces
        u64 valid_moves = attacks & ~friendly_pieces;

        // Loop through the valid destination squares
        while (valid_moves) {
            int to_square = __builtin_ctzll(valid_moves);
            // Create and add the move to the move list
            move_list->moves[move_list->count++] = (Move){ .from = from_square, .to = to_square, .piece = (side == WHITE ? Q : q), .is_capture = (board->occupancies[!side] & (1ULL << to_square)) ? 1 : 0, 0, 0 };
            // Clear the 'to' bit to continue the loop
            valid_moves &= valid_moves - 1;
        }
        // Clear the 'from' bit to continue the outer loop
        queens &= queens - 1;
    }
}

void generate_all_king_moves(const Board* board, MoveList* move_list) {
    int side = board->side_to_move;
    u64 friendly_pieces = board->occupancies[side];
    int from_square = __builtin_ctzll(board->piece_bitboards[side == WHITE ? K : k]);
    u64 attacks = king_attacks[from_square];
    u64 valid_moves = attacks & ~friendly_pieces;

    while (valid_moves) {
        int to_square = __builtin_ctzll(valid_moves);
        move_list->moves[move_list->count++] = (Move){ .from = from_square, .to = to_square, .piece = (side == WHITE ? K : k), .is_capture = (board->occupancies[!side] & (1ULL << to_square)) ? 1 : 0, 0, 0 };
        valid_moves &= valid_moves - 1;
    }

    if (is_square_attacked(from_square, !side, board)) {
        return; // King in check, no castling allowed
    }

    if (side == WHITE) {
        if ((board->castling_rights & WK) && !((board->occupancies[2] >> (f1)) & 1) && !((board->occupancies[2] >> (g1)) & 1)) {
            if (!is_square_attacked(e1, BLACK, board) && !is_square_attacked(f1, BLACK, board) && !is_square_attacked(g1, BLACK, board)) {
                move_list->moves[move_list->count++] = (Move){ .from = e1, .to = g1, .piece = K, .is_castle = 1 };
            }
        }
        if ((board->castling_rights & WQ) && !((board->occupancies[2] >> (d1)) & 1) && !((board->occupancies[2] >> (c1)) & 1) && !((board->occupancies[2] >> (b1)) & 1)) {
            if (!is_square_attacked(e1, BLACK, board) && !is_square_attacked(d1, BLACK, board) && !is_square_attacked(c1, BLACK, board)) {
                move_list->moves[move_list->count++] = (Move){ .from = e1, .to = c1, .piece = K, .is_castle = 1 };
            }
        }
    } else {
        if ((board->castling_rights & BK) && !((board->occupancies[2] >> (f8)) & 1) && !((board->occupancies[2] >> (g8)) & 1)) {
            if (!is_square_attacked(e8, WHITE, board) && !is_square_attacked(f8, WHITE, board) && !is_square_attacked(g8, WHITE, board)) {
                move_list->moves[move_list->count++] = (Move){ .from = e8, .to = g8, .piece = k, .is_castle = 1 };
            }
        }
        if ((board->castling_rights & BQ) && !((board->occupancies[2] >> (d8)) & 1) && !((board->occupancies[2] >> (c8)) & 1) && !((board->occupancies[2] >> (b8)) & 1)) {
            if (!is_square_attacked(e8, WHITE, board) && !is_square_attacked(d8, WHITE, board) && !is_square_attacked(c8, WHITE, board)) {
                move_list->moves[move_list->count++] = (Move){ .from = e8, .to = c8, .piece = k, .is_castle = 1 };
            }
        }
    }
}

void generate_all_moves(const Board* board, MoveList* move_list) {
    move_list->count = 0; // Reset move count
    
    generate_all_pawn_moves(board, move_list);
    generate_all_knight_moves(board, move_list);
    generate_all_bishop_moves(board, move_list);
    generate_all_rook_moves(board, move_list);
    generate_all_queen_moves(board, move_list);
    generate_all_king_moves(board, move_list);
}

// --- Main Initialization Entry Point ---
void init_attack_tables() {
    generate_pawn_attacks();
    generate_knight_attacks();
    generate_king_attacks();
    init_magic_bishop_attacks();
    init_magic_rook_attacks();
}
