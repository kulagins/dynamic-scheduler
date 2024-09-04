#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <utility>
#include <vector>
#include <map>
#include <filesystem>

#include "../extlibs/memdag/src/graph.hpp"
#include "../include/fonda_scheduler/dynSched.hpp"
#include "../extlibs/memdag/src/cluster.hpp"
#include "../include/fonda_scheduler/dynSched.hpp"
#include "../include/fonda_scheduler/io/graphWeightsBuilder.hpp"
#include "json.hpp"

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <cstring>

using namespace Pistache;
using json = nlohmann::json;

graph_t *currentWorkflow=NULL;
string currentName;
Cluster * currentCluster;
vector<Assignment *> currentAssignment;


void update(const Rest::Request& req, Http::ResponseWriter resp)
{
   // const Rest::TypedParam &param = req.param("text");
    //Http::Uri::Query &query = req.query();

    const string &basicString = req.body();
    json bodyjson;
    bodyjson = json::parse(basicString);

    const string &bodyString = to_string(bodyjson);

    double timestamp = 5000; //TODO extract from the query

    //std::string text = req.hasParam(":wf_name") ? req.param(":wf_name").as<std::string>() : "No parameter supplied.";

    currentAssignment.resize(0);
    for (auto &processor: currentCluster->getProcessors()){
        processor->assignSubgraph(NULL);
        processor->isBusy= false;
        processor->readyTime=timestamp;
    }


    if(currentWorkflow!=NULL){
        for (auto element: bodyjson["running_tasks"]) {
            string elem_string = trimQuotes(to_string(element));
           //TODO deal with running tasks!
           // vertex_t *vertex = findVertexByName(currentWorkflow, elem_string);
           // double futureReadyTime = timestamp +
           // doRealAssignmentWithMemoryAdjustments(currentCluster, minFinishTime, bestp, vertexToAssign, procToChange);
           // Assignment * assignment = new Assignment(vertexToAssign, bestp, startTimeToMinFinish, minFinishTime);
           //  assignments.emplace_back(assignment);
          }

        for (auto element: bodyjson["finished_tasks"]) {
            string elem_string = trimQuotes(to_string(element));
            vertex_t *vertex = findVertexByName(currentWorkflow, elem_string);
            if(vertex==NULL)
                cout<<"Update: not found vertex to delete: "<<elem_string<<endl;
            else
                remove_vertex(currentWorkflow,vertex);


        }

        vector<Assignment*> assignments;
        double avgPeakMem=0;
        double d = heuristic(currentWorkflow, currentCluster, 1, 1, assignments, avgPeakMem);
        const string  answerJson =
                answerWithJson(assignments, currentName);

        Http::Uri::Query &query = const_cast<Http::Uri::Query &>(req.query());
        query.as_str();
        //std::string text = req.hasParam(":wf_name") ? req.param(":wf_name").as<std::string>() : "No parameter supplied.";

        currentAssignment.resize(0);
        currentAssignment = assignments;
        resp.send(Http::Code::Ok, answerJson);

    }else
        resp.send(Http::Code::Not_Acceptable, "No workflow has been scheduled yet.");
}

void new_schedule(const Rest::Request& req, Http::ResponseWriter resp)
{
    graph_t *graphMemTopology;
    string filename = "../input/";

    const string &basicString = req.body();
    json bodyjson;
    bodyjson = json::parse(basicString);

    string workflowName = bodyjson["workflow"]["name"];
    currentName= workflowName;
    int algoNumber = bodyjson["algorithm"].get<int>();

    filename+= workflowName;
    filename+="_sparse.dot";
    graphMemTopology = read_dot_graph(filename.c_str(), NULL, NULL, NULL);
    checkForZeroMemories(graphMemTopology);


    Cluster *cluster = Fonda::buildClusterFromJson(bodyjson);
    Fonda::fillGraphWeightsFromExternalSource(graphMemTopology, bodyjson);

    const vector<Assignment *> assignments = runAlgorithm(algoNumber, graphMemTopology, cluster, workflowName);
    const string  answerJson =
            answerWithJson(assignments, workflowName);
    cout<<answerJson<<endl;

    Http::Uri::Query &query = const_cast<Http::Uri::Query &>(req.query());
    query.as_str();

    delete currentWorkflow;
    currentWorkflow = graphMemTopology;
    delete currentCluster;
    currentCluster = cluster;
    currentAssignment.resize(0);
    currentAssignment = assignments;
    resp.send(Http::Code::Ok, answerJson);
}

int main(int argc, char *argv[]) {
    using namespace Rest;
    Debug = false;//true;

    Router router;      // POST/GET/etc. route handler
    Port port(9900);    // port to listen on
    Address addr(Ipv4::any(), port);
    std::shared_ptr<Http::Endpoint> endpoint = std::make_shared<Http::Endpoint>(addr);
    auto opts = Http::Endpoint::options().maxRequestSize(262144).threads(1);
    endpoint->init(opts);

    /* routes! */
    Routes::Post(router, "/wf/:id/update", Routes::bind(&update));
    Routes::Post(router, "/wf/new/", Routes::bind(&new_schedule));


    endpoint->setHandler(router.handler());
    endpoint->serve();


    return 0;
}

vector<Assignment*> runAlgorithm(int algorithmNumber, graph_t * graphMemTopology, Cluster *cluster, string workflowName){
    vector<Assignment*> assignments;
    try {

        auto start = std::chrono::system_clock::now();
        double avgPeakMem=0;
        switch (algorithmNumber) {
            case 1: {
                double d = heuristic(graphMemTopology, cluster, 2, 1, assignments, avgPeakMem);
                cout << workflowName << " " << d << " yes " << avgPeakMem;
                break;
            }
            case 2: {
                double ms =
                        0;
                bool wasCorrect = heft(graphMemTopology, cluster, ms, assignments, avgPeakMem);
                cout << workflowName << " " << ms << (wasCorrect ? " yes" : " no") << " " << avgPeakMem;
                break;
            }
            case 3: {
                double d = heuristic(graphMemTopology, cluster,1 , 1, assignments, avgPeakMem);
                cout << workflowName << " " << d << " yes " << avgPeakMem;
                break;
            }
            case 4: {
                double d = heuristic(graphMemTopology, cluster,3 , 1, assignments, avgPeakMem);
                cout << workflowName << " " << d << ((d==-1)? "no":" yes " )<< avgPeakMem;
               break;
            }
            default:
                cout << "UNKNOWN ALGORITHM" << endl;
        }
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        std::cout <<"duration of algorithm " <<elapsed_seconds.count() << endl;

    }
    catch (std::runtime_error &e) {
        cout << e.what() << endl;
        //return 1;
    }
    catch(...){
        cout<<"Unknown error happened"<<endl;
    }
    return assignments;
}

