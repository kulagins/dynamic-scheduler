//
// Created by kulagins on 26.05.23.
//

#include "../include/fonda_scheduler/common.hpp"
#include "cluster.hpp"
#include "graph.hpp"
#include "fonda_scheduler/dynSched.hpp"

bool Debug;


std::string trimQuotes(const std::string& str) {
    if (str.empty()) {
        return str; // Return the empty string if input is empty
    }

    string result = str, prevResult = str;
   do{
        prevResult = result;
        size_t start = 0;
        size_t end = prevResult.length() - 1;

        // Check for leading quote
        if (prevResult[start] == '"'||prevResult[start] == '\\'|| prevResult[start]==' ') {
            start++;
        }

        // Check for trailing quote
        if (prevResult[end] == '"'|| prevResult[end]=='\\'|| prevResult[end]==' ') {
            end--;
        }

        result = prevResult.substr(start, end - start + 1);
    } while(result!=prevResult);
    return result;
}

string answerWithJson(vector<Assignment *> assignments, string workflowName){

    nlohmann::json jsonObject;

    jsonObject["id"] = workflowName;

    //"schedule": {

    nlohmann::json scheduleJson= nlohmann::json::array();

    // Serialize each Assignment object and add to the schedule object with task name as key
    for (const auto& assignment : assignments) {
        if(!assignment->task->visited)
            scheduleJson. push_back (assignment->toJson());
    }

    // Add the schedule to the main JSON object
    jsonObject["schedule"] = scheduleJson;

    return to_string(jsonObject);
}

void Cluster::printAssignment(){
    int counter =0;
    for (const auto &item: this->getProcessors()){
        if(item->isBusy){
            counter++;
            cout<<"Processor"<<counter<<"."<<endl;
            cout<<"\tMem: "<<item->getMemorySize()<<", Proc: "<<item->getProcessorSpeed()<<endl;

            vertex_t *assignedVertex = item->getAssignedTask();
            if(assignedVertex==NULL)
                cout<<"No assignment."<<endl;
            else{
                cout<<"Assigned subtree, id "<<assignedVertex->id<< ", leader "<<assignedVertex->leader<<", memReq "<< assignedVertex->memoryRequirement<<endl;
                for (vertex_t *u = assignedVertex->subgraph->source; u; u = next_vertex_in_topological_order(assignedVertex->subgraph, u)) {
                    cout<<u->name<<", ";
                }
                cout<<endl;
            }
        }
    }
}


void printDebug(string str){
    if(Debug){
        cout<<str<<endl;
    }
}
void printInlineDebug(string str){
    if(Debug){
        cout<<str;
    }
}


void checkForZeroMemories(graph_t *graph) {
    for (vertex_t *vertex = graph->source; vertex; vertex = next_vertex_in_topological_order(graph, vertex)) {
        if (vertex->memoryRequirement == 0) {
            printDebug("Found a vertex with 0 memory requirement, name: " + vertex->name);
        }
    }
}
