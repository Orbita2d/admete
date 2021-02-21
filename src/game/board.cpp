#include <map>
#include <iostream>
#include <istream>
#include <sstream>
#include <iomanip>
#include <cstdint> //for uint8_t
#include <stdexcept>

#include "board.hpp"

#define toDigit(c) ( c - '0')

std::map<char, Piece> fen_decode_map = {
    {'p', Pieces::Black | Pieces::Pawn},
    {'n', Pieces::Black | Pieces::Knight},
    {'b', Pieces::Black | Pieces::Bishop},
    {'r', Pieces::Black | Pieces::Rook},
    {'q', Pieces::Black | Pieces::Queen},
    {'k', Pieces::Black | Pieces::King},
    {'P', Pieces::White | Pieces::Pawn},
    {'N', Pieces::White | Pieces::Knight},
    {'B', Pieces::White | Pieces::Bishop},
    {'R', Pieces::White | Pieces::Rook},
    {'Q', Pieces::White | Pieces::Queen},
    {'K', Pieces::White | Pieces::King}
};

std::map<char, Square::square_t> file_decode_map = {
    {'a', 0},
    {'b', 1},
    {'c', 2},
    {'d', 3},
    {'e', 4},
    {'f', 5},
    {'g', 6},
    {'h', 7},

};

std::map<Square::square_t, char> file_encode_map = {
    {0, 'a'},
    {1, 'b'},
    {2, 'c'},
    {3, 'd'},
    {4, 'e'},
    {5, 'f'},
    {6, 'g'},
    {7, 'h'},

};

constexpr Square forwards(const Piece colour) {
    if (colour.is_white()) {
        return Squares::N;
    } else {
        return Squares::S;
    }
}

constexpr Square back_rank(const Piece colour) {
    if (colour.is_white()) {
        return Squares::Rank1;
    } else {
        return Squares::Rank8;
    }
}

Square::Square(const std::string rf) {
// eg convert "e4" to (3, 4) to 28
    if (rf.length() != 2) {
        throw std::domain_error("Coordinate length != 2");
    }
    
    uint rank = 7 - (toDigit(rf[1]) - 1);
    uint file = file_decode_map[rf[0]];

    value = rank + 8 * file;
}

std::string Square::pretty_print() const{
    return file_encode_map[file_index()] + std::to_string(8 - rank_index());
};

std::ostream& operator<<(std::ostream& os, const Square move) {
        os << move.pretty_print();
        return os;
    }

std::array<KnightMoveArray, 64> GenerateKnightMoves(){
    std::array<KnightMoveArray, 64> meta_array;
    KnightMoveArray moves;
    for (unsigned int i = 0; i < 64; i++){
        moves = KnightMoveArray();
        Square origin = Square(i);
        if (origin.to_north() >= 2){
            if(origin.to_west() >= 1) {
                moves.push_back(origin + Squares::NNW);
            }
            if(origin.to_east() >= 1) {
                moves.push_back(origin + Squares::NNE);
            }
        }
        if (origin.to_east() >= 2){
            if(origin.to_north() >= 1) {
                moves.push_back(origin + Squares::ENE);
            }
            if(origin.to_south() >= 1) {
                moves.push_back(origin + Squares::ESE);
            }
        }
        if (origin.to_south() >= 2){
            if(origin.to_east() >= 1) {
                moves.push_back(origin + Squares::SSE);
            }
            if(origin.to_west() >= 1) {
                moves.push_back(origin + Squares::SSW);
            }
        }
        if (origin.to_west() >= 2){
            if(origin.to_south() >= 1) {
                moves.push_back(origin + Squares::WSW);
            }
            if(origin.to_north() >= 1) {
                moves.push_back(origin + Squares::WNW);
            }
        }
        meta_array[i] = moves;
    }
    return meta_array;
}

std::array<KnightMoveArray, 64> knight_meta_array = GenerateKnightMoves();

KnightMoveArray knight_moves(const Square origin){
    return knight_meta_array[origin];
}

std::ostream& operator<<(std::ostream& os, const Move move) {
    os << move.pretty_print();
    return os;
}

