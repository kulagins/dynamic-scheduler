
#include <iostream>
#include <iterator>
#include <sstream>

#include <fonda_scheduler/io/graphWeightsBuilder.hpp>
#include "fonda_scheduler/common.hpp"

namespace Fonda {

    Cluster *buildClusterFromJson(nlohmann::json query) {
        Cluster *cluster = new Cluster();
        int id = 0;
        for (auto element: query["cluster"]["machines"]) {
            Processor *p;

            if(   element.contains("speed")) {
                p = new Processor(element["memory"], element["speed"], id);
            }
            else p = new Processor(element["memory"], 1, id);
            id++;
            cluster->addProcessor(p);

        }
        cluster->initHomogeneousBandwidth(cluster->getNumberProcessors(), 1);
        Cluster::setFixedCluster(cluster);
        return cluster;
    }

    void fillGraphWeightsFromExternalSource(graph_t *graphMemTopology, nlohmann::json query) {
        for (const auto& [task_name, task_data] : query["workflow"]["tasks"].items()) {
            //std::cout << "Task name: " << task_name << std::endl;
            vertex_t *vertexToSet = findVertexByName(graphMemTopology, task_name);

            double sumW=0;
            int cntr=0;
            for (const auto &time_value: task_data["time_predicted"].items()){
                sumW+=time_value.value().get<double>();
                cntr++;

            }
            double timeToSet =sumW / cntr ;
            vertexToSet->time = timeToSet;

             sumW=0;
             cntr=0;
            for (const auto &time_value: task_data["memory_predicted"].items()){
                sumW+=time_value.value().get<double>();
                cntr++;

            }
            auto memToSet = sumW/cntr ;
            vertexToSet->memoryRequirement = memToSet;

        }
        retrieveEdgeWeights(graphMemTopology, query);
    }

   void
    retrieveEdgeWeights(graph_t *graphMemTopology, nlohmann::json query) {

        map<vertex_t*, double> wchars, inputsizes;
       for (const auto& [task_name, task_data] : query["workflow"]["tasks"].items()) {
           //std::cout << "Task name: " << task_name << std::endl;
           vertex_t *vertexToSet = findVertexByName(graphMemTopology, task_name);

           double sumW=0;
           int cntr=0;
           for (const auto &time_value: task_data["wchar"].items()){
               sumW+=time_value.value().get<double>();
               cntr++;

           }
           wchars.emplace(vertexToSet, (cntr==0?1: sumW / cntr));

           sumW=0;
           cntr=0;
           for (const auto &time_value: task_data["taskinputsize"].items()){
               sumW+=time_value.value().get<double>();
               cntr++;

           }
           inputsizes.emplace(vertexToSet, (cntr==0?1: sumW / cntr));
       }

        // Rebalance edge values ????

       vertex_t *vertex = graphMemTopology->first_vertex;
       while (vertex != NULL) {
           double totalOutput =0;
           for (int j = 0; j < vertex->in_degree; j++) {
               edge *incomingEdge = vertex->in_edges[j];
               vertex_t *predecessor = incomingEdge->tail;
               totalOutput += wchars.at(predecessor);
           }
           for (int j = 0; j < vertex->in_degree; j++){
               edge *incomingEdge = vertex->in_edges[j];
               vertex_t *predecessor = incomingEdge->tail;
               incomingEdge->weight =(wchars.at(predecessor)/ totalOutput) * inputsizes.at(vertex);
           }

           vertex = vertex->next;
       }

    }



}