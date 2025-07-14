// src/search.c

#include <stdio.h>
#include <stdlib.h> // for qsort
#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "board.h"
#include "transpose.h"

// --- Move Ordering ---
// Assigns a score to each move to help the search algorithm
// prioritize more promising moves.

// MVV-LVA (Most Valuable Victim - Least Valuable Attacker) scores
// The idea is to prioritize captures of high-value pieces by low-value pieces.
// Indexed by [victim][attacker]
const int mvv_lva_scores[12][12] = {
    // Victims: P, N, B, R, Q, K (for white and black)
    {105, 104, 103, 102, 101, 100}, // Attacker: P
    {205, 204, 203, 202, 201, 200}, // Attacker: N
    {305, 304, 303, 302, 301, 300}, // Attacker: B
    {405, 404, 403, 402, 401, 400}, // Attacker: R
    {505, 504, 503, 502, 501, 500}, // Attacker: Q
    {605, 604, 603, 602, 601, 600}, // Attacker: K
    // (Scores for black attackers are the same)
    {105, 104, 103, 102, 101, 100},
    {205, 204, 203, 202, 201, 200},
    {305, 304, 303, 302, 301, 300},
    {405, 404, 403, 402, 401, 400},
    {505, 504, 503, 502, 501, 500},
    {605, 604, 603, 602, 601, 600}
};

// We need to find the piece on a given square to score captures
static int get_piece_on_square(Board* board, int square) {
    for (int piece = P; piece <= k; ++piece) {
        if ((board->piece_bitboards[piece] >> square) & 1) {
            return piece;
        }
    }
    return -1;
}

static int compare_moves(const void* a, const void* b) {
    return ((Move*)b)->score - ((Move*)a)->score;
}

static void score_moves(Board* board, MoveList* move_list) {
    for (int i = 0; i < move_list->count; ++i) {
        int score = 0;
        Move* move = &move_list->moves[i];

        if (move->is_capture) {
            int victim = get_piece_on_square(board, move->to);
            if (victim != -1) {
                int attacker_idx = move->piece % 6;
                int victim_idx = victim % 6;
                score = mvv_lva_scores[victim_idx][attacker_idx] + 10000;
            }
        }
        move->score = score;
    }
    qsort(move_list->moves, move_list->count, sizeof(Move), compare_moves);
}

static int quiescence_search(Board* board, int alpha, int beta) {
    int stand_pat = evaluate(board);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList move_list;
    generate_all_moves(board, &move_list);
    score_moves(board, &move_list);

    int original_side = board->side_to_move;
    for (int i = 0; i < move_list.count; i++) {
        Move move = move_list.moves[i];
        if (!move.is_capture) continue;

        make_move(board, move);
        u64 king_bb = board->piece_bitboards[original_side == WHITE ? K : k];
        if (king_bb != 0) {
            int king_square = __builtin_ctzll(king_bb);
            if (!is_square_attacked(king_square, !original_side, board)) {
                int score = -quiescence_search(board, -beta, -alpha);
                if (score >= beta) {
                    unmake_move(board, move);
                    return beta;
                }
                if (score > alpha) alpha = score;
            }
        }
        unmake_move(board, move);
    }
    return alpha;
}


