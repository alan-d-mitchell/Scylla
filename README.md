Scylla Chess Engine

Scylla is a UCI chess engine written in C.

📖 About The Project

This repository contains the source code for the Scylla chess engine. The engine uses a bitboard representation for the chessboard, which allows for efficient move generation and evaluation. The search algorithm is a negamax implementation with alpha-beta pruning and move ordering to improve performance. The evaluation function is based on the well-respected Stockfish 11, using piece-square tables, mobility scores, and pawn structure analysis to assess the strength of a position.

✨ Features

    Bitboard Representation: A highly efficient way to represent the chessboard, enabling fast operations for move generation and evaluation.

    Move Generation: A comprehensive move generator that can produce all legal moves in a given position, including special moves like castling, en passant, and promotions.

    Evaluation Function: A sophisticated evaluation function inspired by Stockfish 11, considering:

        Material balance

        Piece-square tables (PSTs)

        Mobility

        Pawn structure (passed, doubled, isolated pawns)

    Search Algorithm: A negamax search algorithm with alpha-beta pruning to efficiently search the game tree.

    Move Ordering: Implemented using the Most Valuable Victim - Least Valuable Attacker (MVV-LVA) heuristic to prioritize captures of high-value pieces.

    Quiescence Search: A search extension that only evaluates "quiet" positions, avoiding the horizon effect by analyzing tactical exchanges beyond the nominal search depth.

    Testing Suite: A suite of tests for performance (perft), bitboard representation, move generation, and search/evaluation functions.

🚀 Getting Started

To get a local copy up and running, follow these simple steps.

Prerequisites

You will need a C compiler, such as GCC, and make to build the project.

Building

Simply run make in the root directory to compile the main file
Run make "test_file_name" to compile and run the test suites

♟️ Usage

Currently, the engine's main executable does not have an interactive mode. The search_eval_test provides the best example of how to use the engine to find the best move in a given position. You can modify the FEN strings in tests/search_eval_test.c to analyze different positions.

🗺️ Roadmap

The next major step in the development of Scylla is the implementation of the Universal Chess Interface (UCI) protocol. This will allow the engine to communicate with graphical user interfaces (GUIs) like Arena, Cute Chess, or Shredder, making it much easier to play against and analyze games with.

Future plans beyond UCI include:

    Transposition Table: To store and retrieve previously searched positions, significantly speeding up the search.

    Improved Search Techniques: Incorporating more advanced search techniques like null-move pruning, late move reductions, and aspiration windows.

    More Sophisticated Evaluation: Further refining the evaluation function with more nuanced terms and tuning.

    Endgame Tablebases: Integrating tablebases for perfect play in endgame positions.

🤝 Contributing

Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are greatly appreciated.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".

    Fork the Project

    Create your Feature Branch (git checkout -b feature/AmazingFeature)

    Commit your Changes (git commit -m 'Add some AmazingFeature')

    Push to the Branch (git push origin feature/AmazingFeature)

    Open a Pull Request

📜 License

Distributed under the MIT License. See LICENSE for more information.

🙏 Acknowledgments

    The evaluation function's values and structure are heavily inspired by Stockfish 11.

    The magic bitboard implementation is a classic technique, and the approach used here is a well-established method within the chess programming community.