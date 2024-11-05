#include "SSR.h"
#include <sys/resource.h>
#include <sys/types.h>

SSR::SSR(int numberOfPlanes) {
    numPlanes = numberOfPlanes;  // Initialize numPlanes
    currPeriod = SSR_PERIOD;
    initialize(numberOfPlanes);
}

SSR::~SSR() {
    shm_unlink("flying_planes");
    shm_unlink("airspace");
    shm_unlink("period");
    pthread_mutex_destroy(&mutex);
    delete timer;
}

int SSR::start() {
    time(&startTime);  // Set the start time
    if (pthread_create(&SSRthread, &attr, &SSR::startSSR, (void *)this) != EOK) {
        SSRthread = 0;
    }
    return 0;
}

int SSR::stop() {
    pthread_join(SSRthread, NULL);
    return 0;
}

void *SSR::startSSR(void *context) {
    ((SSR *)context)->operateSSR();
    return NULL;
}

int SSR::initialize(int numberOfPlanes) {
    numPlanes = numberOfPlanes;  // Initialize numPlanes inside the method as well

    // Set thread in detached state
    int rc = pthread_attr_init(&attr);
    if (rc) {
        printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
    }

    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (rc) {
        printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
    }

    // Open list of waiting planes shared memory
    int shm_waitingPlanes = shm_open("waiting_planes", O_RDONLY, 0666);
    if (shm_waitingPlanes == -1) {
        perror("in shm_open() SSR waiting planes");
        exit(1);
    }

    // Set a new limit of max open files to 10000 to allow for more than 200 planes
    struct rlimit curr_limits, new_limits;
    pid_t curr_id = getpid();
    int pid = (int)curr_id;
    new_limits.rlim_cur = 10000;
    new_limits.rlim_max = 10000;
    prlimit(pid, RLIMIT_NOFILE, &new_limits, &curr_limits);

    // Map waiting planes shared memory
    void *waitingPlanesPtr = mmap(0, SIZE_SHM_PSR, PROT_READ, MAP_SHARED, shm_waitingPlanes, 0);
    if (waitingPlanesPtr == MAP_FAILED) {
        perror("in map() SSR waiting planes");
        exit(1);
    }

    std::string FD_buffer = "";

    for (int i = 0; i < SIZE_SHM_PSR; i++) {
        char readChar = *((char *)waitingPlanesPtr + i);

        if (readChar == ',') {
            waitingFileNames.push_back(FD_buffer);  // Add to waitingFileNames
            int shm_plane = shm_open(FD_buffer.c_str(), O_RDONLY, 0666);
            if (shm_plane == -1) {
                perror("in shm_open() SSR plane");
                exit(1);
            }

            void *ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ, MAP_SHARED, shm_plane, 0);
            if (ptr == MAP_FAILED) {
                perror("in map() SSR plane");
                exit(1);
            }

            waitingPtrs.push_back(ptr);  // Add to waitingPtrs
            FD_buffer = "";
            continue;
        } else if (readChar == ';') {
            waitingFileNames.push_back(FD_buffer);  // Add to waitingFileNames
            int shm_plane = shm_open(FD_buffer.c_str(), O_RDONLY, 0666);
            if (shm_plane == -1) {
                perror("in shm_open() SSR plane");
                exit(1);
            }

            void *ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ, MAP_SHARED, shm_plane, 0);
            if (ptr == MAP_FAILED) {
                perror("in map() SSR plane");
                exit(1);
            }

            waitingPtrs.push_back(ptr);  // Add to waitingPtrs
            break;
        }

        FD_buffer += readChar;
    }

    // Open flying planes shared memory
    shm_flyingPlanes = shm_open("flying_planes", O_RDWR, 0666);
    if (shm_flyingPlanes == -1) {
        perror("in shm_open() SSR: flying planes");
        exit(1);
    }

    flyingPlanesPtr = mmap(0, SIZE_SHM_SSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_flyingPlanes, 0);
    if (flyingPlanesPtr == MAP_FAILED) {
        perror("in map() SSR: flying planes");
        exit(1);
    }

    // Open airspace shared memory
    shm_airspace = shm_open("airspace", O_RDWR, 0666);
    if (shm_airspace == -1) {
        perror("in shm_open() SSR: airspace");
        exit(1);
    }

    airspacePtr = mmap(0, SIZE_SHM_AIRSPACE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_airspace, 0);
    if (airspacePtr == MAP_FAILED) {
        perror("in map() SSR: airspace");
        exit(1);
    }

    // Open period shared memory
    shm_period = shm_open("period", O_RDONLY, 0666);
    if (shm_period == -1) {
        perror("in shm_open() SSR: period");
        exit(1);
    }

    periodPtr = mmap(0, SIZE_SHM_PERIOD, PROT_READ, MAP_SHARED, shm_period, 0);
    if (periodPtr == MAP_FAILED) {
        perror("in map() SSR: period");
        exit(1);
    }

    return 0;
}

void *SSR::operateSSR() {
    while (1) {
        time(&finishTime);  // Record finish time
        if (difftime(finishTime, startTime) > currPeriod) {
            readFlyingPlanes();
            writeFlyingPlanes();
            time(&startTime);  // Reset the start time
        }
        usleep(SSR_PERIOD * 1000);  // Adjusted for milliseconds
    }
    return NULL;
}

bool SSR::readFlyingPlanes() {
    auto itName = waitingFileNames.begin();
    auto itPtr = waitingPtrs.begin();

    while (itName != waitingFileNames.end() && itPtr != waitingPtrs.end()) {
        planePtrs.push_back((*itPtr));  // Add plane pointers to planePtrs
        flyingFileNames.push_back(*itName);
        ++itName;
        ++itPtr;
    }

    return true;
}

bool SSR::getPlaneInfo() {
    auto it = planePtrs.begin();
    while (it != planePtrs.end()) {
        // Process each plane pointer (read details, etc.)
        ++it;
    }
    return true;
}

void SSR::writeFlyingPlanes() {
    auto itName = waitingFileNames.begin();
    auto itPtr = waitingPtrs.begin();

    while (itName != waitingFileNames.end() && itPtr != waitingPtrs.end()) {
        planePtrs.push_back((*itPtr));  // Add to plane pointers list
        flyingFileNames.push_back(*itName);
        ++itName;
        ++itPtr;
    }
}
