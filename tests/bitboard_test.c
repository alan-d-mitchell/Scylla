// tests/bitboard_test.c
// A tester for file for bitboard representation of the board

#include <stdio.h>

#include "board.h"
#include "bitboard.h"

void test_starting_position() {
    printf("--- Standard Starting Position ---\n");

    const char* starting_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board board;
    parse_fen(&board, starting_fen);

    printf("All Occupied Squares: \n");
    print_bitboard(board.occupancies[2]);
}

void test_pawns_only() {
    printf("\n--- Pawns Only Position ---\n");

    const char* pawns_fen = "8/pppppppp/8/8/8/8/PPPPPPPP/8 w - - 0 1";
    Board board;
    parse_fen(&board, pawns_fen);

    printf("White pawns: \n");
    print_bitboard(board.piece_bitboards[P]);

    printf("Black pawns: \n");
    print_bitboard(board.piece_bitboards[p]);

    printf("All pawns: \n");
    print_bitboard(board.occupancies[2]);
}

void test_knights_bishops_only() {
    printf("\n--- Knights and Bishops Only ---\n");

    // FEN: Black and white knights and bishops on starting square
    const char* specific_fen = "1nb2bn1/8/8/8/8/8/8/1NB2BN1 w - - 0 1";
    Board board;
    parse_fen(&board, specific_fen);

    printf("White Knights (N):\n");
    print_bitboard(board.piece_bitboards[N]);

    printf("White Bishops (B):\n");
    print_bitboard(board.piece_bitboards[B]);

    printf("Black Knights (n):\n");
    print_bitboard(board.piece_bitboards[n]);
    
    printf("All Occupied Squares:\n");
    print_bitboard(board.occupancies[2]);
}

void test_rooks_only() {
    printf("\n--- Rooks Only ---\n");

    const char *rooks_fen = "r6r/8/8/8/8/8/8/R6R w - - 0 1";
    Board board;
    parse_fen(&board, rooks_fen);

    printf("White Rooks (R):\n");
    print_bitboard(board.piece_bitboards[R]);

    printf("Black Rooks (r):\n");
    print_bitboard(board.piece_bitboards[r]);
    
    printf("All Occupied Squares:\n");
    print_bitboard(board.occupancies[2]);
}

void test_queens_only() {
    printf("\n--- Queens Only ---\n");

    const char *queens_fen = "3q4/8/8/8/8/8/8/3Q4 w - - 0 1";
    Board board;
    parse_fen(&board, queens_fen);

    printf("White Queens (Q):\n");
    print_bitboard(board.piece_bitboards[Q]);

    printf("Black Queens (q):\n");
    print_bitboard(board.piece_bitboards[q]);
    
    printf("All Occupied Squares:\n");
    print_bitboard(board.occupancies[2]);
}

void test_kings_only() {
    printf("\n--- Kings Only ---\n");

    const char *kings_fen = "4k3/8/8/8/8/8/8/4K3 w - - 0 1";
    Board board;
    parse_fen(&board, kings_fen);

    printf("White King (K):\n");
    print_bitboard(board.piece_bitboards[K]);

    printf("Black King (k):\n");
    print_bitboard(board.piece_bitboards[k]);
    
    printf("All Occupied Squares:\n");
    print_bitboard(board.occupancies[2]);
}

void test_complex_position() {
    printf("\n--- Complex Position ---\n");

    const char *kiwipete_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    Board board;
    parse_fen(&board, kiwipete_fen);

    printf("White Rooks (R) - note the positions for castling:\n");
    print_bitboard(board.piece_bitboards[R]);

    printf("Black Pawns (p) - note the complex structure:\n");
    print_bitboard(board.piece_bitboards[p]);
    
    printf("All Occupied Squares:\n");
    print_bitboard(board.occupancies[2]);
}

int main() {
    test_starting_position();
    test_pawns_only();
    test_knights_bishops_only();
    test_rooks_only();
    test_queens_only();
    test_kings_only();
    test_complex_position();

    return 0;
}