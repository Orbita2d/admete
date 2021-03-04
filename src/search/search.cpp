#include <time.h>
#include "search.hpp"

int alphabeta(Board& board, const uint depth, PrincipleLine& line) {
    int nodes_count;
    return alphabeta(board, depth, NEG_INF, POS_INF, board.is_white_move(), line);
}


int alphabeta(Board& board, const uint depth, int alpha, int beta, const bool maximising) {
    // perform alpha-beta pruning search.
    std::vector<Move> legal_moves = board.get_moves();
    if (depth == 0) { return board.evaluate(legal_moves, maximising); }
    if (legal_moves.size() == 0) { return board.evaluate(legal_moves, maximising); }
    if (maximising) {
        int best_score = NEG_INF;
        for (Move move : legal_moves) {
            board.make_move(move);
            int score = alphabeta(board, depth - 1, alpha, beta, false);
            board.unmake_move(move);
            best_score = std::max(best_score, score);
            if (score == mating_score) {
                // Mate in 1.
                break;
            }
            alpha = std::max(alpha, score);
            if (alpha > beta) {
                break; // beta-cutoff
            }
        }
        
        return is_mating(best_score) ? best_score - 1 : best_score;
    } else {
        int best_score = POS_INF;
        for (Move move : legal_moves) {
            board.make_move(move);
            int score = alphabeta(board, depth - 1, alpha, beta, true);
            board.unmake_move(move);
            best_score = std::min(best_score, score);
            if (score == mating_score) {
                // Mate in 1.
                break;
            }
            beta = std::min(beta, score);
            if (beta <= alpha) {
                break; // alpha-cutoff
            }
        }
        return is_mating(-best_score) ? best_score + 1 : best_score;
    }
}

int alphabeta(Board& board, const uint depth, int alpha, int beta, const bool maximising, PrincipleLine& line) {
    // perform alpha-beta pruning search.
    std::vector<Move> legal_moves = board.get_sorted_moves();
    if (depth == 0) { return board.evaluate(legal_moves, maximising); }
    if (legal_moves.size() == 0) { 
        return board.evaluate(legal_moves, maximising); 
    }
    PrincipleLine best_line;
    if (maximising) {
        int best_score = INT32_MIN;
        for (Move move : legal_moves) {
            PrincipleLine temp_line;
            board.make_move(move);
            int score = alphabeta(board, depth - 1, alpha, beta, false, temp_line);
            board.unmake_move(move);
            if (score > best_score) {
                best_score = score;
                best_line = temp_line;
                best_line.push_back(move);
            }
            if (score == mating_score) {
                // Mate in 1.
                break;
            }
            alpha = std::max(alpha, score);
            if (alpha > beta) {
                break; // beta-cutoff
            }
        }
        line = best_line;
        return is_mating(best_score) ? best_score - 1 : best_score;
    } else {
        int best_score = INT32_MAX;
        for (Move move : legal_moves) {
            PrincipleLine temp_line;
            board.make_move(move);
            int score = alphabeta(board, depth - 1, alpha, beta, true, temp_line);
            board.unmake_move(move);
            if (score < best_score) {
                best_score = score;
                best_line = temp_line;
                best_line.push_back(move);
            }
            if (score == mating_score) {
                // Mate in 1.
                break;
            }
            beta = std::min(beta, score);
            if (beta <= alpha) {
                break; // alpha-cutoff
            }
        }
        line = best_line;
        return is_mating(-best_score) ? best_score + 1 : best_score;
    }
}

