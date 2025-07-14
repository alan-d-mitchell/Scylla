// src/search.h

#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"

// A very large number to represent infinity for alpha-beta search
#define INFINITY 50000
// A value representing a checkmate score. The ply is subtracted
// to prefer shorter mates.
#define MATE_SCORE (INFINITY - 100)

// The main entry point for finding the best move in a position.
Move search_position(Board* board, int depth);

#endif // SEARCH_H
