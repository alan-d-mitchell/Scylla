#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"

// A very large number to represent infinity for alpha-beta search
#define INFINITY 50000

// The main entry point for finding the best move in a position.
// This function orchestrates the search by iterating through the root moves
// and calling the negamax algorithm for each one to find the move with the highest score.
Move search_position(Board* board, int depth);

#endif // SEARCH_H