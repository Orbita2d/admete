#include <unordered_map>
#include "../game/piece.hpp"
#include <iostream>

namespace Cache {
    enum TransState {
        EXACT = 0,
        LOWER = 1,
        UPPER = 2
    };

    // 16 bytes
    struct TransElement {
        TransElement() = default;
        TransElement(int eval, int a, int b, int d, Move m) : score(eval), _depth(d), info((eval <= a) ? UPPER : (eval >= b) ? LOWER : EXACT) , hash_move(pack_move(m)) {}; 
        int eval() const{ return score; }
        bool lower() const{ return info == LOWER; }
        bool upper() const{ return info == UPPER; }
        depth_t depth() const{ return _depth; }
        DenseMove move() const { return hash_move; }
    private:
        int score = 0;
        depth_t _depth = 0;
        TransState info = EXACT;
        DenseMove hash_move = NULL_DMOVE;
    };

    typedef std::pair<long, TransElement> tt_pair;
    typedef std::unordered_map<long, TransElement> tt_map ;
    // Limit transposition table to 16MB
    constexpr size_t tt_max = (1<<24) / sizeof(TransElement);

    class TranspositionTable {
    public:
        TranspositionTable() = default;
        bool probe(const long);
        TransElement last_hit() const { return _last_hit; };
        void store(const long hash, const int eval, const int lower, const int upper, const depth_t depth, const Move move);
        void clear() {_data.clear(); index = 0ul; }
        depth_t min_depth() {return _min_depth; }
        void min_depth(depth_t d) { _min_depth = d; }
        bool is_enabled() {return enabled; }
        void enable() {enabled = true;}
        void disable() {enabled = false;}
    private:
        tt_map _data;
        TransElement _last_hit;
        depth_t _min_depth = 0;
        size_t index;
        bool enabled = true;
    };

    inline TranspositionTable transposition_table;

    class KillerTable {
        // Table for the killer heuristic;
    public:
        KillerTable() = default;
        DenseMove probe(const ply_t ply);
        void store(const ply_t ply, const Move move);
        bool is_enabled() {return enabled; }
        void enable() {enabled = true;}
        void disable() {enabled = false;}
    private:
        std::array<DenseMove, MAX_PLY> _data;
        bool enabled = true;
    };

    inline KillerTable killer_table;

    void init(); 
}