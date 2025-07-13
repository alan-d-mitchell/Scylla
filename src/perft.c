// perft.c

#include <stdio.h>

#include "perft.h"
#include "board.h"
#include "movegen.h"
#include "defs.h"

long perft_nodes(Board* board, int depth) {
    if (depth == 0) {
        return 1L;
    }

    MoveList move_list;
    generate_all_moves(board, &move_list);

    long nodes = 0;
    int current_side = board->side_to_move;

    for (int i = 0; i < move_list.count; i++) {
        make_move(board, move_list.moves[i]);

        // Find king bitboard
        u64 king_bb = board->piece_bitboards[current_side == WHITE ? K : k];

        // Fix seg fault related to move leaving king in check
        // which sets the king bb to 0 because of the 
        // __builtin_ctzll function which returns the width
        // of the u64 type, which would be 64, this would then
        // get passed to the is_square_attacked() function 
        // which calls the bishopAttacks() function resulting 
        // in the out bounds error, causing the seg fault
        if (king_bb == 0) {
            unmake_move(board, move_list.moves[i]); // Backtrack

            continue; // Skip to next move
        }

        // Find king of side that just moved
        int king_sq = __builtin_ctzll(king_bb);

        // If king not attacked by opponent piece, move == legal
        if (!is_square_attacked(king_sq, !current_side, board)) {
            nodes += perft_nodes(board, depth - 1);
        }

        unmake_move(board, move_list.moves[i]);
    }

    return nodes;
}