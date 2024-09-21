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

graph_t *currentWorkflow = NULL;
string currentName;
Cluster *currentCluster;
vector<Assignment *> currentAssignment;
double lastTimestamp = 0;
int currentAlgoNum = 0;
int updateCounter=0;
int delayCnt=0;

void update(const Rest::Request &req, Http::ResponseWriter resp) {
    updateCounter++;

    const string &basicString = req.body();
    json bodyjson;
    bodyjson = json::parse(basicString);
    cout << "Update ";
    if (Debug) {
        //cout<<basicString<<endl;
        // cout <<endl;
    } else {
        //   cout <<endl;
    }
    double timestamp = bodyjson["time"];
    cout << "timestamp " << timestamp << ", reason " << bodyjson["reason"] << " ";
    if (timestamp == lastTimestamp) {
        cout << "time doesnt move";
    } else lastTimestamp = timestamp;

    string wholeReason = bodyjson["reason"];
    size_t pos = wholeReason.find("occupied");
    std::string partBeforeOccupied = wholeReason.substr(0, pos);
    std::string partAfterOccupied = wholeReason.substr(pos + 11);

    pos = partBeforeOccupied.find(": ");
    pos += 2;
    std::string nameOfTaskWithProblem = partBeforeOccupied.substr(pos);
    nameOfTaskWithProblem = trimQuotes(nameOfTaskWithProblem);

    pos = partAfterOccupied.find("on machine");
    string nameOfProblemCause = partAfterOccupied.substr(0, pos);
    nameOfProblemCause = trimQuotes(nameOfProblemCause);

    pos = wholeReason.find(':');
    string errorName = wholeReason.substr(0, pos);


    double newStartTime = bodyjson["new_start_time"];
    if(timestamp==newStartTime){
        //cout<<endl<<"SAME TIME"<<endl;
    }

    if (currentWorkflow != NULL) {

        cout << "num finished tasks: " << bodyjson["finished_tasks"].size() << " ";
        for (auto element: bodyjson["finished_tasks"]) {
            const string &elementWithQuotes = to_string(element);
            string elem_string = trimQuotes(elementWithQuotes);
            vertex_t *vertex = findVertexByName(currentWorkflow, elem_string);
            if (vertex == NULL)
                printDebug("Update: not found vertex to set as finished: " + elem_string + "\n");
            else
                //remove_vertex(currentWorkflow,vertex);
                vertex->visited = true;
        }

        Assignment *assignmOfProblem = (*findAssignmentByName(currentAssignment, nameOfTaskWithProblem));
        double isDelayPossible = isDelayPossibleUntil(assignmOfProblem,
                                                      newStartTime, currentAssignment, currentCluster);


        if (isDelayPossible && timestamp!=newStartTime) {
            vector<Assignment *> assignments = currentAssignment;
            assignmOfProblem->startTime = newStartTime;
            assignmOfProblem->finishTime =
                    newStartTime + assignmOfProblem->task->makespan / assignmOfProblem->processor->getProcessorSpeed();

            assignmOfProblem = (*findAssignmentByName(assignments, nameOfTaskWithProblem));
            cout << endl << "DELAY POSSIBLE! " << endl;
            delayCnt++;
            assignmOfProblem->startTime = newStartTime;
            assignmOfProblem->finishTime =
                    newStartTime + assignmOfProblem->task->makespan / assignmOfProblem->processor->getProcessorSpeed();

            if (bodyjson.contains("running_tasks") && bodyjson["running_tasks"].is_array()) {
                const auto &runningTasks = bodyjson["running_tasks"];
                cout << "num running tasks: " << runningTasks.size() << " ";
                for (const auto &item: runningTasks) {
                    // Check if the required fields (name, start, machine) exist in each object
                    if (item.contains("name") && item.contains("start") && item.contains("machine")) {
                        std::string name = item["name"];
                        int start = item["start"];
                        int machine = item["machine"];

                        // Print the values of the fields to the console
                        std::cout << "Name: " << name << ", Start: " << start << ", Machine: " << machine << std::endl;
                        name = trimQuotes(name);
                        std::vector<Assignment *>::iterator position = std::find_if(assignments.begin(),
                                                                                    assignments.end(),
                                                                                    [name](Assignment *a) {
                                                                                        string tn = a->task->name;
                                                                                        std::transform(
                                                                                                tn.begin(),
                                                                                                tn.end(),
                                                                                                tn.begin(),
                                                                                                [](unsigned char c) {
                                                                                                    return std::tolower(
                                                                                                            c);
                                                                                                });
                                                                                        //cout<<"tn is "<<tn<<" name is "<<name<<endl;
                                                                                        return tn ==
                                                                                               name;
                                                                                    });

                        if (position == assignments.end()) {
                            cout << endl<< "not found in assignments" << endl;
                            continue;
                        } else {
                            assignments.erase(position);
                        }

                    }
                }
            }

            const string answerJson =
                    answerWithJson(assignments, currentName);
            cout << "answ with " << answerJson << endl;
            resp.send(Http::Code::Ok, answerJson);

        } else {

            Cluster *updatedCluster = new Cluster(currentCluster);
            vector<Assignment *> assignments, tempAssignments;

            for (auto &processor: updatedCluster->getProcessors()) {
                processor->assignSubgraph(NULL);
                processor->isBusy = false;
                processor->readyTime = timestamp;
                processor->availableMemory = processor->getMemorySize();
                processor->availableBuffer = processor->communicationBuffer;
                processor->pendingInBuffer.clear();
            }


            if (bodyjson.contains("running_tasks") && bodyjson["running_tasks"].is_array()) {
                const auto &runningTasks = bodyjson["running_tasks"];
                cout << "num running tasks: " << runningTasks.size() << " ";
                for (const auto &item: runningTasks) {
                    // Check if the required fields (name, start, machine) exist in each object
                    if (item.contains("name") && item.contains("start") && item.contains("machine")) {
                        std::string name = item["name"];
                        int start = item["start"];
                        int machine = item["machine"];

                        // Print the values of the fields to the console
                        // std::cout << "Name: " << name << ", Start: " << start << ", Machine: " << machine << std::endl;
                        name = trimQuotes(name);
                        if (name == nameOfProblemCause) {
                            //  cout<<"found problem cause"<<endl;
                        }
                        vertex_t *ver = findVertexByName(currentWorkflow, name);
                        ver->visited = true;
                        const vector<Assignment *>::iterator &it_assignm = std::find_if(currentAssignment.begin(),
                                                                                        currentAssignment.end(),
                                                                                        [name](Assignment *a) {
                                                                                            string tn = a->task->name;
                                                                                            std::transform(tn.begin(),
                                                                                                           tn.end(),
                                                                                                           tn.begin(),
                                                                                                           [](unsigned char c) {
                                                                                                               return std::tolower(
                                                                                                                       c);
                                                                                                           });
                                                                                            return tn == name;
                                                                                        });
                        if (it_assignm != currentAssignment.end()) {
                            (*it_assignm)->processor->readyTime = timestamp + (*it_assignm)->task->time /
                                                                              (*it_assignm)->processor->getProcessorSpeed();//(timestamp>(*it_assignm)->finishTime)? (timestamp+(*it_assignm)->task->time/(*it_assignm)->processor->getProcessorSpeed()): (*it_assignm)->finishTime;
                            (*it_assignm)->processor->availableMemory =
                                    (*it_assignm)->processor->getMemorySize() - (*it_assignm)->task->memoryRequirement;
                            //assert( (*it_assignm)->startTime==start);
                            tempAssignments.emplace_back(new Assignment(ver, (*it_assignm)->processor, start,
                                                                        start + ((*it_assignm)->finishTime) -
                                                                        (*it_assignm)->startTime));
                        } else {
                            cout << "running task not found in assignments " << name << endl;
                            std::for_each(currentAssignment.begin(), currentAssignment.end(), [](Assignment *a) {
                                cout << a->task->name << " on " << a->processor->id << ", ";
                            });
                            cout << endl;
                            if (ver != NULL) {
                                Processor *procOfTas = currentCluster->getProcessorById(machine);
                                procOfTas->readyTime = //timestamp>( start +
                                        //   ver->time / procOfTas->getProcessorSpeed())? (timestamp+ start +
                                        //                                                    ver->time / procOfTas->getProcessorSpeed()) :(start +
                                        //                                                ver->time / procOfTas->getProcessorSpeed());
                                procOfTas->availableMemory = procOfTas->getMemorySize() - ver->memoryRequirement;
                            } else {
                                cout << "task also not found in the workflow" << endl;
                            }
                        }


                    } else {
                        std::cerr << "One or more fields missing in a running task object." << std::endl;
                    }
                }

            } else {
                std::cout << "No running tasks found or wrong schema." << std::endl;
            }


            vertex_t *vertex = currentWorkflow->first_vertex;
            while (vertex != nullptr) {
                if (!vertex->visited)
                    for (int j = 0; j < vertex->in_degree; j++) {
                        edge *incomingEdge = vertex->in_edges[j];
                        vertex_t *predecessor = incomingEdge->tail;
                        if (predecessor->visited) {
                            Processor *processorInUpdated = updatedCluster->getProcessorById(
                                    predecessor->assignedProcessor->id);
                            auto foundInPendingMems = find_if(predecessor->assignedProcessor->pendingMemories.begin(),
                                                              predecessor->assignedProcessor->pendingMemories.end(),
                                                              [incomingEdge](edge_t *edge) {
                                                                  return edge->tail->name == incomingEdge->tail->name &&
                                                                         edge->head->name == incomingEdge->head->name;
                                                              });
                            if (foundInPendingMems == predecessor->assignedProcessor->pendingMemories.end()) {
                                auto foundInPendingBufs = find_if(
                                        predecessor->assignedProcessor->pendingInBuffer.begin(),
                                        predecessor->assignedProcessor->pendingInBuffer.end(),
                                        [incomingEdge](edge_t *edge) {
                                            return edge->tail->name == incomingEdge->tail->name &&
                                                   edge->head->name == incomingEdge->head->name;
                                        });
                                if (foundInPendingMems == predecessor->assignedProcessor->pendingMemories.end()) {
                                    processorInUpdated->pendingInBuffer.insert(incomingEdge);
                                    processorInUpdated->availableBuffer -= incomingEdge->weight;
                                } else {
                                    throw new runtime_error(
                                            "edge " + incomingEdge->tail->name + " -> " + incomingEdge->head->name +
                                            " found neither in pending mems, nor in bufs of processor " +
                                            to_string(predecessor->assignedProcessor->id));
                                }
                            } else {
                                processorInUpdated->pendingMemories.insert(incomingEdge);
                                processorInUpdated->availableMemory -= incomingEdge->weight;
                            }
                        }
                    }
                vertex = vertex->next;
            }


            currentAssignment.resize(0);

            double avgPeakMem = 0;
            assignments = runAlgorithm(currentAlgoNum, currentWorkflow, currentCluster, currentName);
            const string answerJson =
                    answerWithJson(assignments, currentName);
            cout << "answered the update request with " << answerJson << endl;

            Http::Uri::Query &query = const_cast<Http::Uri::Query &>(req.query());
            query.as_str();
            //std::string text = req.hasParam(":wf_name") ? req.param(":wf_name").as<std::string>() : "No parameter supplied.";

            assignments.insert(assignments.end(), tempAssignments.begin(), tempAssignments.end());
            currentAssignment = assignments;
            if (assignments.size() == tempAssignments.size()) {
                resp.send(Http::Code::Precondition_Failed, answerJson);
            } else {
                resp.send(Http::Code::Ok, answerJson);
            }
        }


    } else
        resp.send(Http::Code::Not_Acceptable, "No workflow has been scheduled yet.");
    cout<<updateCounter<<" "<<delayCnt<<endl;
}

