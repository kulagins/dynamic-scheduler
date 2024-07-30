//
// Created by kulagins on 26.05.23.
//

#include "../include/fonda_scheduler/common.hpp"
#include "cluster.hpp"
#include "graph.hpp"

bool Debug;

bool copyVertexWeights( graph_t * target ){
    throw new runtime_error("copy vertex weights not implemnented!");
    for(int i=1; i<=target->number_of_vertices; i++){
         target->vertices_by_id[i]->time = -1;
         target->vertices_by_id[i]->memoryRequirement = -1;
    }
  //  for(int i=1; i<=source.nVrtx; i++){
  //      if(i<=target->number_of_vertices){
   //         target->vertices_by_id[i]->time = source.vertexMakespanWeights[i];
    //        target->vertices_by_id[i]->memoryRequirement = source.vertexMemoryRequirements[i];
    //    }
   // }
    for(int i=1; i<=target->number_of_vertices; i++){
       assert( target->vertices_by_id[i]->time>=0);
       assert( target->vertices_by_id[i]->memoryRequirement>=0);
    }
    return true;
}

void Swap::executeSwap(){
    //cout<<"swap: task "<<firstTask->getId()<<" on "<<firstTask->getAssignedProcessorSpeed()<<", "
    // << secondTask->getId()<<" on "<<secondTask->getAssignedProcessorSpeed()<<endl;
    Processor *firstProcessor = firstTask->assignedProcessor;
    Processor *secondProcessor = secondTask->assignedProcessor;
    firstProcessor->assignSubgraph(secondTask);
    secondProcessor->assignSubgraph(firstTask);

    //firstTask->uncomputeMakespanUpUntiRoot();
   // secondTask->uncomputeMakespanUpUntiRoot();

    //    cout<<"end: task "<<firstTask->getId()<<" on "<<firstTask->getAssignedProcessorSpeed()<<", "
    //      << secondTask->getId()<<" on "<<secondTask->getAssignedProcessorSpeed()<<endl;
}

bool Swap::isFeasible(){

    Processor *firstProcessor = firstTask->assignedProcessor;
    Processor *secondProcessor = secondTask->assignedProcessor;

    return firstProcessor!=NULL && secondProcessor!=NULL && firstProcessor->getMemorySize()>= secondTask->memoryRequirement &&
           secondProcessor->getMemorySize()>=firstTask->memoryRequirement;
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

void prettyPrintPartition(){
    /*
    std::vector<std::pair<int, uint64_t>> partitionInputSize;
    for (auto &s: partitionMap) {
        //if (s.first == 0) {
        //    continue;
        //}
        uint64_t maxInputSize = std::numeric_limits<uint64_t>::min();
        for (auto &elem: s.second) {
            res = Fonda::getTaskValue(mongoClient, workflow, elem, fieldIn);
            maxInputSize = std::max(static_cast<uint64_t>(std::stoul(res)), maxInputSize);
        }
        partitionInputSize.push_back(std::make_pair(s.first, maxInputSize));
    }

    std::sort(partitionInputSize.begin(), partitionInputSize.end(), [](auto const &l, auto const &r) {
        return l.second > r.second;
    });
    std::map<std::string, bool> filter;
    filter["CPU"] = true;
    filter["Memory"] = true;
    filter["Machine"] = true;
    filter["Amount"] = true;

    mongoClient.setCollection("node_description");
    auto resFind = mongoClient.getFind(filter);
    resFind = mongoClient.getFind(filter);

    if (resFind.size() == 0)
        std::cout << "ERROR: Missing node information in database." << std::endl;

    std::vector<std::pair<std::string, uint64_t>> resourceSize;
    for (auto &elem: resFind) {
        for (uint64_t i = 0; i < std::stoull(elem["Amount"]); i++)
            resourceSize.push_back(std::make_pair(elem["Machine"], std::stoull(elem["Memory"])));
    }

    std::sort(resourceSize.begin(), resourceSize.end(), [](auto const &l, auto const &r) {
        return l.second > r.second;
    });

    for (int i = 0; i < partitionInputSize.size(); i++) {
        std::cout << "Partition " << partitionInputSize[i].first << " (inputSize: "
                  << (partitionInputSize[i].second / (1000 * 1000 * 1000)) << "GB) ";
        std::cout << "to Machine \"" << resourceSize[i].first << "\" (Memory: " << resourceSize[i].second << "GB)."
                  << std::endl;
    }

    std::cout << std::endl;

 */
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

string buildInputFileAndWorkflow( istream *OpenFile, string arg1, string &inputFile){
    (*OpenFile)  >> inputFile;

    std::size_t found = arg1.find_last_of("/\\");
    const string oString = arg1.substr(0, found);
    inputFile = oString + "/" + inputFile;
    //(argc >= 2) ? argv[1] : "../input/atacseq_sparse.dot"; //"../input/small_example.dot";
    if (inputFile[inputFile.length() - 1] == '/') {
        inputFile.erase(inputFile.end() - 1);
    }

    std::string delimiter = "_";
    std::string workflow = inputFile.substr(0, inputFile.find(delimiter));
    delimiter = "/";
    size_t pos = 0;
    while ((pos = workflow.find(delimiter)) != std::string::npos) {
        workflow = workflow.substr(pos + 1, workflow.length());
    }
return workflow;
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