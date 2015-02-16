#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "My402List.h"

//global variable
int lambda = 1;		//packet arriving rate (lambda packets per second)
double mu = 0.35;	//server can serve mu packets per second
double r = 1.5;		//r tokens arrive at the bucket per second
int B = 10;			//token bucket depth
int P = 3;			//default token required
int num = 20;		//total number of packets to arrive

int n = 4;

pthread_mutex_t mutex;

My402List q1, q2;

int tokenBucket;

pthread_t tdt;


typedef struct packet {
	int packetId;
	int serviceTime;
	int tokenNeeded;
	int arrivalTime;
	int departureTime;
	int responseTime;
	int waitingTime;
}packet;

void setParameter(int argc, char **argv) {
	int c;
	while((c = getopt(argc, argv, "lambda:mu:r:P:n:t:")) != -1) {
		switch(c) {

		}
	} 
}

void* packetArrival(void *arg) {
	
	return (void*)0;
}

void *tokenDeposit(void *arg) {
	int i = 0;
	while(1) {
		pthread_mutex_lock(&mutex);
		tokenBucket++;
		i++;
		tokenBucket = min(tokenBucket, B);
		//usleep(10000);			//sleep for 1s
		usleep(1000000/r);
		pthread_mutex_unlock(&mutex);
		printf("token t%d arrives, token bucket now has %d token\n", i, tokenBucket);
	}
	return (void*)0;
}

void *server(void *arg) {
	return (void*)0;
}

void init() {
	//initialize queue1 and queue2
	memset(&q1, 0, sizeof(My402List));
	memset(&q2, 0, sizeof(My402List));
	(void)My402ListInit(&q1);
	(void)My402ListInit(&q2);
	//initialize tokenBucket
	tokenBucket = 0;
	//mutex
	pthread_mutex_init(&mutex, NULL);

}

void createThread() {
	pthread_create(&tdt, NULL, tokenDeposit, (void*)0);
}

int main(int argc, char **argv) {
	setParameter(argc, argv);
	init();
	createThread();
	while(1) {}
	return 0;
}
