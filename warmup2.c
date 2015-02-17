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
pthread_cond_t emptyQ2;

My402List q1, q2;

int tokenBucket;

pthread_t tdt, pat;



typedef struct packet {
	int packetId;
	int serviceTime;
	int tokenRequired;
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

packet* createPacket(int id) {
	packet *ret = (packet*)malloc(sizeof(packet));
	ret->packetId = id;
	ret->serviceTime = 1000000/mu;
	ret->tokenRequired = P;
	return ret;
}

void move() {
	My402ListElem *firstElem = My402ListFirst(&q1);
	packet *firstPacket = (packet*)firstElem->obj;
	if(firstPacket->tokenRequired <= tokenBucket) {
		tokenBucket -= firstPacket->tokenRequired;
		My402ListAppend(&q2, firstPacket);
		My402ListUnlink(&q1, firstElem);
		printf("p%d leaves Q1, time in Q1 = ms, token bucket now has %d token\n", firstPacket->packetId, tokenBucket);
		if(My402ListEmpty(&q2)) {
			pthread_cond_signal(&emptyQ2);
		}
	}
}


void* packetArrival(void *arg) {
	packet *p;
	int i = 0;
	while(1) {
		pthread_mutex_lock(&mutex);
		usleep(1000000/lambda);
		i++;
		p = createPacket(i);
		printf("p%d arrives, needs %d tokens\n", p->packetId, p->tokenRequired);
		My402ListAppend(&q1,p);
		printf("p%d enters Q1\n", i);
		pthread_mutex_unlock(&mutex);
	}
	return (void*)0;
}

void *tokenDeposit(void *arg) {
	int i = 0;
	while(1) {
		pthread_mutex_lock(&mutex);
		usleep(1000000/r);
		tokenBucket++;
		i++;
		tokenBucket = min(tokenBucket, B);
		if(!My402ListEmpty(&q1)) {
			move();
		}
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
	//condition varibale
	pthread_cond_init(&emptyQ2, NULL);
	//q1 and q2
	My402ListInit(&q1);
	My402ListInit(&q2);
}

void createThread() {
	pthread_create(&tdt, NULL, tokenDeposit, (void*)0);
	pthread_create(&pat, NULL, packetArrival, (void*)0);
}

int main(int argc, char **argv) {
	setParameter(argc, argv);
	init();
	createThread();
	while(1) {}
	return 0;
}
