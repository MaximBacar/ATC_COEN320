#include "PSR.h"

// constructor
PSR::PSR(int numberOfPlanes) {
    currPeriod = PSR_PERIOD;
    numWaitingPlanes = numberOfPlanes;
    initialize(numberOfPlanes);
}

// destructor
PSR::~PSR() {
    shm_unlink("waiting_planes");
    shm_unlink("flying_planes");
    shm_unlink("period");
    for (std::string name : flyingFileNames) {
        shm_unlink(name.c_str());
    }
    pthread_mutex_destroy(&mutex);
    delete timer;
}

// start execution function
void PSR::start() {
    time(&startTime);
    if (pthread_create(&PSRthread, &attr, &PSR::startPSR, (void*)this) != EOK) {
        PSRthread = 0;
    }
}

// join execution thread
int PSR::stop() {
    pthread_join(PSRthread, NULL);
    return 0;
}

// entry point for execution thread
void* PSR::startPSR(void* context) {
    ((PSR*)context)->operatePSR();
    return NULL;
}

int PSR::initialize(int numberOfPlanes) {
    int rc = pthread_attr_init(&attr);
    if (rc) {
        printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
    }
    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (rc) {
        printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
    }

    shm_waitingPlanes = shm_open("waiting_planes", O_RDWR, 0666);
    if (shm_waitingPlanes == -1) {
        perror("in shm_open() PSR");
        exit(1);
    }

    waitingPlanesPtr = mmap(0, SIZE_SHM_PSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_waitingPlanes, 0);
    if (waitingPlanesPtr == MAP_FAILED) {
        perror("in map() PSR");
        exit(1);
    }

    std::string FD_buffer = "";

    for (int i = 0; i < SIZE_SHM_PSR; i++) {
        char readChar = *((char*)waitingPlanesPtr + i);

        if (readChar == ',') {
            waitingFileNames.push_back(FD_buffer);
            int shm_plane = shm_open(FD_buffer.c_str(), O_RDONLY, 0666);
            if (shm_plane == -1) {
                perror("in shm_open() PSR plane");
                exit(1);
            }
            void* ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ, MAP_SHARED, shm_plane, 0);
            if (ptr == MAP_FAILED) {
                perror("in map() PSR");
                exit(1);
            }
            planePtrs.push_back(ptr);
            FD_buffer = "";
            continue;
        } else if (readChar == ';') {
            waitingFileNames.push_back(FD_buffer);
            int shm_plane = shm_open(FD_buffer.c_str(), O_RDONLY, 0666);
            if (shm_plane == -1) {
                perror("in shm_open() PSR plane");
                exit(1);
            }
            void* ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ, MAP_SHARED, shm_plane, 0);
            if (ptr == MAP_FAILED) {
                perror("in map() PSR");
                exit(1);
            }
            planePtrs.push_back(ptr);
            break;
        }
        FD_buffer += readChar;
    }

    shm_flyingPlanes = shm_open("flying_planes", O_RDWR, 0666);
    if (shm_flyingPlanes == -1) {
        perror("in shm_open() PSR: airspace");
        exit(1);
    }

    flyingPlanesPtr = mmap(0, SIZE_SHM_SSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_flyingPlanes, 0);
    if (flyingPlanesPtr == MAP_FAILED) {
        printf("map failed airspace\n");
        return -1;
    }

    shm_period = shm_open("period", O_RDONLY, 0666);
    if (shm_period == -1) {
        perror("in shm_open() PSR: period");
        exit(1);
    }

    periodPtr = mmap(0, SIZE_SHM_PERIOD, PROT_READ, MAP_SHARED, shm_period, 0);
    if (periodPtr == MAP_FAILED) {
        perror("in map() PSR: period");
        exit(1);
    }

    return 0;
}

void* PSR::operatePSR() {
    int chid = ChannelCreate(0);
    Timer* newTimer = new Timer(chid);
    timer = newTimer;
    timer->setTimer(PSR_PERIOD, PSR_PERIOD);

    int rcvid;
    Message msg;

    while (true) {
        if (rcvid == 0) {
            pthread_mutex_lock(&mutex);
            updatePeriod(chid);
            bool move = readWaitingPlanes();
            if (move) {
                writeFlyingPlanes();
            }
            flyingFileNames.clear();
            pthread_mutex_unlock(&mutex);

            if (numWaitingPlanes <= 0) {
                std::cout << "psr done\n";
                return 0;
            }
        }
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
    }

    ChannelDestroy(chid);
    return 0;
}

void PSR::updatePeriod(int chid) {
    int newPeriod = atoi((char*)periodPtr);
    if (newPeriod != currPeriod) {
        currPeriod = newPeriod;
        timer->setTimer(currPeriod, currPeriod);
    }
}

bool PSR::readWaitingPlanes() {
    bool move = false;
    int i = 0;
    auto it = planePtrs.begin();
    while (it != planePtrs.end()) {
        int j = 0;
        for (; j < 4; j++) {
            if (*((char*)*it + j) == ',') break;
        }

        int curr_arrival_time = atoi((char*)(*it) + j + 1);

        time_t et;
        time(&et);
        double t_current = difftime(et, startTime);
        if (curr_arrival_time <= t_current) {
            move = true;
            flyingFileNames.push_back(waitingFileNames.at(i));
            waitingFileNames.erase(waitingFileNames.begin() + i);
            it = planePtrs.erase(it);
            numWaitingPlanes--;
        } else {
            i++;
            ++it;
        }
    }
    return move;
}

void PSR::writeFlyingPlanes() {
    std::string currentAirspace = "";
    std::string currentPlane = "";
    int i = 0;

    while (i < SIZE_SHM_SSR) {
        char readChar = *((char*)flyingPlanesPtr + i);
        if (readChar == ';') {
            if (i == 0) break;
            currentAirspace += currentPlane + ",";
            break;
        } else if (readChar == ',') {
            currentAirspace += currentPlane + ",";
            currentPlane = "";
            i++;
            continue;
        }
        currentPlane += readChar;
        i++;
    }

    for (std::string filename : flyingFileNames) {
        if (i == 0) {
            currentAirspace += filename;
            i++;
        } else {
            currentAirspace += "," + filename;
        }
    }
    currentAirspace += ";";
    sprintf((char*)flyingPlanesPtr, "%s", currentAirspace.c_str());
}
