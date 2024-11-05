#include "Plane.h"

Plane::Plane(int _ID, int _arrivalTime, int _position[3], int _speed[3]) {
    arrivalTime = _arrivalTime;
    ID = _ID;
    for (int i = 0; i < 3; i++) {
        position[i] = _position[i];
        speed[i] = _speed[i];
    }

    commandCounter = 0;
    commandInProgress = false;

    initialize();
}

Plane::~Plane() {
    shm_unlink(fileName.c_str());
    pthread_mutex_destroy(&mutex);
}

int Plane::start() {
    time(&startTime);
    if (pthread_create(&planeThread, &attr, &Plane::startPlane, (void*)this) != EOK) {
        planeThread = 0;
    }
    return 0;
}

bool Plane::stop() {
    pthread_join(planeThread, NULL);
    return 0;
}

void* Plane::startPlane(void* context) {
    ((Plane*)context)->flyPlane();
    return 0;
}

int Plane::initialize() {
    int rc = pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    fileName = "plane_" + std::to_string(ID);
    shm_fd = shm_open(fileName.c_str(), O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SIZE_SHM_PLANES);
    ptr = mmap(0, SIZE_SHM_PLANES, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    updateString();
    sprintf((char*)ptr, "%s0;", planeString.c_str());

    return 0;
}

void* Plane::flyPlane(void) {
    int chid = ChannelCreate(0);
    Timer timer(chid);
    timer.setTimer(arrivalTime * 1000000, PLANE_PERIOD);

    int rcvid;
    Message msg;

    while (true) {
        rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
        pthread_mutex_lock(&mutex);

        answerComm();
        updatePosition();

        if (checkLimits() == 0) {
            ChannelDestroy(chid);
            return 0;
        }

        pthread_mutex_unlock(&mutex);
    }

    ChannelDestroy(chid);
    return 0;
}

void Plane::answerComm() {
    if (commandInProgress) {
        commandCounter--;
        if (commandCounter <= 0) {
            commandInProgress = false;
        }
        return;
    }

    int i = 0;
    for (; i < SIZE_SHM_PLANES; i++) {
        if (*((char*)ptr + i) == ';') break;
    }

    if (*((char*)ptr + i + 1) == ';' || *((char*)ptr + i + 1) == '0') return;

    std::string buffer = "";
    int startIndex = i + 1;
    char readChar = *((char*)ptr + startIndex);

    while (readChar != ';') {
        buffer += readChar;
        readChar = *((char*)ptr + ++startIndex);
    }

    int currParam;
    std::string parseBuf = "";

    for (char currChar : buffer) {
        switch (currChar) {
            case 'x': currParam = 0; break;
            case 'y': currParam = 1; break;
            case 'z': currParam = 2; break;
            case '/':
                speed[currParam] = std::stoi(parseBuf);
                parseBuf = "";
                break;
            case ';':
                speed[currParam] = std::stoi(parseBuf);
                break;
            default:
                parseBuf += currChar;
                break;
        }
    }

    for (int k = 0; k < 500; k += abs(speed[2])) {
        commandCounter++;
    }
    commandInProgress = true;

    sprintf((char*)ptr + startIndex, "0;");
}

void Plane::updatePosition() {
    for (int i = 0; i < 3; i++) {
        position[i] += speed[i];
    }
    updateString();
}

void Plane::updateString() {
    std::string s = ",";
    planeString = std::to_string(ID) + s + std::to_string(arrivalTime) + s + std::to_string(position[0]) + s +
                  std::to_string(position[1]) + s + std::to_string(position[2]) + s + std::to_string(speed[0]) + s +
                  std::to_string(speed[1]) + s + std::to_string(speed[2]) + ";";
}

int Plane::checkLimits() {
    if (position[0] < SPACE_X_MIN || position[0] > SPACE_X_MAX ||
        position[1] < SPACE_Y_MIN || position[1] > SPACE_Y_MAX ||
        position[2] < SPACE_Z_MIN || position[2] > SPACE_Z_MAX) {
        planeString = "terminated";
        sprintf((char*)ptr, "%s0;", planeString.c_str());
        return 0;
    }

    sprintf((char*)ptr, "%s0;", planeString.c_str());
    return 1;
}

void Plane::Print() {
    std::cout << planeString << "\n";
}