void Board::fen_decode(const std::string& fen){
    uint N = fen.length(), board_position;
    uint rank = 0, file = 0;
    char my_char;
    
    std::stringstream stream;
    // First, go through the board position part of the fen string
    for (uint i = 0; i < N; i++) {
        my_char = fen[i];
        if (my_char == '/'){
            rank++;
            file = 0;
            continue;
        }
        if (isdigit(my_char)) {
            file += toDigit(my_char);
            continue;
        }
        if (my_char == ' ') {
            // Space is at the end of the board position section
            board_position = i;
            break;
        }
        // Otherwise should be a character for a piece
        pieces[Square::to_index(rank, file)] = fen_decode_map[my_char];
        file ++;
    }
    
    search_kings();
    search_pins(white_move);
    search_pins(black_move);

    std::string side_to_move, castling, en_passent;
    int halfmove = 0, counter = 0;
    stream = std::stringstream(fen.substr(board_position));
    stream >> std::ws;
    stream >> side_to_move >> std::ws;
    stream >> castling >> std::ws;
    stream >>  en_passent >> std::ws;
    stream >> halfmove >> std::ws;
    stream >> counter >> std::ws;

    // Side to move
    if (side_to_move.length() > 1) {
        throw std::domain_error("<Side to move> length > 1");
    }
    switch (side_to_move[0])
    {
    case 'w':
        whos_move = white_move;
        break;

    case 'b':
        whos_move = black_move;
        break;
    
    default:
        throw std::domain_error("Unrecognised <Side to move> character");
    }

    castle_black_kingside = false;
    castle_black_queenside = false;
    castle_white_kingside = false;
    castle_white_queenside = false;
    // Castling rights
    if (castling.length() > 4) {
        throw std::domain_error("<Castling> length > 4");
    }
    if (castling[0] == '-') {
        // No castling rights, continue
    } else {
        for (uint i = 0; i < castling.length(); i++) {
            switch (castling[i])
            {
            case 'q':
                castle_black_queenside = true;
                break;

            case 'Q':
                castle_white_queenside = true;
                break;

            case 'k':
                castle_black_kingside = true;
                break;

            case 'K':
                castle_white_kingside = true;
                break;

            default:
                throw std::domain_error("Unrecognised <Castling> character");
            }
        }
    }

    // En passent
    if (en_passent.length() > 2) {
        throw std::domain_error("<en passent> length > 2");
    }
    if (en_passent[0] == '-') {
        // No en passent, continue
        en_passent_target = 0;
    } else {
        en_passent_target = Square(en_passent);
    }

    // Halfmove clock
    halfmove_clock = halfmove;
    // Fullmove counter
    fullmove_counter = counter;
    
};

void Board::print_board_idx() {
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            uint idx = 8*rank +file;
            std::cout << std::setw(2) << std::setfill('0')  << idx << ' ';
        }
            std::cout << std::endl;
    }
};

void Board::print_board() {
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            Square::square_t idx = 8*rank +file;
            if (idx == en_passent_target & pieces[idx].is_blank() & en_passent_target.get_value() != 0){
                std::cout << '!';
            } else {
                std::cout << pieces[idx].pretty_print() << ' ';
            }
        }
        std::cout << std::endl;
    }
}

void Board::print_board_extra(){
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            Square::square_t idx = 8*rank +file;
            if (idx == en_passent_target & en_passent_target.get_value() != 0 & pieces[idx].is_blank()){
                std::cout << '-';
            } else {
                std::cout << pieces[idx].pretty_print();
            }
            std::cout << ' ';
        }
        if (rank == 0 & whos_move == black_move){
            std::cout << "  ***";
        }
        if (rank == 7 & whos_move == white_move){
            std::cout << "  ***";
        }
        if (rank == 1){
            std::cout << "  ";
            if (castle_black_kingside) { std::cout << 'k'; }
            if (castle_black_queenside) { std::cout << 'q'; }
        }
        if (rank == 6){
            std::cout << "  ";
            if (castle_white_kingside) { std::cout << 'K'; }
            if (castle_white_queenside) { std::cout << 'Q'; }
        }
        if (rank == 3){
            std::cout << "  " << std::setw(3) << std::setfill(' ') << halfmove_clock;
            
        }
        if (rank == 4){
            std::cout << "  " << std::setw(3) << std::setfill(' ') << fullmove_counter;
            
        }
        std::cout << std::endl;
    }
};

void add_pawn_promotions(const Move move, std::vector<Move> &moves) {
    // Add all variations of promotions to a move.
    Move my_move = move;
    my_move.make_knight_promotion();
    moves.push_back(my_move);
    my_move.make_bishop_promotion();
    moves.push_back(my_move);
    my_move.make_rook_promotion();
    moves.push_back(my_move);
    my_move.make_queen_promotion();
    moves.push_back(my_move);
}

