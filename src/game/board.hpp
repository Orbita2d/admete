#pragma once
#include <map>
#include <array>
#include <vector>
#include <string>
#include "piece.hpp"

class Square {
public:
    typedef signed int square_t;
    constexpr Square(const square_t val) : value(val) {};
    constexpr Square(const square_t rank, const square_t file) {
        value = to_index(rank, file);
    };
    Square(const std::string rf);
    Square() = default;

    bool operator==(const Square that) const { return value == that.value; }
    bool operator!=(const Square that) const { return value != that.value; }

    constexpr Square operator+(const Square that) const {return Square(value + that.value);};
    constexpr Square operator-(const Square that) const {return Square(value - that.value);};
    constexpr Square operator|(const Square that) const {return Square(value | that.value);};

    constexpr square_t get_value() const{
        return value;
    }

    constexpr square_t rank_index() const{
        return value / 8;
    }
    constexpr square_t file_index() const{
        return value % 8;
    }

    constexpr Square file() const{
        return value & 0x07;
    }

    constexpr Square rank() const{
        return value & 0x38;
    }

    constexpr square_t right_diagonal() const{
        return rank_index() + file_index();
    }
    constexpr square_t left_diagonal() const{
        return rank_index() - file_index() + 7;
    }

    square_t to_north() const { return rank_index(); };
    square_t to_east() const { return 7-file_index(); };
    square_t to_south() const { return 7-rank_index(); };
    square_t to_west() const { { return file_index(); }};

    square_t to_northeast() const { return std::min(to_north(), to_east()); };
    square_t to_southeast() const { return std::min(to_south(), to_east()); };
    square_t to_southwest() const { return std::min(to_south(), to_west()); };
    square_t to_northwest() const { return std::min(to_north(), to_west()); };
    square_t to_dirx(const int dirx) const {
        switch (dirx)
        {
        case 0:
            return to_north();
        case 1:
            return to_east();
        case 2:
            return to_south();
        case 3:
            return to_west();
        case 4:
            return to_northeast();
        case 5:
            return to_southeast();
        case 6:
            return to_southwest();
        case 7:
            return to_northwest();
        default:
            return 0;
        }
    }

    static constexpr square_t to_index(const square_t rank, const square_t file){
        return 8 * rank + file;
    }

    constexpr operator square_t() const {
        return value;
    };

    std::string pretty_print() const;
    operator std::string() const {
        return pretty_print();
    };
private:
    square_t value = 0;
};
std::ostream& operator<<(std::ostream& os, const Square square);

namespace Squares {
    static constexpr Square N = -8;
    static constexpr Square E =  1;
    static constexpr Square S = 8;
    static constexpr Square W = -1;
    static constexpr Square NW = N + W;
    static constexpr Square NE = N + E;
    static constexpr Square SE = S + E;
    static constexpr Square SW = S + W;
    static constexpr Square NNW = N + N + W;
    static constexpr Square NNE = N + N + E;
    static constexpr Square ENE = E + N + E;
    static constexpr Square ESE = E + S + E;
    static constexpr Square SSE = S + S + E;
    static constexpr Square SSW = S + S + W;
    static constexpr Square WSW = W + S + W;
    static constexpr Square WNW = W + N + W;
    
    static constexpr Square Rank1 = 7 * 8;
    static constexpr Square Rank2 = 6 * 8;
    static constexpr Square Rank3 = 5 * 8;
    static constexpr Square Rank4 = 4 * 8;
    static constexpr Square Rank5 = 3 * 8;
    static constexpr Square Rank6 = 2 * 8;
    static constexpr Square Rank7 = 1 * 8;
    static constexpr Square Rank8 = 0 * 8;

    static constexpr Square FileA = 0;
    static constexpr Square FileB = 1;
    static constexpr Square FileC = 2;
    static constexpr Square FileD = 3;
    static constexpr Square FileE = 4;
    static constexpr Square FileF = 5;
    static constexpr Square FileG = 6;
    static constexpr Square FileH = 7;

    static constexpr std::array<Square, 8> by_dirx = {N, E, S, W, NE, SE, SW, NW};
}

class KnightMoveArray {
    // Class just to hold the iterator for a knight move so it doesn't have to be a vector.
public:
    KnightMoveArray() = default;
    Square operator[](const unsigned int i) { return move_array[i];};
    Square operator[](const unsigned int i) const { return move_array[i];};
    unsigned int len = 0;
    typedef Square * iterator;
    typedef const Square * const_iterator;
    iterator begin() { return &move_array[0]; }
    iterator end() { return &move_array[len]; }
    void push_back(const Square in) {
        move_array[len] = in;
        len++;
    }
private:
    std::array<Square, 8> move_array; 
};


KnightMoveArray knight_moves(const Square origin);

class Move {
public:
    Move(const Square o, const Square t) : origin(o), target(t) {};
    Move() = default;

    Square origin;
    Square target;

    std::string pretty_print() const{
        std::string move_string;
        if (capture){ move_string =  std::string(origin) + "x" + std::string(target); }
        else if(is_king_castle()) {move_string =  "O-O"; }
        else if(is_queen_castle()) {move_string =  "O-O-O"; }
        else {move_string = std::string(origin) + std::string(target);}
        if (is_knight_promotion()) {move_string= move_string + "n";}
        else if (is_bishop_promotion()) {move_string= move_string + "b";}
        else if (is_rook_promotion()) {move_string= move_string + "r";}
        else if (is_queen_promotion()) {move_string= move_string + "q";}
        return move_string;
    }

