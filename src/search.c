#include <stdio.h>

#include "search.h"
#include "evaluate.h"
#include "movegen.h"

/**
 * @brief The core negamax search function with alpha-beta pruning.
 * @param board Pointer to the current board state.
 * @param depth Remaining depth to search.
 * @param alpha The lower bound of the search window.
 * @param beta The upper bound of the search window.
 * @return The score of the position from the current player's perspective.
 */
static int negamax(Board* board, int depth, int alpha, int beta) {
    // Base case: if we've reached the desired depth, return the static evaluation.
    if (depth == 0) {
        return evaluate(board);
    }

    MoveList move_list;
    generate_all_moves(board, &move_list);

    int moves_made = 0;
    int best_score = -INFINITY;
    int original_side = board->side_to_move;

    for (int i = 0; i < move_list.count; i++) {
        // Make the move on the board
        make_move(board, move_list.moves[i]);

        // Check if the move was legal (did not leave the king in check)
        int king_square = __builtin_ctzll(board->piece_bitboards[original_side == WHITE ? K : k]);
        if (!is_square_attacked(king_square, !original_side, board)) {
            moves_made++;
            // Recursive call with negated alpha/beta for the other player
            int score = -negamax(board, depth - 1, -beta, -alpha);
            
            // Undo the move to restore the board state
            unmake_move(board, move_list.moves[i]);

            // Found a new best move for this node
            if (score > best_score) {
                best_score = score;
            }
            
            // Alpha update (we found a better move for ourselves)
            if (best_score > alpha) {
                alpha = best_score;
            }

            // Beta cutoff (the opponent has a better move earlier in the tree)
            if (alpha >= beta) {
                return beta; // Prune this branch
            }
        } else {
            // If the move was illegal, just undo it and continue
            unmake_move(board, move_list.moves[i]);
        }
    }

    // Handle checkmate or stalemate
    if (moves_made == 0) {
        int king_square = __builtin_ctzll(board->piece_bitboards[original_side == WHITE ? K : k]);
        if (is_square_attacked(king_square, !original_side, board)) {
            return -INFINITY + board->ply; // Checkmate (add ply to find shorter mates)
        } else {
            return 0; // Stalemate
        }
    }

    return best_score;
}

/**
 * @brief Root search function to find the best move.
 */
Move search_position(Board* board, int depth) {
    Move best_move = {0};
    int best_score = -INFINITY;
    int original_side = board->side_to_move;

    MoveList move_list;
    generate_all_moves(board, &move_list);

    // Iterate through moves at the root of the search tree
    for (int i = 0; i < move_list.count; i++) {
        make_move(board, move_list.moves[i]);

        int king_square = __builtin_ctzll(board->piece_bitboards[original_side == WHITE ? K : k]);
        if (is_square_attacked(king_square, !original_side, board)) {
            unmake_move(board, move_list.moves[i]);
            continue; // Skip illegal moves
        }

        int score = -negamax(board, depth - 1, -INFINITY, INFINITY);
        
        unmake_move(board, move_list.moves[i]);

        // If this move gives a better score, update our best move
        if (score > best_score) {
            best_score = score;
            best_move = move_list.moves[i];
            printf("info score cp %d move ", score);
            // You'll need a print_move function here to see the output
            // print_move_algebraic(best_move); 
            printf("\n");
        }
    }

    return best_move;
}