void new_schedule(const Rest::Request &req, Http::ResponseWriter resp) {

    cout << fixed;
    graph_t *graphMemTopology;


    const string &basicString = req.body();
    json bodyjson;
    bodyjson = json::parse(basicString);
    // cout<<"Scheduler received a new workflow schedule request: "<<endl;//basicString<<endl;

    string workflowName = bodyjson["workflow"]["name"];
    currentName = workflowName;
    int algoNumber = bodyjson["algorithm"].get<int>();
    cout << "new, algo " << algoNumber << " ";

    string filename = "../input/";
    string suffix = "00";
    if (workflowName.substr(workflowName.size() - suffix.size()) == suffix) {
        filename += "generated/";//+filename;
    }
    filename += workflowName;

    filename += workflowName.substr(workflowName.size() - suffix.size()) == suffix ? ".dot" : "_sparse.dot";
    graphMemTopology = read_dot_graph(filename.c_str(), NULL, NULL, NULL);
    checkForZeroMemories(graphMemTopology);


    Cluster *cluster = Fonda::buildClusterFromJson(bodyjson);
    Fonda::fillGraphWeightsFromExternalSource(graphMemTopology, bodyjson);

    double maxMemReq = 0;
    vertex_t *vertex = graphMemTopology->first_vertex;
    while (vertex != nullptr) {
        if (vertex->memoryRequirement > maxMemReq) {
            maxMemReq = vertex->memoryRequirement;
        }

        double sumIn = 0, sumOut = 0;

        for (int i = 0; i < vertex->in_degree; i++) {
            sumIn += vertex->in_edges[i]->weight;
        }
        for (int i = 0; i < vertex->out_degree; i++) {
            sumOut += vertex->out_edges[i]->weight;
        }
        if (sumIn > maxMemReq) { cout << "InEdges exceed MemReq on " << vertex->name << endl; }
        if (sumOut > maxMemReq) { cout << "OutEdges exceed MemReq on " << vertex->name << endl; }

        if (sumIn > maxMemReq) { maxMemReq = sumIn; }
        if (sumOut > maxMemReq) { maxMemReq = sumOut; }
        vertex = vertex->next;
    }
    //  cout<<"MAX MEM REQ "<<maxMemReq<<endl;
    const vector<Assignment *> assignments = runAlgorithm(algoNumber, graphMemTopology, cluster, workflowName);
    const string answerJson =
            answerWithJson(assignments, workflowName);
    // cout<<answerJson<<endl;

    Http::Uri::Query &query = const_cast<Http::Uri::Query &>(req.query());
    query.as_str();

    delete currentWorkflow;
    currentWorkflow = graphMemTopology;
    delete currentCluster;
    currentCluster = cluster;
    currentAssignment.resize(0);
    currentAssignment = assignments;
    currentAlgoNum = algoNumber;
    resp.send(Http::Code::Ok, answerJson);
}