    void make_quiet() {
        promotion = false;
        capture = false;
        special1 = false;
        special2 = false;
    }
    void make_double_push() {
        promotion = false;
        capture = false;
        special1 = false;
        special2 = true;
    }
    void make_king_castle() {
        promotion = false;
        capture = false;
        special1 = true;
        special2 = false;
    }
    void make_queen_castle() {
        promotion = false;
        capture = false;
        special1 = true;
        special2 = true;
    }
    void make_capture() {
        promotion = false;
        capture = true;
        special1 = false;
        special2 = false;
    }
    void make_en_passent() {
        promotion = false;
        capture = true;
        special1 = false;
        special2 = true;
    }

    void make_bishop_promotion() {
        promotion = true;
        special1 = false;
        special2 = false;
    }
    void make_knight_promotion() {
        promotion = true;
        special1 = false;
        special2 = true;
    }
    void make_rook_promotion() {
        promotion = true;
        special1 = true;
        special2 = false;
    }
    void make_queen_promotion() {
        promotion = true;
        special1 = true;
        special2 = true;
    }
    constexpr bool is_capture() const {
        return capture;
    }
    constexpr bool is_ep_capture() const {
        return ((promotion == false) & 
                (capture == true) & 
                (special1 == false) & 
                (special2 == true));
    }
    constexpr bool is_double_push() const {
        return ((promotion == false) & 
                (capture == false) & 
                (special1 == false) & 
                (special2 == true));
    }
    constexpr bool is_king_castle() const {
        return ((promotion == false) & 
                (capture == false) & 
                (special1 == true) & 
                (special2 == false));
    }
    constexpr bool is_queen_castle() const {
        return ((promotion == false) & 
                (capture == false) & 
                (special1 == true) & 
                (special2 == true));
    }
    constexpr bool is_knight_promotion() const {
        return ((promotion == true) & 
                (special1 == false) & 
                (special2 == false));
    }
    constexpr bool is_bishop_promotion() const {
        return ((promotion == true) & 
                (special1 == false) & 
                (special2 == true));
    }
    constexpr bool is_rook_promotion() const {
        return ((promotion == true) & 
                (special1 == true) & 
                (special2 == false));
    }
    constexpr bool is_queen_promotion() const {
        return ((promotion == true) & 
                (special1 == true) & 
                (special2 == true));
    }
    constexpr bool is_promotion() const {
        return promotion;
    }
    Piece captured_peice;
private:
    bool promotion = 0;
    bool capture = 0;
    bool special1 = 0;
    bool special2 = 0;
};
std::ostream& operator<<(std::ostream& os, const Move move);

constexpr bool white_move = false;
constexpr bool black_move = true;

typedef std::array<std::array<bool, 64>, 64> bitboard;

struct AuxilliaryInfo {
    // Information that is game history dependent, that would otherwise need to be encoded in a move.
    bool castle_white_kingside = true;
    bool castle_white_queenside = true;
    bool castle_black_kingside = true;
    bool castle_black_queenside = true;
    uint halfmove_clock = 0;
    Square en_passent_target;
    bool is_check = false;
};

class Board {
public:
    Board() {
        pieces.fill(Pieces::Blank);
    };

    void fen_decode(const std::string& fen);

    void print_board_idx();

    void print_board();

    void print_board_extra();

    std::string print_move(const Move move, std::vector<Move> &legal_moves) const;
    bool is_free(const Square target) const;
    Square slide_to_edge(const Square origin, const Square direction, const uint to_edge) const;
    void get_sliding_moves(const Square origin, const Piece colour, const Square direction, const uint to_edge, std::vector<Move> &moves) const;
    void get_step_moves(const Square origin, const Square target, const Piece colour, std::vector<Move> &moves) const;
    void get_pawn_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_bishop_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_rook_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_queen_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_knight_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_king_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_castle_moves(const Piece colour, std::vector<Move> &moves) const;
    std::vector<Move> get_pseudolegal_moves() const;
    std::vector<Move> get_moves();
    bool is_check(const Square square, const Piece colour) const;
    bool is_in_check() const;

    void make_move(Move &move);
    void unmake_move(const Move move);
    void unmake_last() {unmake_move(last_move); }

    void try_move(const std::string move_sting);
    Square find_king(const Piece colour) const;
    void search_kings();
    void search_pins(const Piece colour);
    void search_pins(const Piece colour, const Square origin, const Square target);
    bool is_pinned(const Square origin) const;
    void search_sliding_checks(const Piece colour, const Square origin);
    void search_step_checks(const Piece colour, const Square origin);

    std::array<Piece, 64> pieces;
private:
    bool whos_move = white_move;
    uint fullmove_counter = 1;
    uint ply_counter = 0;
    Move last_move;
    std::array<Square, 2> king_square;
    // Array of absolute pins for legal move generation. Max 8 pieces per king.
    std::array<Square, 16> pinned_pieces;
    std::array<AuxilliaryInfo, 256> aux_history;
    AuxilliaryInfo aux_info;
};

constexpr Square forwards(const Piece colour);
constexpr Square back_rank(const Piece colour);