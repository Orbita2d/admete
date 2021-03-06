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

template <Colour us, GenType gen> void gen_pawn_moves(const Board &board, const Square origin, MoveList &moves) {
    Colour them = ~us;
    Square target;
    Move move;
    // Moves are North
    // Look for double pawn push possibility
    if constexpr (gen == QUIET) {
        if (origin.rank() == relative_rank(us, Squares::Rank2)) {
            target = origin + (forwards(us) + forwards(us));
            if (board.is_free(target) & board.is_free(origin + forwards(us))) {
                move = Move(PAWN, origin, target);
                move.make_double_push();
                moves.push_back(move);
            }
        }
        // Normal pushes.
        target = origin + (forwards(us));
        if (board.is_free(target)) {
            move = Move(PAWN, origin, target);
            if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
    } else if constexpr (gen == CAPTURES) {
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

template <Colour us, GenType gen>
void gen_pawn_moves(const Board &board, const Square origin, MoveList &moves, Bitboard target_mask) {
    Colour them = ~us;
    Square target;
    Move move;
    // Moves are North
    // Look for double pawn push possibility
    if constexpr (gen == QUIET) {
        if (origin.rank() == relative_rank(us, Squares::Rank2)) {
            target = origin + (forwards(us) + forwards(us));
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
    } else if constexpr (gen == CAPTURES) {
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
            const Square ep_square = origin.rank() | target.file();
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

template <GenType gen> void gen_king_moves(const Board &board, const Square origin, MoveList &moves) {
    Colour us = board.who_to_play();
    const Colour them = ~us;
    Bitboard atk = Bitboards::attacks<KING>(board.pieces(), origin);
    if constexpr (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            // Check legality of the move here.
            if (board.is_attacked(sq, us)) {
                continue;
            }
            Move move = Move(KING, origin, sq);
            moves.push_back(move);
        }
    } else if constexpr (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            // Check legality of the move here.
            if (board.is_attacked(sq, us)) {
                continue;
            }
            Move move = Move(KING, origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}
template <GenType gen, PieceType pt> void gen_moves(const Board &board, const Square origin, MoveList &moves) {
    if constexpr (pt == KING) {
        return gen_king_moves<gen>(board, origin, moves);
    } else {
        Colour us = board.who_to_play();
        const Colour them = ~us;
        Bitboard atk = Bitboards::attacks<pt>(board.pieces(), origin);
        if constexpr (gen == QUIET) {
            atk &= ~board.pieces();
            while (atk) {
                Square sq = pop_lsb(&atk);
                Move move = Move(pt, origin, sq);
                moves.push_back(move);
            }
        } else if constexpr (gen == CAPTURES) {
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

template <GenType gen, PieceType pt>
void gen_moves(const Board &board, const Square origin, MoveList &moves, Bitboard target_mask) {
    Colour us = board.who_to_play();
    const Colour them = ~us;
    Bitboard atk = Bitboards::attacks<pt>(board.pieces(), origin);
    // Mask the target squares.
    atk &= target_mask;
    if constexpr (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(pt, origin, sq);
            moves.push_back(move);
        }
    } else if constexpr (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(pt, origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}

template <Colour us, CastlingSide side> void gen_castle_moves(const Board &board, MoveList &moves) {
    // Check if castling is legal, and if so add it to the move list. Assumes we aren't in check
    Move move;
    // You can't castle through check, or while in check
    if (board.can_castle(us, QUEENSIDE) && (side == QUEENSIDE)) {
        // Check for overlap of squares that need to be free, and occupied bb.
        if (!(Bitboards::castle(us, QUEENSIDE) & board.pieces())) {
            if (!board.is_attacked(Squares::FileD | back_rank(us), us) &
                !board.is_attacked(Squares::FileC | back_rank(us), us)) {
                move = Move(KING, Squares::FileE | back_rank(us), Squares::FileC | back_rank(us));
                move.make_queen_castle();
                moves.push_back(move);
            }
        }
    }
    if (board.can_castle(us, KINGSIDE) && (side == KINGSIDE)) {
        if (!(Bitboards::castle(us, KINGSIDE) & board.pieces())) {
            if (!board.is_attacked(Squares::FileF | back_rank(us), us) &
                !board.is_attacked(Squares::FileG | back_rank(us), us)) {
                move = Move(KING, Squares::FileE | back_rank(us), Squares::FileG | back_rank(us));
                move.make_king_castle();
                moves.push_back(move);
            }
        }
    }
}

template <Colour us> void gen_castle_moves(const Board &board, MoveList &moves) {
    // Check if castling is legal, and if so add it to the move list. Assumes we aren't in check
    gen_castle_moves<us, KINGSIDE>(board, moves);
    gen_castle_moves<us, QUEENSIDE>(board, moves);
}

template <GenType gen> void gen_moves(const Board &board, MoveList &moves) {
    Colour us = board.who_to_play();
    Bitboard occ;
    Square ks = board.find_king(us);

    // If we are generating quiet moves, add castling
    if constexpr (gen == QUIET) {
        if (us == WHITE) {
            gen_castle_moves<WHITE>(board, moves);
        } else {
            gen_castle_moves<BLACK>(board, moves);
        }
    }

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

template <> void gen_moves<QUIETCHECKS>(const Board &board, MoveList &moves) {
    Colour us = board.who_to_play();
    Bitboard occ;
    Square ks = board.find_king(us);

    // Caslting with check can happen is the square the rook ends up on is a rook check square.
    if (board.check_squares(ROOK) & sq_to_bb(RookCastleSquares[us][KINGSIDE])) {
        if (us == WHITE) {
            gen_castle_moves<WHITE, KINGSIDE>(board, moves);
        } else {
            gen_castle_moves<BLACK, KINGSIDE>(board, moves);
        }
    }
    if (board.check_squares(ROOK) & sq_to_bb(RookCastleSquares[us][QUEENSIDE])) {
        if (us == WHITE) {
            gen_castle_moves<WHITE, QUEENSIDE>(board, moves);
        } else {
            gen_castle_moves<BLACK, QUEENSIDE>(board, moves);
        }
    }

    // Pawns which aren't pinned
    occ = board.pieces(us, PAWN) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        if (us == WHITE) {
            gen_pawn_moves<WHITE, QUIET>(board, sq, moves, board.check_squares(PAWN));
        } else {
            gen_pawn_moves<BLACK, QUIET>(board, sq, moves, board.check_squares(PAWN));
        }
    }
    // Pawns which are
    occ = board.pieces(us, PAWN) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks) & board.check_squares(PAWN);
        if (us == WHITE) {
            gen_pawn_moves<WHITE, QUIET>(board, sq, moves, target_squares);
        } else {
            gen_pawn_moves<BLACK, QUIET>(board, sq, moves, target_squares);
        }
    }
    // Pinned knights cannot move
    occ = board.pieces(us, KNIGHT) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, KNIGHT>(board, sq, moves, board.check_squares(KNIGHT));
    }
    // Bishops which aren't pinned
    occ = board.pieces(us, BISHOP) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, BISHOP>(board, sq, moves, board.check_squares(BISHOP));
    }
    // Bisops which are pinned
    occ = board.pieces(us, BISHOP) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks) & board.check_squares(BISHOP);
        gen_moves<QUIET, BISHOP>(board, sq, moves, target_squares);
    }
    // Rooks which aren't pinned
    occ = board.pieces(us, ROOK) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, ROOK>(board, sq, moves, board.check_squares(ROOK));
    }
    // Rooks which are pinned
    occ = board.pieces(us, ROOK) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks) & board.check_squares(ROOK);
        gen_moves<QUIET, ROOK>(board, sq, moves, target_squares);
    }
    occ = board.pieces(us, QUEEN) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, QUEEN>(board, sq, moves, board.check_squares(QUEEN));
    }
    occ = board.pieces(us, QUEEN) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks) & board.check_squares(QUEEN);
        gen_moves<QUIET, QUEEN>(board, sq, moves, target_squares);
    }
}

