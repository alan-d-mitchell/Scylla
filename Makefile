# Makefile for the Scylla Chess Engine

# --- Variables ---
CC = gcc
# ADDED -Itests to the include paths
CFLAGS = -Wall -Wextra -O2 -g -Isrc -Itests

# --- Directories ---
BIN_DIR = bin
SRC_DIR = src
TEST_DIR = tests

# --- File Lists ---

# Source files for the main program (scylla)
# ADDED perft.c to the main sources
MAIN_SRCS = scylla.c $(SRC_DIR)/bitboard.c $(SRC_DIR)/board.c \
            $(SRC_DIR)/movegen.c $(SRC_DIR)/evaluate.c $(SRC_DIR)/search.c

# Source files for the PERFT test
# CORRECTED: This now includes all dependencies for perft_test.c
PERFT_TEST_SRCS = $(TEST_DIR)/perft_test.c $(SRC_DIR)/perft.c $(SRC_DIR)/movegen.c $(SRC_DIR)/board.c $(SRC_DIR)/bitboard.c

# Source files for the original movegen test
MOVEGEN_TEST_SRCS = $(TEST_DIR)/movegen_test.c $(SRC_DIR)/movegen.c $(SRC_DIR)/board.c $(SRC_DIR)/bitboard.c


# --- Target Executables ---
TARGET = $(BIN_DIR)/scylla
PERFT_TEST_TARGET = $(BIN_DIR)/run_perft_tests
MOVEGEN_TEST_TARGET = $(BIN_DIR)/run_movegen_tests


# --- Build Rules ---

# The default rule: 'make' or 'make all' builds the main program
all: $(TARGET)

# Rule to build the main program executable
$(TARGET): $(MAIN_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build the perft test executable
$(PERFT_TEST_TARGET): $(PERFT_TEST_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to build the original movegen test executable
$(MOVEGEN_TEST_TARGET): $(MOVEGEN_TEST_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^


# --- Utility Rules ---

# Rule to create the bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# RENAMED: 'make perft_test' now builds and runs the perft test
perft_test: $(PERFT_TEST_TARGET)
	@echo "--- Running Perft Tests ---"
	./$(PERFT_TEST_TARGET)

# KEPT: 'make movegen_test' can still be used for the other test
movegen_test: $(MOVEGEN_TEST_TARGET)
	@echo "--- Running Movegen Tests ---"
	./$(MOVEGEN_TEST_TARGET)

# Rule to clean up all compiled files
clean:
	rm -rf $(BIN_DIR)

# Phony targets are rules that don't produce a file with the same name.
.PHONY: all perft_test movegen_test clean