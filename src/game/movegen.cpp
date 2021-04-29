#include "movegen.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include <algorithm>

// Move Generation

void add_pawn_promotions(const Move move, MoveList &moves) {
    // Add all variations of promotions to a move.
    Move my_move = move;
    my_move.make_queen_promotion();
    moves.push_back(my_move);
    my_move.make_rook_promotion();
    moves.push_back(my_move);
    my_move.make_knight_promotion();
    moves.push_back(my_move);
    my_move.make_bishop_promotion();
    moves.push_back(my_move);
}

template<Colour us, GenType gen>
void gen_pawn_moves(const Board &board, const Square origin, MoveList &moves){
    Colour them = ~us;
    Square target;
    Move move;
    // Moves are North
    // Look for double pawn push possibility
    if (gen == QUIET) {
        if (origin.rank() == relative_rank(us, Squares::Rank2)) {
            target = origin + (forwards(us) + forwards(us) );
            if (board.is_free(target) & board.is_free(origin + forwards(us) )) {
                move = Move(PAWN, origin, target);
                move.make_double_push();
                moves.push_back(move);
            }
        }
        // Normal pushes.
        target = origin + (forwards(us) );
        if (board.is_free(target)) {
            move = Move(PAWN, origin, target);
            if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
    } else if (gen == CAPTURES) {
        // Normal captures.
        Bitboard atk = Bitboards::pawn_attacks(us, origin);
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(PAWN, origin, sq);
            move.make_capture();
            if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }

        // En-passent
        if (origin.rank() == relative_rank(us, Squares::Rank5)) {
            atk = Bitboards::pawn_attacks(us, origin) & board.en_passent();
            if (atk) {
                target = lsb(atk);
                Square ks = board.find_king(us);
                    // This can open a rank. if the king is on that rank it could be a problem.
                if (ks.rank() == origin.rank()) {
                    Bitboard mask = Bitboards::line(ks, origin);
                    Bitboard occ = board.pieces();
                    // Rook attacks from the king
                    Bitboard r_atk = rook_attacks(occ, ks);
                    // Rook xrays from the king
                    r_atk = rook_attacks(occ ^ (occ & r_atk), ks);
                    // Rook double xrays from the king
                    r_atk = rook_attacks(occ ^ (occ & r_atk), ks);
                    r_atk &= mask;
                    if (!(r_atk & board.pieces(them, ROOK, QUEEN))) {
                        move = Move(PAWN, origin, board.en_passent()); 
                        move.make_en_passent();
                        moves.push_back(move);
                    } 
                } else {
                    move = Move(PAWN, origin, board.en_passent()); 
                    move.make_en_passent();
                    moves.push_back(move);
                }
            }
        }
    }
}