template <> void gen_moves<EVASIONS>(const Board &board, MoveList &moves) {
    Colour us = board.who_to_play();
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
    occ = Bitboards::attacks<KNIGHT>(pieces_bb, ts) & board.pieces(us, KNIGHT) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        Move move = Move(KNIGHT, sq, ts);
        move.make_capture();
        moves.push_back(move);
    }
    const Bitboard bq_atk = Bitboards::attacks<BISHOP>(pieces_bb, ts) & can_move;
    occ = bq_atk & board.pieces(us, BISHOP);
    while (occ) {
        Square sq = pop_lsb(&occ);
        Move move = Move(BISHOP, sq, ts);
        move.make_capture();
        moves.push_back(move);
    }
    const Bitboard rq_atk = Bitboards::attacks<ROOK>(pieces_bb, ts) & can_move;
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
    // Squares where a pawn could capture the checker
    Bitboard pawn_atk = Bitboards::pawn_attacks(~us, target_square) |
                        (Bitboards::shift<W>(target_square) | Bitboards::shift<E>(target_square));
    occ = board.pieces(us, PAWN) & can_move & pawn_atk;
    while (occ) {
        Square sq = pop_lsb(&occ);
        if (us == WHITE) {
            // Weird pawn en-passent checks just make my life so much harder.
            gen_pawn_moves<WHITE, CAPTURES>(board, sq, moves, target_square);
        } else {
            // Weird pawn en-passent checks just make my life so much harder.
            gen_pawn_moves<BLACK, CAPTURES>(board, sq, moves, target_square);
        }
    }

    // Or we can block the check
    Bitboard between_squares = Bitboards::between(ks, ts);

    // All of our pawns that can move that have some posibility of blocking the check. (We verify they can later)
    occ = board.pieces(us, PAWN) & can_move & Bitboards::reverse_pawn_push(us, between_squares);
    ;
    while (occ) {
        Square sq = pop_lsb(&occ);
        if (us == WHITE) {
            gen_pawn_moves<WHITE, QUIET>(board, sq, moves, between_squares);
        } else {
            gen_pawn_moves<BLACK, QUIET>(board, sq, moves, between_squares);
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

template <GenType gen> void generate_moves(const Board &board, MoveList &moves) {
    // Generate all legal non-evasion moves
    if constexpr (gen == CAPTURES) {
        gen_moves<CAPTURES>(board, moves);
    } else if constexpr (gen == LEGAL) {
        gen_moves<CAPTURES>(board, moves);
        gen_moves<QUIET>(board, moves);
    }
    return;
}

template <> void generate_moves<EVASIONS>(const Board &board, MoveList &moves) {
    // Generate all legal moves
    const Colour us = board.who_to_play();
    const Square king_square = board.find_king(us);
    const int number_checkers = board.number_checkers();
    if (number_checkers == 2) {
        // Double check. Generate king moves.
        gen_moves<QUIET, KING>(board, king_square, moves);
        gen_moves<CAPTURES, KING>(board, king_square, moves);
    } else {
        gen_moves<EVASIONS>(board, moves);
    }
    return;
}

MoveList Board::get_moves() const {
    MoveList moves;
    moves.reserve(MAX_MOVES);
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        generate_moves<LEGAL>(*this, moves);
    }
    return moves;
}

MoveList Board::get_quiessence_moves() const {
    MoveList moves;
    moves.reserve(MAX_MOVES);
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        // For later, currently unfortunately increases branching factor too much to be useful.
        // gen_moves<QUIETCHECKS>(*this, moves);
        generate_moves<CAPTURES>(*this, moves);
    }
    return moves;
}

MoveList Board::get_capture_moves() const {
    MoveList moves;
    moves.reserve(MAX_MOVES);
    generate_moves<CAPTURES>(*this, moves);
    return moves;
}

MoveList Board::get_evasion_moves() const {
    MoveList moves;
    moves.reserve(MAX_MOVES);
    generate_moves<EVASIONS>(*this, moves);
    return moves;
}

void Board::get_moves(MoveList &moves) const {
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        generate_moves<LEGAL>(*this, moves);
    }
}

void Board::get_capture_moves(MoveList &moves) const { generate_moves<CAPTURES>(*this, moves); }

void Board::get_evasion_moves(MoveList &moves) const { generate_moves<EVASIONS>(*this, moves); }

void Board::get_quiessence_moves(MoveList &moves) const {
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        // For later, currently unfortunately increases branching factor too much to be useful.
        // gen_moves<QUIETCHECKS>(*this, moves);
        generate_moves<CAPTURES>(*this, moves);
    }
}
