#ifndef inputParse_h
#define inputParse_h

#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
//#include <json.hpp>
#include <vector>
#include "fonda_scheduler/cluster.hpp"
#include "json.hpp"


using json = nlohmann::json;
using namespace std;

class InputParser {
protected:

    string workingDirectory;
    string pathToTree;
    string clusterListDir;
    ClusteringModes clusteringMode;
    vector<string> *clusterList;
    vector<string>::iterator clusterIterator;
    string chooseSubtree;
    string chooseNode;
    string assignChooseSubtree;
    int cutWhat;

protected:
    bool runA1;
    bool verbose;
    bool skipPartB;
public:

    InputParser(int argc, char **argv) {
        //cout << argc << endl;
        //cout << argv << endl;
        if (argc < 7) errorFunction(0);

        this->workingDirectory = initWorkingDir(argv[1]);
        cout << workingDirectory << endl;
        this->pathToTree = this->workingDirectory + argv[2];
        string clusterlistPath = argv[3];
        this->clusterList = initClusterList(clusterlistPath);


        resetClusterIterator();

        int cluMode = atoi(argv[4]);
        switch (cluMode) {
            case 0:
                this->clusteringMode = treeDependent;
                break;
            case 1:
                this->clusteringMode = staticClustering;
                break;

            default:
                cout << "clustering mode " << cluMode << endl;
                errorFunction(2);
                break;
        }

        this->runA1 = (bool) atoi((argv[5]));
        this->verbose = (bool) atoi((argv[6]));
        this->skipPartB = (bool) atoi((argv[7]));
        if (argc > 8) {
            this->chooseSubtree = argv[8];
            this->chooseNode = argv[9];
            this->assignChooseSubtree = argv[10];
            this->cutWhat = atoi(argv[11]);
        } else {
            this->chooseSubtree = "a";
            this->chooseNode = "a";
            this->assignChooseSubtree = "a";
            this->cutWhat = 0;
        }


    }

    ~InputParser() {
        delete this->clusterList;
    }

    string getWorkingDirectory() {
        return this->workingDirectory;
    }

    string getPathToTreeList() {
        return this->pathToTree;
    }

    string getPathToCluster() {
        return this->clusterListDir + *(this->clusterIterator);
    }

    ClusteringModes getClusteringMode() {
        return this->clusteringMode;
    }

    bool getVerbosity() {
        return verbose;
    }

    bool getRunA1() {
        return runA1;
    }


    bool getSkipPartB() {
        return skipPartB;
    }



    const string &getChooseSubtree() const {
        return chooseSubtree;
    }


    const string &getPathToTree() const {
        return pathToTree;
    }


    const string &getChooseNode() const {
        return chooseNode;
    }

    const string &getAssignChooseSubtree() const {
        return chooseSubtree;
    }

    const int &getCutWhat() const {
        return cutWhat;
    }


    void setClusterFromFile(double normedMemory){
        cout <<this->getPathToCluster()<<endl;
        ifstream inputFile(this->getPathToCluster());
        json clusterDescription;
        inputFile >> clusterDescription;

        vector<unsigned int> processorCounts;
        vector<double> mems;
        vector<double> speeds;
        
        for (auto& element : clusterDescription["groups"]) {
            processorCounts.push_back(element["number"]);
            mems.push_back(element["memory"]);
            speeds.push_back(element["speed"]);
        }

        if (this->getClusteringMode() == treeDependent){
            for (auto it = begin(mems); it != end(mems); it++)
            {
                *it = (*it) * normedMemory;
            }
        }

        Cluster *cluster = new Cluster(&processorCounts,&mems,&speeds);
       //cout << cluster->getPrettyClusterString()<<endl;
        Cluster::setFixedCluster(cluster);

    }


   void setClusterFromFile(double normedMemory, double shrinkingFactor = 1) {
        ifstream inputFile(this->getPathToCluster());
        json clusterDescription;
        if (!inputFile.is_open()) {
            throw "Could not open cluster file!";
        }
        inputFile >> clusterDescription;

        vector<unsigned int> *processorCounts = new vector<unsigned int>();
        vector<double> *mems = new vector<double>();
        vector<double> *speeds = new vector<double>();
        bool heterogeneous = false;
        vector<double> *BW_inside = new vector<double>();
        vector<double> *BW_outside = new vector<double>();

        for (auto element: clusterDescription["groups"]) {
            if (element == clusterDescription["groups"].front()) {
                if (element.contains("BandwInside")) {
                    heterogeneous = true;
                }
            }

            int numberProcessors = element["number"];
            processorCounts->push_back(ceil(numberProcessors / shrinkingFactor));
            mems->push_back(element["memory"]);
            speeds->push_back(element["speed"]);
            if (heterogeneous) {
                BW_inside->push_back(element["BandwInside"]);
                BW_outside->push_back(element["BandwOutside"]);
            }

        }

        if (this->getClusteringMode() == treeDependent) {
            for (auto it = mems->begin(); it != mems->end(); it++) {
                *it = (*it) * normedMemory;
            }
        }

        Cluster *cluster;
        if (heterogeneous) {
            cluster = new Cluster(processorCounts, mems, speeds);//, BW_inside, BW_outside);
        } else {
            cluster = new Cluster(processorCounts, mems, speeds);
        }
       // clusterDescription["groups"].size() == 1 ? cluster->setMemoryHomogeneity(true) :
       // cluster->setMemoryHomogeneity(false);

        Cluster::setFixedCluster(cluster);

        delete processorCounts;
        delete mems;
        delete speeds;
        delete BW_inside;
        delete BW_outside;
        inputFile.close();
    }


    void errorFunction(int reason) {
        switch (reason) {
            case 0:
                cout << "Please give the appropriate amount of commant line arguments" << endl;
                exit(-1);
                break;
            case 1:
                cout << "Please give an appropriate value for the Heterogenity Level" << endl;
                exit(-1);
                break;
            case 2:
                cout << "Please give an appropriate value for the Clustering Mode" << endl;
                exit(-1);
                break;
            default:
                exit(-1);
                break;
        }
    }

    string initWorkingDir(string path) {
        string workingDir = path;
        if (workingDir.substr(workingDir.find_last_of(".") + 1) == "txt") {
            workingDir = workingDir.substr(0, path.find_last_of("/") + 1);
        }
        return workingDir;
    }

    vector<string> *initClusterList(string path) {
        vector<string> *list = new vector<string>();
        if (path.substr(path.find_last_of(".") + 1) == "json") {

            this->clusterListDir = path.substr(0, path.find_last_of("/") + 1);
            list->push_back(path.substr(path.find_last_of("/") + 1));
            return list;
        }

        ifstream OpenFile(path);
        string line;
        stringstream line_stream;

        while (getline(OpenFile, line)) {
            line_stream.clear();
            line_stream.str(line);

            if (!line_stream.str().empty()) {
                string p;
                line_stream >> p;
                list->push_back(p);
            }

        }
        OpenFile.close();
        this->clusterListDir = path.substr(0, path.find_last_of("/") + 1);
        return list;
    }

    bool nextCluster() {
        this->clusterIterator++;
        return (this->clusterIterator != this->clusterList->end());
    }

    void resetClusterIterator() {
        this->clusterIterator = this->clusterList->begin();
        if (this->clusterIterator == this->clusterList->end()) {
            throw "No Cluster Input Given";
        }
    }
};


#endif