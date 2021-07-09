#include "board.hpp"
#include <gtest/gtest.h>
#include <tuple>

TEST(Zobrist, MaterialKeyEqual) {
  Board board = Board();
  std::pair<std::string, std::string> testcases[] = {
      {"8/2k5/8/8/8/4K3/8/8 w - - 0 1", "8/5k2/8/2K5/8/8/8/8 b - - 0 1"},
      {"8/2k5/8/8/8/4K3/8/8 w - - 0 1", "7K/8/8/8/8/8/8/k7 w - - 0 1"},
      {"8/8/1n3K2/8/8/2k5/8/8 w - - 0 1", "8/8/5K2/8/8/2k5/6n1/8 w - - 0 1"},
      {"8/8/2N3K1/8/8/2k5/8/8 w - - 0 1", "8/3K4/6k1/8/8/5N2/8/8 w - - 0 1"},
      {"8/3K4/6k1/8/8/5N2/1p6/8 w - - 0 1",
       "8/1p1K4/6k1/8/8/5N2/8/8 w - - 0 1"},
      {"8/1Q1K4/6k1/8/8/5N2/1q6/8 w - - 0 1",
       "8/1q1K4/6k1/8/8/5N2/1Q6/8 w - - 0 1"},
  };
  for (const auto &[fen1, fen2] : testcases) {
    board.fen_decode(fen1);
    zobrist_t hash1 = Zobrist::material(board);
    board.fen_decode(fen2);
    zobrist_t hash2 = Zobrist::material(board);
    EXPECT_EQ(hash1, hash2);
  }
}

TEST(Zobrist, MaterialKeyNotEqual) {
  Board board = Board();
  std::pair<std::string, std::string> testcases[] = {
      {"8/1k6/8/8/8/3N1K2/8/8 w - - 0 1", "8/1k6/8/8/8/3n1K2/8/8 w - - 0 1"},
      {"7k/8/8/8/8/8/q7/R6K w - - 0 1", "7k/8/8/8/8/8/Q7/r6K w - - 0 1"},
      {"n6k/8/8/8/8/8/8/7K w - - 0 1", "b6k/8/8/8/8/8/8/7K w - - 0 1"},
      {"7k/8/8/8/p7/8/8/7K w - - 0 1", "7k/8/8/8/P7/8/8/7K w - - 0 1"},
  };
  for (const auto &[fen1, fen2] : testcases) {
    board.fen_decode(fen1);
    zobrist_t hash1 = Zobrist::material(board);
    board.fen_decode(fen2);
    zobrist_t hash2 = Zobrist::material(board);
    EXPECT_NE(hash1, hash2);
  }
}
