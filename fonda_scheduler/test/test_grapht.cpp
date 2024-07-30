//
// Created by kulagins on 26.06.23.
//

#include "../../extlibs/googletest/googletest/include/gtest/gtest.h"
#include "graph.hpp"
#include "fonda_scheduler/partSched.hpp"

graph_t *buildBiggerGraphTWithNodeMems() {
    graph_t *graph = new_graph();

    vertex_t *va = new_vertex2Weights(graph, "a", 10, 5, NULL);
    vertex_t *vb = new_vertex2Weights(graph, "b", 10, 5, NULL);
    vertex_t *vc = new_vertex2Weights(graph, "c", 10, 5, NULL);
    vertex_t *vd = new_vertex2Weights(graph, "d", 10, 5, NULL);
    vertex_t *ve = new_vertex2Weights(graph, "e", 10, 5, NULL);
    vertex_t *vf = new_vertex2Weights(graph, "f", 10, 5, NULL);
    vertex_t *vg = new_vertex2Weights(graph, "g", 10, 5, NULL);

    edge_t *edge1 = new_edge(graph, va, vc, 2, NULL);
    edge1 = new_edge(graph, va, vd, 2, NULL);
    edge1 = new_edge(graph, vb, vd, 2, NULL);
    edge1 = new_edge(graph, vc, ve, 2, NULL);
    edge1 = new_edge(graph, ve, vf, 2, NULL);
    edge1 = new_edge(graph, vd, vf, 2, NULL);
    edge1 = new_edge(graph, vf, vg, 2, NULL);

    return graph;
}

graph_t *buildTinyGraphTWithNodeMems() {
    graph_t *graph = new_graph();
                                            //time, memReq
    vertex_t *va = new_vertex2Weights(graph, "a", 10, 5, NULL);
    vertex_t *vb = new_vertex2Weights(graph, "b", 10, 5, NULL);
    vertex_t *vc = new_vertex2Weights(graph, "c", 10, 5, NULL);
    edge_t *edge1 = new_edge(graph, va, vb, 2, NULL);
    edge_t *edge2 = new_edge(graph, va, vc, 2, NULL);
    graph->source= va;
    graph->target = vc;
   // enforce_single_source_and_target(graph);
    return graph;
}



graph_t *buildLongGraph(edge_t *edge1, edge_t *edge2) {
    graph_t *graph = new_graph();

    vertex_t *v1 = new_vertex2Weights(graph, "1", 5, 5, NULL);
    vertex_t *v2 = new_vertex2Weights(graph, "2", 5, 5, NULL);
    vertex_t *v3 = new_vertex2Weights(graph, "3", 5, 5, NULL);
    vertex_t *v4 = new_vertex2Weights(graph, "4", 5, 5, NULL);
    vertex_t *v5 = new_vertex2Weights(graph, "5", 5, 5, NULL);
    vertex_t *v6 = new_vertex2Weights(graph, "6", 5, 5, NULL);

    edge_t *edge = new_edge(graph, v1, v2, 2, NULL);
    edge_t *edgem = new_edge(graph, v2, v3, 2, NULL);
    edge2 = new_edge(graph, v1, v4, 2, NULL);
    edge1 = new_edge(graph, v3, v5, 2, NULL);
    edge_t *edges = new_edge(graph, v4, v5, 2, NULL);
    edge_t *edget = new_edge(graph, v5, v6, 2, NULL);
    graph->source= v1;
    graph->target= v6;

    return graph;
}


TEST (rollout, graphtTests) {

    graph_t *graph = buildTinyGraphTWithNodeMems();

    graph_t *graphBig = convertToNonMemRepresentation(graph);
    assert(graphBig->number_of_vertices == graph->number_of_vertices*2);
    assert(graphBig->number_of_edges ==8);

    vertex_t *vertex=graphBig->source;

    assert(strcmp(vertex->name, "a-in")==0);
    vertex=next_vertex_in_sorted_topological_order(graphBig,vertex,&sort_by_increasing_top_level);

    assert(strcmp(vertex->name, "a-out")==0);
    vertex=next_vertex_in_sorted_topological_order(graphBig,vertex,&sort_by_increasing_top_level);

    assert(strcmp(vertex->name, "b-in")==0);
    vertex=next_vertex_in_sorted_topological_order(graphBig,vertex,&sort_by_increasing_top_level);

    assert(strcmp(vertex->name, "c-in")==0);
    vertex=next_vertex_in_sorted_topological_order(graphBig,vertex,&sort_by_increasing_top_level);
    assert(strcmp(vertex->name, "b-out")==0);
    vertex = graphBig->target;
    assert(strcmp(vertex->name, "GRAPH_TARGET-out")==0);
}

TEST(criticalPath, ownFunctions){
    graph_t * graph = buildTinyGraphTWithNodeMems();
    enforce_single_source_and_target(graph);

    double maxWeight=0;
    vector<vertex_t*> vertices = criticalPath(graph,maxWeight);
    assert(maxWeight==2);
    assert(vertices.size()==3);

    free_graph(graph);
    edge_t *edge1, *edge2;
    graph = buildLongGraph(edge1, edge2);

    maxWeight=0;
    vertices = criticalPath(graph,maxWeight);
    assert(vertices.size()==5);
    assert(maxWeight==8);


}


TEST(makespan, ownFunctions){
    graph_t * graph = buildTinyGraphTWithNodeMems();

    enforce_single_source_and_target(graph);
    double ms = makespan(graph);
    assert(ms==30.0);

    graph->source->out_edges[0]->status = edge_status_t::IN_CUT;
    ms = makespan(graph);
    assert(ms==32.0);

    edge_t *edge1, *edge2;
    free_graph(graph);
    graph = buildLongGraph(edge1, edge2);
    cout<<"bla"<<endl;
    ms = makespan(graph);
    // TODO: THIS IS WRONG!!!
    assert(ms==40.0);

    edge1->status = edge_status_t::IN_CUT;
    edge2->status = edge_status_t::IN_CUT;

    ms = makespan(graph);
    // TODO: THIS IS WRONG!!!
    assert(ms==40.0);
    //free_graph(graph);
    //CORE DUMP here
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}