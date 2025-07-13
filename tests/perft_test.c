// perft_test.c

#include <stdio.h>

#include "perft.h"
#include "board.h"
#include "movegen.h"
#include "defs.h"

int main() {
    init_attack_tables();
    Board board;

    // // Test 1: Starting Position
    // const char* start_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    // parse_fen(&board, start_pos);

    // printf("\n--- Position: Start ---\n");
    // printf("Depth 1: %-10ld (Expected: 20)\n", perft_nodes(&board, 1));
    // printf("Depth 2: %-10ld (Expected: 400)\n", perft_nodes(&board, 2));
    // printf("Depth 3: %-10ld (Expected: 8902)\n", perft_nodes(&board, 3));

    // Test 2: Kiwipete (a complex middle-game position)
    const char* kiwipete_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    parse_fen(&board, kiwipete_fen);

    printf("\n--- Position: Kiwipete ---\n");
    printf("Depth 1: %-10ld (Expected: 48)\n", perft_nodes(&board, 1));
    printf("Depth 2: %-10ld (Expected: 2039)\n", perft_nodes(&board, 2));

    //  // Test 3: Position with promotions and en-passant
    // const char* pos3_fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
    // parse_fen(&board, pos3_fen);

    // printf("\n--- Position: 3 ---\n");
    // printf("Depth 1: %-10ld (Expected: 44)\n", perft_nodes(&board, 1));
    // printf("Depth 2: %-10ld (Expected: 1486)\n", perft_nodes(&board, 2));
}