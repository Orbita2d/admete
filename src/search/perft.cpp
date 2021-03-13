#include "../game/board.hpp"
#include "search.hpp"
#include "transposition.hpp"
#include <iostream>

unsigned int perft(unsigned int depth, Board &board) {
    transposition_table.clear();
    // Transposition tables very tricky here because the keys cannot distinguish by depth
    /*           R
                / \ 
               0   1
             /  \ /  \
            00 01 10 11
        If 10 and 0 have the same key (because it's the same position, possible with depth >= 5, especially in the endgame), it will count 10 as having two children 
    */
    return perft_bulk(depth, board);
}

unsigned int perft_bulk(unsigned int depth, Board &board) {

    std::vector<Move> legal_moves = board.get_moves();
    if (depth == 1) {
        return legal_moves.size();
    }

    long hash = board.hash();
    unsigned int nodes = 0;
    for (Move move : legal_moves) {
        board.make_move(move);
        nodes += perft_bulk(depth-1, board);
        board.unmake_move(move);
    }
    return nodes;
}

void perft_divide(unsigned int depth, Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    unsigned int nodes = 0;
    unsigned int child_nodes;
    for (auto move : legal_moves) {
        board.make_move(move);
        child_nodes =  perft(depth-1, board);
        std::cout << move.pretty_print() << ": "<< std::dec << child_nodes << std::endl;
        nodes += child_nodes;
        board.unmake_move(move);
    }
    std::cout << "nodes searched: " << nodes << std::endl;
}