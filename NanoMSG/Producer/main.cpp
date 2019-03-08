#include <iostream>
#include <chrono>


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <nanomsg/nn.h>
//#include <nanomsg/pubsub.h>
#include <nanomsg/pair.h>

#define SERVER "server"
#define CLIENT "client"

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

int server(const char *url)
{
    packet* msg;
    int sock;
    int index = 0;



    //if ((sock = nn_socket(AF_SP, NN_PUB)) < 0) {
    if ((sock = nn_socket(AF_SP, NN_PAIR)) < 0) {
            fatal("nn_socket");
    }
    if (nn_bind(sock, url) < 0) {
            fatal("nn_bind");
    }

    msg = new packet();


    while(1)
    {
        string text = "message[" + to_string(index) + "]: ";
        msg->value = index;
        strcpy(msg->str, text.c_str());

        msg->tp = high_resolution_clock::now();

        int bytes = nn_send(sock, msg, sizeof(packet), 0);

        if (bytes < 0) {
            fatal("nn_send");
        }

        index++;
        //sleep(1);
        //usleep(1);
    }
}



int main(const int argc, const char **argv)
{
    map_thread_to_cpu(1);
    server("ipc:///tmp/pubsub.ipc");

    return 1;
}
