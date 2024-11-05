#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#include "Limits.h"
#include "Timer.h"

const int block_count = (int)MARGIN / (int)SCALER + 1;

class Display {
public:
    Display();
    ~Display();
    int initialize();
    void start();
    int stop();
    static void* startDisplay(void* context);
    void* updateDisplay(void);
    void printMap();

private:
    time_t startTime;
    time_t finishTime;

    pthread_t displayThread;
    pthread_attr_t attr;
    pthread_mutex_t mutex;

    std::string map[block_count][block_count] = { {""} };
    std::string height_display = "";

    int shm_display;
    void* ptr_display;
};

#endif /* DISPLAY_H_ */
