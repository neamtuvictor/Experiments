#include <iostream>
#include <chrono>
#include <thread>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <nanomsg/nn.h>
//#include <nanomsg/pubsub.h>
#include <nanomsg/pair.h>

#include "BlockingConcurrentQueue.hpp"
#include "ConcurrentQueue.hpp"


#define SERVER "server"
#define CLIENT "client"

using namespace std;
using namespace std::chrono;
using namespace moodycamel;

BlockingConcurrentQueue<string> log_queue;

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



void fatal(const char *func)
{
        fprintf(stderr, "%s: %s\n", func, nn_strerror(nn_errno()));
}

char* date(void)
{
        time_t now = time(&now);
        struct tm *info = localtime(&now);
        char *text = asctime(info);
        text[strlen(text)-1] = '\0'; // remove '\n'
        return (text);
}


void read_message(packet* msg)
{
    string text = msg->str  + to_string(duration_cast<nanoseconds>(high_resolution_clock::now() - msg->tp).count());


    log_queue.enqueue(text);
    //log_queue.enqueue(msg);
}

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

int client(const char *url, const char *name)
{
    int sock;
    packet* buf = NULL;

    if ((sock = nn_socket(AF_SP, NN_PAIR)) < 0) {
            fatal("nn_socket");
    }

    //if (nn_setsockopt(sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) < 0) {
    /*if (nn_setsockopt(sock, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof (to)) < 0) {
            fatal("nn_setsockopt");
    }*/



    if (nn_connect(sock, url) < 0) {
            fatal("nn_connect");
    }

    while(1)
    {

        int bytes = nn_recv(sock, &buf, NN_MSG, NN_DONTWAIT);
        /*if (bytes < 0) {
           fatal("nn_recv");
        }*/

        if (bytes > 0)
        read_message(buf);

        //nn_freemsg(buf);
    }
}



int main(const int argc, const char **argv)
{
    map_thread_to_cpu(2);

    thread log = thread(&logging);
    log.detach();

    client ("ipc:///tmp/pubsub.ipc", "consumer");

    return 1;
}