void Board::get_pawn_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const{
    Square target;
    Move move;
    if (colour.is_white()) {
        // Moves are North
        // Normal pushes.
        target = origin + (Squares::N);
        if (pieces[target].is_piece(Pieces::Blank)) {
            move = Move(origin, target);
            if (origin.rank() == Squares::Rank7) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
        // Normal captures.
        if (origin.to_west() != 0) {
            target = origin + (Squares::NW);
            if (pieces[target].is_colour(~colour)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank7) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }

        if (origin.to_east() != 0) {
            target = origin + (Squares::NE);
            if (pieces[target].is_colour(~colour)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank7) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }
        // En-passent
        if (origin.rank() == Squares::Rank5) {
            if (origin.to_west() != 0 & en_passent_target == origin + Squares::NW | 
                origin.to_east() != 0 & en_passent_target == origin + Squares::NE ) {
                move = Move(origin, en_passent_target); 
                move.make_en_passent();
                moves.push_back(move);
            }
        }

        // Look for double pawn push possibility
        if (origin.rank() == Squares::Rank2) {
            target = origin + (Squares::N + Squares::N);
            if (pieces[target].is_piece(Pieces::Blank) & pieces[origin + Squares::N].is_piece(Pieces::Blank)) {
                move = Move(origin, target);
                move.make_double_push();
                moves.push_back(move);
            }
        }
    } else
    {
        // Moves are South
        // Normal pushes.
        target = origin + (Squares::S);
        if (pieces[target].is_piece(Pieces::Blank)) {
            move = Move(origin, target);
            if (origin.rank() == Squares::Rank2) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
        // Normal captures.
        if (origin.to_west() != 0) {
            target = origin + (Squares::SW);
            if (pieces[target].is_colour(~colour)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank2) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }

        if (origin.to_east() != 0) {
            target = origin + (Squares::SE);
            if (pieces[target].is_colour(~colour)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank2) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }
        // En-passent
        if (origin.rank() == Squares::Rank4) {
            if (origin.to_west() != 0 & en_passent_target == origin + Squares::SW | 
                origin.to_east() != 0 & en_passent_target == origin + Squares::SE ) {
                move = Move(origin, en_passent_target); 
                move.make_en_passent();
                moves.push_back(move);
            }
        }

        // Look for double pawn push possibility
        if (origin.rank() == Squares::Rank7) {
            target = origin + (Squares::S + Squares::S);
            if (pieces[target].is_piece(Pieces::Blank) & pieces[origin + Squares::S].is_piece(Pieces::Blank)) {
                move = Move(origin, target);
                move.make_double_push();
                moves.push_back(move);
            }
        }
    }
}

void Board::get_sliding_moves(const Square origin, const Piece colour, const Square direction, const uint to_edge, std::vector<Move> &moves) const {
    Square target = origin;
    Move move;
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (pieces[target].is_piece(Pieces::Blank)) {
            // Blank Square
            moves.push_back(Move(origin, target));
            continue;
        } else if (pieces[target].is_colour(~colour)) {
            // Enemy piece
            move = Move(origin, target);
            move.make_capture();
            moves.push_back(move);
            return;
        } else if (pieces[target].is_colour(colour)) {
            // Our piece, no more legal moves.
            return;
        }
    };
}

void Board::get_rook_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    get_sliding_moves(origin, colour, Squares::N, origin.to_north(), moves);
    get_sliding_moves(origin, colour, Squares::E, origin.to_east(), moves);
    get_sliding_moves(origin, colour, Squares::S, origin.to_south(), moves);
    get_sliding_moves(origin, colour, Squares::W, origin.to_west(), moves);
    
}

void Board::get_bishop_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    get_sliding_moves(origin, colour, Squares::NE, std::min(origin.to_north(), origin.to_east()), moves);
    get_sliding_moves(origin, colour, Squares::SE, std::min(origin.to_east(), origin.to_south()), moves);
    get_sliding_moves(origin, colour, Squares::SW, std::min(origin.to_south(), origin.to_west()), moves);
    get_sliding_moves(origin, colour, Squares::NW, std::min(origin.to_west(), origin.to_north()), moves);
    
}

