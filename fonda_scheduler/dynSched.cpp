//
// Created by kulagins on 11.03.24.
//

#include "fonda_scheduler/dynSched.hpp"
//#include "fonda_scheduler/partSched.hpp"

void copyVertexNames(graph_t* graph, nlohmann::json body){
    throw new runtime_error("copy vertex names not implemented!");
}
void takeOverPartNumbers(graph_t *graph, int *parts, int numLeaders) {

    std::set<int> sa(parts + 1, parts + numLeaders + 1);
    auto maxIterator = std::max_element(sa.begin(), sa.end());

    for (int i = 1; i <= graph->number_of_vertices; i++) {
        graph->vertices_by_id[i]->leader = parts[i];
        // cout<<"Assignt to vertex "<<graph->vertices_by_id[i]->name<<" part "<<parts [i+1]<<endl;
    }

    if (graph->number_of_vertices > numLeaders) {
        int firstExtraIndex = numLeaders;
        int cntr = 1;
        for (int i = firstExtraIndex + 1; i <= graph->number_of_vertices; i++) {
            graph->vertices_by_id[i]->leader = *maxIterator + cntr;
            //cout<<"Assign extra to vertex "<<graph->vertices_by_id[i]->name<<" part "<<graph->vertices_by_id[i]->leader<<endl;
            cntr++;
        }
    }
}

double makespan(graph_t *graph, bool allowUnassigned, double beta) {

    graph->target->bottom_level = graph->target->time;
    for (vertex_t *u = graph->target; u; u = next_vertex_in_anti_topological_order(graph, u)) {
        u->bottom_level = 0;
        for (int i = 0; i < u->out_degree; i++) {
            edge *edgeuv = u->out_edges[i];
            vertex_t *v = edgeuv->head; // edge u->v
            //if (edgeuv->status == edge_status_t::IN_CUT) {
            double v_bl = v->bottom_level + edgeuv->weight/beta;
            u->bottom_level = max_memdag(u->bottom_level, v_bl);
            //  } else {
            //     u->bottom_level += v->bottom_level;
            // }
        }
        //auto processorSpeed =  u->assignedProcessor==NULL? 1: u->assignedProcessor->getProcessorSpeed();
        if(!allowUnassigned && u->assignedProcessor==NULL && u->name!= "GRAPH_TARGET" && u->name!= "GRAPH_SOURCE"){
            cout<<"unassigned vertex "<<u->name<<endl;
            throw new runtime_error("unassigned vertex "+u->name);
        }

        auto processorSpeed = (u->assignedProcessor==NULL && (u->time==0 || allowUnassigned))? 1: u->assignedProcessor->getProcessorSpeed();
        u->bottom_level += u->time / processorSpeed;
    }

    return graph->source->bottom_level;
}

graph_t *convertToNonMemRepresentation(graph_t *withMemories,map<int, int> &noMemToWithMem) {
    enforce_single_source_and_target(withMemories);
    graph_t *noNodeMemories = new_graph();

    for (vertex_t *vertex = withMemories->source; vertex; vertex = next_vertex_in_sorted_topological_order(withMemories,
                                                                                                           vertex,
                                                                                                           &sort_by_increasing_top_level)) {
        vertex_t *invtx = new_vertex(noNodeMemories, vertex->name + "-in", vertex->time, NULL);
        noMemToWithMem.insert({invtx->id, vertex->id});
        if (!noNodeMemories->source) {
            noNodeMemories->source = invtx;
        }
        vertex_t *outvtx = new_vertex(noNodeMemories, vertex->name + "-out", 0.0, NULL);
        noMemToWithMem.insert({outvtx->id, vertex->id});
        edge_t *e = new_edge(noNodeMemories, invtx, outvtx, vertex->memoryRequirement, NULL);
        noNodeMemories->target = outvtx;

        for (int i = 0; i < vertex->in_degree; i++) {
            edge *inEdgeOriginal = vertex->in_edges[i];
            string expectedName = inEdgeOriginal->tail->name + "-out";
            vertex_t *outVtxOfCopiedInVtxOfEdge = findVertexByName(noNodeMemories, expectedName);

            if (outVtxOfCopiedInVtxOfEdge == NULL) {
                print_graph_to_cout(noNodeMemories);
                cout << "expected: " << expectedName << endl;
                throw std::invalid_argument(" no vertex found for expected name.");

            }
            edge_t *e_new = new_edge(noNodeMemories, outVtxOfCopiedInVtxOfEdge, invtx, inEdgeOriginal->weight, NULL);
        }
    }

    return noNodeMemories;
}


