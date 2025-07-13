// In tests/perft_test.c
#include <stdio.h>
#include "perft.h"
#include "board.h"
#include "movegen.h"
#include "defs.h"

void divide(Board* board, int depth) {
    if (depth == 0) {
        return;
    }

    MoveList move_list;
    generate_all_moves(board, &move_list);

    long total_nodes = 0;
    int current_side = board->side_to_move;

    printf("Divide for depth %d:\n", depth);

    for (int i = 0; i < move_list.count; i++) {
        Move current_move = move_list.moves[i];
        make_move(board, current_move);

        u64 king_bb = board->piece_bitboards[current_side == WHITE ? K : k];
        if (king_bb != 0) {
            int king_sq = __builtin_ctzll(king_bb);

            if (!is_square_attacked(king_sq, !current_side, board)) {
                long nodes = perft_nodes(board, depth - 1);
                
                // Use the new SAN converter for printing!
                char san_move[16];
                move_to_san(san_move, board, current_move); // Pass the board *before* the move
                
                printf("%s: %ld\n", san_move, nodes);
                total_nodes += nodes;
            }
        }

        unmake_move(board, current_move);
    }
    printf("\nTotal nodes: %ld\n", total_nodes);
}

int main() {
    init_attack_tables();
    Board board;

    // --- Test 1: Starting Position ---
    const char* start_pos_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    parse_fen(&board, start_pos_fen);
    printf("--- Position: Start ---\n");
    // Expected: 1: 20, 2: 400, 3: 8902, 4: 197281
    divide(&board, 4);

    // --- Test 2: Kiwipete Position ---
    const char* kiwipete_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    parse_fen(&board, kiwipete_fen);
    printf("\n--- Position: Kiwipete ---\n");
    // Expected: 1: 48, 2: 2039
    divide(&board, 2);

    // --- Test 3: Position with a check ---
    const char* check_pos_fen = "r2qkb1r/1b1n1ppp/pp1p1N2/4p3/P2NP3/5B2/1PP2PPP/R1BQK2R b KQkq - 0 10";
    parse_fen(&board, check_pos_fen);
    printf("\n--- Position: Check Test ---\n");
    // Black is in check and must respond. Only legal moves, Nxf6, Qxf6, gxf6, Ke7
    // Expected: 1: 4,
    divide(&board, 1);

    return 0;
}