void Board::get_queen_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    // Queen moves are the union superset of rook and bishop moves
    get_bishop_moves(origin,colour, moves);
    get_rook_moves(origin, colour, moves);
}
void Board::get_step_moves(const Square origin, const Square target, const Piece colour, std::vector<Move> &moves) const {
    Move move;
    if (pieces[target].is_colour(colour)) {
        // Piece on target is our colour.
        return;
    } else if (pieces[target].is_colour(~colour)) {
        //Piece on target is their colour.
        move = Move(origin, target);
        move.make_capture();
        moves.push_back(move);
        return;
    } else {
        // Space is blank.
        move = Move(origin, target);
        moves.push_back(move);
        return;
    }
}
void Board::get_king_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    // We should really be careful that we aren't moving into check here.
    // Look to see if we are on an edge.
    if (origin.to_north() != 0) {
        get_step_moves(origin, origin + Squares::N, colour, moves);
    }
    if (origin.to_east() != 0) {
        get_step_moves(origin, origin + Squares::E, colour, moves);
    }
    if (origin.to_south() != 0) {
        get_step_moves(origin, origin + Squares::S, colour, moves);
    }
    if (origin.to_west() != 0) {
        get_step_moves(origin, origin + Squares::W, colour, moves);
    }
    if (origin.to_north() != 0 & origin.to_east() != 0) {
        get_step_moves(origin, origin + Squares::NE, colour, moves);
    }
    if (origin.to_south() != 0 & origin.to_east() != 0) {
        get_step_moves(origin, origin + Squares::SE, colour, moves);
    }
    if (origin.to_south() != 0 & origin.to_west() != 0) {
        get_step_moves(origin, origin + Squares::SW, colour, moves);
    }
    if (origin.to_north() != 0 & origin.to_west() != 0) {
        get_step_moves(origin, origin + Squares::NW, colour, moves);
    }
}

bool Board::is_free(const Square target) const{  return pieces[target].is_blank(); };

void Board::get_castle_moves(const Piece colour, std::vector<Move> &moves) const {
    Move move;
    if (colour.is_white()) {
        // You can't castle through check, or while in check
        if (is_check(Squares::FileE | Squares::Rank1, colour)) {return; }
        if (castle_white_queenside 
            & is_free(Squares::FileD | Squares::Rank1) 
            & is_free(Squares::FileC | Squares::Rank1)
            & is_free(Squares::FileB | Squares::Rank1)
            & !is_check(Squares::FileD | Squares::Rank1, colour)) {
            move = Move(Squares::FileE | Squares::Rank1, Squares::FileC | Squares::Rank1);
            move.make_queen_castle();
            moves.push_back(move);
        }
        if (castle_white_kingside 
            & is_free(Squares::FileF | Squares::Rank1) 
            & is_free(Squares::FileG | Squares::Rank1)
            & !is_check(Squares::FileF | Squares::Rank1, colour)
            & !is_check(Squares::FileE | Squares::Rank1, colour)) {
            move = Move(Squares::FileE | Squares::Rank1, Squares::FileG | Squares::Rank1);
            move.make_king_castle();
            moves.push_back(move);
        }
    } else
    {
        if (is_check(Squares::FileE | Squares::Rank8, colour)) {return; }
        if (castle_black_queenside 
            & is_free(Squares::FileD | Squares::Rank8) 
            & is_free(Squares::FileC | Squares::Rank8)
            & is_free(Squares::FileB | Squares::Rank8)
            & !is_check(Squares::FileD | Squares::Rank8, colour)) {
            move = Move(Squares::FileE | Squares::Rank8, Squares::FileC | Squares::Rank8);
            move.make_queen_castle();
            moves.push_back(move);
        }
        if (castle_black_kingside 
            & is_free(Squares::FileF | Squares::Rank8) 
            & is_free(Squares::FileG | Squares::Rank8)
            & !is_check(Squares::FileF | Squares::Rank8, colour)) {
            move = Move(Squares::FileE | Squares::Rank8, Squares::FileG | Squares::Rank8);
            move.make_king_castle();
            moves.push_back(move);
        }
    }
    
}

void Board::get_knight_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    for (Square target : knight_moves(origin)) {
        get_step_moves(origin, target, colour, moves);
    }
};

std::vector<Move> Board::get_pseudolegal_moves() const {
    Piece colour = whos_move;
    
    Piece piece;
    std::vector<Square> targets;
    std::vector<Move> moves;
    moves.reserve(256);
    for (Square::square_t i = 0; i < 64; i++) {
        Square square = Square(i);
        piece = pieces[square];
        if (not piece.is_colour(colour)) {continue; }
        if (piece.is_knight()) {
            get_knight_moves(square, colour, moves);
        } else if (piece.is_pawn()) {
            get_pawn_moves(square, colour, moves);
        } else if (piece.is_rook()) {
            get_rook_moves(square, colour, moves);
        } else if (piece.is_bishop()) {
            get_bishop_moves(square, colour, moves);
        } else if (piece.is_queen()) {
            get_queen_moves(square, colour, moves);
        } else if (piece.is_king()) {
            get_king_moves(square, colour, moves);
        }
    }
    get_castle_moves(colour, moves);
    return moves;
}