bool heft(graph_t *G, Cluster *cluster, double & makespan, vector<Assignment*> &assignments, double & avgPeakMem) {
    Cluster *clusterToCheckCorrectness = new Cluster(cluster);
    vector<pair<vertex_t *, int>> ranks = calculateBottomLevels(G, 1);
    sort(ranks.begin(), ranks.end(),[](pair<vertex_t *, int > a, pair<vertex_t *, int > b){ return  a.second> b.second;});

    bool errorsHappened = false;
    //choose the best processor
    vector<Processor *> assignedProcessor(G->number_of_vertices, NULL); // -1 indicates unassigned
    double maxFinishTime=0;
    for (const pair<vertex_t *, int > item: ranks) {
        int i = 0;
            vertex_t *vertex = item.first;
            //cout<<"assign vertex "<<vertex->name<<endl;
            double bestTime =  numeric_limits<double>::max();
            double startTimeToBestFinish;
            Processor * bestProc = NULL;
            for (size_t j = 0; j < cluster->getNumberProcessors(); ++j) {
                Processor *tentativeProcessor = cluster->getProcessors().at(j);
                double startTime;
                double finishTime = getFinishTimeWithPredecessorsAndBuffers(vertex, tentativeProcessor, cluster, startTime);
                if ( finishTime <= bestTime) {
                     bestTime = finishTime;
                     bestProc = tentativeProcessor;
                     startTimeToBestFinish = startTime;
                }
            }
            if(bestProc==NULL){
                return  numeric_limits<double>::max();
            }
            assignedProcessor[i] = bestProc;
            vertex->assignedProcessor = bestProc;
            bestProc->readyTime = bestTime;

            Assignment * assignment = new Assignment(vertex, bestProc, startTimeToBestFinish, bestTime);
            assignments.emplace_back(assignment);

            Processor *processorToCheckCorrectness = clusterToCheckCorrectness->getProcessorById(bestProc->id);
            double FT = numeric_limits<double>::max();
            bool isValid= true;
            double peakMem=0, startTime;
            Processor * pinfake = tentativeAssignmentDespiteMemory(vertex,processorToCheckCorrectness,cluster,FT, startTime, isValid, peakMem);
            bool isChange = pinfake != NULL;
            pinfake = isChange ? pinfake : processorToCheckCorrectness;
            if(FT ==numeric_limits<double>::max()){
                errorsHappened=true;
            }
            bestProc->peakMemConsumption=peakMem;
            if(!isValid){
                errorsHappened= true;
            }
            if(!errorsHappened){
                doRealAssignmentWithMemoryAdjustments(clusterToCheckCorrectness,FT,pinfake,
                                                      vertex, processorToCheckCorrectness);
            }
            if(isChange)
                delete pinfake;

            //cout<<" to "<<bestProc->id<<" ready at "<<bestProc->readyTime<<endl;
            correctRtJJsOnPredecessors(cluster, vertex, bestProc);

            vertex->makespan = bestTime;
            maxFinishTime = max(maxFinishTime, bestTime);
            i++;

    }
    for (const auto &item: cluster->getProcessors()){
        avgPeakMem+=item->peakMemConsumption;
    }
    avgPeakMem/=cluster->getNumberProcessors();
    makespan = maxFinishTime;
    return !errorsHappened;
}

double calculateSimpleBottomUpRank(vertex_t *task) {

    double maxCost = 0.0;
    for (int j = 0; j < task->out_degree; j++) {
        double communicationCost = task->out_edges[j]->weight;
        double successorCost = calculateSimpleBottomUpRank(task->out_edges[j]->head);
        double cost = communicationCost + successorCost;
        maxCost = max(maxCost, cost);
    }
    double retur = task->time + maxCost;
    return retur;
}

double calculateBLCBottomUpRank(vertex_t *task) {

    double maxCost = 0.0;
    for (int j = 0; j < task->out_degree; j++) {
        double communicationCost = task->out_edges[j]->weight;
        double successorCost = calculateBLCBottomUpRank(task->out_edges[j]->head);
        double cost = communicationCost + successorCost;
        maxCost = max(maxCost, cost);
    }
    double simpleBl = task->time + maxCost;

    double maxInputCost=0.0;
    for (int j = 0; j < task->in_degree; j++) {
        double communicationCost = task->in_edges[j]->weight;
        maxInputCost = max(maxInputCost, communicationCost);
    }
    double retur = simpleBl + maxInputCost;
    return retur;
}