int main(int argc, char *argv[]) {
    using namespace Rest;
    Debug = false;//true;

    Router router;      // POST/GET/etc. route handler
    Port port(9900);    // port to listen on
    Address addr(Ipv4::any(), port);
    std::shared_ptr<Http::Endpoint> endpoint = std::make_shared<Http::Endpoint>(addr);
    auto opts = Http::Endpoint::options().maxRequestSize(9291456).threads(1);

    endpoint->init(opts);

    /* routes! */
    Routes::Post(router, "/wf/:id/update", Routes::bind(&update));
    Routes::Post(router, "/wf/new/", Routes::bind(&new_schedule));


    endpoint->setHandler(router.handler());
    endpoint->serve();


    return 0;
}

vector<Assignment *>
runAlgorithm(int algorithmNumber, graph_t *graphMemTopology, Cluster *cluster, string workflowName) {
    vector<Assignment *> assignments;
    try {

        auto start = std::chrono::system_clock::now();
        double avgPeakMem = 0;
        switch (algorithmNumber) {
            case 1: {
                double d = heuristic(graphMemTopology, cluster, 1, 0, assignments, avgPeakMem);
                cout << workflowName << " " << d << ((d == -1) ? "no" : " yes ") << avgPeakMem;
                break;
            }
            case 2: {
                double d = heuristic(graphMemTopology, cluster, 1, 1, assignments, avgPeakMem);
                cout << workflowName << " " << d << ((d == -1) ? "no" : " yes ") << avgPeakMem;
                break;
            }
            case 3: {
                double d = heuristic(graphMemTopology, cluster, 2, 0, assignments, avgPeakMem);
                cout << workflowName << " " << d << ((d == -1) ? "no" : " yes ") << avgPeakMem;
                break;
            }
            case 4: {
                double d = heuristic(graphMemTopology, cluster, 2, 1, assignments, avgPeakMem);
                cout << workflowName << " " << d << ((d == -1) ? "no" : " yes ") << avgPeakMem;
                break;
            }
            case 5: {
                double d = heuristic(graphMemTopology, cluster, 3, 0, assignments, avgPeakMem);
                cout << workflowName << " " << d << ((d == -1) ? "no" : " yes ") << avgPeakMem;
                break;
            }
            case 6: {
                double d = heuristic(graphMemTopology, cluster, 3, 1, assignments, avgPeakMem);
                cout << workflowName << " " << d << ((d == -1) ? "no" : " yes ") << avgPeakMem;
                break;
            }
            case 7: {
                double ms =
                        0;
                bool wasCorrect = heft(graphMemTopology, cluster, ms, assignments, avgPeakMem);
                cout << workflowName << " " << ms << (wasCorrect ? " yes" : " no") << " " << avgPeakMem;
                break;
            }
            default:
                cout << "UNKNOWN ALGORITHM" << endl;
        }
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        std::cout << " duration of algorithm " << elapsed_seconds.count() << endl;

    }
    catch (std::runtime_error &e) {
        cout << e.what() << endl;
        //return 1;
    }
    catch (...) {
        cout << "Unknown error happened" << endl;
    }
    return assignments;
}

