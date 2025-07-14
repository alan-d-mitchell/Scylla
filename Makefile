# Makefile for the Scylla Chess Engine

# --- Variables ---
CC = gcc
# Include directories for src and tests
CFLAGS = -Wall -Wextra -O2 -g -Isrc

# --- Directories ---
BIN_DIR = bin
SRC_DIR = src
OBJ_DIR = obj
TEST_DIR = tests

# --- File Lists ---

# All source files in the src directory
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Object files for the main program (scylla)
# This automatically converts a list like 'src/a.c src/b.c' to 'obj/a.o obj/b.o'
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Main executable
TARGET = $(BIN_DIR)/scylla

# --- Test Executables & Objects ---
PERFT_TEST_TARGET = $(BIN_DIR)/perft_test
PERFT_TEST_OBJS = $(OBJ_DIR)/perft_test.o $(OBJ_DIR)/perft.o $(OBJ_DIR)/movegen.o $(OBJ_DIR)/board.o $(OBJ_DIR)/bitboard.o

SEARCH_TEST_TARGET = $(BIN_DIR)/search_eval_test
SEARCH_TEST_OBJS = $(OBJ_DIR)/search_eval_test.o $(OBJ_DIR)/search.o $(OBJ_DIR)/evaluate.o $(OBJ_DIR)/movegen.o $(OBJ_DIR)/board.o $(OBJ_DIR)/bitboard.o


# --- Build Rules ---

# The default rule: 'make' or 'make all' builds the main program
all: $(TARGET)

# Rule to link the main program executable from its object files
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to link the perft test executable
$(PERFT_TEST_TARGET): $(PERFT_TEST_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Rule to link the search and evaluation test executable
$(SEARCH_TEST_TARGET): $(SEARCH_TEST_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^


# --- Pattern Rules for Compiling ---

# Generic rule to compile any .c file from the src directory into an object file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Generic rule to compile any .c file from the tests directory into an object file
$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<


# --- Utility Rules ---

# Rule to create directories if they don't exist
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

# Rule to run perft test
perft_test: $(PERFT_TEST_TARGET)
	@echo "--- Running Perft Tests ---"
	./$(PERFT_TEST_TARGET)

# Rule to run search and eval test
search_test: $(SEARCH_TEST_TARGET)
	@echo "--- Running Search & Eval Tests ---"
	./$(SEARCH_TEST_TARGET)

# Rule to clean up all compiled files
clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

# Phony targets are rules that don't produce a file with the same name.
.PHONY: all perft_test search_test clean