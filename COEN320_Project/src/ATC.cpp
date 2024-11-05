#include "ATC.h"
#include <iostream>
#include <fstream>

ATC::ATC() {
    std::cout << "Initializing ATC system..." << std::endl;
    initialize();
    std::cout << "Starting ATC system..." << std::endl;
    start();
}

ATC::~ATC() {
    std::cout << "Cleaning up ATC system..." << std::endl;

    // Release all shared memory pointers
    shm_unlink("airspace");
    shm_unlink("waiting_planes");
    shm_unlink("flying_planes");
    shm_unlink("period");
    shm_unlink("display");

    // Clean up plane objects
    for (Plane* plane : planes) {
        delete plane;
    }

    // Clean up other objects
    delete psr;
    delete ssr;
    delete display;
    delete computerSystem;
}

int ATC::start() {
    std::cout << "Starting threaded objects..." << std::endl;

    // Start threaded objects
    psr->start();
    ssr->start();
    display->start();
    computerSystem->start();

    for (Plane* plane : planes) {
        plane->start();
    }

    // Join threaded objects after execution time
    std::cout << "Joining threads after execution..." << std::endl;
    for (Plane* plane : planes) {
        plane->stop();
    }

    psr->stop();
    ssr->stop();
    display->stop();
    computerSystem->stop();

    return 0;
}

int ATC::readInput() {
    // Adjust file path for flexibility with working directory
	std::string filename = "/output/build/input.txt"; // Using relative path

    std::cout << "Trying to open file: " << filename << std::endl;

    std::ifstream inputFile(filename);

    if (!inputFile.is_open()) {
        std::cerr << "Error: Can't find or open input.txt file at path: " << filename << std::endl;
        return 1;
    }

    // Plane data variables
    int ID, arrivalTime, arrivalCordX, arrivalCordY, arrivalCordZ, arrivalSpeedX, arrivalSpeedY, arrivalSpeedZ;

    std::cout << "Reading input from input.txt..." << std::endl;

    // Parse input.txt and create plane objects
    while (inputFile >> ID >> arrivalTime >> arrivalCordX >> arrivalCordY >> arrivalCordZ >> arrivalSpeedX >> arrivalSpeedY >> arrivalSpeedZ) {
        int pos[3] = {arrivalCordX, arrivalCordY, arrivalCordZ};
        int vel[3] = {arrivalSpeedX, arrivalSpeedY, arrivalSpeedZ};

        // Create plane object and add to planes vector
        Plane* plane = new Plane(ID, arrivalTime, pos, vel);
        planes.push_back(plane);
    }

    inputFile.close();
    std::cout << "Finished reading input.txt." << std::endl;
    return 0;
}

int ATC::initialize() {
    // Read input data
    if (readInput() != 0) {
        std::cerr << "Error in reading input data!" << std::endl;
        return -1;
    }

    // Initialize shared memory for waiting planes
    std::cout << "Initializing shared memory for waiting planes..." << std::endl;
    shm_waitingPlanes = shm_open("waiting_planes", O_CREAT | O_RDWR, 0666);
    if (shm_waitingPlanes == -1) {
        perror("Error opening shm: waiting planes");
        exit(1);
    }
    ftruncate(shm_waitingPlanes, SIZE_SHM_PSR);
    waitingPtr = mmap(0, SIZE_SHM_PSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_waitingPlanes, 0);
    if (waitingPtr == MAP_FAILED) {
        std::cerr << "Error mapping shared memory for waiting planes." << std::endl;
        return -1;
    }

    // Save file descriptors of waiting planes
    int i = 0;
    for (Plane* plane : planes) {
        sprintf((char*)waitingPtr + i, "%s,", plane->getFD());
        i += (strlen(plane->getFD()) + 1);
    }
    sprintf((char*)waitingPtr + i - 1, ";");  // Termination character

    // Initialize shared memory for flying planes (initially empty)
    std::cout << "Initializing shared memory for flying planes..." << std::endl;
    shm_flyingPlanes = shm_open("flying_planes", O_CREAT | O_RDWR, 0666);
    if (shm_flyingPlanes == -1) {
        perror("Error opening shm: flying planes");
        exit(1);
    }
    ftruncate(shm_flyingPlanes, SIZE_SHM_SSR);
    flyingPtr = mmap(0, SIZE_SHM_SSR, PROT_READ | PROT_WRITE, MAP_SHARED, shm_flyingPlanes, 0);
    if (flyingPtr == MAP_FAILED) {
        std::cerr << "Error mapping shared memory for flying planes." << std::endl;
        return -1;
    }
    sprintf((char*)flyingPtr, ";");

    // Initialize shared memory for airspace
    std::cout << "Initializing shared memory for airspace..." << std::endl;
    shm_airspace = shm_open("airspace", O_CREAT | O_RDWR, 0666);
    if (shm_airspace == -1) {
        perror("Error opening shm: airspace");
        exit(1);
    }
    ftruncate(shm_airspace, SIZE_SHM_AIRSPACE);
    airspacePtr = mmap(0, SIZE_SHM_AIRSPACE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_airspace, 0);
    if (airspacePtr == MAP_FAILED) {
        std::cerr << "Error mapping shared memory for airspace." << std::endl;
        return -1;
    }
    sprintf((char*)airspacePtr, ";");

    // Initialize shared memory for period update
    std::cout << "Initializing shared memory for period..." << std::endl;
    shm_period = shm_open("period", O_CREAT | O_RDWR, 0666);
    if (shm_period == -1) {
        perror("Error opening shm: period");
        exit(1);
    }
    ftruncate(shm_period, SIZE_SHM_PERIOD);
    periodPtr = mmap(0, SIZE_SHM_PERIOD, PROT_READ | PROT_WRITE, MAP_SHARED, shm_period, 0);
    if (periodPtr == MAP_FAILED) {
        std::cerr << "Error mapping shared memory for period." << std::endl;
        return -1;
    }
    int per = CS_PERIOD;
    std::string CSPeriod = std::to_string(per);
    sprintf((char*)periodPtr, CSPeriod.c_str());

    // Initialize shared memory for display
    std::cout << "Initializing shared memory for display..." << std::endl;
    shm_display = shm_open("display", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_display, SIZE_SHM_DISPLAY);
    displayPtr = mmap(0, SIZE_SHM_DISPLAY, PROT_READ | PROT_WRITE, MAP_SHARED, shm_display, 0);
    if (displayPtr == MAP_FAILED) {
        std::cerr << "Error mapping shared memory for display." << std::endl;
        return -1;
    }
    sprintf((char*)displayPtr, ";");

    // Create and initialize threaded objects
    std::cout << "Creating and initializing subsystems..." << std::endl;
    psr = new PSR(planes.size());
    ssr = new SSR(planes.size());
    display = new Display();
    computerSystem = new ComputerSystem(planes.size());

    return 0;
}
