// src/search.c

#include <stdio.h>
#include <stdlib.h> // for qsort
#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "board.h"

// --- NEW: Move Ordering ---
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
    return -1; // Should not happen in a valid position
}

// Comparison function for qsort to sort moves by score
static int compare_moves(const void* a, const void* b) {
    Move* moveA = (Move*)a;
    Move* moveB = (Move*)b;
    // We want to sort in descending order
    return moveB->score - moveA->score;
}

static void score_moves(Board* board, MoveList* move_list) {
    for (int i = 0; i < move_list->count; ++i) {
        int score = 0;
        Move* move = &move_list->moves[i];

        if (move->is_capture) {
            int victim = get_piece_on_square(board, move->to);
            // Ensure victim is valid and of the opponent's color
            if (victim != -1) {
                // Normalize piece indices for the mvv_lva_scores table
                int attacker_idx = move->piece % 6;
                int victim_idx = victim % 6;
                score = mvv_lva_scores[victim_idx][attacker_idx] + 10000; // Add a large bonus for any capture
            }
        }
        // In a more advanced engine, we would add scores for killer moves,
        // history heuristic, etc. here. For now, we just prioritize captures.

        move->score = score;
    }
    // Sort the move list based on the scores we just assigned
    qsort(move_list->moves, move_list->count, sizeof(Move), compare_moves);
}

// Forward declarations
static int quiescence_search(Board* board, int alpha, int beta);
static int negamax(Board* board, int depth, int alpha, int beta);

// --- Search Functions (largely the same, but now use the sorted move list) ---

static int quiescence_search(Board* board, int alpha, int beta) {
    int stand_pat = evaluate(board);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    MoveList move_list;
    generate_all_moves(board, &move_list);
    score_moves(board, &move_list); // Score and sort moves

    int original_side = board->side_to_move;
    for (int i = 0; i < move_list.count; i++) {
        Move move = move_list.moves[i];
        if (!move.is_capture) continue; // Only consider captures

        make_move(board, move);
        int king_square = __builtin_ctzll(board->piece_bitboards[original_side == WHITE ? K : k]);
        if (!is_square_attacked(king_square, !original_side, board)) {
            int score = -quiescence_search(board, -beta, -alpha);
            unmake_move(board, move);
            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        } else {
            unmake_move(board, move);
        }
    }
    return alpha;
}

static int negamax(Board* board, int depth, int alpha, int beta) {
    if (depth == 0) return quiescence_search(board, alpha, beta);

    MoveList move_list;
    generate_all_moves(board, &move_list);
    score_moves(board, &move_list); // Score and sort moves

    int moves_made = 0;
    int best_score = -INFINITY;
    int original_side = board->side_to_move;
    for (int i = 0; i < move_list.count; i++) {
        make_move(board, move_list.moves[i]);
        int king_square = __builtin_ctzll(board->piece_bitboards[original_side == WHITE ? K : k]);
        if (!is_square_attacked(king_square, !original_side, board)) {
            moves_made++;
            int score = -negamax(board, depth - 1, -beta, -alpha);
            unmake_move(board, move_list.moves[i]);

            if (score > best_score) best_score = score;
            if (best_score > alpha) alpha = best_score;
            if (alpha >= beta) return beta;
        } else {
            unmake_move(board, move_list.moves[i]);
        }
    }

    if (moves_made == 0) {
        int king_square = __builtin_ctzll(board->piece_bitboards[original_side == WHITE ? K : k]);
        if (is_square_attacked(king_square, !original_side, board)) {
            return -MATE_SCORE + board->ply;
        } else {
            return 0;
        }
    }
    return best_score;
}

Move search_position(Board* board, int depth) {
    Move best_move = {0};
    int best_score = -INFINITY;
    int original_side = board->side_to_move;

    MoveList move_list;
    generate_all_moves(board, &move_list);
    score_moves(board, &move_list); // Score and sort root moves

    printf("info string searching depth %d\n", depth);
    for (int i = 0; i < move_list.count; i++) {
        Move current_move = move_list.moves[i];
        make_move(board, current_move);
        int king_square = __builtin_ctzll(board->piece_bitboards[original_side == WHITE ? K : k]);
        if (is_square_attacked(king_square, !original_side, board)) {
            unmake_move(board, current_move);
            continue;
        }

        int score = -negamax(board, depth - 1, -INFINITY, INFINITY);
        unmake_move(board, current_move);

        if (score > best_score) {
            best_score = score;
            best_move = current_move;
            char san_move[16];
            move_to_san(san_move, board, current_move);
            printf("info score cp %d move %s\n", score, san_move);
        }
    }
    
    char san_best_move[16];
    move_to_san(san_best_move, board, best_move);
    printf("bestmove %s\n", san_best_move);
    return best_move;
}