#include <iostream>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <chrono>


#define SHM_MAX 1048576
#define BUFFER_SIZE 1000

using namespace std;
using namespace std::chrono;

struct packet
{
    high_resolution_clock::time_point tp;
    char str[20];
    int value;
};

int map_thread_to_cpu(int core_id)
{

   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
   if (core_id < 0 || core_id >= num_cores) {
        return -1;
   }

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);
   pthread_t current_thread = pthread_self();


   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

int main()
{

    string message;

    map_thread_to_cpu(1);
    cout << " packet "<< sizeof(packet) << endl;

   //key_t key = ftok("shmfile",65); //generate unique key

   int shmid_buffer = shmget(1000, SHM_MAX, 0666|IPC_CREAT);
   if(shmid_buffer<0) {
       cout << "error " << endl;
       return -1;
   }

   int shmid_last = shmget(201, 4, 0666|IPC_CREAT);
   if(shmid_last < 0) {
       cout << "error " << endl;
       return -1;
   }

   packet* buffer = (packet*)shmat(shmid_buffer, (void*)0, 0); //attach buf to shared memory
   int *last = (int*)shmat(shmid_last, (void*)0,0);

   while(1)
   {
       for (int i = 0; i < BUFFER_SIZE; ++i)
       {

            message = "msg[" + to_string(i) + "]: ";
            strcpy(buffer[i].str, message.c_str());
            buffer[i].tp = high_resolution_clock::now();

            *last = i;


            sleep(60);
            //usleep(1);
       }
   }

   shmdt(buffer); //detach buf from shared memory
}
