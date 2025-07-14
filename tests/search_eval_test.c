// tests/search_eval_test.c

#include <stdio.h>
#include "board.h"
#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "transpose.h"

void run_test(const char* fen, int depth) {
    printf("\n--- Testing Position ---\n");
    printf("FEN: %s\n", fen);

    Board board;
    parse_fen(&board, fen);

    search_position(&board, depth);
    printf("------------------------\n");
}

int main() {
    init_evaluation_masks();
    init_zobrist_keys();
    init_attack_tables();
    init_transposition_table();

    // --- Test 1: Starting Position ---
    const char* start_pos_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    run_test(start_pos_fen, 7);

    // --- Test 2: Kiwipete Position (Complex tactical position) ---
    // A good test to see if the engine can navigate tactical complexities.
    // Best move according to the top engines for white would be Bxa6
    const char* kiwipete_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    run_test(kiwipete_fen, 4);

    // --- Test 3: Simple Mate-in-1 ---
    // The engine should find Qxf2#.
    // NOTE: Only finds Qxf2# on a depth of 2 for some reason
    const char* mate_in_1_fen = "r1b1k1nr/pppp1ppp/5q2/N1b5/4P3/8/PPP2PPP/RNBQKB1R w KQkq - 2 6";
    run_test(mate_in_1_fen, 2);


    // --- Test 4: Simple Endgame ---
    const char* endgame_fen = "8/k7/p7/P1p5/2P5/8/1K6/8 w - - 0 1";
    run_test(endgame_fen, 13);

    return 0;
}