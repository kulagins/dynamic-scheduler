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
        string tn = task->name;
        std::transform(tn.begin(), tn.end(), tn.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return nlohmann::json{
            {"task", tn},
            {"start", startTime}, {"machine", processor->id}, {"finish", finishTime}};
    }
};

 bool heft(graph_t *G, Cluster *cluster, double & makespan, vector<Assignment*> &assignments, double & avgPeakMem);

double calculateSimpleBottomUpRank(vertex_t *task);
double calculateBLCBottomUpRank(vertex_t *task);
std::vector < std::pair< vertex_t *, int> >  calculateMMBottomUpRank(graph_t * graph);

Processor * tentativeAssignment(vertex_t * v, Processor * pj ,  Cluster* cluster, double &finishTime);

double heuristic(graph_t * graph, Cluster * cluster, int bottomLevelVariant, int evictionVariant, vector<Assignment*> &assignments, double & peakMem);
vector<pair<vertex_t *, int>> calculateBottomLevels(graph_t *graph, int bottomLevelVariant);

double getFinishTimeWithPredecessorsAndBuffers(vertex_t *v, const Processor *pj, const Cluster *cluster, double &startTime);
void correctRtJJsOnPredecessors(Cluster *cluster, const vertex_t *vertexToAssign, const Processor *procToChange);

void kickEdgesThatNeededToKickedToFreeMemForTask(Processor *bestp, Processor *procToChange);
double removeInputPendingEdgesFromEverywherePendingMemAndBuffer(const Cluster *cluster, const vertex_t *vertexToAssign,
                                                                Processor *procToChange, double availableMem);

double howMuchMemoryIsStillAvailableOnProcIfTaskScheduledThere(const vertex_t *v, const Processor *pj);
double howMuchMemoryIsStillAvailableOnProcIfTaskScheduledThere3Part(const vertex_t *v, const Processor *pj);

void evict(Processor *modifiedProc, double &currentlyAvailableBuffer, double &stillTooMuch, bool evictBiggestFirst);

Processor * tentativeAssignmentDespiteMemory(vertex_t *v, Processor *pj, Cluster* cluster, double &finishTime, double & startTime, bool &isValid, double &peakMem, bool evictBiigestFirst=false);
void doRealAssignmentWithMemoryAdjustments(Cluster *cluster, double futureReadyTime, Processor *bestp, vertex_t *vertexToAssign,
                                           Processor *procToChange);
graph_t *convertToNonMemRepresentation(graph_t *withMemories, map<int, int> &noMemToWithMem);

string answerWithJson(vector<Assignment *> assignments, string workflowName);
vector<Assignment*> runAlgorithm(int algorithmNumber, graph_t * graphMemTopology, Cluster *cluster, string workflowName);
vertex_t * getLongestPredecessorWithBuffers(vertex_t *child, const Cluster *cluster, double &latestPredecessorFinishTime);
double isDelayPossibleUntil(Assignment* assignmentToDelay, double newStartTime, vector<Assignment*> assignments, Cluster* cluster);
std::vector<Assignment*>::iterator findAssignmentByName(vector<Assignment *> assignments, string name);

#endif //RESHI_TXT_DYNSCHED_HPP
