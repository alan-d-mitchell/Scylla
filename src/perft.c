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

        // Find king of side that just moved
        int king_sq = __builtin_ctzll(board->piece_bitboards[current_side == WHITE ? K : k]);

        // If king not attacked by opponent piece, move == legal
        if (!is_square_attacked(king_sq, !current_side, board)) {
            nodes += perft_nodes(board, depth - 1);
        }

        unmake_move(board, move_list.moves[i]);
    }

    return nodes;
}