std::vector < std::pair< vertex_t *, int> >  calculateMMBottomUpRank(graph_t * graphWMems){

    map<int, int> noMemToWithMem;
    graph_t *graph = convertToNonMemRepresentation(graphWMems, noMemToWithMem);

    SP_tree_t *sp_tree = NULL;
    graph_t *sp_graph = NULL;

    enforce_single_source_and_target(graph);
    sp_tree = build_SP_decomposition_tree(graph);
    if (sp_tree) {
        sp_graph = graph;
    } else {
        sp_graph = graph_sp_ization(graph);
        sp_tree = build_SP_decomposition_tree(sp_graph);
    }


    std::vector < std::pair< vertex_t *, int> > scheduleOnOriginal;

    if (sp_tree) {
        vertex_t **schedule = compute_optimal_SP_traversal(sp_graph, sp_tree);

        for(int i=0; i<sp_graph->number_of_vertices; i++){
            vertex_t *vInSp = schedule[i];
            const map<int, int>::iterator &it = noMemToWithMem.find(vInSp->id);
            if (it != noMemToWithMem.end()) {
                vertex_t *vertexWithMem = graphWMems->vertices_by_id[(*it).second];
                if (std::find_if(scheduleOnOriginal.begin(), scheduleOnOriginal.end(),
                                 [vertexWithMem](std::pair<vertex_t *, int> p) {
                                     return p.first->name == vertexWithMem->name;
                                 }) == scheduleOnOriginal.end()) {
                    scheduleOnOriginal.emplace_back(vertexWithMem, sp_graph->number_of_vertices-i);// TODO: #vertices - i?
                }
            }
        }

    } else {
        throw new runtime_error("No tree decomposition");
    }
    delete sp_tree;
    delete sp_graph;
    //delete graph;
    return scheduleOnOriginal;
}

double makespanFromHEFT(graph_t *graph, vector<int> assignedProcessor, Cluster * cluster) {
    int *array = new int[assignedProcessor.size()];
    std::copy(assignedProcessor.begin(), assignedProcessor.end(), array);

    takeOverPartNumbers(graph, array, assignedProcessor.size());
    delete[] array;


    vertex_t *vertex = graph->first_vertex;
    while (vertex != NULL ) {
        if( vertex->name!= "GRAPH_TARGET" && vertex->name!= "GRAPH_SOURCE")
            vertex->assignedProcessor = cluster->getProcessors().at(vertex->leader);
        vertex = vertex->next;
    }
    auto ms = makespan(graph, false, cluster->getBandwidth());
    return ms;
}


void evict(Processor *modifiedProc, double &currentlyAvailableBuffer, double &stillTooMuch) {

    if (modifiedProc->pendingMemories.size() > 0)
        assert((*modifiedProc->pendingMemories.begin())->weight <= (*modifiedProc->pendingMemories.rbegin())->weight);
    auto it = modifiedProc->pendingMemories.begin();

    // Iterate over the vector
    printInlineDebug("kick out ");
    int a=0;
    while (it != modifiedProc->pendingMemories.end() && stillTooMuch < 0) {
        stillTooMuch += (*it)->weight;
        currentlyAvailableBuffer -= (*it)->weight;
        a++;
        it = modifiedProc->pendingMemories.erase(it);
    }
}

double buildRes(const vertex_t *v, const Processor *pj) {
    double Res = pj->availableMemory - v->memoryRequirement;

    //sum c_uv not on pj
    for (int j = 0; j < v->in_degree; j++) {
        edge *incomingEdge = v->in_edges[j];
        vertex_t *predecessor = incomingEdge->tail;
        if (predecessor->assignedProcessor->id != pj->id) {
            Res -= incomingEdge->weight;
        }
    }

    // sum c_uw on successors
    for (int j = 0; j < v->out_degree; j++) {
        edge *outgoindEdge = v->out_edges[j];
        Res -= outgoindEdge->weight;
    }
    return Res;
}