template<Colour us, GenType gen>
void gen_pawn_moves(const Board &board, const Square origin, MoveList &moves, Bitboard target_mask){
    Colour them = ~us;
    Square target;
    Move move;
    // Moves are North
    // Look for double pawn push possibility
    if (gen == QUIET) {
        if (origin.rank() == relative_rank(us, Squares::Rank2)) {
            target = origin + (forwards(us) + forwards(us) );
            if (target_mask & target) {
                if (board.is_free(target) & board.is_free(origin + forwards(us))) {
                    move = Move(PAWN, origin, target);
                    move.make_double_push();
                    moves.push_back(move);
                }
            }
        }
        // Normal pushes.
        target = origin + forwards(us);
        if (target_mask & target) {
            if (board.is_free(target)) {
                move = Move(PAWN, origin, target);
                if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }
    } else if (gen == CAPTURES) {
        // Normal captures.
        Bitboard atk = Bitboards::pawn_attacks(us, origin);
        atk &= board.pieces(them);
        atk &= target_mask;
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(PAWN, origin, sq);
            move.make_capture();
            if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
        // En-passent
        // This is a really weird case where the target square is not where you end up.
        // target_mask is always pieces we need to capture if this is called with captures
        if (origin.rank() == relative_rank(us, Squares::Rank5)) {
            atk = Bitboards::pawn_attacks(us, origin) & board.en_passent();
            target = board.en_passent();
            const Square ep_square = origin.rank()|target.file();
            if ((bool)atk && (bool)(target_mask & ep_square)) {
                Square ks = board.find_king(us);
                // This can open a rank. if the king is on that rank it could be a problem.
                if (ks.rank() == origin.rank()) {
                    Bitboard mask = Bitboards::line(ks, origin);
                    Bitboard occ = board.pieces();
                    // Rook attacks from the king
                    Bitboard r_atk = rook_attacks(occ, ks);
                    // Rook xrays from the king
                    r_atk = rook_attacks(occ ^ (occ & r_atk), ks);
                    // Rook double xrays from the king
                    r_atk = rook_attacks(occ ^ (occ & r_atk), ks);
                    r_atk &= mask;
                    if (!(r_atk & board.pieces(them, ROOK, QUEEN))) {
                        move = Move(PAWN, origin, board.en_passent()); 
                        move.make_en_passent();
                        moves.push_back(move);
                    } 
                } else {
                        move = Move(PAWN, origin, board.en_passent()); 
                        move.make_en_passent();
                        moves.push_back(move);
                }
            }
        }
    }
}

template<GenType gen>
void gen_king_moves(const Board &board, const Square origin, MoveList &moves) {
    Colour us = board.who_to_play();
    const Colour them = ~us;
    Bitboard atk = Bitboards::attacks<KING>(board.pieces(), origin);
    if (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            // Check legality of the move here.
            if (board.is_attacked(sq, us)) {continue;}
            Move move = Move(KING, origin, sq);
            moves.push_back(move);
        }
    } else if (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            // Check legality of the move here.
            if (board.is_attacked(sq, us)) { continue;}
            Move move = Move(KING, origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}
template<GenType gen, PieceType pt>
void gen_moves(const Board &board, const Square origin, MoveList &moves) {
    if (pt == KING) {
        return gen_king_moves<gen>(board, origin, moves);
    } else {
        Colour us = board.who_to_play();
        const Colour them = ~us;
        Bitboard atk = Bitboards::attacks<pt>(board.pieces(), origin);
        if (gen == QUIET) {
            atk &= ~board.pieces();
            while (atk) {
                Square sq = pop_lsb(&atk);
                Move move = Move(pt, origin, sq);
                moves.push_back(move);
            }
        } else if (gen == CAPTURES) {
            atk &= board.pieces(them);
            while (atk) {
                Square sq = pop_lsb(&atk);
                Move move = Move(pt, origin, sq);
                move.make_capture();
                moves.push_back(move);
            }
        }
    }
}

template<GenType gen, PieceType pt>
void gen_moves(const Board &board, const Square origin, MoveList &moves, Bitboard target_mask) {
    Colour us = board.who_to_play();
    const Colour them = ~us;
    Bitboard atk = Bitboards::attacks<pt>(board.pieces(), origin);
    // Mask the target squares.
    atk &= target_mask;
    if (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(pt, origin, sq);
            moves.push_back(move);
        }
    } else if (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(pt, origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}

template<Colour us>
void gen_castle_moves(const Board &board, MoveList &moves) {
    // Check if castling is legal, and if so add it to the move list. Assumes we aren't in check
    Move move;
    // You can't castle through check, or while in check
    if (board.can_castle(us, QUEENSIDE)) 
    {
        // Check for overlap of squares that need to be free, and occupied bb.
        // In the future we should keep a 
        if (!(Bitboards::castle(us, QUEENSIDE) & board.pieces())) {
            if (!board.is_attacked(Squares::FileD | back_rank(us), us)
                & !board.is_attacked(Squares::FileC | back_rank(us), us)) 
            {
                move = Move(KING, Squares::FileE | back_rank(us), Squares::FileC | back_rank(us));
                move.make_queen_castle();
                moves.push_back(move);
            }
        }
    }
    if (board.can_castle(us, KINGSIDE)) 
    {
        if (!(Bitboards::castle(us, KINGSIDE) & board.pieces())) {
            if (!board.is_attacked(Squares::FileF | back_rank(us), us)
                & !board.is_attacked(Squares::FileG | back_rank(us), us)) 
            {
                move = Move(KING, Squares::FileE | back_rank(us), Squares::FileG | back_rank(us));
                move.make_king_castle();
                moves.push_back(move);
            }
        }
    }
}

template<GenType gen>
void generate_pseudolegal_moves(const Board &board, MoveList &moves) {
    Colour us = board.who_to_play();
    Bitboard occ;
    Square ks = board.find_king(us);
    // Pawns which aren't pinned
    occ = board.pieces(us, PAWN) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        if (us == WHITE) {
            gen_pawn_moves<WHITE, gen>(board, sq, moves);
        } else {
            gen_pawn_moves<BLACK, gen>(board, sq, moves);
        }
    }
    // Pawns which are
    occ = board.pieces(us, PAWN) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks);
        if (us == WHITE) {
            gen_pawn_moves<WHITE, gen>(board, sq, moves, target_squares);
        } else {
            gen_pawn_moves<BLACK, gen>(board, sq, moves, target_squares);
        }
    }
    // Pinned knights cannot move
    occ = board.pieces(us, KNIGHT) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, KNIGHT>(board, sq, moves);
    }
    // Bishops which aren't pinned
    occ = board.pieces(us, BISHOP) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, BISHOP>(board, sq, moves);
    }
    // Bisops which are pinned
    occ = board.pieces(us, BISHOP) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks);
        gen_moves<gen, BISHOP>(board, sq, moves, target_squares);
    }
    // Rooks which aren't pinned
    occ = board.pieces(us, ROOK) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, ROOK>(board, sq, moves);
    }
    // Rooks which are pinned
    occ = board.pieces(us, ROOK) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks);
        gen_moves<gen, ROOK>(board, sq, moves, target_squares);
    }
    occ = board.pieces(us, QUEEN) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, QUEEN>(board, sq, moves);
    }
    occ = board.pieces(us, QUEEN) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks);
        gen_moves<gen, QUEEN>(board, sq, moves, target_squares);
    }
    gen_moves<gen, KING>(board, ks, moves);
    
}

