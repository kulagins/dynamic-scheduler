//
// Created by kulagins on 21.06.23.
//

#ifndef FONDA_SCHED_PARTSCHED_H
#define FONDA_SCHED_PARTSCHED_H
/*
#include "option.hpp"
#include "graph.hpp"
#include "dagP.hpp"
#include "cluster.hpp"
void init();

std::map<int, std::vector<std::string>> partSched(dgraph *G, graph_t *graphWithNodeMems, int nbParts, MLGP_option * opt, Cluster *cluster);
idxType*  buildPartsNaively(graph_t *graph, int actualNumVertices, Cluster *cluster,  map<int, Processor*> & partsToProcessors);
double computeNaiveMakespan(const dgraph *G, graph_t *graphWithNodeMems, Cluster *cluster);

std::map<int, std::vector<std::string>> step1(dgraph *G, int nbParts, MLGP_option * opt, idxType *parts);
vector<pair<vertex_t *, double>> step2(graph_t * source, dgraph *source_original, idxType *leaders, int nbParts,  MLGP_option *opt_orig, Cluster *cluster );
graph_t * step3(graph_t * source, Cluster *cluster, idxType  * leaders,int nbParts);
double swapUntilBest(graph_t *quotientGraph, Cluster * cluster);
double moveAroundOnFreeProcessors(Cluster * cluster, graph_t *quotientGraph);

graph_t *buildQuotientGraph(graph_t *source, idxType *leaders, int nbLeaders);

graph_t *convertToNonMemRepresentation(graph_t *withMemories, map<int, int> &noMemToWithMem);
double makespan(graph_t *graph, bool allowUnassigned, double beta);
vector<vertex_t*> criticalPath(graph_t* graph, double & maxDist );
double callMem(graph_t *subgraph);
bool isAcyclic(graph_t* graph);
vertex_t * find2VertexCycle(graph_t *graph, vertex_t * mergedVertex);

void takeCutEdgesOver(idxType* leaders, graph_t* graph, dgraph * G);
void takeOverPartNumbers(graph_t *graph, idxType *parts, int i);
void copyVertexNames(dgraph * G,graph_t* graph);

vertex_t* merge2VerticesPutOnFirstProc(graph_t * originalGraph, graph_t* quotientGraph, vertex_t* v1, vertex_t* v2, vector<int> leadersToFilterFor, bool finalize);
vertex_t* tryMergeWith( graph_t* quotienGraph,  graph_t* source, vertex_t* unassignedVertex, vector<vertex_t*> criticalP, bool ignoreCP, vertex_t * &optionalThirdVertex, double beta);

graph_t *cutSmallGraphFromBigWhenLeadersAreInVertices(graph_t *bigGraph, idxType part);
graph_t *cutSmallGraphFromBigWhenLeadersAreExternal(graph_t *bigGraph, idxType *leaders, idxType part, map<int,int > oldToNewIds);
graph_t *cutNPartsFromBigGraph(graph_t *bigGraph, vector<idxType> parts, bool finalize);

vertex_t *buildQuotientVertexInNewGraph(graph_t *oldGraph,  idxType desiredPart, graph_t * newGraph);
vertex_t *buildQuotientVertex1(graph_t *graph,  idxType *newParts,  idxType desiredPart, double &peakMem, map<int,int> oldToNewIds);

vector<vertex_t *> substituteAssignedVerticesAndBuildUnassigne(Cluster *cluster, graph_t *quotienGraph);
vertex_t *
tryOutVertices(graph_t *quotienGraph, graph_t *source, const vertex_t *unassignedVertex, vector<vertex_t *> &criticalP,
               bool ignoreCP, double &minMakespan, int degree, edge_t *const *edgesToConsider, vertex_t * &optional3rdVertex, double bandwidth);

void printMaxDegrees(graph_t *graphWithNodeMems);
*/
#endif //FONDA_SCHED_PARTSCHED_H

