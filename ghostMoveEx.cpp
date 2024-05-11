#include <iostream>
#include <pthread.h>
#include <mutex>
#include <semaphore.h>
#include <cstdlib>
#include <unistd.h>
using namespace std;

sem_t coutSem;
bool needsKey[] = {1, 1, 1, 1}, needsPermit[] = {1, 1, 1, 1}, keyAvailable = true, permitAvailable = true;

void coutPls(string arg) {
    sem_wait(&coutSem);
    cout << arg << endl;
    sem_post(&coutSem);
}


void leaveHouse(int ghostID) 
{
    cout << "Thread" << ghostID << " in fun" << endl;
    while(!keyAvailable) {};

    if (needsKey[ghostID - 1] == true) 
    {
        cout << "Thread" << ghostID << " needs key" << endl;

        keyAvailable = false;
        needsKey[ghostID - 1] = false;

        cout << "Thread" << ghostID << " has key" << endl;
        while(!permitAvailable) {};

        if (needsPermit[ghostID - 1] == true) 
        {
            cout << "Thread" << ghostID << " needs permit" << endl;
            
            permitAvailable = false;
            needsPermit[ghostID - 1] = false;

            cout << "Thread" << ghostID << " has permit" << endl;

            keyAvailable = true;
            permitAvailable = true;

            cout << "Thread" << ghostID << " is relinquishing all resources !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
        }
        // has key and permit
    }
}

void* ghost1Movement(void* arg)
{
    int* ghost = (int*)arg;
    int& ghostID = *ghost;
    leaveHouse(ghostID);
    pthread_exit(0);
}

void* ghost2Movement(void* arg)
{
    int* ghost = (int*)arg;
    int& ghostID = *ghost;
    leaveHouse(ghostID);
    pthread_exit(0);
}

void* ghost3Movement(void* arg)
{
    int* ghost = (int*)arg;
    int& ghostID = *ghost;
    leaveHouse(ghostID);
    pthread_exit(0);
}

void* ghost4Movement(void* arg)
{
    int* ghost = (int*)arg;
    int& ghostID = *ghost;
    leaveHouse(ghostID);
    pthread_exit(0);
}

int main() 
{
    while (1) {
        int gID1 = 1, gID2 = 2, gID3 = 3, gID4 = 4;

        pthread_t ghost1Thread, ghost2Thread, ghost3Thread, ghost4Thread;

        pthread_create(&ghost1Thread, nullptr, &ghost1Movement, (void*)&gID1);
        pthread_create(&ghost2Thread, nullptr, &ghost2Movement, (void*)&gID2);
        pthread_create(&ghost3Thread, nullptr, &ghost3Movement, (void*)&gID3);
        pthread_create(&ghost4Thread, nullptr, &ghost4Movement, (void*)&gID4);

        void* s1 = nullptr, *s2 = nullptr, *s3 = nullptr, *s4 = nullptr;

        pthread_join(ghost1Thread, &s1);
        pthread_join(ghost2Thread, &s2);
        pthread_join(ghost3Thread, &s3);
        pthread_join(ghost4Thread, &s4);

        usleep(1000000);

        keyAvailable = true;
        permitAvailable = true;
        for(int a=0; a<4; ++a) {
            needsKey[a] = true;
            needsPermit[a] = true;
        }

        cout << endl << endl << endl;
    }

    return 0;
}
