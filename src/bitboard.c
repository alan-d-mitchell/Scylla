#include <stdio.h>
#include <inttypes.h>

#include "bitboard.h"

// Helper function
void set_bit(u64* bitboard, int square, int action) {
    if (action) {
        *bitboard |= (1ULL << square); // Set bit
    } else {
        *bitboard &= ~(1ULL << square); // Clear bit
    }
}

void print_bitboard(u64 bitboard) {
    printf("Bitboard repr: %" PRIu64 "\n", bitboard);

    for (int rank = 7; rank >= 0; rank--) {
        printf(" %d |", rank + 1);
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            if ((bitboard >> square) & 1) {
                printf(" X");
            } else {
                printf(" .");
            }
        }
        printf("\n");
    }
    printf("   +----------------\n     a b c d e f g h\n\n");
}

int popcount(u64 bb) {
    int count = 0;

    while (bb) {
        bb &= bb - 1;
        count++;
    }
    
    return count;
}