template<>
void generate_pseudolegal_moves<EVASIONS>(const Board &board, MoveList &moves) {
    Colour us = board.who_to_play();
    Colour them = ~us;
    Bitboard occ;
    const Bitboard can_move = ~board.pinned();
    occ = board.pieces(us, KING);
    Square ks = board.find_king(us);
    gen_moves<QUIET, KING>(board, ks, moves);
    gen_moves<CAPTURES, KING>(board, ks, moves);

    Bitboard pieces_bb = board.pieces();
    // We can capture the checker
    Square ts = board.checkers(0);
    Bitboard target_square = sq_to_bb(ts);
    occ = Bitboards::attacks<KNIGHT>(pieces_bb,  ts) & board.pieces(us, KNIGHT) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        Move move = Move(KNIGHT, sq, ts);
        move.make_capture();
        moves.push_back(move);
    }
    const Bitboard bq_atk = Bitboards::attacks<BISHOP>(pieces_bb,  ts) & can_move;
    occ = bq_atk & board.pieces(us, BISHOP);
    while (occ) {
        Square sq = pop_lsb(&occ);
        Move move = Move(BISHOP, sq, ts);
        move.make_capture();
        moves.push_back(move);
    }
    const Bitboard rq_atk = Bitboards::attacks<ROOK>(pieces_bb,  ts) & can_move;
    occ = rq_atk & board.pieces(us, ROOK);
    while (occ) {
        Square sq = pop_lsb(&occ);
        Move move = Move(ROOK, sq, ts);
        move.make_capture();
        moves.push_back(move);
    }
    occ = (rq_atk | bq_atk) & board.pieces(us, QUEEN);
    while (occ) {
        Square sq = pop_lsb(&occ);
        Move move = Move(QUEEN, sq, ts);
        move.make_capture();
        moves.push_back(move);
    }

    // Or we can block the check
    Bitboard between_squares = Bitboards::between(ks, ts);
    occ = board.pieces(us, PAWN) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        if (us == WHITE) {
            gen_pawn_moves<WHITE, QUIET>(board, sq, moves, between_squares);
            // Weird pawn en-passent checks just make my life so much harder.
            gen_pawn_moves<WHITE, CAPTURES>(board, sq, moves, target_square);
        } else {
            gen_pawn_moves<BLACK, QUIET>(board, sq, moves, between_squares);
            // Weird pawn en-passent checks just make my life so much harder.
            gen_pawn_moves<BLACK, CAPTURES>(board, sq, moves, target_square);
        }
    }
    occ = board.pieces(us, KNIGHT) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, KNIGHT>(board, sq, moves, between_squares);
    }
    occ = board.pieces(us, BISHOP) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, BISHOP>(board, sq, moves, between_squares);
    }
    occ = board.pieces(us, ROOK) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, ROOK>(board, sq, moves, between_squares);
    }
    occ = board.pieces(us, QUEEN) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, QUEEN>(board, sq, moves, between_squares);
    }
}