double getFinishTimeWithPredecessorsAndBuffers(vertex_t *v, const Processor *pj, const Cluster *cluster, double & startTime) {
    double maxRtFromPredecessors=0;
    double bandwidth = cluster->getBandwidth();
    for (int j = 0; j < v->in_degree; j++) {
        edge *incomingEdge = v->in_edges[j];
        vertex_t *predecessor = incomingEdge->tail;
        if (predecessor->assignedProcessor->id != pj->id && predecessor->assignedProcessor!=NULL) {
            // Finish time from predecessor

            double rtFromPredecessor = predecessor->makespan + incomingEdge->weight / bandwidth;
            // communication buffer
            double buffer = cluster->readyTimesBuffers.at(predecessor->assignedProcessor->id).at(pj->id) +
                            +incomingEdge->weight / bandwidth;
            rtFromPredecessor = max(rtFromPredecessor, buffer);
            maxRtFromPredecessors = max(rtFromPredecessor, maxRtFromPredecessors);
        }
    }

    startTime = max(pj->readyTime, maxRtFromPredecessors);
    double finishtime = startTime + v->time/pj->getProcessorSpeed();
    return finishtime;
}

double heuristic(graph_t * graph, Cluster * cluster, int bottomLevelVariant, int evictionVariant,  vector<Assignment*> &assignments, double& avgPeakMem){
    vector<pair<Processor*, double>> peakMems;
    vector<pair<vertex_t *, int>> ranks = calculateBottomLevels(graph, bottomLevelVariant);
    sort(ranks.begin(), ranks.end(),[](pair<vertex_t *, int > a, pair<vertex_t *, int > b){ return  a.second> b.second;});
    double maxFinishTime=0;
    for (const pair<vertex_t *, int > item: ranks){
        double minFinishTime= numeric_limits<double>::max();
        double startTimeToMinFinish;
        Processor * bestp = nullptr;
        double peakMemOnBestp=0;
        bool isChange = false;
        vertex_t *vertexToAssign = item.first;
        if( vertexToAssign->name== "GRAPH_TARGET") continue;
        printInlineDebug("assign vertex "+vertexToAssign->name);
        for (const auto &p: cluster->getProcessors()){
            double finishTime, startTime;
            //Processor *pinfake  = tentativeAssignment(vertexToAssign, p, cluster, finishTime);
            bool isValid= true;
            double peakMem=0;
            Processor *pinfake  = tentativeAssignmentDespiteMemory(vertexToAssign, p, cluster, finishTime, startTime, isValid, peakMem);

            if(isValid && minFinishTime>finishTime){
                minFinishTime = finishTime;
                startTimeToMinFinish = startTime;
                if(isChange)
                    delete bestp;
                isChange= pinfake != NULL;
                bestp = isChange? pinfake: p;
                peakMemOnBestp = peakMem;
            }
        }
        if(minFinishTime==numeric_limits<double>::max() ){
            cout<<"Failed to find a processor for "<<vertexToAssign->name<<", FAIL"<<endl;
            assignments.resize(0);
            return -1;
        }

        Processor *procToChange = cluster->getProcessorById(bestp->id);
        printDebug("Really assigning to proc "+ to_string(procToChange->id));

        doRealAssignmentWithMemoryAdjustments(cluster, minFinishTime, bestp, vertexToAssign, procToChange);
        Assignment * assignment = new Assignment(vertexToAssign, bestp, startTimeToMinFinish, minFinishTime);
        assignments.emplace_back(assignment);

        if(isChange)
            delete bestp;
        //assignments.emplace_back(new Assignment(vertexToAssign, new Processor(procToChange), minFinishTime,0));
       maxFinishTime = max(maxFinishTime, minFinishTime);
       if(procToChange->peakMemConsumption<peakMemOnBestp) procToChange->peakMemConsumption= peakMemOnBestp;
    }
    for (const auto &item: cluster->getProcessors()){
        avgPeakMem+=item->peakMemConsumption;
    }
    avgPeakMem/=cluster->getNumberProcessors();
    return maxFinishTime;
}

