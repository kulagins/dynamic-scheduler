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


vector<Assignment*> runAlgorithm(int algorithmNumber, graph_t * graphMemTopology, Cluster *cluster, string workflowName){
    try {
        std::map<int, std::vector<std::string>> partitionMap;
        auto start = std::chrono::system_clock::now();
        vector<Assignment*> assignments;
        double avgPeakMem=0;
        switch (algorithmNumber) {
            case 1:
                throw new runtime_error("no part sched anymore");
            case 2: {
                double ms =
                        0;
                bool wasCorrect = heft(graphMemTopology, cluster, ms, assignments, avgPeakMem);
                cout << workflowName << " " << ms << (wasCorrect ? " yes" : " no") << " " << avgPeakMem;
                return assignments;
            }
                break;
            case 3: {
                double d = heuristic(graphMemTopology, cluster,1 , 1, assignments, avgPeakMem);
                cout << workflowName << " " << d << " yes " << avgPeakMem;
                return assignments;
            }
            case 4: {
                double d = heuristic(graphMemTopology, cluster,2 , 1, assignments, avgPeakMem);
                cout << workflowName << " " << d << " yes " << avgPeakMem;
                break;
            }
            case 5: {
                vector<Assignment*> assignments;
                double avgPeakMem=0;
                double d = heuristic(graphMemTopology, cluster,3 , 1, assignments, avgPeakMem);
                cout << workflowName << " " << d << ((d==-1)? "no":" yes " )<< avgPeakMem;
                return assignments;
            }
            case 6: {
                Cluster *cluster2 = new Cluster(cluster);
                Cluster *cluster3 = new Cluster(cluster);
                vector<Assignment*> assignments;
                double avgPeakMem=0;
                double d = heuristic(graphMemTopology, cluster,3 , 1, assignments, avgPeakMem);
                applyExponentialTransformationWithFactor(0.15, graphMemTopology);
                double d2 = retrace(graphMemTopology, cluster2);
                double d3 = heuristic(graphMemTopology, cluster3, 3,1,assignments, avgPeakMem);
                return assignments;
            }
            default:
                cout << "UNKNOWN ALGORITHM" << endl;
        }
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        std::cout <<" " <<elapsed_seconds.count() << endl;

    }
    catch (std::runtime_error &e) {
        cout << e.what() << endl;
        //return 1;
    }
}


void hello(const Rest::Request& request, Http::ResponseWriter response)
{
    response.send(Http::Code::Ok, "world!");
}

void update(const Rest::Request& req, Http::ResponseWriter resp)
{
   // const Rest::TypedParam &param = req.param("text");
    //Http::Uri::Query &query = req.query();

    std::string text = req.hasParam(":wf_name") ? req.param(":wf_name").as<std::string>() : "No parameter supplied.";
    resp.send(Http::Code::Ok, text);
}

void new_schedule(const Rest::Request& req, Http::ResponseWriter resp)
{
    graph_t *graphMemTopology;
    string filename = "../input/";

    const string &basicString = req.body();
    json bodyjson;
    bodyjson = json::parse(basicString);

    string workflowName = bodyjson["workflow"]["name"];
    int algoNumber = bodyjson["algorithm"].get<int>();

    filename+= workflowName;
    filename+="_sparse.dot";
    graphMemTopology = read_dot_graph(filename.c_str(), NULL, NULL, NULL);
    checkForZeroMemories(graphMemTopology);

    Cluster *cluster = Fonda::buildClusterFromJson(bodyjson);
    Fonda::fillGraphWeightsFromExternalSource(graphMemTopology, bodyjson);

   // long biggestMemInGraph = getBiggestMem(graphMemTopology);
   // double maxMemInCluster = cluster->getMemBiggestFreeProcessor()->getMemorySize();
  //  if (biggestMemInGraph > maxMemInCluster) {
 //       normalizeToBiggestProcessorMem(graphMemTopology, maxMemInCluster, biggestMemInGraph);
  //  }


    const vector<Assignment *> assignments = runAlgorithm(algoNumber, graphMemTopology, cluster, workflowName);
    const string  answerJson =
            answerWithJson(assignments);

    Http::Uri::Query &query = const_cast<Http::Uri::Query &>(req.query());
    query.as_str();
    //std::string text = req.hasParam(":wf_name") ? req.param(":wf_name").as<std::string>() : "No parameter supplied.";

    delete graphMemTopology;
    delete cluster;
    resp.send(Http::Code::Ok, answerJson);
}



int main(int argc, char *argv[]) {
    using namespace Rest;
    Debug = false;//true;

    Router router;      // POST/GET/etc. route handler
    Port port(9900);    // port to listen on
    Address addr(Ipv4::any(), port);
    std::shared_ptr<Http::Endpoint> endpoint = std::make_shared<Http::Endpoint>(addr);
    auto opts = Http::Endpoint::options().threads(1);   // how many threads for the server
    endpoint->init(opts);

    /* routes! */
    Routes::Get(router, "/hello", Routes::bind(&hello));
    //Routes::Get(router, "/wf/:id/update?", Routes::bind(&update));
    Routes::Post(router, "/wf/new/", Routes::bind(&new_schedule));
    //Routes::Get(router, "/http_get", Routes::bind(&http_get));

    endpoint->setHandler(router.handler());
    endpoint->serve();

   // auto start = std::chrono::system_clock::now();
   // auto end = std::chrono::system_clock::now();

    //std::chrono::duration<double> elapsed_seconds = end - start;

    return 0;
}