int alphabeta_negamax(Board& board, const uint depth, int alpha, int beta, PrincipleLine& line) {
    // perform alpha-beta pruning search.
    std::vector<Move> legal_moves = board.get_sorted_moves();
    if (depth == 0) { return board.evaluate_negamax(legal_moves); }
    if (legal_moves.size() == 0) { 
        return board.evaluate_negamax(legal_moves); 
    }
    PrincipleLine best_line;
    int best_score = NEG_INF;
    for (Move move : legal_moves) {
        PrincipleLine temp_line;
        board.make_move(move);
        int score = -alphabeta_negamax(board, depth - 1, -beta, -alpha, temp_line);
        board.unmake_move(move);
        if (score > best_score) {
            best_score = score;
            best_line = temp_line;
            best_line.push_back(move);
        }
        if (score == mating_score - depth) {
            // Mate in 1.
            break;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
    }
    line = best_line;
    return is_mating(best_score) ? best_score - 1 : best_score;
}

int pv_search_negamax(Board& board, const uint depth, int alpha, int beta, PrincipleLine& principle, const uint pv_depth, PrincipleLine& line) {
    // perform alpha-beta pruning search with principle variation optimisation.
    std::vector<Move> legal_moves = board.get_sorted_moves();
    if (depth == 0) { return board.evaluate_negamax(legal_moves); }
    if (legal_moves.size() == 0) { 
        return board.evaluate_negamax(legal_moves); 
    }
    if (pv_depth == 0) {
        // End of the principle variation, just evaluate this node using alphabeta()
        return alphabeta_negamax(board, depth, alpha, beta, line);
    }
    PrincipleLine best_line;
    // -1 here is because we are indexing at 0. If there is 1 move left, that's at index 0;
    Move pv_move = principle.at(pv_depth - 1);
    board.make_move(pv_move);
    int pv_score = -pv_search_negamax(board, depth - 1, -beta, -alpha, principle, pv_depth - 1, best_line);
    best_line.push_back(pv_move);
    board.unmake_move(pv_move);

    int best_score = NEG_INF;
    for (Move move : legal_moves) {
        PrincipleLine temp_line;
        board.make_move(move);
        int score = -alphabeta_negamax(board, depth - 1, -beta, -alpha, temp_line);
        board.unmake_move(move);
        if (score > best_score) {
            best_score = score;
            best_line = temp_line;
            best_line.push_back(move);
        }
        if (score == mating_score - depth) {
            // Mate in 1.
            break;
        }
        alpha = std::max(alpha, score);
        if (alpha > beta) {
            break; // beta-cutoff
        }
    }
    line = best_line;
    return is_mating(best_score) ? best_score - 1 : best_score;
}

int pv_search(Board& board, const uint depth, int alpha, int beta, const bool maximising, PrincipleLine& principle, const uint pv_depth, PrincipleLine& line) {
    // perform alpha-beta pruning search with principle variation optimisation.
    std::vector<Move> legal_moves = board.get_sorted_moves();
    int cutoffs = 0;
    if (depth == 0) { return board.evaluate(legal_moves, maximising); }
    if (legal_moves.size() == 0) { 
        return board.evaluate(legal_moves, maximising); 
    }
    if (pv_depth == 0) {
        // End of the principle variation, just evaluate this node using alphabeta()
        return alphabeta(board, depth, alpha, beta, maximising, line);
    }
    PrincipleLine best_line;
    // -1 here is because we are indexing at 0. If there is 1 move left, that's at index 0;
    Move pv_move = principle.at(pv_depth - 1);
    board.make_move(pv_move);
    int pv_score = pv_search(board, depth - 1, alpha, beta, !maximising, principle, pv_depth - 1, best_line);
    best_line.push_back(pv_move);
    board.unmake_move(pv_move);
    if (maximising) {
        alpha = std::max(alpha, pv_score);
        int best_score = pv_score;
        for (Move move : legal_moves) {
            PrincipleLine temp_line;
            board.make_move(move);
            int score = alphabeta(board, depth - 1, alpha, beta, false, temp_line);
            board.unmake_move(move);
            if (score > best_score) {
                best_score = score;
                best_line = temp_line;
                best_line.push_back(move);
            }
            if (is_mating(score)) {
                // Mate in 1.
                break;
            }
            alpha = std::max(alpha, score);
            if (alpha > beta) {
                break; // beta-cutoff
            }
        }
        line = best_line;
        return is_mating(best_score) ? best_score - 1 : best_score;
    } else {
        int best_score = pv_score;
        beta = std::min(beta, pv_score);
        for (Move move : legal_moves) {
            PrincipleLine temp_line;
            board.make_move(move);
            int score = alphabeta(board, depth - 1, alpha, beta, true, temp_line);
            board.unmake_move(move);
            if (score < best_score) {
                best_score = score;
                best_line = temp_line;
                best_line.push_back(move);
            }
            if (is_mating(-score)) {
                // Mate in 1.
                break;
            }
            beta = std::min(beta, score);
            if (beta <= alpha) {
                break; // alpha-cutoff
            }
        }
        line = best_line;
        return is_mating(-best_score) ? best_score + 1 : best_score;
    }
}

int iterative_deepening(Board& board, const uint depth, PrincipleLine& line) {
    PrincipleLine principle;
    const bool maximising = board.is_white_move();
    int score = alphabeta(board, 2, INT32_MIN, INT32_MAX, maximising, principle);
    // Start at 2 ply for a best guess first move.
    for (int i = 4; i <= depth; i+=2) {
        PrincipleLine temp_line;
        score = alphabeta(board, i, INT32_MIN, INT32_MAX, maximising, temp_line);
        principle = temp_line;
        if (maximising & is_mating(score)) { break; }
        if (!maximising & is_mating(-score)) { break; }
    }
    line = principle;
    return score;
}

int iterative_deepening_negamax(Board& board, const uint depth, PrincipleLine& line) {
    PrincipleLine principle;
    int score = alphabeta_negamax(board, 2, NEG_INF, POS_INF, principle);
    // Start at 2 ply for a best guess first move.
    for (int i = 4; i <= depth; i+=2) {
        PrincipleLine temp_line;
        score = alphabeta_negamax(board, i, NEG_INF, POS_INF, temp_line);
        principle = temp_line;
        if (is_mating(score)) { break; }
    }
    line = principle;
    return score;
}
/*
int find_best_random(Board& board, const uint max_depth, const int random_weight, PrincipleLine& line) {
    std::srand(time(NULL));
    PrincipleLine principle;
    const bool maximising = board.is_white_move();
    std::vector<Move> legal_moves = board.get_sorted_moves();
    int n = legal_moves.size();
    std::vector<PrincipleLine> line_array(legal_moves.size());
    std::vector<int> score_array(legal_moves.size());
    for (int i = 0; i < max_depth; i +=2) {
        if (maximising) {
            int best_score = INT32_MIN;
            for ( int j = 0; j< n ; j++) {
                PrincipleLine temp_line;
                Move move = legal_moves[j];
                board.make_move(move);
                int score = pv_search(board, i, INT32_MIN, INT32_MAX, maximising, principle, principle.size(), temp_line);
                board.unmake_move(move);
                if (score > best_score) {
                    best_score = score;
                    principle = temp_line;
                    principle.push_back(move);
                }
                score_array[j] = best_score;
                line_array[j] = principle;
                if (is_mating(score)) { break; }
            }
            line = principle;
            return is_mating(best_score) ? best_score - 1 : best_score;
        } else {
            int best_score = INT32_MAX;
            for (Move move : legal_moves) {
                PrincipleLine temp_line;
                board.make_move(move);
                int score = iterative_deepening(board, max_depth - 1, temp_line);
                int offset = is_mating(-score) ? 0 : ((std::rand() % 2*random_weight) - random_weight);
                score += offset;
                board.unmake_move(move);
                if (score < best_score) {
                    best_score = score;
                    principle = temp_line;
                    principle.push_back(move);
                }
                if (is_mating(-score)) { break; }
            }
            line = principle;
            return is_mating(-best_score) ? best_score + 1 : best_score;
        }
    }
    //int offset = is_mating(score) ? 0 : ((std::rand() % (2*random_weight)) - random_weight);
}
*/


int find_best_random(Board& board, const uint depth, const int weight, PrincipleLine& line) {
    std::srand(time(NULL));
    PrincipleLine principle;
    const bool maximising = board.is_white_move();
    std::vector<Move> legal_moves = board.get_sorted_moves();
    int nodes_count;
    if (maximising) {
        int best_score = INT32_MIN;
        for (Move move : legal_moves) {
            PrincipleLine temp_line;
            board.make_move(move);
            int score = iterative_deepening(board, depth - 1, temp_line);
            int offset = is_mating(score) ? 0 : ((std::rand() % (2*weight)) - weight);
            score += offset;
            board.unmake_move(move);
            if (score > best_score) {
                best_score = score;
                principle = temp_line;
                principle.push_back(move);
            }
            if (is_mating(score)) { break; }
        }
        line = principle;
        return is_mating(best_score) ? best_score - 1 : best_score;
    } else {
        int best_score = INT32_MAX;
        for (Move move : legal_moves) {
            PrincipleLine temp_line;
            board.make_move(move);
            int score = iterative_deepening(board, depth - 1, temp_line);
            int offset = is_mating(-score) ? 0 : ((std::rand() % 2*weight) - weight);
            score += offset;
            board.unmake_move(move);
            if (score < best_score) {
                best_score = score;
                principle = temp_line;
                principle.push_back(move);
            }
            if (is_mating(-score)) { break; }
        }
        line = principle;
        return is_mating(-best_score) ? best_score + 1 : best_score;
    }
}