void doRealAssignmentWithMemoryAdjustments(Cluster *cluster, double minFinishTime, Processor *bestp, vertex_t *vertexToAssign,
                                           Processor *procToChange) {
    vertexToAssign->assignedProcessor= procToChange;

    set<edge_t *>::iterator it;
    edge_t *pEdge;
    kickEdgesThatNeededToKickedToFreeMemForTask(bestp, procToChange);

    //remove pending memories incoming in this vertex, release their memory
    it = procToChange->pendingMemories.begin();
    while (it != procToChange->pendingMemories.end()) {
        pEdge = *it;
        if (pEdge->head->name == vertexToAssign->name )  {
            procToChange->availableMemory += pEdge->weight;
            it = procToChange->pendingMemories.erase(it);
        } else {
            if (pEdge->tail->name == vertexToAssign->name ) {
                cout<<"Vertex on tail, something wrong!"<<endl;
            }
            it++;
        }
    }

    //correct rt_j,j' for all predecessor buffers
    correctRtJJsOnPredecessors(cluster, vertexToAssign, procToChange);

    //compute available mem and add outgoing edges to pending memories
    double availableMem = procToChange->getAvailableMemory();
    for (int j = 0; j < vertexToAssign->out_degree; j++) {
       procToChange->pendingMemories.insert(vertexToAssign->out_edges[j]);
       availableMem-= vertexToAssign->out_edges[j]->weight;
    }
    availableMem = removeInputPendingEdgesFromEverywherePendingMemAndBuffer(cluster, vertexToAssign, procToChange,
                                                                            availableMem);
    assert(availableMem>=0);
    printDebug("to proc "+ to_string(procToChange->id)+ " remaining mem "+ to_string(availableMem)+ "ready time "+
                                                                                                    to_string(minFinishTime)+"\n");
    assert(availableMem <= (procToChange->getMemorySize() + 0.001));
    procToChange->setAvailableMemory(availableMem);
    procToChange->readyTime = minFinishTime;
    vertexToAssign->makespan = minFinishTime;
}

void correctRtJJsOnPredecessors(Cluster *cluster, const vertex_t *vertexToAssign, const Processor *procToChange) {
    for(int i=0; i < vertexToAssign->in_degree; i++){
        vertex_t *predecessor = vertexToAssign->in_edges[i]->tail;
        if(predecessor->assignedProcessor==NULL)
            cout<<"predecessor not assigned proc! "<<endl;
        if(predecessor->assignedProcessor->id!=procToChange->id){
            cluster->readyTimesBuffers.at(predecessor->assignedProcessor->id).at(procToChange->id) += vertexToAssign->in_edges[i]->weight/cluster->getBandwidth();
                    }

    }
}

double removeInputPendingEdgesFromEverywherePendingMemAndBuffer(const Cluster *cluster, const vertex_t *vertexToAssign,
                                                                Processor *procToChange, double availableMem) {
    for (int j = 0; j < vertexToAssign->in_degree; j++) {
        edge *edge = vertexToAssign->in_edges[j];
        for (auto processor: cluster->getProcessors()) {
            for (auto it = processor->pendingMemories.begin(); it != processor->pendingMemories.end();) {
                if ((*it)->tail->name == edge->tail->name && (*it)->head->name == edge->head->name) {
                    if(processor->id == procToChange->id){
                        availableMem += edge->weight;
                    }
                    else processor->availableMemory+= edge->weight;
                    it = processor->pendingMemories.erase(it);
                } else {
                    ++it;
                }
            }

            auto it = find_if(processor->pendingInBuffer.begin(), processor->pendingInBuffer.end(),
                              [](edge_t *edge) { return 0; });
            if (it != processor->pendingInBuffer.end()) {
                processor->availableBuffer += edge->weight;
                processor->pendingInBuffer.erase(it);
            }
            assert(processor->availableBuffer <= processor->communicationBuffer);
            assert(processor->availableMemory <= (processor->getMemorySize()+0.001));
        }
    }
    return availableMem;
}

void kickEdgesThatNeededToKickedToFreeMemForTask(Processor *bestp, Processor *procToChange) {
    auto it = procToChange->pendingMemories.begin();
    edge_t * pEdge = NULL;
    int i = 0;
    int numProcMems = procToChange->pendingMemories.size();
    if(!procToChange->pendingMemories.empty()){
        int bp_size = bestp->pendingMemories.size();
         int numIterations =0;
        while (it != procToChange->pendingMemories.end() && procToChange->pendingMemories.size()!= bp_size) {
            numIterations++;
            pEdge = *it;
            auto foundInbestP = find_if(bestp->pendingMemories.begin(), bestp->pendingMemories.end(),
                                             [pEdge](edge_t *edge) {
                                                 return edge->tail->name == pEdge->tail->name &&
                                                        edge->head->name == pEdge->head->name;
                                             });
            if (foundInbestP == bestp->pendingMemories.end() )  {
                printDebug("really kicking edge " +(*it)->tail->name+"->" +(*it)->head->name+", ");
                procToChange->availableBuffer -= (*it)->weight;
                procToChange->pendingInBuffer.insert(*it);
                procToChange->availableMemory+=(*it)->weight;
                it = procToChange->pendingMemories.erase(it);
                i++;

            } else {
                ++it;
            }
        }
       // if(numIterations>0) cout<<numIterations<<" iterations vs "<<numProcMems<<endl;
    }
    assert(procToChange->availableBuffer >= 0);
    assert(procToChange->pendingMemories.size() == bestp->pendingMemories.size());
    assert(procToChange->pendingInBuffer.size() >= i);
}

