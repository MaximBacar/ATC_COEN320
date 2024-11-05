#include "Display.h"
#include <iostream>

Display::Display() {
    initialize();
}

Display::~Display() {
    shm_unlink("display");
}

int Display::initialize() {
    int rc = pthread_attr_init(&attr);
    if (rc) {
        printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
    }

    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (rc) {
        printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
    }

    shm_display = shm_open("display", O_RDWR, 0666);
    if (shm_display == -1) {
        perror("in shm_open() Display");
        exit(1);
    }

    ptr_display = mmap(0, SIZE_SHM_DISPLAY, PROT_READ | PROT_WRITE, MAP_SHARED, shm_display, 0);

    if (ptr_display == MAP_FAILED) {
        perror("in map() Display");
        exit(1);
    }

    return 0;
}

void Display::start() {
    time(&startTime);
    if (pthread_create(&displayThread, &attr, &Display::startDisplay, (void*)this) != EOK) {
        displayThread = 0;
    }
}

int Display::stop() {
    pthread_join(displayThread, NULL);
    return 0;
}

void* Display::startDisplay(void* context) {
    ((Display*)context)->updateDisplay();
    return NULL;
}

void* Display::updateDisplay(void) {
    int chid = ChannelCreate(0);
    if (chid == -1) {
        std::cout << "couldn't create display channel!\n";
    }

    Timer timer(chid);
    timer.setTimer(PERIOD_D, PERIOD_D);

    int rcvid;
    Message msg;
    std::ofstream out("log");

    while (true) {
        if (rcvid == 0) {
            pthread_mutex_lock(&mutex);

            // Parsing and updating display logic
            pthread_mutex_unlock(&mutex);
        }
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
    }

    ChannelDestroy(chid);
    out.close();
    return 0;
}

void Display::printMap() {
    for (int j = 0; j < block_count; j++) {
        for (int k = 0; k < block_count; k++) {
            if (map[j][k] == "") {
                std::cout << "_|";
            } else {
                std::cout << map[j][k] << "|";
            }
        }
        std::cout << std::endl;
    }
    printf("%s\n", height_display.c_str());
}