std::string Board::print_move(const Move move, std::vector<Move> &legal_moves) const{
    Piece moving_piece = pieces[move.origin];
    bool ambiguity_flag = false;
    // Pawn captures are special.
    if (moving_piece.is_piece(Pieces::Pawn) & move.is_capture()) {
        return std::string(1, file_encode_map[move.origin.file_index()]) + "x" + move.target.pretty_print();
    }
    // Castles are special
    if (move.is_king_castle() | move.is_queen_castle()) {
        return move.pretty_print();
    }
    for (Move a_move : legal_moves) {
        // Ignore moves targeting somewhere else.
        if (move.target != a_move.target) {continue;}
        // Ignore this move when we find it.
        if (move.origin == a_move.origin) {continue;}
        // Check for ambiguity
        if (moving_piece.is_piece(pieces[a_move.origin])) { ambiguity_flag = true; } 
    }
    std::string notation;
    if (ambiguity_flag) {
        // This is ambiguous, use full disambiguation for now.
        notation = move.origin.pretty_print() + moving_piece.get_algebraic_character();
    }else {
        // Unambiguous move
        notation = moving_piece.get_algebraic_character();
    };
    if (move.is_capture()) {
        notation = notation + "x" + move.target.pretty_print();
    } else {
        notation = notation + move.target.pretty_print();
    }
    if (move.is_knight_promotion()) {
        notation = notation + "=N";
    } else if (move.is_bishop_promotion()) {
        notation = notation + "=B";
    } else if (move.is_rook_promotion()) {
        notation = notation + "=R";
    } else if (move.is_queen_promotion()) {
        notation = notation + "=Q";
    } 
    return notation;
}

void Board::make_move(Move &move) {
    last_move = move;
    // Iterate counters.
    if (whos_move == black_move) {fullmove_counter++;}
    move.halfmove_clock = halfmove_clock;
    if (move.is_capture() | pieces[move.origin].is_piece(Pieces::Pawn)) {
        halfmove_clock = 0;
    } else {halfmove_clock++ ;}
    // Track en-passent square
    move.en_passent_target = en_passent_target;
    if (move.is_double_push()) {
        // Little hacky but this is the square in between.
        en_passent_target = (move.origin.get_value() + move.target.get_value())/2;
    } else {
        en_passent_target = 0;
    }
    
    move.black_kingside_rights = castle_black_kingside;
    move.black_queenside_rights = castle_black_queenside;
    move.white_kingside_rights = castle_white_kingside;
    move.white_queenside_rights = castle_white_queenside;

    if (pieces[move.origin].is_king()) {
        if (whos_move == white_move) {
            king_square[0] = move.target;
        } else {
            king_square[1] = move.target;
        }
    }

    // Castling is special
    if (move.is_king_castle()) {
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Pieces::Blank;
        pieces[move.origin + Squares::E] = pieces[move.origin.rank() | Squares::FileH];
        pieces[move.origin.rank() | Squares::FileH] = Pieces::Blank;
        if (whos_move == white_move) {
            castle_white_kingside  = false;
            castle_white_queenside = false;
        } else {
            castle_black_kingside  = false;
            castle_black_queenside = false;
        }
    } else if (move.is_queen_castle()) {
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Pieces::Blank;
        pieces[move.origin + Squares::W] = pieces[move.origin.rank() | Squares::FileA];
        pieces[move.origin.rank() | Squares::FileA] = Pieces::Blank;
        if (whos_move == white_move) {
            castle_white_kingside  = false;
            castle_white_queenside = false;
        } else {
            castle_black_kingside  = false;
            castle_black_queenside = false;
        }
    } else if(move.is_ep_capture()) {
        // En-passent is weird too.
        const Square captured_square = move.origin.rank() | move.target.file();
        // Make sure to lookup and record the piece captured 
        move.captured_peice = pieces[captured_square];
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Pieces::Blank;
        pieces[captured_square] = Pieces::Blank;
    } else if (move.is_capture()){
        // Make sure to lookup and record the piece captured 
        move.captured_peice = pieces[move.target];
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Pieces::Blank;
    } else {
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Pieces::Blank;
    }
    // And now do the promotion if it is one.
    if (move.is_knight_promotion()) {
        pieces[move.target] = pieces[move.target].get_colour() | Pieces::Knight;
    } else if (move.is_bishop_promotion()){
        pieces[move.target] = pieces[move.target].get_colour() | Pieces::Bishop;
    } else if (move.is_rook_promotion()){
        pieces[move.target] = pieces[move.target].get_colour() | Pieces::Rook;
    } else if (move.is_queen_promotion()){
        pieces[move.target] = pieces[move.target].get_colour() | Pieces::Queen;
    }
    
    if ((castle_white_kingside | castle_white_queenside) & whos_move == white_move){
        if (move.origin == (Squares::Rank1 | Squares::FileE)) {
            // Check if they've moved their King
            castle_white_kingside  = false;
            castle_white_queenside  = false;
        }
        if (move.origin == (Squares::FileH | Squares::Rank1)) {
            // Check if they've moved their rook.
            castle_white_kingside  = false;
        } else if (move.origin == (Squares::FileA | Squares::Rank1)) {
            // Check if they've moved their rook.
            castle_white_queenside  = false;
        }
    }
    if ((castle_black_kingside | castle_black_queenside) & whos_move == black_move){
        if (move.origin == (Squares::Rank8 | Squares::FileE)) {
            // Check if they've moved their King
            castle_black_kingside  = false;
            castle_black_queenside  = false;
        }
        if (move.origin == (Squares::FileH | Squares::Rank8)) {
            // Check if they've moved their rook.
            castle_black_kingside  = false;
        } else if (move.origin == (Squares::FileA | Squares::Rank8)) {
            // Check if they've moved their rook.
            castle_black_queenside  = false;
        }
    }

    if ((castle_black_kingside | castle_black_queenside) & whos_move == white_move){
        // Check for rook captures.
        if (move.target == (Squares::FileH | Squares::Rank8)) {
            castle_black_kingside = false;
        }
        if (move.target == (Squares::FileA | Squares::Rank8)) {
            castle_black_queenside = false;
        }
    }
    if ((castle_white_kingside | castle_white_queenside) & whos_move == black_move){
        // Check for rook captures.
        if (move.target == (Squares::FileH | Squares::Rank1)) {
            castle_white_kingside = false;
        }
        if (move.target == (Squares::FileA | Squares::Rank1)) {
            castle_white_queenside = false;
        }
    }
    // Switch whos turn it is to play
    whos_move = ! whos_move;
    if (is_in_check()) {
        currently_check = true;
    } else {
        currently_check = false;
    }
    // Update the pins for the person whos turn it is. 
    search_pins(whos_move);
}

