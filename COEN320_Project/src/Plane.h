#ifndef PLANE_H_
#define PLANE_H_

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <sys/siginfo.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "Limits.h"
#include "Timer.h"

class Plane {
public:
    Plane(int _ID, int _arrivalTime, int _position[3], int _speed[3]);
    ~Plane();
    int start();
    bool stop();
    static void* startPlane(void* context);

    const char* getFD() { return fileName.c_str(); }

private:
    int initialize();
    void* flyPlane(void);
    void answerComm();
    void updatePosition();
    void updateString();
    int checkLimits();
    void Print();

    int arrivalTime;
    int ID;
    int position[3];
    int speed[3];

    int commandCounter;
    bool commandInProgress;

    pthread_t planeThread;
    pthread_attr_t attr;
    pthread_mutex_t mutex;

    time_t startTime;
    time_t finishTime;

    int shm_fd;
    void* ptr;
    std::string planeString;
    std::string fileName;
};

#endif /* PLANE_H_ */
