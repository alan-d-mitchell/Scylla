#include "defs.h"
#include "movegen.h"
#include "datagen.h" // Include the new data generation header

int main() {
    init_attack_tables();

    // Generate training data for 10 games
    generate_training_data(10);

    return 0;
}