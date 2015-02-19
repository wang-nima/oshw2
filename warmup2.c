#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "my402list.h"

//global variable
//default value
double lambda = 1;		//packet arriving rate (lambda packets per second)
double mu = 0.35;	//server can serve mu packets per second
double r = 1.5;		//r tokens arrive at the bucket per second
unsigned int B = 10;			//token bucket depth
unsigned int P = 3;			//default token required
unsigned int num = 20;		//total number of packets to arrive

pthread_mutex_t mutex;
pthread_cond_t q2NotEmpty;

My402List q1, q2;

int tokenBucket;

pthread_t tdt, pat, s1, s2, pat_td;

struct timeval start;

FILE *fp;

char filePath[20];

int traceDrivenMode = 0;

unsigned long lastPacketArrivalTimeInMicroSecond = 0;

int packetArrivedQ1 = 0;

typedef struct packet {
	unsigned int packetId;
	unsigned int serviceTime;			//in micro second
	unsigned int tokenRequired;

	unsigned long arrivalQ1;			//time stamps
	unsigned long arrivalQ2;
	unsigned long beginService;
	unsigned long departureTime;

	unsigned int interArrivalTime;		//in micro second
}packet;



void setParameter(int argc, char **argv) {
	int i;
	for(i = 1; i < argc; i += 2) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i] + 1, "lambda") == 0) {
				lambda = atof(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "mu") == 0) {
				mu = atof(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "r") == 0) {
				r = atof(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "B") == 0) {
				B = atoi(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "P") == 0) {
				P = atoi(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "n") == 0) {
				num = atoi(argv[i+1]);
			} else if(strcmp(argv[i] + 1, "t") == 0) {
				//read from file
				fp = fopen(argv[i+1],"r");
				strcpy(filePath, argv[i+1]);
				traceDrivenMode = 1;
			} else {
				printf("invalid option\n");
			}
		} else {
			printf("no parameter entered for the option\n");
			return;
		}
	}
}

unsigned long currentTimeToMicroSecond() {
	unsigned long ret;
	struct timeval t;
	gettimeofday(&t, NULL);
	ret = (t.tv_sec * 1000000 + t.tv_usec) - 
		  (start.tv_sec * 1000000 + start.tv_usec);
	return ret;
}

void printTime() {
	long time = currentTimeToMicroSecond();
	long ms, us;
	ms = time / 1000;
	us = time % 1000;
	printf("%08ld.%03ldms :", ms, us);
}

packet* createPacket(int id) {
	packet *ret = (packet*)malloc(sizeof(packet));
	ret->packetId = id;
	ret->serviceTime = 1000000 / mu;
	ret->tokenRequired = P;
	ret->arrivalQ1 = currentTimeToMicroSecond();
	ret->interArrivalTime = ret->arrivalQ1 - lastPacketArrivalTimeInMicroSecond;
	lastPacketArrivalTimeInMicroSecond = ret->arrivalQ1;
	return ret;
}

packet* createPacketByTraceFile(int id) {
	packet *ret = (packet*)malloc(sizeof(packet));
	ret->packetId = id;
	fscanf(fp, "%d", &(ret->interArrivalTime));
	fscanf(fp, "%d", &(ret->tokenRequired));
	fscanf(fp, "%d", &(ret->serviceTime));
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
		firstPacket->arrivalQ2 = currentTimeToMicroSecond();
		int interval = firstPacket->arrivalQ2 - 
					   firstPacket->arrivalQ1;
		int x = interval / 1000;
		int y = interval % 1000;
		printf("p%d leaves Q1, time in Q1 = %d.%dms, token bucket now has %d token\n",
				firstPacket->packetId, x, y, tokenBucket);
		printTime();
		printf("p%d enters Q2\n", firstPacket->packetId);
		if(!My402ListEmpty(&q2)) {
			pthread_cond_broadcast(&q2NotEmpty);
		}
	}
}

void sleepWithinTenSecond(unsigned long x) {
	if(x > 10000000) {
		usleep(10000000);
	} else {
		usleep(x);
	}
}

void* packetArrival(void *arg) {
	packet *p;
	int i = 0;
	while(1) {
		if(i == num) {
			return (void*)0;
		}
		sleepWithinTenSecond(1000000/lambda);
		pthread_mutex_lock(&mutex);
		i++;
		p = createPacket(i);
		printTime();
		printf("p%d arrives, needs %d tokens, inter-arrival time = %d.%dms\n",
				p->packetId, p->tokenRequired, 
				p->interArrivalTime/1000,
				p->interArrivalTime%1000);
		My402ListAppend(&q1,p);
		printTime();
		printf("p%d enters Q1\n", i);
		pthread_mutex_unlock(&mutex);
	}
}

