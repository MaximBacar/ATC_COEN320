#ifndef SSR_H_
#define SSR_H_

#include <string>
#include <vector>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

#define SSR_PERIOD 1.0  // Example value, can be adjusted
#define SIZE_SHM_PSR 1024  // Shared memory size for PSR
#define SIZE_SHM_PLANES 256  // Shared memory size for planes
#define SIZE_SHM_SSR 2048  // Shared memory size for SSR
#define SIZE_SHM_AIRSPACE 1024  // Shared memory size for airspace
#define SIZE_SHM_PERIOD 4  // Shared memory size for period

class Timer;  // Forward declaration

class SSR {
public:
    SSR(int numberOfPlanes);  // Constructor with number of planes
    virtual ~SSR();  // Destructor

    int start();  // Start the SSR operation
    int stop();  // Stop the SSR operation

    static void* startSSR(void* context);  // Static function to start SSR
    bool getPlaneInfo();  // Get information about the planes
    bool readFlyingPlanes();  // Read flying planes from shared memory

private:
    int initialize(int numberOfPlanes);  // Initialize SSR
    void* operateSSR();  // Operate SSR in a loop
    void writeFlyingPlanes();  // Write flying planes into shared memory

    pthread_t SSRthread;  // Thread for SSR
    pthread_attr_t attr;  // Thread attributes
    pthread_mutex_t mutex;  // Mutex for thread synchronization

    int numPlanes;  // Number of planes
    double currPeriod;  // Current SSR period

    Timer* timer;  // Timer instance for SSR
    time_t startTime;  // Start time for SSR
    time_t finishTime;  // Finish time for SSR

    int shm_flyingPlanes;  // Shared memory for flying planes
    int shm_airspace;  // Shared memory for airspace
    int shm_period;  // Shared memory for period

    void* flyingPlanesPtr;  // Pointer to flying planes data
    void* airspacePtr;  // Pointer to airspace data
    void* periodPtr;  // Pointer to period data

    std::vector<void*> planePtrs;  // Plane pointers
    std::vector<void*> waitingPtrs;  // Waiting plane pointers
    std::vector<std::string> waitingFileNames;  // File names for waiting planes
    std::vector<std::string> flyingFileNames;  // File names for flying planes
};

#endif /* SSR_H_ */