static int negamax(Board* board, int depth, int alpha, int beta, int is_null) {
    int hash_flag = HASH_FLAG_ALPHA;
    int score = probe_hash(board->hash_key, depth, alpha, beta);
    if (score != NO_HASH_ENTRY && !is_null) {
        return score;
    }

    if (depth == 0) {
        return quiescence_search(board, alpha, beta);
    }

    // --- Safe Null-Move Pruning ---
    u64 king_bb = board->piece_bitboards[board->side_to_move == WHITE ? K : k];
    if (!is_null && king_bb != 0) {
        int king_sq = __builtin_ctzll(king_bb);
        if (!is_square_attacked(king_sq, !board->side_to_move, board)) {
            // Manually update board state for the null move
            int original_ep_square = board->enpassant_square;
            board->ply++;
            board->side_to_move = !board->side_to_move;
            board->hash_key ^= side_key;
            if (board->enpassant_square != -1) {
                board->hash_key ^= enpassant_keys[board->enpassant_square];
            }
            board->enpassant_square = -1;
            
            score = -negamax(board, depth - 1 - 2, -beta, -beta + 1, 1);
            
            // Manually restore the board state
            board->ply--;
            board->side_to_move = !board->side_to_move;
            board->hash_key ^= side_key;
            board->enpassant_square = original_ep_square;
             if (board->enpassant_square != -1) {
                board->hash_key ^= enpassant_keys[board->enpassant_square];
            }

            if (score >= beta) {
                return beta;
            }
        }
    }

    MoveList move_list;
    generate_all_moves(board, &move_list);
    score_moves(board, &move_list);

    int moves_made = 0;
    int best_score = -INFINITY;
    int original_side = board->side_to_move;

    for (int i = 0; i < move_list.count; i++) {
        make_move(board, move_list.moves[i]);
        u64 current_king_bb = board->piece_bitboards[original_side == WHITE ? K : k];
        if (current_king_bb != 0) {
            int king_sq = __builtin_ctzll(current_king_bb);
            if (!is_square_attacked(king_sq, !original_side, board)) {
                moves_made++;
                if (moves_made > 4 && depth > 2 && !move_list.moves[i].is_capture) {
                    score = -negamax(board, depth - 2, -alpha -1, -alpha, 0);
                } else {
                    score = -negamax(board, depth - 1, -alpha -1, -alpha, 0);
                }

                if (score > alpha && score < beta) {
                     score = -negamax(board, depth - 1, -beta, -alpha, 0);
                }

                if (score > best_score) {
                    best_score = score;
                    if (best_score > alpha) {
                        alpha = best_score;
                        hash_flag = HASH_FLAG_EXACT;
                        if (alpha >= beta) {
                            unmake_move(board, move_list.moves[i]);
                            record_hash(board->hash_key, depth, beta, HASH_FLAG_BETA);
                            return beta;
                        }
                    }
                }
            }
        }
        unmake_move(board, move_list.moves[i]);
    }

    if (moves_made == 0) {
        int king_sq_final = __builtin_ctzll(board->piece_bitboards[original_side == WHITE ? K : k]);
        if (is_square_attacked(king_sq_final, !original_side, board)) {
            return -MATE_SCORE + board->ply;
        } else {
            return 0;
        }
    }

    record_hash(board->hash_key, depth, best_score, hash_flag);

    return best_score;
}

Move search_position(Board* board, int depth) {
    Move best_move = {0};
    int best_score = -INFINITY;
    int original_side = board->side_to_move;

    // Aspiration Windows
    int alpha = -INFINITY, beta = INFINITY;
    int delta = 25;

    for (int current_depth = 1; current_depth <= depth; ++current_depth) {
        best_score = negamax(board, current_depth, alpha, beta, 0);

        if (best_score <= alpha || best_score >= beta) {
            alpha = -INFINITY;
            beta = INFINITY;
            best_score = negamax(board, current_depth, alpha, beta, 0);
        }

        alpha = best_score - delta;
        beta = best_score + delta;
        
        MoveList move_list;
        generate_all_moves(board, &move_list);
        score_moves(board, &move_list);

        printf("info string searching depth %d\n", current_depth);
        for (int i = 0; i < move_list.count; i++) {
            Move current_move = move_list.moves[i];
            make_move(board, current_move);

            u64 king_bb = board->piece_bitboards[original_side == WHITE ? K : k];
            if (king_bb == 0) {
                unmake_move(board, current_move);

                continue;
            }
            int king_square = __builtin_ctzll(king_bb);
            if (is_square_attacked(king_square, !original_side, board)) {
                unmake_move(board, current_move);

                continue;
            }

            int score = -negamax(board, current_depth - 1, -INFINITY, INFINITY, 0);
            unmake_move(board, current_move);

            if (score > best_score) {
                best_score = score;
                best_move = current_move;
                char san_move[16];

                move_to_san(san_move, board, current_move);
                printf("info score cp %d move %s\n", score, san_move);
            }
        }
    }
    
    char san_best_move[16];
    move_to_san(san_best_move, board, best_move);
    printf("bestmove %s\n", san_best_move);
    return best_move;
}