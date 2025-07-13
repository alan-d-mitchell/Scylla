#include <stdlib.h> // For abs()
#include <string.h> // For parse_fen

#include "board.h"

// This array is used to efficiently update castling rights during make_move.
const int castling_rights_update[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11,
};

// --- Helper Functions ---
static void move_piece(Board* board, int from, int to, int piece) {
    u64 from_to_bb = (1ULL << from) | (1ULL << to);
    int side = (piece < 6) ? WHITE : BLACK;

    board->piece_bitboards[piece] ^= from_to_bb;
    board->occupancies[side] ^= from_to_bb;
    board->occupancies[BOTH] ^= from_to_bb;
}

static void add_piece(Board* board, int square, int piece) {
    u64 sq_bb = 1ULL << square;
    int side = (piece < 6) ? WHITE : BLACK;

    board->piece_bitboards[piece] ^= sq_bb;
    board->occupancies[side] ^= sq_bb;
    board->occupancies[BOTH] ^= sq_bb;
}

// --- Main Functions ---
void make_move(Board* board, Move move) {
    board->history[board->ply].castling_rights = board->castling_rights;
    board->history[board->ply].enpassant_square = board->enpassant_square;
    board->history[board->ply].captured_piece = -1;

    board->castling_rights &= castling_rights_update[move.from];
    board->castling_rights &= castling_rights_update[move.to];

    board->enpassant_square = -1;

    if (move.is_capture) {
        int captured_start = (board->side_to_move == WHITE) ? p : P;
        int captured_end = (board->side_to_move == WHITE) ? k : K;

        for (int p = captured_start; p <= captured_end; p++) {
            if ((1ULL << move.to) & board->piece_bitboards[p]) {
                board->piece_bitboards[p] ^= (1ULL << move.to);
                board->occupancies[!board->side_to_move] ^= (1ULL << move.to);
                board->occupancies[BOTH] ^= (1ULL << move.to);
                board->history[board->ply].captured_piece = p;

                break;
            }
        }
    }

    if (move.piece == P || move.piece == p) {
        if (abs(move.from - move.to) == 16) {
            board->enpassant_square = (board->side_to_move == WHITE) ? move.to - 8 : move.to + 8;
        } else if (move.is_enpassant) {
            int captured_pawn_sq = (board->side_to_move == WHITE) ? move.to - 8 : move.to + 8;
            int captured_pawn = (board->side_to_move == WHITE) ? p : P;

            board->piece_bitboards[captured_pawn] ^= (1ULL << captured_pawn_sq);
            board->occupancies[!board->side_to_move] ^= (1ULL << captured_pawn_sq);
            board->occupancies[BOTH] ^= (1ULL << captured_pawn_sq);
            board->history[board->ply].captured_piece = captured_pawn;
        }
    }

    move_piece(board, move.from, move.to, move.piece);

    if (move.promotion != -1) {
        board->piece_bitboards[move.piece] ^= (1ULL << move.to);
        board->piece_bitboards[move.promotion] ^= (1ULL << move.to);
    }
    
    if (move.is_castle) {
        switch (move.to) {
            case g1: move_piece(board, h1, f1, R); break;
            case c1: move_piece(board, a1, d1, R); break;
            case g8: move_piece(board, h8, f8, r); break;
            case c8: move_piece(board, a8, d8, r); break;
        }
    }

    board->side_to_move = !board->side_to_move;
    board->ply++;
}

void unmake_move(Board* board, Move move) {
    board->ply--;
    UndoInfo undo = board->history[board->ply];

    board->side_to_move = !board->side_to_move;
    board->castling_rights = undo.castling_rights;
    board->enpassant_square = undo.enpassant_square;

    if (move.is_castle) {
        switch (move.to) {
            case g1: move_piece(board, f1, h1, R); break;
            case c1: move_piece(board, d1, a1, R); break;
            case g8: move_piece(board, f8, h8, r); break;
            case c8: move_piece(board, d8, a8, r); break;
        }
    }

    if (move.promotion != -1) {
        board->piece_bitboards[move.promotion] ^= (1ULL << move.to);
        board->piece_bitboards[move.piece] ^= (1ULL << move.to);
    }
    
    move_piece(board, move.to, move.from, move.piece);

    if (undo.captured_piece != -1) {
        int captured_sq = move.to;

        if (move.is_enpassant) {
            captured_sq = (board->side_to_move == WHITE) ? move.to - 8 : move.to + 8;
        }
        add_piece(board, captured_sq, undo.captured_piece);
    }
}

void parse_fen(Board* board, const char* fen) {
    // Reset board state
    memset(board->piece_bitboards, 0, sizeof(board->piece_bitboards));
    memset(board->occupancies, 0, sizeof(board->occupancies));
    board->side_to_move = 0;
    board->enpassant_square = -1;
    board->castling_rights = 0;
    
    // Use a mutable copy of the FEN string for strtok
    char fen_copy[256];
    strncpy(fen_copy, fen, 255);
    fen_copy[255] = '\0';

    char* token;

    // 1. Piece Placement
    token = strtok(fen_copy, " ");
    int rank = 7;
    int file = 0;
    for (size_t i = 0; i < strlen(token); i++) {
        char c = token[i];
        if (c == '/') {
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
        } else {
            int square = rank * 8 + file;
            int piece_type = -1;
            switch(c) {
                case 'P': piece_type=P; break; case 'N': piece_type=N; break;
                case 'B': piece_type=B; break; case 'R': piece_type=R; break;
                case 'Q': piece_type=Q; break; case 'K': piece_type=K; break;
                case 'p': piece_type=p; break; case 'n': piece_type=n; break;
                case 'b': piece_type=b; break; case 'r': piece_type=r; break;
                case 'q': piece_type=q; break; case 'k': piece_type=k; break;
            }
            if (piece_type != -1) {
                set_bit(&board->piece_bitboards[piece_type], square, 1);
            }
            file++;
        }
    }

    // 2. Side to Move
    token = strtok(NULL, " ");
    board->side_to_move = (strcmp(token, "w") == 0) ? WHITE : BLACK;

    // 3. Castling Rights
    token = strtok(NULL, " ");
    for (size_t i = 0; i < strlen(token); i++) {
        switch (token[i]) {
            case 'K': board->castling_rights |= WK; break;
            case 'Q': board->castling_rights |= WQ; break;
            case 'k': board->castling_rights |= BK; break;
            case 'q': board->castling_rights |= BQ; break;
        }
    }

    // 4. En Passant Square
    token = strtok(NULL, " ");
    if (strcmp(token, "-") != 0) {
        int ep_file = token[0] - 'a';
        int ep_rank = token[1] - '1';
        board->enpassant_square = ep_rank * 8 + ep_file;
    }

    // Populate occupancy bitboards
    for (int piece = P; piece <= K; piece++) board->occupancies[WHITE] |= board->piece_bitboards[piece];
    for (int piece = p; piece <= k; piece++) board->occupancies[BLACK] |= board->piece_bitboards[piece];
    board->occupancies[2] = board->occupancies[WHITE] | board->occupancies[BLACK];
}