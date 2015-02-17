#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "my402list.h"

//global variable
int lambda = 1;		//packet arriving rate (lambda packets per second)
double mu = 0.35;	//server can serve mu packets per second
double r = 1.5;		//r tokens arrive at the bucket per second
int B = 10;			//token bucket depth
int P = 3;			//default token required
int num = 20;		//total number of packets to arrive

int n = 4;

pthread_mutex_t mutex;
pthread_cond_t q2NotEmpty;

My402List q1, q2;

int tokenBucket;

pthread_t tdt, pat, s1, s2;



typedef struct packet {
	int packetId;
	double serviceTime;
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
	ret->serviceTime = (double)(1/mu*1000);
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
		printf("p%d enters Q2\n", firstPacket->packetId);
		if(!My402ListEmpty(&q2)) {
			pthread_cond_broadcast(&q2NotEmpty);
		}
	}
}

void* packetArrival(void *arg) {
	packet *p;
	int i = 0;
	while(1) {
		usleep(1000000/lambda);
		pthread_mutex_lock(&mutex);
		//if(i == n) {
		//	return (void*)0;
		//	pthread_mutex_unlock(&mutex);
		//}
		i++;
		p = createPacket(i);
		printf("p%d arrives, needs %d tokens\n", p->packetId, p->tokenRequired);
		My402ListAppend(&q1,p);
		printf("p%d enters Q1\n", i);
		pthread_mutex_unlock(&mutex);
	}
}

void *tokenDeposit(void *arg) {
	int i = 0;
	while(1) {
		usleep(1000000/r);
		pthread_mutex_lock(&mutex);
		tokenBucket++;
		i++;
		printf("token t%d arrives, token bucket now has %d token\n", i, tokenBucket);
		tokenBucket = min(tokenBucket, B);
		if(!My402ListEmpty(&q1)) {
			move();
		}
		pthread_mutex_unlock(&mutex);
	}
	return (void*)0;
}

packet *deleteFirstFromQ2() {
	My402ListElem *firstElem = My402ListFirst(&q2);
	packet *firstPacket = (packet*)firstElem->obj;
	My402ListUnlink(&q2, firstElem);
	printf("p%d leaves Q2, time in Q2 = ms\n", firstPacket->packetId);
	return firstPacket;
}

void *server1(void *arg) {
	packet *p;
	while(1) {
		pthread_mutex_lock(&mutex);
		while(My402ListEmpty(&q2)) {
			pthread_cond_wait(&q2NotEmpty, &mutex);
		}
		p = deleteFirstFromQ2();
		printf("p%d begins service at S1, requesting %lfms of service\n", p->packetId, p->serviceTime);
		pthread_mutex_unlock(&mutex);
		usleep(1000000/mu);
		pthread_mutex_lock(&mutex);
		printf("p%d departs from S1, service time = ms, time in system = ms\n", p->packetId);
		
		pthread_mutex_unlock(&mutex);
	}
	return (void*)0;
}

//server2 copy server1 code

void init() {
	//initialize queue1 and queue2
	memset(&q1, 0, sizeof(My402List));
	memset(&q2, 0, sizeof(My402List));
	My402ListInit(&q1);
	My402ListInit(&q2);
	//initialize tokenBucket
	tokenBucket = 0;
	//mutex
	pthread_mutex_init(&mutex, NULL);
	//condition varibale
	pthread_cond_init(&q2NotEmpty, NULL);
}

void printParamter() {
	printf("emulation begins\n");
}

void createThread() {
	pthread_create(&tdt, NULL, tokenDeposit, (void*)0);
	pthread_create(&pat, NULL, packetArrival, (void*)0);
	pthread_create(&s1, NULL, server1, (void*)0);
	//pthread_create(&s2, NULL, server2, (void*)0);
}

int main(int argc, char **argv) {
	//setParameter(argc, argv);
	init();
	createThread();
	while(1) {}
	return 0;
}
