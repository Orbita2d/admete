#include "../game/board.hpp"
#include "search.hpp"
#include "transposition.hpp"
#include <iostream>

unsigned long perft_bulk(depth_t depth, Board &board);
unsigned long perft_spenny(depth_t depth, Board &board);

unsigned long perft_comparison(depth_t depth, Board &board) {
    // Perft with no bulk counting, no transposition tables for nodes/second calculation.
    return perft_spenny(depth, board);
}

unsigned long perft(depth_t depth, Board &board) {
    Cache::transposition_table.clear();
    Cache::transposition_table.min_depth(0);
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

unsigned long perft_bulk(depth_t depth, Board &board) {

    std::vector<Move> legal_moves = board.get_moves();
    if (depth == 1) {
        return legal_moves.size();
    }

    const long hash = board.hash();
    if (Cache::transposition_table.probe(hash)) {
        const Cache::TransElement hit = Cache::transposition_table.last_hit();
        if (hit.depth() == depth) {
            // The saved score is an exact value for the subtree
            return hit.eval();
        }
    }
    unsigned long nodes = 0;
    for (Move move : legal_moves) {
        board.make_move(move);
        nodes += perft_bulk(depth-1, board);
        board.unmake_move(move);
    }
    Cache::transposition_table.store(hash, nodes, NEG_INF, POS_INF, depth, NULL_MOVE);
    return nodes;
}

void perft_divide(depth_t depth, Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    unsigned long nodes = 0;
    unsigned long child_nodes;
    for (auto move : legal_moves) {
        board.make_move(move);
        child_nodes =  perft(depth-1, board);
        std::cout << move.pretty() << ": "<< std::dec << child_nodes << std::endl;
        nodes += child_nodes;
        board.unmake_move(move);
    }
    std::cout << "nodes searched: " << nodes << std::endl;
}

unsigned long perft_spenny(depth_t depth, Board &board) {

    std::vector<Move> legal_moves = board.get_moves();
    if (depth == 0) {
        return 1;
    }

    unsigned long nodes = 0;
    for (Move move : legal_moves) {
        board.make_move(move);
        nodes += perft_spenny(depth-1, board);
        board.unmake_move(move);
    }
    return nodes;
}