vector<pair<vertex_t *, int>> calculateBottomLevels(graph_t *graph, int bottomLevelVariant) {
    vector<pair<vertex_t *, int > > ranks;
    switch (bottomLevelVariant) {
        case 1: {
            vertex_t *vertex = graph->first_vertex;
            while (vertex != NULL) {
                double rank = calculateSimpleBottomUpRank(vertex);
                ranks.push_back({vertex, rank});
                vertex = vertex->next;
            }
            break;
        }
        case 2: {
            vertex_t *vertex = graph->first_vertex;
            while (vertex != NULL) {
                double rank = calculateBLCBottomUpRank(vertex);
                ranks.push_back({vertex, rank});
                vertex = vertex->next;
            }
            break;
        }
        case 3:
            ranks = calculateMMBottomUpRank(graph);
            break;
    }
    return ranks;
}


Processor * tentativeAssignmentDespiteMemory(vertex_t *v, Processor *pj, Cluster* cluster, double &finishTime, double & startTime, bool &isValid, double & peakMem) {
    Processor * modifiedProc = NULL;
    double maxInputCost = 0.0;

    //step 1
    for (int j = 0; j < v->in_degree; j++) {
        edge *incomingEdge = v->in_edges[j];
        vertex_t *predecessor = incomingEdge->tail;
        if (predecessor->assignedProcessor->id == pj->id) {
            if (std::find_if(pj->pendingMemories.begin(), pj->pendingMemories.end(),
                             [incomingEdge](edge_t *edge) {
                                 return incomingEdge->head->name == edge->head->name &&
                                        incomingEdge->tail->name == edge->tail->name;
                             })
                == pj->pendingMemories.end()) {
                printDebug("Failed because predecessors memory has been evicted");
               isValid = false;
            }
        }
    }
    //step 2

    printDebug(" proc"+ to_string(pj->id)+"avail mem "+ to_string(pj->availableMemory));
    double Res = buildRes(v, pj);
    peakMem = (Res<0)? 1:(pj->getMemorySize()-Res)/pj->getMemorySize();

    if(Res <0){
        //evict
        printDebug(" need to evict extra "+ to_string(Res));
        modifiedProc = new Processor(pj);
        double currentlyAvailableBuffer;
        double stillTooMuch;
        stillTooMuch= Res;
        currentlyAvailableBuffer= modifiedProc->availableBuffer;
        evict(modifiedProc, currentlyAvailableBuffer, stillTooMuch);

        cout<<stillTooMuch<<endl;

        if (stillTooMuch < 0 || currentlyAvailableBuffer < 0) {
            // could not evict enough
            printInlineDebug("could not evict enough ");
            if(stillTooMuch<0)printDebug("due to mem ");
            if(currentlyAvailableBuffer<0) printDebug("due to buffer");
            isValid = false;
        }
    }

    //step 3: tentatively assign

    finishTime = getFinishTimeWithPredecessorsAndBuffers(v, pj, cluster, startTime);
    return modifiedProc;
}

