#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>

// More convenient typedefs
typedef uint32_t u32;
typedef uint64_t u64;
typedef int16_t i16;
typedef int32_t i32;

// --- Enums ---
enum { P, N, B, R, Q, K, p, n, b, r, q, k };
enum { WHITE, BLACK, BOTH };
enum { WK = 1, WQ = 2, BK = 4, BQ = 8 }; // For castling rights

// Enum for board squares
enum {
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
};

// Struct to represent a single chess move
typedef struct {
    int from;   // Starting square
    int to;     // Destination square
    int piece;  // The piece being moved
    int promotion; // The piece to promote to
    int is_capture;
    int is_enpassant;
    int is_castle;
    int score;
} Move;

// A list to store all generated moves for a position
#define MAX_MOVES 256
typedef struct {
    Move moves[MAX_MOVES];
    int count;
} MoveList;

#endif