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

    size_t start = 0;
    size_t end = str.length() - 1;

    // Check for leading quote
    if (str[start] == '"') {
        start++;
    }

    // Check for trailing quote
    if (str[end] == '"') {
        end--;
    }

    // Return the trimmed string
    return str.substr(start, end - start + 1);
}

string answerWithJson(vector<Assignment *> assignments, string workflowName){

    nlohmann::json jsonObject;

    jsonObject["id"] = workflowName;

    //"schedule": {

    nlohmann::json scheduleJson;

    // Serialize each Assignment object and add to the schedule object with task name as key
    for (const auto& assignment : assignments) {
        scheduleJson[assignment->task->name] = assignment->toJson();
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

long getBiggestMem(graph_t* graph){
    vertex_t *vertex = graph->first_vertex;
    long maxMem = 0;
    while(vertex!=NULL){

        if(vertex->memoryRequirement>maxMem){
            maxMem = vertex->memoryRequirement;
        }
        vertex = vertex->next;
    }
    return maxMem;
}

void normalizeToBiggestProcessorMem(graph_t* graph, double biggestProcMem, long biggestMemInGraph){
    vertex_t *vertex = graph->first_vertex;

    while(vertex!=NULL){
        if(vertex->memoryRequirement>1){
            double sumEdges = 0;
            for (int j = 0; j < vertex->out_degree; j++) {
                sumEdges += vertex->out_edges[j]->weight;
            }
            for (int j = 0; j < vertex->in_degree; j++) {
                sumEdges += vertex->in_edges[j]->weight;
            }
            double newMem =  vertex->memoryRequirement * biggestProcMem / biggestMemInGraph;
            if(vertex->memoryRequirement == biggestMemInGraph){
               newMem = newMem - sumEdges;
            }

            vertex->memoryRequirement = newMem>1? newMem: vertex->memoryRequirement;
        }
        vertex = vertex->next;
    }
}

double fullMemReqOfVertex(vertex_t* vertexToAssign){
    double Res = vertexToAssign->memoryRequirement;

    //sum c_uv not on pj
    for (int j = 0; j < vertexToAssign->in_degree; j++) {
        edge *incomingEdge = vertexToAssign->in_edges[j];
        vertex_t *predecessor = incomingEdge->tail;
        Res -= incomingEdge->weight;
    }

    // sum c_uw on successors
    for (int j = 0; j < vertexToAssign->out_degree; j++) {
        edge *outgoindEdge = vertexToAssign->out_edges[j];
        Res -= outgoindEdge->weight;
    }
    return Res;

}