#pragma once
#include "types.hpp"
#include "piece.hpp"

typedef unsigned long int Bitboard;

inline Bitboard PseudolegalAttacks[N_PIECE][N_SQUARE];
inline Bitboard PawnAttacks[N_COLOUR][N_SQUARE];
inline Bitboard RankBBs[8] = {0x00000000000000FF, 0x000000000000ff00, 0x0000000000ff0000, 0x00000000ff000000, 0x000000ff00000000, 0x0000ff0000000000, 0x00ff000000000000, 0xff00000000000000};
inline Bitboard FileBBs[8] = {0x0101010101010101, 0x0202020202020202, 0x0404040404040404, 0x0808080808080808, 0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080};
inline Bitboard LineBBs[N_SQUARE][N_SQUARE];
constexpr Bitboard CastleBBs[N_COLOUR][N_CASTLE] = {{0x6000000000000000, 0xe00000000000000}, {0x60, 0xe}};
inline Bitboard SquareBBs[N_SQUARE];

inline Bitboard sq_to_bb(const int s) {
  // The space-time balance doesn't work out here, cheaper to calculate than look up.
    return Bitboard(1) << s;
}
inline Bitboard sq_to_bb(const Square s) {
    return Bitboard(1) << s;
}

inline Bitboard  operator&( Bitboard  b, Square s) { return b &  sq_to_bb(s); }
inline Bitboard  operator|( Bitboard  b, Square s) { return b |  sq_to_bb(s); }
inline Bitboard  operator^( Bitboard  b, Square s) { return b ^  sq_to_bb(s); }
inline Bitboard& operator|=(Bitboard& b, Square s) { return b |= sq_to_bb(s); }
inline Bitboard& operator^=(Bitboard& b, Square s) { return b ^= sq_to_bb(s); }


namespace Bitboards
{
  void pretty(Bitboard);
  void init();
  constexpr Bitboard h_file_bb = 0x0101010101010101;
  constexpr Bitboard a_file_bb = 0x8080808080808080;
  constexpr Bitboard rank_1_bb = 0xff00000000000000;
  constexpr Bitboard rank_8_bb = 0x00000000000000FF;
  constexpr Bitboard omega = ~Bitboard(0);


  template<Direction dir>
  constexpr Bitboard shift(const Bitboard bb) {
    // Bitboards are stored big-endian but who can remember that, so this hides the implementation a little
    if      (dir == Direction::N ) { return (bb >> 8);}
    else if (dir == Direction::S ) { return (bb << 8);}
    else if (dir == Direction::E ) { return (bb << 1) & ~Bitboards::h_file_bb;}
    else if (dir == Direction::W ) { return (bb >> 1) & ~Bitboards::a_file_bb;}
    else if (dir == Direction::NW) { return (bb >> 9) & ~Bitboards::a_file_bb;}
    else if (dir == Direction::NE) { return (bb >> 7) & ~Bitboards::h_file_bb;}
    else if (dir == Direction::SW) { return (bb << 7) & ~Bitboards::a_file_bb;}
    else if (dir == Direction::SE) { return (bb << 9) & ~Bitboards::h_file_bb;}
  }

  inline Bitboard attacks(const PieceEnum p, const Square s) {
      // Get the value from the pseudolegal attacks table
      return PseudolegalAttacks[p][s];
  }

  inline Bitboard pawn_attacks(const Colour c, const Square s) {
      return PawnAttacks[c][s];
  }

  inline Bitboard rank(const Square s) {
    return RankBBs[s.rank_index()];
  }

  inline Bitboard file(const Square s) {
    return FileBBs[s.file_index()];
  }
  inline Bitboard line(const Square s1, const Square s2) {
    return LineBBs[s1][s2];
  }
  inline Bitboard between(const Square s1, const Square s2) {
    Bitboard bb = line(s1, s2);
    if (line) {
      bb &= ((omega << s1) ^ (omega << s2));
      return bb & (bb - 1);
    } else {
      return 0;
    }
  }

  constexpr Bitboard castle(const Colour c, const CastlingSide cs) {
    return CastleBBs[c][cs];
  }
}

// Trying to implement the magic bitboard in https://www.chessprogramming.org/Magic_Bitboards "fancy"
struct Magic {
  // Pointer to attacks bitboard
  Bitboard* ptr;
  Bitboard mask;
  Bitboard magic;
  int shift;
  int index(Bitboard occ) const{
    return unsigned(((occ & mask) * magic) >> shift);
  }
};

inline Bitboard BishopTable[0x1480]; // The length of these I grabbed from stockfish but cannot justify.
inline Bitboard RookTable[0x19000];

inline Magic BishopMagics[N_SQUARE];
inline Magic RookMagics[N_SQUARE];

inline Bitboard bishop_attacks(Bitboard occ, const Square sq)  {
  const Magic magic = BishopMagics[sq];
  Bitboard* aptr = magic.ptr;
  return aptr[magic.index(occ)];
}

inline Bitboard rook_attacks(Bitboard occ, const Square sq)  {
  const Magic magic = RookMagics[sq];
  Bitboard* aptr = magic.ptr;
  return aptr[magic.index(occ)];
}

inline Bitboard bishop_xrays(Bitboard occ, const Square sq)  {
  const Bitboard atk = bishop_attacks(occ, sq);
  return bishop_attacks(occ ^ (occ & atk), sq);
}

inline Bitboard rook_xrays(Bitboard occ, const Square sq)  {
  const Bitboard atk = rook_attacks(occ, sq);
  return rook_attacks(occ ^ (occ & atk), sq);
}

// Following bit magic shamelessly stolen from Stockfish

/// lsb() and msb() return the least/most significant bit in a non-zero bitboard

#if defined(__GNUC__)  // GCC, Clang, ICC

inline Square lsb(Bitboard b) {
  return Square(__builtin_ctzll(b));
}

inline Square msb(Bitboard b) {
  return Square(63 ^ __builtin_clzll(b));
}

#else  // Compiler is neither GCC nor MSVC compatible

#error "Compiler not supported."

#endif


/// pop_lsb() finds and clears the least significant bit in a non-zero bitboard

inline Square pop_lsb(Bitboard* b) {
  const Square s = lsb(*b);
  *b &= *b - 1;
  return s;
}


/// frontmost_sq() returns the most advanced square for the given color,
/// requires a non-zero bitboard.
inline Square frontmost_sq(Colour c, Bitboard b) {
  return c == WHITE ? msb(b) : lsb(b);
}
