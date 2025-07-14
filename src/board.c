#include <stdlib.h> // For abs()
#include <string.h> // For strtok, strcpy, etc.
#include <ctype.h>  // For toupper()

#include "board.h"
#include "movegen.h" 

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

const char* square_to_algebraic[] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

// Helper array to map piece enum to a character. Note: No leading space.
const char piece_to_char[] = "PNBRQKpnbrqk";

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

    board->piece_bitboards[piece] |= sq_bb;
    board->occupancies[side] |= sq_bb;
    board->occupancies[BOTH] |= sq_bb;
}

static void remove_piece(Board* board, int square, int piece) {
    u64 sq_bb = 1ULL << square;
    int side = (piece < 6) ? WHITE : BLACK;

    board->piece_bitboards[piece] &= ~sq_bb;
    board->occupancies[side] &= ~sq_bb;
    board->occupancies[BOTH] &= ~sq_bb;
}

// --- Main Functions ---
void make_move(Board* board, Move move) {
    board->history[board->ply].castling_rights = board->castling_rights;
    board->history[board->ply].enpassant_square = board->enpassant_square;
    board->history[board->ply].captured_piece = -1;

    // Handle captures before moving the piece to avoid overwriting the target square.
    if (move.is_capture) {
        if (move.is_enpassant) {
            int captured_pawn_sq = (board->side_to_move == WHITE) ? move.to - 8 : move.to + 8;
            int captured_pawn = (board->side_to_move == WHITE) ? p : P;

            remove_piece(board, captured_pawn_sq, captured_pawn);
            board->history[board->ply].captured_piece = captured_pawn;
        } 
        else {
            // Find and remove the piece on the destination square
            int captured_start = (board->side_to_move == WHITE) ? p : P;
            int captured_end = (board->side_to_move == WHITE) ? k : K;

            for (int piece = captured_start; piece <= captured_end; piece++) {
                if ((1ULL << move.to) & board->piece_bitboards[piece]) {
                    remove_piece(board, move.to, piece);
                    board->history[board->ply].captured_piece = piece;

                    break;
                }
            }
        }
    }
    
    // Now move the piece
    move_piece(board, move.from, move.to, move.piece);

    // Update castling rights
    board->castling_rights &= castling_rights_update[move.from];
    board->castling_rights &= castling_rights_update[move.to];

    // Update en-passant square, resetting it first
    board->enpassant_square = -1;
    if (move.piece == P || move.piece == p) {
        if (abs(move.from - move.to) == 16) {
            board->enpassant_square = (board->side_to_move == WHITE) ? move.to - 8 : move.to + 8;
        }
    }
    
    // Handle other special move types
    if (move.promotion) {
        remove_piece(board, move.to, move.piece); // Remove the pawn
        add_piece(board, move.to, move.promotion); // Add the new piece
    }
    else if (move.is_castle) {
        switch (move.to) {
            case g1: move_piece(board, h1, f1, R); break;
            case c1: move_piece(board, a1, d1, R); break;
            case g8: move_piece(board, h8, f8, r); break;
            case c8: move_piece(board, a8, d8, r); break;
        }
    }

    // Update side to move and ply
    board->side_to_move = !board->side_to_move;
    board->ply++;
}

void unmake_move(Board* board, Move move) {
    board->ply--;
    UndoInfo undo = board->history[board->ply];

    board->side_to_move = !board->side_to_move;
    board->castling_rights = undo.castling_rights;
    board->enpassant_square = undo.enpassant_square;

    // Determine which piece to move back (handles promotions)
    int piece_that_moved = move.promotion ? move.promotion : move.piece;
    move_piece(board, move.to, move.from, piece_that_moved);

    // If it was a promotion, revert the piece type
    if (move.promotion) {
        remove_piece(board, move.from, move.promotion);
        add_piece(board, move.from, move.piece);
    }

    // Un-do castling by moving the rook back
    if (move.is_castle) {
        switch (move.to) {
            case g1: move_piece(board, f1, h1, R); break;
            case c1: move_piece(board, d1, a1, R); break;
            case g8: move_piece(board, f8, h8, r); break;
            case c8: move_piece(board, d8, a8, r); break;
        }
    }
    
    // Add back any captured piece LAST
    if (undo.captured_piece != -1) {
        int captured_sq = move.to;

        if (move.is_enpassant) {
            captured_sq = (board->side_to_move == WHITE) ? move.to - 8 : move.to + 8;
        }
        add_piece(board, captured_sq, undo.captured_piece);
    }
}