void Board::unmake_move(const Move move) {
    // Iterate counters.
    if (whos_move == white_move) {fullmove_counter--;}
    halfmove_clock = move.halfmove_clock;
    // Switch whos turn it is to play
    whos_move = ! whos_move;
    // Track en-passent square
    en_passent_target = move.en_passent_target;
    // Castling is special

    if (pieces[move.target].is_king()) {
        if (whos_move == white_move) {
            king_square[0] = move.origin;
        } else {
            king_square[1] = move.origin;
        }
    }

    if (move.is_king_castle()) {
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Pieces::Blank;
        pieces[move.origin.rank() | Squares::FileH] = pieces[move.origin + Squares::E];
        pieces[move.origin + Squares::E] = Pieces::Blank;
    } else if (move.is_queen_castle()) {
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Pieces::Blank;
        pieces[move.origin.rank() | Squares::FileA] = pieces[move.origin + Squares::W];
        pieces[move.origin + Squares::W] = Pieces::Blank;
    } else if(move.is_ep_capture()) {
        // En-passent is weird too.
        const Square captured_square = move.origin.rank() | move.target.file();
        // Make sure to lookup and record the piece captured 
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Pieces::Blank;
        pieces[captured_square] = move.captured_peice;
    } else if (move.is_capture()){
        // Make sure to lookup and record the piece captured 
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = move.captured_peice;
    } else {
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Pieces::Blank;
    }
    // And now do the promotion if it is one.
    if (move.is_promotion()) {
        pieces[move.origin] = pieces[move.origin].get_colour() | Pieces::Pawn;
    }
    castle_black_kingside = move.black_kingside_rights;
    castle_black_queenside = move.black_queenside_rights;
    castle_white_kingside = move.white_kingside_rights;
    castle_white_queenside = move.white_queenside_rights;

    if (is_in_check()) {
        currently_check = true;
    } else {
        currently_check = false;
    }
    search_pins(whos_move);
}

void Board::try_move(const std::string move_sting) {
    bool flag = false;
    std::vector<Move> legal_moves = get_moves();
    for (Move move : legal_moves) {
        if (move_sting == print_move(move, legal_moves)) {
            make_move(move);
            flag = true;
            break;
        } else if (move_sting == move.pretty_print()){
            make_move(move);
            flag = true;
            break;
        }
    }
    if (!flag) {
        std::cout << "Move not found: " << move_sting << std::endl;
    }
}

