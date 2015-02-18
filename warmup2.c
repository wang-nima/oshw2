#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "my402list.h"

//global variable
//default value
int lambda = 1;		//packet arriving rate (lambda packets per second)
double mu = 0.35;	//server can serve mu packets per second
double r = 1.5;		//r tokens arrive at the bucket per second
int B = 10;			//token bucket depth
int P = 3;			//default token required
int num = 20;		//total number of packets to arrive


pthread_mutex_t mutex;
pthread_cond_t q2NotEmpty;

My402List q1, q2;

int tokenBucket;

pthread_t tdt, pat, s1, s2;

//clock_t start;
struct timeval start;


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
	int i;
	for(i = 1; i < argc; i += 2) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i] + 1, "lambda") == 0) {
				lambda = atoi(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "mu") == 0) {
				mu = atoi(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "r") == 0) {
				r = atoi(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "B") == 0) {
				B = atoi(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "P") == 0) {
				P = atoi(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "n") == 0) {
				num = atoi(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "t") == 0) {
				//do something to read from file
			} else {
				printf("invalid option\n");
			}
		} else {
			printf("no parameter entered for the option\n");
			return;
		}
	}
}

void printTime() {
	struct timeval t;
	//pthread_mutex_lock(&mutex);
	gettimeofday(&t, NULL);
	long time = (t.tv_sec * 1000000 + t.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
	long ms, us;
	ms = time / 1000;
	us = time % 1000;
	printf("%08ld.%03ldms :", ms, us);
	//pthread_mutex_unlock(&mutex);
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
		printTime();
		printf("p%d leaves Q1, time in Q1 = ms, token bucket now has %d token\n", firstPacket->packetId, tokenBucket);
		printTime();
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
		if(i == num) {
			pthread_mutex_unlock(&mutex);
			return (void*)0;
		}
		i++;
		p = createPacket(i);
		printTime();
		printf("p%d arrives, needs %d tokens\n", p->packetId, p->tokenRequired);
		My402ListAppend(&q1,p);
		printTime();
		printf("p%d enters Q1\n", i);
		pthread_mutex_unlock(&mutex);
	}
}

void *tokenDeposit(void *arg) {
	int i = 0;
	while(1) {
		usleep(1000000/r);
		pthread_mutex_lock(&mutex);
		i++;
		if(tokenBucket < B) {
			tokenBucket++;
			printTime();
			printf("token t%d arrives, token bucket now has %d token\n", i, tokenBucket);
		} else {
			printTime();
			printf("token t%d arrives, dropped\n", i);
		}
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
	printTime();
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
		printTime();
		printf("p%d begins service at S1, requesting %lfms of service\n", p->packetId, p->serviceTime);
		pthread_mutex_unlock(&mutex);
		usleep(1000000/mu);
		pthread_mutex_lock(&mutex);
		printTime();
		printf("p%d departs from S1, service time = ms, time in system = ms\n", p->packetId);
		pthread_mutex_unlock(&mutex);
	}
	return (void*)0;
}

//server2 copy server1 code
void *server2(void *arg) {
	packet *p;
	while(1) {
		pthread_mutex_lock(&mutex);
		while(My402ListEmpty(&q2)) {
			pthread_cond_wait(&q2NotEmpty, &mutex);
		}
		p = deleteFirstFromQ2();
		printTime();
		printf("p%d begins service at S2, requesting %lfms of service\n", p->packetId, p->serviceTime);
		pthread_mutex_unlock(&mutex);
		usleep(1000000/mu);
		pthread_mutex_lock(&mutex);
		printTime();
		printf("p%d departs from S2, service time = ms, time in system = ms\n", p->packetId);
		pthread_mutex_unlock(&mutex);
	}
	return (void*)0;
}


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
	printf("Emulation Parameters:\n");
	printf("    number to arrive = %d\n", num);
	printf("    lambda = %d\n", lambda);
	printf("    mu = %lf\n", mu);
	printf("    r = %lf\n", r);
	printf("    B = %d\n", B);
	printf("    P = %d\n", P);
	printf("\n");
}


void createThread() {
	printTime();
	printf("emulation begins\n");
	pthread_create(&tdt, NULL, tokenDeposit, (void*)0);
	pthread_create(&pat, NULL, packetArrival, (void*)0);
	pthread_create(&s1, NULL, server1, (void*)0);
	pthread_create(&s2, NULL, server2, (void*)0);
}

void setClock() {
	gettimeofday(&start, NULL);
}

int main(int argc, char **argv) {
	setParameter(argc, argv);
	init();
	printParamter();
	setClock();
	createThread();
	while(1) {}
	return 0;
}
