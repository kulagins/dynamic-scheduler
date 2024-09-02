//
// Created by kulagins on 26.05.23.
//

#ifndef FONDA_SCHED_COMMON_HPP
#define FONDA_SCHED_COMMON_HPP



#include "../../extlibs/memdag/src/graph.hpp"

using namespace std;
void printDebug(string str);
void printInlineDebug(string str);

string buildInputFileAndWorkflow( istream *OpenFile, string arg1, string &inputFile);

void checkForZeroMemories(graph_t *graph);

long getBiggestMem(graph_t* graph);

void normalizeToBiggestProcessorMem(graph_t* graph, double biggestProcMem, long biggestMemInGraph);
std::string trimQuotes(const std::string& str);

#endif //FONDA_SCHED_COMMON_HPP
