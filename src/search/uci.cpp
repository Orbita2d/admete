
#include "uci.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "search.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#define ENGINE_NAME "admete"
#define ENGINE_AUTH "orbita"

typedef std::chrono::high_resolution_clock my_clock;

bool uci_enable = false;

namespace UCI {
void init_uci() {
    std::cout << "id name " << ENGINE_NAME << std::endl;
    std::cout << "id author " << ENGINE_AUTH << std::endl;
    std::cout << "setoption name Nullmove value false" << std::endl;
    std::cout << "uciok" << std::endl;
    ::uci_enable = true;
}

void position(Board &board, std::istringstream &is) {
    /*
        position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
        set up the position described in fenstring on the internal board and
        play the moves on the internal chess board.
        if the game was played  from the start position the string "startpos" will be sent
        Note: no "new" command is needed. However, if this position is from a different game than
        the last position sent to the engine, the GUI should have sent a "ucinewgame" inbetween.
    */
    std::string token, fen;
    is >> std::ws >> token;
    // Need to know what the position is.
    if (token == "startpos") {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        is >> token;
    } else if (token == "fen") {
        while (is >> token && token != "moves") {
            // munch through the command string
            if (token == "\"") {
                continue;
            }
            fen += token + " ";
        }
    } else {
        // This is invalid. Just ignore it
        std::cerr << "Invalid position string: \"" << token << "\"" << std::endl;
        return;
    }
    board.fen_decode(fen);
    if (token == "moves") {
        while (is >> token) {
            // munch through the command string
            board.try_uci_move(token);
        }
    }
}

void bestmove(Board &board, const Move move) {
    /*
    bestmove <move1> [ ponder <move2> ]
        the engine has stopped searching and found the move <move> best in this position.
        the engine can send the move it likes to ponder on. The engine must not start pondering automatically.
        this command must always be sent if the engine stops searching, also in pondering mode if there is a
        "stop" command, so for every "go" command a "bestmove" command is needed!
        Directly before that the engine should send a final "info" command with the final search information,
        the the GUI has the complete statistics about the last search.
    */
    MoveList legal_moves = board.get_moves();
    if (is_legal(move, legal_moves)) {
        std::cout << "bestmove " << move.pretty() << std::endl;
    } else {
        std::cerr << "illegal move!: " << board.fen_encode() << std::endl;
        std::cout << "bestmove " << legal_moves[0].pretty() << std::endl;
    }
}

void do_search(Board *board, depth_t max_depth, const int max_millis, Search::SearchOptions *options) {
    PrincipleLine line;
    line.reserve(max_depth);
    options->running_flag.store(true);
    options->stop_flag = false;
    options->nodes = 0;
    int score = Search::search(*board, max_depth, max_millis, line, *options);
    options->eval = score;
    Move first_move = line.back();
    bestmove(*board, first_move);
    // Set this so that the thread can be joined.
    options->running_flag.store(false);
}

void cleanup_thread(Search::SearchOptions &options) {
    if (!options.is_running() && options.running_thread.joinable()) {
        options.running_thread.join();
    }
}

void go(Board &board, std::istringstream &is, Search::SearchOptions &options) {
    /*
    * go
    start calculating on the current position set up with the "position" command.
    There are a number of commands that can follow this command, all will be sent in the same string.
    If one command is not sent its value should be interpreted as it would not influence the search.
    * searchmoves <move1> .... <movei>
        restrict search to this moves only
        Example: After "position startpos" and "go infinite searchmoves e2e4 d2d4"
        the engine should only search the two moves e2e4 and d2d4 in the initial position.
    * ponder
        start searching in pondering mode.
        Do not exit the search in ponder mode, even if it's mate!
        This means that the last move sent in in the position string is the ponder move.
        The engine can do what it wants to do, but after a "ponderhit" command
        it should execute the suggested move to ponder on. This means that the ponder move sent by
        the GUI can be interpreted as a recommendation about which move to ponder. However, if the
        engine decides to ponder on a different move, it should not display any mainlines as they are
        likely to be misinterpreted by the GUI because the GUI expects the engine to ponder
        on the suggested move.
    * wtime <x>
        white has x msec left on the clock
    * btime <x>
        black has x msec left on the clock
    * winc <x>
        white increment per move in mseconds if x > 0
    * binc <x>
        black increment per move in mseconds if x > 0
    * movestogo <x>
        there are x moves to the next time control,
        this will only be sent if x > 0,
        if you don't get this and get the wtime and btime it's sudden death
    * depth <x>
        search x plies only.
    * nodes <x>
        search x nodes only,
    * mate <x>
        search for a mate in x moves
    * movetime <x>
        search exactly x mseconds
    * infinite
        search until the "stop" command. Do not exit the search without being told so in this mode!
    */

    int wtime = POS_INF, btime = POS_INF;
    int winc = 0, binc = 0, move_time = POS_INF;
    uint max_depth = 20;
    std::string token;
    while (is >> token) {
        // munch through the command string
        if (token == "wtime") {
            is >> wtime;
        } else if (token == "btime") {
            is >> btime;
        } else if (token == "winc") {
            is >> winc;
        } else if (token == "binc") {
            is >> binc;
        } else if (token == "depth") {
            is >> max_depth;
        } else if (token == "movetime") {
            is >> move_time;
        } else if (token == "mate") {
            is >> options.mate_depth;
        }
    }
    const int our_time = board.is_white_move() ? wtime : btime;
    const int our_inc = board.is_white_move() ? winc : binc;
    int cutoff_time = our_time == POS_INF ? POS_INF : our_time / 18 + (our_inc * 3) / 5;
    cutoff_time = std::min(move_time, cutoff_time);
    options.running_thread = std::thread(&do_search, &board, (depth_t)max_depth, cutoff_time, &options);
}

void do_perft(Board *board, const depth_t depth, Search::SearchOptions *options) {
    // Mark the search thread as running.
    options->running_flag.store(true);
    options->stop_flag = false;
    options->nodes = 0;

    my_clock::time_point time_origin = my_clock::now();

    options->nodes = Search::perft(depth, *board, *options);

    std::chrono::duration<double, std::milli> time_span = my_clock::now() - time_origin;
    unsigned long nps = int(1000 * (options->nodes / time_span.count()));

    if (!options->stop()) {
        uci_info(depth, options->nodes, nps, time_span.count());
    }

    // Mark the search thread as finished (and ready to join)
    options->running_flag.store(false);
}

void perft(Board &board, std::istringstream &is, Search::SearchOptions &options) {
    /* perft <depth> <fen>
     */

    // This command ignores the <fen> part (and relies on position being set already).
    // Returns the perft score for that depth.

    // std::istringstream >> uint8_t doesn't do what you think.
    int depth;
    is >> depth;

    options.running_thread = std::thread(&do_perft, &board, (depth_t)depth, &options);
}

void stop(Search::SearchOptions &options) {
    /*
    stop calculating as soon as possible,
    don't forget the "bestmove" and possibly the "ponder" token when finishing the search
    */
    if (options.is_running()) {
        options.set_stop();
        options.running_thread.join();
    }
}

void uci() {
    init_uci();
    std::string command, token;
    Board board = Board();
    Search::SearchOptions options = Search::SearchOptions();
    while (true) {
        std::getline(std::cin, command);
        cleanup_thread(options);
        std::istringstream is(command);
        is >> std::ws >> token;
        if (token == "isready") {
            // interface is asking if we can continue, if we are here, we clearly can.
            stop(options);
            std::cout << "readyok" << std::endl;
        } else if (token == "ucinewgame") {
            board.initialise_starting_position();
        } else if (token == "position") {
            stop(options);
            position(board, is);
        } else if (token == "go") {
            stop(options);
            go(board, is, options);
        } else if (token == "perft") {
            stop(options);
            perft(board, is, options);
        } else if (token == "stop") {
            stop(options);
        } else if (token == "quit") {
            exit(EXIT_SUCCESS);
        } else if (token == "d") {
            board.pretty();
        } else if (token == "h") {
            score_t v = Evaluation::heuristic(board);
            std::cout << std::dec << (int)v << std::endl;
        } else {
            // std::cerr << "!#" << token << ":"<< command << std::endl;
        }
    }
}

void uci_info(depth_t depth, score_t eval, unsigned long nodes, unsigned long nps, PrincipleLine principle,
              unsigned int time, ply_t root_ply) {
    if (!::uci_enable) {
        return;
    }
    std::cout << std::dec;
    std::cout << "info";
    std::cout << " depth " << (uint)depth;

    if (is_mating(eval)) {
        // Mate for white. Score is (MATING_SCORE - mate_ply)
        score_t n = (MATING_SCORE - (eval)-root_ply + 1) / 2;
        std::cout << " score mate " << (int)n;
    } else if (is_mating(-eval)) {
        // Mate for black. Score is (mate_ply - MATING_SCORE)
        score_t n = (eval + MATING_SCORE - root_ply) / 2;
        std::cout << " score mate " << -(int)n;
    } else {
        std::cout << " score cp " << (int)eval;
    }
    if (nodes > 0) {
        std::cout << " nodes " << nodes;
    }
    if (nps > 0) {
        std::cout << " nps " << nps;
    }
    std::cout << " pv ";
    for (PrincipleLine::reverse_iterator it = principle.rbegin(); it != principle.rend(); ++it) {
        std::cout << it->pretty() << " ";
    }
    std::cout << " time " << time;
    std::cout << std::endl;
}

void uci_info(depth_t depth, unsigned long nodes, unsigned long nps, unsigned int time) {
    // This one only used for perft.
    if (!::uci_enable) {
        return;
    }
    std::cout << std::dec;
    std::cout << "info";
    std::cout << " depth " << (uint)depth;

    if (nodes > 0) {
        std::cout << " nodes " << nodes;
    }
    if (nps > 0) {
        std::cout << " nps " << nps;
    }
    std::cout << " time " << time;
    std::cout << std::endl;
}

void uci_info_nodes(unsigned long nodes, unsigned long nps) {
    if (!::uci_enable) {
        return;
    }
    if (nodes > 0) {
        std::cout << "nodes " << nodes;
    }
    if (nps > 0) {
        std::cout << " nps " << nps;
    }
    std::cout << std::endl;
}
} // namespace UCI