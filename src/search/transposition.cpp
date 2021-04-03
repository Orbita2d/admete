#include "transposition.hpp"
#include <array>

std::array<long, tt_max> key_array;

bool TranspositionTable::probe(const long hash) {
    tt_map::iterator it = _data.find(hash);
    if (it == _data.end()) {
        return false;
    } else {
        _last_hit = it->second;
        return true;
    }
}

void TranspositionTable::store(const long hash, const int eval, const int lower, const int upper, const unsigned int depth, const Move move) {
    const TransElement elem = TransElement(eval, lower, upper, depth, move);
    if (index < tt_max) {
        // We are doing the first fill of the table;
        _data[hash] = elem;
        key_array[index] = hash;
    } else {
        // Replace oldest value
        const size_t i = index % tt_max;
        const long old_hash = key_array[i];
        key_array[i] = hash;
        _data.erase(old_hash);
        _data[hash] = elem;
    }
    index++;
}