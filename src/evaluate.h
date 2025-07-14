#ifndef EVALUATE_H
#define EVALUATE_H

#include "board.h"

// A structure to hold both middlegame and endgame scores for a term.
// This is a common pattern in modern chess engines.
typedef struct {
    int mg;
    int eg;
} Score;

// The main evaluation function. It returns a score in centipawns
// from the perspective of the side to move.
int evaluate(Board* board);
void init_evaluation_masks();

#endif // EVALUATE_H