template<GenType gen>
void generate_moves(Board &board, MoveList &moves) {
    // Generate all legal non-evasion moves
    Colour us = board.who_to_play();
    moves.reserve(MAX_MOVES);
    if (gen == CAPTURES) {
        generate_pseudolegal_moves<CAPTURES>(board, moves);
    } else if (gen == LEGAL) {
        generate_pseudolegal_moves<CAPTURES>(board, moves);
        generate_pseudolegal_moves<QUIET>(board, moves);
    }    
    if (us == WHITE) {
        gen_castle_moves<WHITE>(board, moves);
    } else {
        gen_castle_moves<BLACK>(board, moves);
    }
    return;

}

template<>
void generate_moves<EVASIONS>(Board &board, MoveList &moves) {
    // Generate all legal moves
    Colour us = board.who_to_play();
    Square king_square = board.find_king(us);
    MoveList pseudolegal_moves;
    moves.reserve(MAX_MOVES);
    const int number_checkers = board.number_checkers();
    if (number_checkers == 2) {
        // Double check. Generate king moves.
        gen_moves<QUIET, KING>(board, king_square, moves);
        gen_moves<CAPTURES, KING>(board, king_square, moves);
    } else {
        generate_pseudolegal_moves<EVASIONS>(board, moves);
    }
    return;
}

MoveList Board::get_moves(){
    MoveList moves;
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        generate_moves<LEGAL>(*this, moves);
    }
    return moves;
}

bool cmp(Move &m1, Move &m2) {
// Comparison for the sort function. We want to sort in reverse order.
    return m1.score > m2.score;
}

#include <iostream>
MoveList Board::get_sorted_moves() {
    MoveList legal_moves = get_moves();
    MoveList checks, quiet_moves, captures, sorted_moves;
    size_t n_moves = legal_moves.size();
    checks.reserve(MAX_MOVES);
    quiet_moves.reserve(MAX_MOVES);
    captures.reserve(MAX_MOVES);
    // The moves from get_moves already have captures first
    for (Move move : legal_moves) {
        if (gives_check(move)) {
            checks.push_back(move);
        } else if(move.is_ep_capture()) {
            // En-passent is weird too.
            const Square captured_square = move.origin.rank() | move.target.file();
            move.captured_piece = pieces(captured_square);
        } else if (move.is_capture()){
            // Make sure to lookup and record the piece captured 
            move.captured_piece = pieces(move.target);
            // Magic number, part of move sorting heuristic.
            move.score = piece_value(to_enum_piece(move.captured_piece)) - piece_value(move.moving_piece);
            captures.push_back(move);
        } else {
            quiet_moves.push_back(move);
        }
    }
    sorted_moves = checks;
    sorted_moves.reserve(MAX_MOVES);
    std::sort(captures.begin(), captures.end(), cmp);
    sorted_moves.insert(sorted_moves.end(), captures.begin(), captures.end());
    sorted_moves.insert(sorted_moves.end(), quiet_moves.begin(), quiet_moves.end());
    return sorted_moves;
}


MoveList Board::get_captures() {
    MoveList moves;
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        generate_moves<CAPTURES>(*this, moves);
    }
    return moves;
}