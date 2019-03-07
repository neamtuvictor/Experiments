#include "ConcurrentQueue.hpp"
#include "BlockingConcurrentQueue.hpp"


#include <unistd.h>
#include <sched.h>
#include <pthread.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace std;
using namespace std::chrono;
using namespace moodycamel;


high_resolution_clock::time_point start;

atomic_int index(0);
BlockingConcurrentQueue<string> log_queue;


BlockingConcurrentQueue<high_resolution_clock::time_point> blocking_queue;
ConcurrentQueue<high_resolution_clock::time_point> nonblocking_queue;

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

void produce()
{
    map_thread_to_cpu(1);
    while(1)
    {
        start = high_resolution_clock::now();
        //blocking_queue.enqueue(high_resolution_clock::now());

        index++;
        usleep(1000000);
    }
}

void consume1()
{
    int my_index;
    string msg;

    map_thread_to_cpu(2);
    while(1)
    {
        //blocking_queue.wait_dequeue(start);
        if (my_index != index)
        {

            msg = "consumer1: " + to_string(duration_cast<nanoseconds>(high_resolution_clock::now() - start).count());
            log_queue.enqueue(msg);

            my_index = index;
        }

    }
}

void consume2()
{
    int my_index;
    string msg;

    map_thread_to_cpu(2);
    while(1)
    {
        //blocking_queue.wait_dequeue(start);
        if (my_index != index)
        {
            msg = "consumer2: " + to_string(duration_cast<nanoseconds>(high_resolution_clock::now() - start).count());

            log_queue.enqueue(msg);
            my_index = index;
        }
    }
}

void logging()
{
    string msg;
    map_thread_to_cpu(3);

    while(1)
    {
        log_queue.wait_dequeue(msg);
        cout << msg << endl;
    }
}

int main(int argc, char *argv[])
{
    thread p,c1,c2,log;

    p = thread(&produce);
    c1 = thread(&consume1);
    c2 = thread(&consume2);

    log = thread(&logging);

    p.join();

    return 0;
}
