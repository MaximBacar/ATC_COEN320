#ifndef PSR_H_
#define PSR_H_

#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "Limits.h"
#include "Plane.h"
#include "SSR.h"
#include "Timer.h"

class PSR {
public:
    PSR(int numberOfPlanes);
    ~PSR();
    void start();
    int stop();
    static void* startPSR(void* context);

private:
    int initialize(int numberOfPlanes);
    void* operatePSR();
    void updatePeriod(int chid);
    bool readWaitingPlanes();
    void writeFlyingPlanes();

    Timer* timer;

    int numWaitingPlanes;
    int currPeriod;

    pthread_t PSRthread;
    pthread_attr_t attr;
    pthread_mutex_t mutex;

    time_t startTime;

    int shm_waitingPlanes;
    void* waitingPlanesPtr;
    std::vector<std::string> waitingFileNames;
    std::vector<void*> planePtrs;

    int shm_flyingPlanes;
    void* flyingPlanesPtr;
    std::vector<std::string> flyingFileNames;

    int shm_period;
    void* periodPtr;
};

#endif /* PSR_H_ */
