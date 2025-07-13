#ifndef BITBOARD_H
#define BITBOARD_H

#include "defs.h"

void print_bitboard(u64 bitboard);
void set_bit(u64* bitboard, int square, int action);

int popcount(u64 bb);


#endif // BITBOARD_H