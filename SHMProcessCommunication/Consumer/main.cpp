#include "BlockingConcurrentQueue.hpp"
#include "ConcurrentQueue.hpp"


#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>

#include <thread>
#include <iostream>
#include <string>
#include <sched.h>
#include <mutex>

#include <chrono>


//#define SHM_MAX 1024
#define SHM_MAX 1048576
#define BUFFER_SIZE 1000

using namespace std;
using namespace std::chrono;
using namespace moodycamel;

struct packet
{
    high_resolution_clock::time_point tp; //8bytes
    char str[20];
    int value;
};


BlockingConcurrentQueue<string> log_queue;
//ConcurrentQueue<packet> log_queue;

/*static inline void spinlock(volatile int *lock)
{
    while(!__sync_bool_compare_and_swap(lock, 0, 1))
    {
        sched_yield();

    }
}

static inline void spinunlock(volatile int *lock)
{
    *lock = 0;
}*/

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

void read_message(const packet& msg)
{
    string text = msg.str  + to_string(duration_cast<nanoseconds>(high_resolution_clock::now() - msg.tp).count());


    log_queue.enqueue(text);
    //log_queue.enqueue(msg);
}

/*void logging() //logging packets directly
{
    map_thread_to_cpu(0);

    packet msg;
    int latency;
    int min = 0;
    int max = 0;

    string text;

    while(1)
    {
        if (log_queue.try_dequeue(msg)) {


            latency = duration_cast<nanoseconds>(high_resolution_clock::now() - msg.tp).count();

            if ((latency < min) || (min == 0)) min = latency;
            if ((latency > max) || (max == 0)) max = latency;

            text = msg.str  + to_string(latency) + " min: " + to_string(min) + " max: " + to_string(max);

            cout << text << endl;
        }
        else {
            sched_yield();
        }
    }
}*/

void logging() //simple logging only strings
{
    string msg;
    map_thread_to_cpu(0);

    while(1)
    {
        log_queue.wait_dequeue(msg);
        cout << msg << endl;
    }
}

int main()
{
    map_thread_to_cpu(2);

    thread log = thread(&logging);
    log.detach();

    //key_t key = ftok("shmfile",65); //generate unique key
    int shmid_buffer = shmget(1000, SHM_MAX, 0666|IPC_CREAT);

    if(shmid_buffer<0){
        cout << "shmid_buffer error allocating memory" << endl;
        return -1;
    }
    int shmid_last = shmget(201, 4, 0666|IPC_CREAT);
    if(shmid_last<0){
        cout << "shmid_last error allocating memory" << endl;
        return -1;
    }

    packet* buffer = (packet*) shmat(shmid_buffer, NULL, 0); //attach buf to shared memory
    volatile int* last = (int*)shmat(shmid_last, NULL, 0);

    int local_index = *last;


    while(1)
    {
        if (local_index != *last)
        {
            do
            {

                if (local_index + 1 >= BUFFER_SIZE){
                    local_index = 0;
                }
                else{
                    local_index++;
                }

                read_message(buffer[local_index]);

            }while(local_index != *last);
        }
    }//while


    shmdt(buffer); //detach buf from shared memory
    //shmctl(shmid_buffer, IPC_RMID, NULL); //destroy shared memory
}
