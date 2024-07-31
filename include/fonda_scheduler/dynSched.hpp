//
// Created by kulagins on 11.03.24.
//

#ifndef RESHI_TXT_DYNSCHED_HPP
#define RESHI_TXT_DYNSCHED_HPP


#include "graph.hpp"
#include "cluster.hpp"
#include "sp-graph.hpp"
#include "common.hpp"
#include "json.hpp"

class Assignment{


public:
    vertex_t * task;
    Processor * processor;
    double startTime;
    double finishTime;

    Assignment(vertex_t * t, Processor * p, double st, double ft){
        this->task =t;
        this->processor =p;
        this->startTime = st;
        this->finishTime = ft;
    }

    nlohmann::json toJson() const {
        return nlohmann::json{
            //{"taskName", task->name},
            {"start", startTime}, {"machine", processor->id}};
    }
};

 bool heft(graph_t *G, Cluster *cluster, double & makespan, vector<Assignment*> &assignments, double & avgPeakMem);

double calculateSimpleBottomUpRank(vertex_t *task);
double calculateBLCBottomUpRank(vertex_t *task);
std::vector < std::pair< vertex_t *, int> >  calculateMMBottomUpRank(graph_t * graph);

double makespanFromHEFT( graph_t * graph, vector<int> assignedProcessor, Cluster * cluster);
Processor * tentativeAssignment(vertex_t * v, Processor * pj ,  Cluster* cluster, double &finishTime);

double heuristic(graph_t * graph, Cluster * cluster, int bottomLevelVariant, int evictionVariant, vector<Assignment*> &assignments, double & peakMem);
vector<pair<vertex_t *, int>> calculateBottomLevels(graph_t *graph, int bottomLevelVariant);

double getFinishTimeWithPredecessorsAndBuffers(vertex_t *v, const Processor *pj, const Cluster *cluster, double &startTime);
void correctRtJJsOnPredecessors(Cluster *cluster, const vertex_t *vertexToAssign, const Processor *procToChange);

void kickEdgesThatNeededToKickedToFreeMemForTask(Processor *bestp, Processor *procToChange);
double removeInputPendingEdgesFromEverywherePendingMemAndBuffer(const Cluster *cluster, const vertex_t *vertexToAssign,
                                                                Processor *procToChange, double availableMem);

double buildRes(const vertex_t *v, const Processor *pj);

void evict(Processor *modifiedProc, double &currentlyAvailableBuffer, double &stillTooMuch);

Processor * tentativeAssignmentDespiteMemory(vertex_t *v, Processor *pj, Cluster* cluster, double &finishTime, double & startTime, bool &isValid, double &peakMem);
void doRealAssignmentWithMemoryAdjustments(Cluster *cluster, double minFinishTime, Processor *bestp, vertex_t *vertexToAssign,
                                           Processor *procToChange);

double retrace(graph_t* graph, Cluster* cluster);

void applyExponentialTransformationWithFactor(double factor, graph_t* graph);

double makespan(graph_t *graph, bool allowUnassigned, double beta);
void takeOverPartNumbers(graph_t *graph, int *parts, int i);
graph_t *convertToNonMemRepresentation(graph_t *withMemories, map<int, int> &noMemToWithMem);
void copyVertexNames(graph_t* graph, nlohmann::json body);

string answerWithJson(vector<Assignment *> assignments);

#endif //RESHI_TXT_DYNSCHED_HPP
