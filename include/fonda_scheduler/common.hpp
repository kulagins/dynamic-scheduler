
#ifndef FONDA_SCHED_COMMON_HPP
#define FONDA_SCHED_COMMON_HPP



#include "../../extlibs/memdag/src/graph.hpp"

using namespace std;
void printDebug(string str);
void printInlineDebug(string str);
void checkForZeroMemories(graph_t *graph);
std::string trimQuotes(const std::string& str);

#endif