#pragma once
#include "search.hpp"
// Start the uci interface
void uci();
void uci_info(unsigned long depth, int eval, unsigned long nodes, unsigned long nps, PrincipleLine principle, unsigned int time);