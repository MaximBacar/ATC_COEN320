#ifndef COMPUTERSYSTEM_H_
#define COMPUTERSYSTEM_H_

#include <cstdlib>
#include <errno.h>
#include <fstream>
#include <list>
#include <pthread.h>
#include <stdio.h>
#include <sys/neutrino.h>
#include <sys/siginfo.h>
#include <time.h>
#include <vector>

#include "Display.h"
#include "Limits.h"
#include "Plane.h"
#include "SSR.h"
#include "Timer.h"

class Plane;

struct trajectoryPrediction {
    int id;
    std::vector<int> posX;
    std::vector<int> posY;
    std::vector<int> posZ;
    int t;
    bool keep;
};

struct aircraft {
    int id;
    int t_arrival;
    int pos[3];
    int vel[3];
    bool keep;
    bool moreInfo;
    int commandCounter;
};

class ComputerSystem {
public:
    ComputerSystem(int numberOfPlanes);
    ~ComputerSystem();

    void start();
    int stop();

    static void* startComputerSystem(void* context);

private:
    int initialize();
    void* calculateTrajectories();
    bool readAirspace();
    void cleanPredictions();
    void computeViolations(std::ofstream* out);
    void writeToDisplay();
    void updatePeriod(int chid);

    Timer* timer;

    int numPlanes;
    int currPeriod;

    pthread_t computerSystemThread;
    pthread_attr_t attr;
    pthread_mutex_t mutex;

    time_t startTime;
    time_t finishTime;

    int shm_airspace;
    void* airspacePtr;
    std::vector<aircraft*> flyingPlanesInfo;
    std::vector<trajectoryPrediction*> trajectoryPredictions;

    int shm_period;
    void* periodPtr;

    int shm_display;
    void* displayPtr;

    std::vector<void*> commPtrs;
    std::vector<std::string> commNames;
};

#endif /* COMPUTERSYSTEM_H_ */
