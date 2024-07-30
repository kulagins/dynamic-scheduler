/*
 * MongoDBReader.hpp
 *
 *  Created on: 21.04.2022
 *      Author: Fabian Brandt-Tumescheit, fabratu
 */


#include <iostream>
#include <iterator>
#include <sstream>

#include <fonda_scheduler/io/graphWeightsBuilder.hpp>
#include "fonda_scheduler/common.hpp"

namespace Fonda {

    Cluster *buildClusterLikeJonathansWithMaxMem(double maxMem) {
        vector<int> memSizes = {16, 32, 64, 16, 8, 192};
        vector<int> procSpeeds = {16, 32, 6, 12, 8, 32};
        Cluster *cluster = new Cluster();

        int id = 0;
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < memSizes.size(); j++) {
                double adaptedMem = maxMem * memSizes.at(j) / 192;
                Processor *p = new Processor(adaptedMem, procSpeeds.at(j), id);
                id++;
                cluster->addProcessor(p);
            }
        }
        cluster->initHomogeneousBandwidth(36);
        return cluster;
    }

    Cluster *buildHomogeneousClusterWithMaxMem(double maxMem) {

        Cluster *cluster = new Cluster();

        int id = 0;
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {
                double adaptedMem = maxMem * maxMem / 192;
                Processor *p = new Processor(adaptedMem, 32, id);
                id++;
                cluster->addProcessor(p);
            }
        }
        cluster->initHomogeneousBandwidth(36);
        return cluster;
    }


    Cluster *buildClusterWithVariableMaxMemAndFixedRest(int maxMem) {
        vector<int> memSizes = {16, 32, 64, 16, 8, maxMem};
        vector<int> procSpeeds = {16, 32, 6, 12, 8, 32};
        Cluster *cluster = new Cluster();

        int id = 0;
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < memSizes.size(); j++) {

                Processor *p = new Processor(memSizes.at(i), procSpeeds.at(j), id);
                id++;
                cluster->addProcessor(p);
            }
        }
        cluster->initHomogeneousBandwidth(36);
        return cluster;
    }

    Cluster *buildClusterLikeJonathansFewHeterogeneity(int maxMem) {
        vector<int> memSizes = {64, 64, 128, 64, 32, 192};
        vector<int> procSpeeds = {8, 16, 12, 12, 16, 16};
        Cluster *cluster = new Cluster();

        int id = 0;
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < memSizes.size(); j++) {
                double adaptedMem = maxMem * memSizes.at(j) / 192;
                Processor *p = new Processor(adaptedMem, procSpeeds.at(j), id);
                id++;
                cluster->addProcessor(p);
            }
        }
        cluster->initHomogeneousBandwidth(36);
        return cluster;
    }

    Cluster *buildClusterLikeJonathansMuchHeterogeneity(int maxMem) {
        vector<int> memSizes = {8, 64, 128, 8, 4, 384};
        vector<int> procSpeeds = {32, 64, 3, 24, 4, 64};
        Cluster *cluster = new Cluster();

        int id = 0;
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < memSizes.size(); j++) {
                double adaptedMem = maxMem * memSizes.at(j) / 384;
                Processor *p = new Processor(adaptedMem, procSpeeds.at(j), id);
                id++;
                cluster->addProcessor(p);
            }
        }
        cluster->initHomogeneousBandwidth(36);
        return cluster;
    }

    Cluster *buildClusterLikeJonathansNumPcs(int maxMem, int numPcs) {
        vector<int> memSizes = {16, 32, 64, 16, 8, 192};
        vector<int> procSpeeds = {16, 32, 6, 12, 8, 32};
        Cluster *cluster = new Cluster();

        int id = 0;
        for (int i = 0; i < numPcs; i++) {
            for (int j = 0; j < memSizes.size(); j++) {
                double adaptedMem = maxMem * memSizes.at(j) / 192;
                Processor *p = new Processor(adaptedMem, procSpeeds.at(j), id);
                id++;
                cluster->addProcessor(p);
            }
        }
        cluster->initHomogeneousBandwidth(6 * numPcs);
        return cluster;
    }

    Cluster *buildClusterLikeJonathansBeta(int maxMem, double beta) {
        vector<int> memSizes = {16, 32, 64, 16, 8, 192};
        vector<int> procSpeeds = {16, 32, 6, 12, 8, 32};
        Cluster *cluster = new Cluster();

        int id = 0;
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < memSizes.size(); j++) {
                double adaptedMem = maxMem * memSizes.at(j) / 192;
                Processor *p = new Processor(adaptedMem, procSpeeds.at(j), id);
                id++;
                cluster->addProcessor(p);
            }
        }
        cluster->initHomogeneousBandwidth(36, beta);
        return cluster;
    }

    Cluster *buildClusterLikeJonathansFatAndThin(int maxMem) {
        vector<int> memSizes = {16, 32, 64, 16, 8, 192};
        vector<int> procSpeeds = {12, 8, 6, 12, 32, 4};
        Cluster *cluster = new Cluster();

        int id = 0;
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < memSizes.size(); j++) {
                double adaptedMem = maxMem * memSizes.at(j) / 192;
                Processor *p = new Processor(adaptedMem, procSpeeds.at(j), id);
                id++;
                cluster->addProcessor(p);
            }
        }
        cluster->initHomogeneousBandwidth(36);
        return cluster;
    }

    Cluster *buildClusterWithMongoOrType(bool buildWithoutMongo, int type, double maxMem) {
        Cluster *cluster;
        if (buildWithoutMongo) {
            switch (type) {
                case 1:
                    cluster = Fonda::buildClusterLikeJonathansWithMaxMem(maxMem);
                    break;
                case 2:
                    cluster = Fonda::buildClusterWithVariableMaxMemAndFixedRest(maxMem);
                    break;

                case 3:
                    cluster = Fonda::buildClusterLikeJonathansFewHeterogeneity(maxMem);
                    break;
                case 4:
                    cluster = Fonda::buildClusterLikeJonathansMuchHeterogeneity(maxMem);
                    break;

                case 5:
                    cluster = Fonda::buildClusterLikeJonathansNumPcs(maxMem, 1);
                    break;
                case 6:
                    cluster = Fonda::buildClusterLikeJonathansNumPcs(maxMem, 3);
                    break;
                case 7:
                    cluster = Fonda::buildClusterLikeJonathansNumPcs(maxMem, 10);
                    break;

                case 8:
                    cluster = Fonda::buildClusterLikeJonathansBeta(maxMem, 0.1);
                    break;
                case 9:
                    cluster = Fonda::buildClusterLikeJonathansBeta(maxMem, 0.5);
                    break;
                case 10:
                    cluster = Fonda::buildClusterLikeJonathansBeta(maxMem, 3);
                    break;
                case 11:
                    cluster = Fonda::buildClusterLikeJonathansBeta(maxMem, 5);
                    break;
                case 12:
                    cluster = Fonda::buildClusterLikeJonathansFatAndThin(maxMem);
                    break;
                case 13:
                    cluster = Fonda::buildHomogeneousClusterWithMaxMem(maxMem);
                    break;
            }
        } else {
            throw new runtime_error("BUild cluster with mongo not supported anymore!");
        }
        return cluster;
    }

    Cluster *buildClusterFromJson(nlohmann::json query) {
        Cluster *cluster = new Cluster();
        int id = 0;
        for (auto element: query["cluster"]["machines"]) {

            Processor *p = new Processor(element["memory"], element["speed"], id);
            id++;
            cluster->addProcessor(p);

        }
        cluster->initHomogeneousBandwidth(cluster->getNumberProcessors(), 1);
        Cluster::setFixedCluster(cluster);
    }

    void fillGraphWeightsFromExternalSource(graph_t *graphMemTopology, nlohmann::json query) {
        for (const auto& [task_name, task_data] : query["workflow"]["tasks"].items()) {
            //std::cout << "Task name: " << task_name << std::endl;
            vertex_t *vertexToSet = findVertexByName(graphMemTopology, task_name);

            auto timeToSet = task_data["time_predicted"].items().begin().value().get<double>() ;
            vertexToSet->time = timeToSet;
            auto memToSet = task_data["memory_predicted"].items().begin().value().get<double>() ;
            vertexToSet->memoryRequirement = memToSet;

        }
    }


}