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
    const string &basicString = req.body();
    json bodyjson;
    bodyjson = json::parse(basicString);
    cout<<"Scheduler received an update request: "<<basicString<<endl;
    double timestamp = 5000; //TODO extract from the query

    Cluster *updatedCluster = new Cluster(currentCluster);

    for (auto &processor: updatedCluster->getProcessors()){
        processor->assignSubgraph(NULL);
        processor->isBusy= false;
        processor->readyTime=timestamp;
        processor->availableMemory = processor->getMemorySize();
        processor->availableBuffer = processor-> communicationBuffer;
        processor->pendingInBuffer.clear();
    }


    if(currentWorkflow!=NULL){
        if (bodyjson.contains("running_tasks") &&  bodyjson["running_tasks"].is_array()) {
            const auto& runningTasks = bodyjson["running_tasks"];
            for (const auto& item : runningTasks) {
                // Check if the required fields (name, start, machine) exist in each object
                if (item.contains("name") && item.contains("start") && item.contains("machine")) {
                    std::string name = item["name"];
                    int start = item["start"];
                    int machine = item["machine"];

                    // Print the values of the fields to the console
                   // std::cout << "Name: " << name << ", Start: " << start << ", Machine: " << machine << std::endl;

                    const vector<Assignment *>::iterator &it_assignm = std::find_if(currentAssignment.begin(),
                                                                                    currentAssignment.end(),
                                                                                    [name](Assignment *a) { return a->task->name==name; });
                    if(it_assignm!=currentAssignment.end()){
                        (*it_assignm)->processor->readyTime= (*it_assignm)->finishTime;

                    }
                    else cout<<"running task not found in assignments "<<name<<endl;


                } else {
                    std::cerr << "One or more fields missing in a running task object." << std::endl;
                }
            }

        } else {
            std::cout << "No running tasks found or wrong schema." << std::endl;
        }

        for (auto element: bodyjson["finished_tasks"]) {
            string elem_string = trimQuotes(to_string(element));
            vertex_t *vertex = findVertexByName(currentWorkflow, elem_string);
            if(vertex==NULL)
                printDebug("Update: not found vertex to set as finished: "+elem_string+"\n");
            else
                //remove_vertex(currentWorkflow,vertex);
                vertex->visited=true;
        }

        vertex_t *vertex = currentWorkflow->first_vertex;
        while(vertex!= nullptr){
            if(!vertex->visited)
            for (int j = 0; j < vertex->in_degree; j++) {
                edge *incomingEdge = vertex->in_edges[j];
                vertex_t *predecessor = incomingEdge->tail;
               if(predecessor->visited){
                   Processor *processorInUpdated = updatedCluster->getProcessorById(predecessor->assignedProcessor->id);
                   auto foundInPendingMems = find_if(predecessor->assignedProcessor->pendingMemories.begin(), predecessor->assignedProcessor->pendingMemories.end(),
                                               [incomingEdge](edge_t *edge) {
                                                   return edge->tail->name == incomingEdge->tail->name &&
                                                          edge->head->name == incomingEdge->head->name;
                                               });
                   if(foundInPendingMems==predecessor->assignedProcessor->pendingMemories.end()){
                       auto foundInPendingBufs = find_if(predecessor->assignedProcessor->pendingInBuffer.begin(),
                                                         predecessor->assignedProcessor->pendingInBuffer.end(),
                                                         [incomingEdge](edge_t *edge) {
                                                             return edge->tail->name == incomingEdge->tail->name &&
                                                                    edge->head->name == incomingEdge->head->name;
                                                         });
                       if(foundInPendingMems==predecessor->assignedProcessor->pendingMemories.end()){
                           processorInUpdated->pendingInBuffer.insert(incomingEdge);
                           processorInUpdated->availableBuffer-= incomingEdge->weight;
                       }
                       else{throw new runtime_error("edge "+incomingEdge->tail->name+" -> "+incomingEdge->head->name+
                       " found neither in pending mems, nor in bufs of processor "+ to_string(predecessor->assignedProcessor->id));}
                   }
                   else {
                       processorInUpdated->pendingMemories.insert(incomingEdge);
                       processorInUpdated->availableMemory-= incomingEdge->weight;
                   }
               }
            }
            vertex = vertex->next;
        }



        currentAssignment.resize(0);
        vector<Assignment*> assignments;
        double avgPeakMem=0;
        double d = heuristic(currentWorkflow, currentCluster, 1, 1, assignments, avgPeakMem);
        cout<<"updated makespan is "<<d<<endl;
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
    cout<<"Scheduler received a new workflow schedule request: "<<basicString<<endl;

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