void* packetArrivalTraceDriven(void *arg) {
	packet *p;
	int i = 0;
	while(1) {
		if(i == num) {
			return (void*)0;
		}
		i++;
		p = createPacketByTraceFile(i);
		usleep(p->interArrivalTime * 1000);
		p->arrivalQ1 = currentTimeToMicroSecond();
		p->interArrivalTime = p->arrivalQ1 - lastPacketArrivalTimeInMicroSecond;
		lastPacketArrivalTimeInMicroSecond = p->arrivalQ1;
		if(p->tokenRequired > B) {
			printTime();
			printf("p%d arrives, needs %d tokens, inter-arrival time = %d.%dms, dropped\n",
					i, p->tokenRequired, p->interArrivalTime / 1000, p->interArrivalTime % 1000);
			continue;
		}

		pthread_mutex_lock(&mutex);
		printTime();
		printf("p%d arrives, needs %d tokens, inter-arrival time = %d.%dms\n",
				p->packetId, p->tokenRequired, p->interArrivalTime/1000, p->interArrivalTime%1000);
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
	firstPacket->beginService = currentTimeToMicroSecond();
	My402ListUnlink(&q2, firstElem);
	printTime();
	int interval = firstPacket->beginService - firstPacket->arrivalQ2;
	int x = interval / 1000;
	int y = interval % 1000;
	printf("p%d leaves Q2, time in Q2 = %d.%dms\n", firstPacket->packetId, x, y);
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
		printf("p%d begins service at S1, requesting %d.%dms of service\n",
				p->packetId, p->serviceTime / 1000, p->serviceTime % 1000);
		pthread_mutex_unlock(&mutex);
		sleepWithinTenSecond(1000000 / mu);
		pthread_mutex_lock(&mutex);
		printTime();
		p->departureTime = currentTimeToMicroSecond();
		int interval = p->departureTime- p->beginService;
		int x = interval / 1000;
		int y = interval % 1000;
		int intervalTotal = p->departureTime - p->arrivalQ1;
		int a = intervalTotal / 1000;
		int b = intervalTotal % 1000;
		printf("p%d departs from S1, service time = %d.%dms, time in system = %d.%dms\n",
				p->packetId, x, y, a ,b);
		pthread_mutex_unlock(&mutex);
	}
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
		printf("p%d begins service at S2, requesting %d.%dms of service\n",
				p->packetId, p->serviceTime / 1000, p->serviceTime % 1000);
		pthread_mutex_unlock(&mutex);
		sleepWithinTenSecond(1000000 / mu);
		pthread_mutex_lock(&mutex);
		printTime();
		p->departureTime = currentTimeToMicroSecond();
		int interval = p->departureTime- p->beginService;
		int x = interval / 1000;
		int y = interval % 1000;
		int intervalTotal = p->departureTime - p->arrivalQ1;
		int a = intervalTotal / 1000;
		int b = intervalTotal % 1000;
		printf("p%d departs from S2, service time = %d.%dms, time in system = %d.%dms\n",
				p->packetId, x, y, a ,b);
		pthread_mutex_unlock(&mutex);
	}
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

	if(traceDrivenMode) {
		fscanf(fp,"%d",&num);
	}
}


void printParamter() {
	printf("Emulation Parameters:\n");
	printf("    number to arrive = %d\n", num);
	if(!traceDrivenMode) {
		printf("    lambda = %lf\n", lambda);
		printf("    mu = %lf\n", mu);
	}
	printf("    r = %lf\n", r);
	printf("    B = %d\n", B);
	if(!traceDrivenMode) {
		printf("    P = %d\n", P);
	}
	if(traceDrivenMode) {
		printf("tsfile = %s\n", filePath);
	}
	printf("\n");
}


void createThread() {
	printTime();
	printf("emulation begins\n");
	if(!traceDrivenMode) {
		pthread_create(&pat, NULL, packetArrival, (void*)0);
	} else {
		pthread_create(&pat_td, NULL, packetArrivalTraceDriven, (void*)0);
	}
	pthread_create(&tdt, NULL, tokenDeposit, (void*)0);
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
