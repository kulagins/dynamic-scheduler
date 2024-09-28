
#ifndef FONDA_SCHED_COMMON_HPP
#define FONDA_SCHED_COMMON_HPP



#include "../../extlibs/memdag/src/graph.hpp"
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include "json.hpp"
#include "cluster.hpp"

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


using namespace Pistache;
using json = nlohmann::json;

using namespace std;
void printDebug(string str);
void printInlineDebug(string str);
void checkForZeroMemories(graph_t *graph);
std::string trimQuotes(const std::string& str);

void completeRecomputationOfSchedule(Http::ResponseWriter &resp, const json &bodyjson, double timestamp, vertex_t * vertexThatHasAProblem);
void removeSourceAndTarget(graph_t *graph, vector<pair<vertex_t *, double>> &ranks);

Cluster *
prepareClusterWithChangesAtTimestamp(const json &bodyjson, double timestamp, vector<Assignment *> &tempAssignments);


void delayOneTask(Http::ResponseWriter &resp, const json &bodyjson, string &nameOfTaskWithProblem, double newStartTime,
                  Assignment *assignmOfProblem);

#endif