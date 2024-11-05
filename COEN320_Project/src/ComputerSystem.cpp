#include "ComputerSystem.h"

// constructor
ComputerSystem::ComputerSystem(int numberOfPlanes) {
    numPlanes = numberOfPlanes;
    currPeriod = CS_PERIOD;
    initialize();
}

// destructor
ComputerSystem::~ComputerSystem() {
    shm_unlink("airspace");
    shm_unlink("display");
    for (std::string name : commNames) {
        shm_unlink(name.c_str());
    }
    pthread_mutex_destroy(&mutex);

    delete timer;
}

// start computer system
void ComputerSystem::start() {
    time(&startTime);
    if (pthread_create(&computerSystemThread, &attr, &ComputerSystem::startComputerSystem, (void *)this) != EOK) {
        computerSystemThread = 0;
    }
}

// join computer system thread
int ComputerSystem::stop() {
    pthread_join(computerSystemThread, NULL);
    return 0;
}

// entry point for execution function
void *ComputerSystem::startComputerSystem(void *context) {
    ((ComputerSystem *)context)->calculateTrajectories();
    return NULL;
}

int ComputerSystem::initialize() {
    // initialize thread members

    // set threads in detached state
    int rc = pthread_attr_init(&attr);
    if (rc) {
        printf("ERROR, RC from pthread_attr_init() is %d \n", rc);
    }

    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    if (rc) {
        printf("ERROR; RC from pthread_attr_setdetachstate() is %d \n", rc);
    }

    // shared memory members

    // shared memory from ssr for the airspace
    shm_airspace = shm_open("airspace", O_RDONLY, 0666);
    if (shm_airspace == -1) {
        perror("in compsys shm_open() airspace");
        exit(1);
    }

    airspacePtr = mmap(0, SIZE_SHM_AIRSPACE, PROT_READ, MAP_SHARED, shm_airspace, 0);
    if (airspacePtr == MAP_FAILED) {
        perror("in compsys map() airspace");
        exit(1);
    }

    // open period shm
    shm_period = shm_open("period", O_RDWR, 0666);
    if (shm_period == -1) {
        perror("in shm_open() PSR: period");
        exit(1);
    }

    // map period shm
    periodPtr = mmap(0, SIZE_SHM_PERIOD, PROT_READ | PROT_WRITE, MAP_SHARED, shm_period, 0);
    if (periodPtr == MAP_FAILED) {
        perror("in map() PSR: period");
        exit(1);
    }

    // open shm display
    shm_display = shm_open("display", O_RDWR, 0666);
    if (shm_display == -1) {
        perror("in compsys shm_open() display");
        exit(1);
    }

    // map display shm
    displayPtr = mmap(0, SIZE_SHM_DISPLAY, PROT_READ | PROT_WRITE, MAP_SHARED, shm_display, 0);
    if (displayPtr == MAP_FAILED) {
        perror("in compsys map() display");
        exit(1);
    }

    return 0;
}

void *ComputerSystem::calculateTrajectories() {
    // create channel to communicate with timer
    int chid = ChannelCreate(0);
    if (chid == -1) {
        std::cout << "couldn't create channel\n";
    }

    Timer *newTimer = new Timer(chid);
    timer = newTimer;
    timer->setTimer(CS_PERIOD, CS_PERIOD);

    int rcvid;
    Message msg;
    std::ofstream out("command");

    while (true) {
        // Rest of the logic for calculating trajectories

        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
    }

    out.close();
    ChannelDestroy(chid);
    return 0;
}
