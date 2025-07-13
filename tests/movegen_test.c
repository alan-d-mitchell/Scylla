// tests/movegen_test.c
// A tester file for testing the pre-computed attack table and masks

#include <stdio.h>

#include "board.h"
#include "movegen.h"
#include "defs.h"

void print_test(const char* title, u64 bitboard) {
    printf("--- %s ---\n", title);

    print_bitboard(bitboard);
}

// Helper array to map squares on the board
const char* square_to_algebraic[] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

// Helper array to map piece enum to a character for printing promotions
const char piece_to_char[] = "PNBRQKpnbrqk";

void print_move_list(const MoveList* move_list) {
    printf("Found %d moves:\n", move_list->count);
    for (int i = 0; i < move_list->count; i++) {
        Move move = move_list->moves[i];
        
        // If the move is a promotion
        if (move.promotion) {
            printf("  %s%s%s=%c\n", 
                   square_to_algebraic[move.from],
                   move.is_capture ? "x" : "-",
                   square_to_algebraic[move.to],
                   piece_to_char[move.promotion]);
        } else { // For all other moves
            printf("  %s%s%s\n", 
                   square_to_algebraic[move.from],
                   move.is_capture ? "x" : "-",
                   square_to_algebraic[move.to]);
        }
    }
}

 // The w after the main string with slashes can be changed to a b in order to see the black moves
 // Most tests use the kiwipete fen string

 // Should return 29 moves for white and 16 for black
void pawn_promotion_test() {
    const char* fen = "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/8/P1P1P1PP/RNBQKBNR b KQkq e6 0 1"; // This string demonstrates possible moves for pawn promotions
    Board board;
    parse_fen(&board, fen);

    MoveList move_list = { .count = 0 };
    generate_all_pawn_moves(&board, &move_list);

    printf("Testing pawn promotion moves from FEN: %s\n", fen);
    print_move_list(&move_list);

}

// Should return 8 moves for both
void pawn_move_test() {
    const char* fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1";
    Board board;
    parse_fen(&board, fen);

    MoveList move_list = { .count = 0};
    generate_all_pawn_moves(&board, &move_list);

    printf("Testing pawn moves from FEN: %s\n", fen);
    print_move_list(&move_list);
}

// Should return 11 for white and 8 for black
void knight_move_test() {
    const char* fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    Board board;
    parse_fen(&board, fen);

    MoveList move_list = { .count = 0 };

    generate_all_knight_moves(&board, &move_list);

    printf("Testing all knight moves from FEN: %s\n", fen);
    print_move_list(&move_list);
}

// Should return 11 for white and 6 for black
void bishop_move_test() {
    const char* fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    Board board;
    parse_fen(&board, fen);

    MoveList move_list = { .count = 0 };

    generate_all_bishop_moves(&board, &move_list);

    printf("Testing bishop moves from FEN: %s\n", fen);
    print_move_list(&move_list);
}

// Should return 5 for both
void rook_move_test() {
    const char* fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    Board board;
    parse_fen(&board, fen);

    MoveList move_list = { .count = 0};

    generate_all_rook_moves(&board, &move_list);

    printf("Testing rook moves from FEN: %s\n", fen);
    print_move_list(&move_list);
}

// Should return 9 for white and 4 for black
void queen_move_test() {
    const char* fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    Board board;
    parse_fen(&board, fen);

    MoveList move_list = { .count = 0};

    generate_all_queen_moves(&board, &move_list);

    printf("Testing queen moves from FEN: %s\n", fen);
    print_move_list(&move_list);
}

// Should return 7 moves for white and 
void king_castling_test() {
    // FEN designed for testing castling
    const char* fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    Board board;
    parse_fen(&board, fen);

    MoveList move_list = { .count = 0 };
    generate_all_king_moves(&board, &move_list);

    printf("Testing castling for king moves from FEN: %s\n", fen);
    print_move_list(&move_list);
}

// Should return 4 moves for white
void king_move_test() {
    const char* fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    Board board;
    parse_fen(&board, fen);

    MoveList move_list = { .count = 0 };
    generate_all_king_moves(&board, &move_list);

    printf("Testing king moves from FEN: %s\n", fen);
    print_move_list(&move_list);
}

int main() {
    init_attack_tables();

    pawn_promotion_test();
    pawn_move_test();
    knight_move_test();
    bishop_move_test();
    rook_move_test();
    queen_move_test();
    king_move_test();
    king_castling_test();

    return 0;
}