Processor * tentativeAssignment(vertex_t *v, Processor *pj, Cluster* cluster, double &finishTime) {
    Processor * modifiedProc = NULL;
    double maxInputCost = 0.0;
    //step 1
    for (int j = 0; j < v->in_degree; j++) {
        edge *incomingEdge = v->in_edges[j];
        vertex_t *predecessor = incomingEdge->tail;
        if (predecessor->assignedProcessor->id == pj->id) {
            if (std::find_if(pj->pendingMemories.begin(), pj->pendingMemories.end(),
                             [incomingEdge](edge_t *edge) {
                                 return incomingEdge->head->name == edge->head->name &&
                                        incomingEdge->tail->name == edge->tail->name;
                             })
                == pj->pendingMemories.end()) {
                printDebug("Failed because predecessors memory has been evicted");
                finishTime= numeric_limits<double>::max();
                return NULL;//pj;
            }
        }
    }
    //step 2

    double Res = buildRes(v, pj);


    if(Res <0){
        //evict

        modifiedProc = new Processor(pj);
        double currentlyAvailableBuffer;
        double stillTooMuch;
        stillTooMuch= Res;
        currentlyAvailableBuffer= modifiedProc->availableBuffer;
        evict(modifiedProc, currentlyAvailableBuffer, stillTooMuch);


        if (stillTooMuch < 0 || currentlyAvailableBuffer < 0) {
            // could not evict enough
            printInlineDebug("could not evict enough");
            if(stillTooMuch<0) printDebug("due to mem ");
            if(currentlyAvailableBuffer<0) printDebug("due to buffer");
            return NULL;
        }
    }

    //step 3: tentatively assign
    double startTime;
    finishTime = getFinishTimeWithPredecessorsAndBuffers(v, pj, cluster, startTime);

    return modifiedProc;
}

double retrace(graph_t* graph, Cluster* cluster){
    auto ranks = calculateMMBottomUpRank(graph);
    double maxFinishTime=0;
    for (const auto &item: ranks){
        auto vertexToCheck = item.first;
        if(vertexToCheck->name=="GRAPH_TARGET") continue;
        auto processor = vertexToCheck->assignedProcessor;
        if(processor==NULL)
            throw new runtime_error("No processor assigned");
        auto processorToCheck= cluster->getProcessorById(processor->id);
        if(processorToCheck==NULL)
            throw new runtime_error("Processor unavailable anymore.");

        double finishTime, startTime, peakMem;
        bool isValid=true;
        tentativeAssignmentDespiteMemory(vertexToCheck,processorToCheck, cluster,finishTime, startTime,isValid, peakMem);

        if(!isValid)
            throw new runtime_error("Invalid on task "+vertexToCheck->name);


        doRealAssignmentWithMemoryAdjustments(cluster,finishTime,processorToCheck,
                                                  vertexToCheck, processorToCheck);

        vertexToCheck->makespan = finishTime;
        maxFinishTime = max(maxFinishTime, finishTime);
    }
    return maxFinishTime;
}

double exponentialTransformation(double x, double median, double scaleFactor) {
    //cout<<x;
    if (x < median) {
       // cout<<" "<<x - (median - x) * scaleFactor<<endl;
        return x - (median - x) * scaleFactor;
    } else {
      //  cout<<" "<<x + (x - median) * scaleFactor<<endl;
        return x + (x - median) * scaleFactor;
    }

}

void applyExponentialTransformationWithFactor(double factor, graph_t* graph){
        vector<double> memories;
        vector<double> makespans;
       for(int i=1; i<graph->next_vertex_index;i++){
           memories.emplace_back(graph->vertices_by_id[i]->memoryRequirement);
           makespans.emplace_back(graph->vertices_by_id[i]->time);
       }
    std::sort(makespans.begin(), makespans.end());
    double medianMs;
    if (makespans.size() % 2 == 0) {
        medianMs = (makespans[makespans.size() / 2 - 1] + makespans[makespans.size() / 2]) / 2.0;
    } else {
        medianMs = makespans[makespans.size() / 2];
    }

    std::sort(memories.begin(), memories.end());
    double medianMem;
    if (memories.size() % 2 == 0) {
        medianMem = (memories[memories.size() / 2 - 1] + memories[memories.size() / 2]) / 2.0;
    } else {
        medianMem = memories[memories.size() / 2];
    }

    for(int i=1; i<graph->next_vertex_index;i++){
        double mem_exp = exponentialTransformation(graph->vertices_by_id[i]->memoryRequirement, medianMem, factor);
        cout<<graph->vertices_by_id[i]->memoryRequirement;
        graph->vertices_by_id[i]->memoryRequirement = (mem_exp<0? 0.1:mem_exp );
        cout<<" "<<graph->vertices_by_id[i]->memoryRequirement<<endl;
        cout<<graph->vertices_by_id[i]->time;
        double ms_exp = exponentialTransformation(graph->vertices_by_id[i]->time, medianMs, factor);
        graph->vertices_by_id[i]->time =  (ms_exp<0? 0.1:ms_exp );
        cout<<" "<<graph->vertices_by_id[i]->time<<endl;
    }
}