void parse_fen(Board* board, const char* fen) {
    memset(board->piece_bitboards, 0, sizeof(board->piece_bitboards));
    memset(board->occupancies, 0, sizeof(board->occupancies));

    board->side_to_move = 0;
    board->enpassant_square = -1;
    board->castling_rights = 0;
    board->ply = 0;
    
    char fen_copy[256];
    strncpy(fen_copy, fen, 255);
    fen_copy[255] = '\0';

    char* token = strtok(fen_copy, " ");
    int rank = 7, file = 0;

    for (size_t i = 0; i < strlen(token); i++) {
        char c = token[i];
        if (c == '/') { 
            rank--; file = 0; 
        }
        else if (c >= '1' && c <= '8') {
            file += c - '0'; 
        }
        else {
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

    token = strtok(NULL, " ");
    board->side_to_move = (strcmp(token, "w") == 0) ? WHITE : BLACK;

    token = strtok(NULL, " ");
    for (size_t i = 0; i < strlen(token); i++) {
        switch (token[i]) {
            case 'K': board->castling_rights |= WK; break;
            case 'Q': board->castling_rights |= WQ; break;
            case 'k': board->castling_rights |= BK; break;
            case 'q': board->castling_rights |= BQ; break;
        }
    }

    token = strtok(NULL, " ");
    if (strcmp(token, "-") != 0) {
        board->enpassant_square = (token[0] - 'a') + (token[1] - '1') * 8;
    }

    for (int piece = P; piece <= K; piece++) board->occupancies[WHITE] |= board->piece_bitboards[piece];
    for (int piece = p; piece <= k; piece++) board->occupancies[BLACK] |= board->piece_bitboards[piece];
    board->occupancies[BOTH] = board->occupancies[WHITE] | board->occupancies[BLACK];
}

void move_to_san(char* san_string, Board* board, Move move) {
    if (move.is_castle) {
        if (move.to > move.from) strcpy(san_string, "O-O");
        else strcpy(san_string, "O-O-O");
    } else {
        char to_str[3];
        strcpy(to_str, square_to_algebraic[move.to]);
        san_string[0] = '\0';

        if (move.piece != P && move.piece != p) {
            char piece_ch[2] = { toupper(piece_to_char[move.piece]), '\0' };
            strcat(san_string, piece_ch);
        } else if (move.is_capture) {
            char from_file[2] = { square_to_algebraic[move.from][0], '\0' };
            strcat(san_string, from_file);
        }

        // Disambiguation logic (simplified for now)
        if (move.piece != P && move.piece != p) {
            MoveList all_moves;
            generate_all_moves(board, &all_moves);
            int file_ambiguous = 0, rank_ambiguous = 0, needs_disambiguation = 0;
            for (int i = 0; i < all_moves.count; i++) {
                Move other = all_moves.moves[i];
                if (other.from != move.from && other.to == move.to && other.piece == move.piece) {
                    needs_disambiguation = 1;
                    if ((other.from % 8) == (move.from % 8)) rank_ambiguous = 1;
                    if ((other.from / 8) == (move.from / 8)) file_ambiguous = 1;
                }
            }
            if (needs_disambiguation) {
                if (file_ambiguous && rank_ambiguous) {
                    strcat(san_string, square_to_algebraic[move.from]);
                } else if (file_ambiguous) {
                    char from_rank[2] = { square_to_algebraic[move.from][1], '\0' };
                    strcat(san_string, from_rank);
                } else { // Default to file if ambiguous at all
                    char from_file[2] = { square_to_algebraic[move.from][0], '\0' };
                    strcat(san_string, from_file);
                }
            }
        }
        
        if (move.is_capture) strcat(san_string, "x");
        strcat(san_string, to_str);

        if (move.promotion) {
            char promo_ch[3] = {'=', toupper(piece_to_char[move.promotion]), '\0'};
            strcat(san_string, promo_ch);
        }
    }

    // --- Check and Checkmate Detection ---
    Board board_after_move = *board;
    make_move(&board_after_move, move);
    
    int opponent_side = board_after_move.side_to_move;
    int opponent_king_piece = (opponent_side == WHITE) ? K : k;
    u64 opponent_king_bb = board_after_move.piece_bitboards[opponent_king_piece];

    // Make sure the king is still on the board (wasn't a bugged capture)
    if (opponent_king_bb == 0) return;

    int opponent_king_sq = __builtin_ctzll(opponent_king_bb);

    // Is the opponent in check?
    if (is_square_attacked(opponent_king_sq, !opponent_side, &board_after_move)) {
        // To check for mate, we see if the opponent has any legal moves.
        MoveList opponent_moves;
        generate_all_moves(&board_after_move, &opponent_moves);
        int has_legal_move = 0;

        for (int i = 0; i < opponent_moves.count; ++i) {
            Board board_after_reply = board_after_move;
            make_move(&board_after_reply, opponent_moves.moves[i]);

            int reply_king_sq = __builtin_ctzll(board_after_reply.piece_bitboards[opponent_king_piece]);
            
            // If the king is NOT attacked after the reply, it's a legal move.
            if (!is_square_attacked(reply_king_sq, !opponent_side, &board_after_reply)) {
                has_legal_move = 1;
                break; // Found a legal move, so it's not mate.
            }
        }

        if (has_legal_move) {
            strcat(san_string, "+"); // It's just a check
        } else {
            strcat(san_string, "#"); // No legal moves, it's checkmate
        }
    }
}