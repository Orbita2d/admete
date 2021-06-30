#include "board.hpp"
#include <assert.h>
#include <iomanip>
#include <iostream>
#include <istream>
#include <sstream>

#define toDigit(c) (c - '0')

namespace Printing {

std::string piece_name(const PieceType p) {
    switch (p) {
    case PAWN:
        return "Pawn";
    case KNIGHT:
        return "Knight";
    case BISHOP:
        return "Bishop";
    case ROOK:
        return "Rook";
    case QUEEN:
        return "Queen";
    case KING:
        return "King";
    case NO_PIECE:
        return "No Piece";
    default:
        break;
    }
}
} // namespace Printing

std::map<char, Square::square_t> file_decode_map = {
    {'a', 0}, {'b', 1}, {'c', 2}, {'d', 3}, {'e', 4}, {'f', 5}, {'g', 6}, {'h', 7},

};

std::map<Square::square_t, char> file_encode_map = {
    {0, 'a'}, {1, 'b'}, {2, 'c'}, {3, 'd'}, {4, 'e'}, {5, 'f'}, {6, 'g'}, {7, 'h'},

};

std::string Board::fen_encode() const {
    uint space_counter = 0;
    std::stringstream ss;
    for (uint rank = 0; rank < 8; rank++) {
        for (uint file = 0; file < 8; file++) {
            int idx = rank * 8 + file;
            if (pieces(idx).is_blank()) {
                space_counter++;
                continue;
            }
            if (space_counter > 0) {
                // Found a piece after a little space.
                ss << space_counter;
                space_counter = 0;
            }
            ss << pieces(idx).pretty();
        }
        if (space_counter > 0) {
            // Found a piece after a little space.
            ss << space_counter;
            space_counter = 0;
        }
        if (rank < 7) {
            ss << "/";
        }
    }

    ss << " ";
    ss << (whos_move == Colour::WHITE ? "w" : "b");
    ss << " ";

    if (aux_info->castling_rights[WHITE][KINGSIDE]) {
        ss << "K";
    }
    if (aux_info->castling_rights[WHITE][QUEENSIDE]) {
        ss << "Q";
    }
    if (aux_info->castling_rights[BLACK][KINGSIDE]) {
        ss << "k";
    }
    if (aux_info->castling_rights[BLACK][QUEENSIDE]) {
        ss << "q";
    }

    if (!(can_castle(WHITE) | can_castle(BLACK))) {
        ss << "-";
    }
    ss << " ";
    // En passent
    if (aux_info->en_passent_target == Square(0)) {
        ss << "-";
    } else {
        ss << aux_info->en_passent_target.pretty();
    }

    ss << " " << aux_info->halfmove_clock << " " << fullmove_counter;
    return ss.str();
}

void Board::pretty() const {
    for (uint rank = 0; rank < 8; rank++) {
        for (uint file = 0; file < 8; file++) {
            Square::square_t idx = 8 * rank + file;
            Square sq = Square(idx);
            if ((sq == aux_info->en_passent_target) & (aux_info->en_passent_target.get_value() != 0) &
                pieces(sq).is_blank()) {
                std::cout << "! ";
            } else {
                std::cout << pieces(sq).pretty();
            }
            std::cout << " ";
        }
        if ((rank == 0) & (whos_move == Colour::BLACK)) {
            std::cout << "  ***";
        }
        if ((rank == 7) & (whos_move == Colour::WHITE)) {
            std::cout << "  ***";
        }
        if (rank == 1) {
            std::cout << "  ";
            if (aux_info->castling_rights[BLACK][KINGSIDE]) {
                std::cout << 'k';
            }
            if (aux_info->castling_rights[BLACK][QUEENSIDE]) {
                std::cout << 'q';
            }
        }
        if (rank == 6) {
            std::cout << "  ";
            if (aux_info->castling_rights[WHITE][KINGSIDE]) {
                std::cout << 'K';
            }
            if (aux_info->castling_rights[WHITE][QUEENSIDE]) {
                std::cout << 'Q';
            }
        }
        if (rank == 3) {
            std::cout << "  " << std::setw(3) << std::setfill(' ') << aux_info->halfmove_clock;
        }
        if (rank == 4) {
            std::cout << "  " << std::setw(3) << std::setfill(' ') << fullmove_counter;
        }
        std::cout << std::endl;
    }
    std::cout << fen_encode() << std::endl;
    std::cout << std::hex << hash() << std::endl;
};

// Square

Square::Square(const std::string rf) {
    // eg convert "e4" to (3, 4) to 28
    if (rf.length() != 2) {
        throw std::domain_error("Coordinate length != 2");
    }

    uint rank = 7 - (toDigit(rf[1]) - 1);
    uint file = file_decode_map[rf[0]];

    value = 8 * rank + file;
}

std::string Square::pretty() const { return file_encode_map[file_index()] + std::to_string(8 - rank_index()); };

std::ostream &operator<<(std::ostream &os, const Square move) {
    os << move.pretty();
    return os;
}

std::ostream &operator<<(std::ostream &os, const Move move) {
    os << move.pretty();
    return os;
}

std::string print_score(const score_t score) {
    // Returns a human readable form of the score, rounding to nearest 10 cp.
    assert(score > -MATING_SCORE && score < MATING_SCORE);
    std::stringstream ss;
    if (is_mating(score)) {
        // Make for white.
        signed int n = MATING_SCORE - score;
        ss << "#" << n;
    } else if (is_mating(-score)) {
        // Make for black.
        signed int n = (score + MATING_SCORE);
        ss << "#-" << n;
    } else if (score > 5) {
        // White winning
        ss << "+" << score / 100 << "." << (score % 100) / 10;
    } else if (score < -5) {
        // Black winning
        ss << "-" << -score / 100 << "." << (-score % 100) / 10;
    } else {
        // White winning
        ss << "0.0";
    }
    std::string out;
    ss >> out;
    return out;
}

std::string Piece::pretty() const {
    const PieceType p = get_piece();
    const Colour c = get_colour();

    if (p == PAWN && c == WHITE) {
        return "P";
    } else if (p == PAWN && c == BLACK) {
        return "p";
    } else if (p == KNIGHT && c == WHITE) {
        return "N";
    } else if (p == KNIGHT && c == BLACK) {
        return "n";
    } else if (p == BISHOP && c == WHITE) {
        return "B";
    } else if (p == BISHOP && c == BLACK) {
        return "b";
    } else if (p == ROOK && c == WHITE) {
        return "R";
    } else if (p == ROOK && c == BLACK) {
        return "r";
    } else if (p == QUEEN && c == WHITE) {
        return "Q";
    } else if (p == QUEEN && c == BLACK) {
        return "q";
    } else if (p == KING && c == WHITE) {
        return "K";
    } else if (p == KING && c == BLACK) {
        return "k";
    } else {
        return ".";
    }
}