Square Board::slide_to_edge(const Square origin, const Square direction, const uint to_edge) const {
    // Look along a direction until you get to the edge of the board or a piece.
    Square target = origin;
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (is_free(target)) {
            // Blank Square
            continue;
        } else {
            return target;
        }
    };
    return target;
}

bool Board::is_check(const Square origin, const Piece colour) const{
    // Want to (semi-efficiently) see if a square is attacked, ignoring pins.
    // First off, if the square is attacked by a knight, it's definitely in check.
    for (Square target : knight_moves(origin)) {
        if (pieces[target] == (~colour | Pieces::Knight)) {
            return true;
        }
    }

    // Pawn square
    Square target;
    if (origin.to_west() != 0) {
        target = origin + (Squares::W + forwards(colour));
        if (pieces[target] == (~colour | Pieces::Pawn)) {
            return true;
        }
    }
    if (origin.to_east() != 0) {
        target = origin + (Squares::E + forwards(colour));
        if (pieces[target] == (~colour | Pieces::Pawn)) {
            return true;
        }
    }


    // Sliding moves.
    Piece target_piece;
    std::array<Square, 4> targets;
    targets[0] = slide_to_edge(origin, Squares::N, origin.to_north());
    targets[1] = slide_to_edge(origin, Squares::E, origin.to_east());
    targets[2] = slide_to_edge(origin, Squares::S, origin.to_south());
    targets[3] = slide_to_edge(origin, Squares::W, origin.to_west());
    for (Square target : targets){
        target_piece = pieces[target];
        if ((target_piece == (~colour | Pieces::Rook)) | (target_piece == (~colour | Pieces::Queen))) {
            return true;
        }
    }
    
    // Diagonals
    targets[0] = slide_to_edge(origin, Squares::NE, std::min(origin.to_north(), origin.to_east()));
    targets[1] = slide_to_edge(origin, Squares::SE, std::min(origin.to_south(), origin.to_east()));
    targets[2] = slide_to_edge(origin, Squares::SW, std::min(origin.to_south(), origin.to_west()));
    targets[3] = slide_to_edge(origin, Squares::NW, std::min(origin.to_north(), origin.to_west()));
    for (Square target : targets){
        target_piece = pieces[target];
        if ((target_piece == (~colour | Pieces::Bishop)) | (target_piece == (~colour | Pieces::Queen))) {
            return true;
        }
    }
    
    // King's can't be next to each other in a game, but this is how we enforce that.
    if (origin.to_north() != 0) {
        if (pieces[origin + Squares::N] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
        if (origin.to_east() != 0) {
            if (pieces[origin + Squares::NE] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
        if (origin.to_west() != 0) {
            if (pieces[origin + Squares::NW] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
    } 
    if (origin.to_south() != 0) {
        if (pieces[origin + Squares::S] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
        if (origin.to_east() != 0) {
            if (pieces[origin + Squares::SE] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
        if (origin.to_west() != 0) {
            if (pieces[origin + Squares::SW] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
    } 
    if (origin.to_east() != 0) {
        if (pieces[origin + Squares::E] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
    }
    if (origin.to_west() != 0) {
        if (pieces[origin + Squares::W] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
    }
    
    // Nothing so far, that's good, no checks.
    return false;
}

void Board::search_kings() {
    for (Square::square_t i = 0; i < 64; i++) {
        if (pieces[i].is_king()) { 
            if (pieces[i].is_white()) {
                king_square[0] = i;
            } else {
                king_square[1] = i;
            }
        }
    }
}

Square Board::find_king(const Piece colour) const{
    return colour.is_white() ? king_square[0] : king_square[1];
}

std::vector<Move> Board::get_moves(){
    Piece colour = whos_move;
    const std::vector<Move> pseudolegal_moves = get_pseudolegal_moves();
    std::vector<Move> legal_moves;
    legal_moves.reserve(256);
    Square king_square = find_king(colour);
    if (is_check(king_square, colour)) {
        // To test if a move is legal, make the move and see if we are in check. 
        for (Move move : pseudolegal_moves) {
            make_move(move);
            if (move.origin == king_square) {
                // This is a king move, check where he's gone.
                if (!is_check(move.target, colour)) {legal_moves.push_back(move); }
            } else {
                if (!is_check(king_square, colour)) {legal_moves.push_back(move); }
            }
            unmake_move(move);
        }
        return legal_moves;
    } 
    
    // Otherwise we can be smarter.
    for (Move move : pseudolegal_moves) {
        if (move.origin == king_square) {
            // This is a king move, check where he's gone.
            make_move(move);
            if (!is_check(move.target, colour)) {legal_moves.push_back(move); }
            unmake_move(move);  
        } else if (is_pinned(move.origin)) {
            // This piece is pinned, double check that the move doesn't put us in check.
            make_move(move);
            if (!is_check(king_square, colour)) {legal_moves.push_back(move); }
            unmake_move(move);
        } else if (move.is_ep_capture()) {
            // en_passent's are weird.
            make_move(move);
            if (!is_check(king_square, colour)) {legal_moves.push_back(move); }
            unmake_move(move);
        } else {
            // Piece isn't pinned, it can do what it wants. 
            legal_moves.push_back(move);
        }
    }
    return legal_moves;
}

bool Board::is_in_check() const {
    Piece colour = whos_move;
    Square king_square = find_king(colour);
    return is_check(king_square, colour);
}

bool in_line(const Square origin, const Square target){
    if (origin.file() == target.file()) {return true; }
    if (origin.rank() == target.rank()) {return true; }
    if (origin.left_diagonal() == target.left_diagonal()) {return true; }
    if (origin.right_diagonal() == target.right_diagonal()) {return true; }
    return false;
}
bool Board::is_pinned(const Square origin) const{
    for (Square target : pinned_pieces) {
        if (origin == target) {
            return true;
        }
    }
    return false;
}
Square slide_rook_pin(const Board &board, const Square origin, const Square direction, const uint to_edge, const Piece colour) {
    // Slide from the origin in a direction, to look for a peice pinned by some sliding piece of ~colour
    Square target = origin;
    // Flag of if we have found a piece in the way yet.
    bool flag = false;
    Square found_piece;
    // First look south
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (board.is_free(target)) {
            // Blank Square
            continue;
        }
        if (board.pieces[target].is_colour(colour)) {
            // It's our piece.
            if (flag) {
                // This piece isn't pinned.
                return origin;
            } else {
                flag = true;
                found_piece = target;
                continue;
            }
        } else {
            // It's their piece.
            if (flag) {
                // The last piece we found may be pinned.
                return (board.pieces[target].is_rook() | board.pieces[target].is_queen()) ? found_piece : origin;
            } else {
                // Origin is attacked, but no pins here.
                return origin;
            }
        }
    };
    return origin;
}

Square slide_bishop_pin(const Board &board, const Square origin, const Square direction, const uint to_edge, const Piece colour) {
    // Slide from the origin in a direction, to look for a peice pinned by some sliding piece of ~colour
    Square target = origin;
    // Flag of if we have found a piece in the way yet.
    bool flag = false;
    Square found_piece;
    // First look south
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (board.is_free(target)) {
            // Blank Square
            continue;
        }
        if (board.pieces[target].is_colour(colour)) {
            // It's our piece.
            if (flag) {
                // This piece isn't pinned.
                return origin;
            } else {
                flag = true;
                found_piece = target;
                continue;
            }
        } else {
            // It's their piece.
            if (flag) {
                // The last piece we found may be pinned.
                return (board.pieces[target].is_bishop() | board.pieces[target].is_queen()) ? found_piece : origin;
            } else {
                // Origin is attacked, but no pins here.
                return origin;
            }
        }
    };
    return origin;
}

void Board::search_pins(const Piece colour) {
    // Max 8 sliding pieces in line. 
    // Sliding moves.
    Square origin = find_king(colour);
    Piece target_piece;
    uint start_index = colour.is_white() ? 0 : 8;
    std::array<Square, 16> targets;
    targets[0] = slide_rook_pin(*this, origin, Squares::N, origin.to_north(), colour);
    targets[1] = slide_rook_pin(*this, origin, Squares::E, origin.to_east(), colour);
    targets[2] = slide_rook_pin(*this, origin, Squares::S, origin.to_south(), colour);
    targets[3] = slide_rook_pin(*this, origin, Squares::W, origin.to_west(), colour);
    targets[4] = slide_bishop_pin(*this, origin, Squares::NE, origin.to_northeast(), colour);
    targets[5] = slide_bishop_pin(*this, origin, Squares::SE, origin.to_southeast(), colour);
    targets[6] = slide_bishop_pin(*this, origin, Squares::SW, origin.to_southwest(), colour);
    targets[7] = slide_bishop_pin(*this, origin, Squares::NW, origin.to_northwest(), colour);

    for (int i = 0; i < 8; i++){
        pinned_pieces[i + start_index] = targets[i];
    }
    
}