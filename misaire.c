#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#define MAX 1000
typedef struct queue {
	int head;
	int tail;
	int key[MAX];
	int length;
} queue;
queue q;
void push(queue q, int x)
{
	q.key[q.tail] = x;
	if (q.tail == q.length) {
		q.tail = 1;
	}
	else {
		q.tail++;
	}
}
int pop(queue q)
{
	int x;
	x = q.key[q.head];
	if (q.head == q.length) {
		q.head = 1;
	}
	else {
		q.head++;
	}
	return x;
}

int count(queue q)
{
	return q.length;
}


void * check_deadlock(void*arg);
void active_Car();
void * car_NorthtoSouth(void *arg);
void * car_SouthtoNorth(void *arg);
void * car_EasttoWest(void *arg);
void * car_WesttoEast(void *arg);

pthread_cond_t deadlock;
pthread_cond_t deadlock_solve;
pthread_cond_t firstWest;
pthread_cond_t firstEast;
pthread_cond_t firstNorth;
pthread_cond_t firstSouth;
pthread_cond_t GoWest;
pthread_cond_t GoEast;
pthread_cond_t GoNorth;
pthread_cond_t GoSouth;

pthread_mutex_t wait_West;
pthread_mutex_t wait_East;
pthread_mutex_t wait_North;
pthread_mutex_t wait_South;
pthread_mutex_t mutex_car;
pthread_mutex_t mutex_dir;
pthread_mutex_t mutex_deadlock;
pthread_t thread_pool[MAX];

bool is_deadlock = false;
typedef enum { West, North, South, East } diru;

diru dir;

int Cars_id[MAX];
queue car_North;
queue car_South;
queue car_East;
queue car_West;

int current_North_id;
int current_South_id;
int current_East_id;
int current_West_id;
int total_car = 0;
int West_for_resource1;
int East_for_resource1;
int North_for_resource1;
int South_for_resource1;


int resource = 4;
int car_has_dealed = 0;

void* check_deadlock(void*arg) {
	usleep(500);
	active_Car();
	while (car_has_dealed != total_car) {
		pthread_mutex_lock(&mutex_deadlock);
		pthread_cond_wait(&deadlock, &mutex_deadlock);
		printf("DEADLOCK： car jam,dir is %d", dir);
		is_deadlock = true;
	}
	switch (dir) {
	case North: {
		printf("let east pass\n");
		pthread_cond_signal(&GoEast);
		break;
	}
	case South: {
		printf("let west pass\n");
		pthread_cond_signal(&GoWest);
		break;
	}
	case East: {
		printf("let south pass\n");
		pthread_cond_signal(&GoSouth);
		break;
	}
	case West: {
		printf("let North pass\n");
		pthread_cond_signal(&GoNorth);
		break;
	}
	}
	pthread_mutex_unlock(&mutex_deadlock);
}
void active_Car() {
	is_deadlock = false;
	North_for_resource1 = 0;
	South_for_resource1 = 0;
	East_for_resource1 = 0;
	West_for_resource1 = 0;
	if (count(car_North) > 0) {
		current_North_id = pop(car_North);
		pthread_cond_signal(&firstNorth);
	}
	if (count(car_South) > 0) {
		current_South_id = pop(car_South);
		pthread_cond_signal(&firstSouth);
	}
	if (count(car_East) > 0) {
		current_East_id = pop(car_East);
		pthread_cond_signal(&firstEast);
	}
	if (count(car_West) > 0) {
		current_West_id = pop(car_West);
		pthread_cond_signal(&firstWest);
	}
}
void * car_NorthtoSouth(void *arg)
{
	int remain;
	pthread_mutex_lock(&wait_North);
	pthread_cond_wait(&firstNorth, &wait_North);
	pthread_mutex_unlock(&wait_North);
	pthread_mutex_lock(&mutex_dir);
	dir = North;
	North_for_resource1 = 1;
	remain = --resource;
	int tmp = 0;
	for (int i = 0; i < total_car; i++)
	{
		if (tmp < number[i]) tmp = number[i];
		number[current_North_id] = tmp + 1;
	}
	pthread_mutex_unlock(&mutex_dir);
	printf("car %d from N arrives\n", current_North_id);
	for (int i = 0; i < total_car; i++)
	{
		while (number[i] != 0 && number[current_North_id] > number[i]);
	}
	usleep(1000);
	pthread_mutex_lock(&mutex_dir);
	number[current_North_id] = 0;
	if (remain == 0 && dir == North) {
		pthread_cond_signal(&deadlock);
		pthread_cond_wait(&deadlock_solve, &mutex_dir);
		printf("car %d from N left\n", current_North_id);
		car_has_dealed++;
		resource++;
		North_for_resource1 = 0;
		active_Car();
		pthread_mutex_unlock(&mutex_dir);
		return NULL;
	}
	else if (West_for_resource1) {
		if (is_deadlock) {
			printf("car %d from N left\n", current_North_id);
			car_has_dealed++;
			resource++;
			if (dir == East) {
				pthread_cond_signal(&deadlock_solve);
			}
			else pthread_cond_signal(&GoEast);
			North_for_resource1 = 1;
			pthread_mutex_unlock(&mutex_dir);
			return NULL;
		}
		else {
			North_for_resource1 = 0;
			printf("car %d from North left\n", current_North_id);
			car_has_dealed++;
			resource++;
			if (East_for_resource1) pthread_cond_signal(&GoEast);
			if (count(car_North) > 0) {
				current_North_id = pop(car_North);
				pthread_cond_signal(&firstNorth);
			}
			pthread_mutex_unlock(&mutex_dir);
			return NULL;
		}
	}
	else {
		printf("car %d from N left\n", current_North_id);
		car_has_dealed++;
		resource++;
		North_for_resource1 = 0;
		if (East_for_resource1) pthread_cond_signal(&GoEast);
		if (count(car_North) > 0) {
			current_North_id = pop(car_North);
			pthread_cond_signal(&firstNorth);
		}
	}
	pthread_mutex_unlock(&mutex_dir);
	return NULL;
}
void * car_SouthtoNorth(void *arg)
{
	int remain;
	pthread_mutex_lock(&wait_South);
	pthread_cond_wait(&firstSouth, &wait_South);
	pthread_mutex_unlock(&wait_South);
	pthread_mutex_lock(&mutex_dir);
	dir = South;
	South_for_resource1 = 1;
		remain = --resource;
	int tmp = 0;
	for (int i = 0; i < total_car; i++)
	{
		if (tmp < number[i]) tmp = number[i];
		number[current_South_id] = tmp + 1;
	}
	pthread_mutex_unlock(&mutex_dir);
	printf("car %d from S arrives\n", current_South_id);
	for (int i = 0; i < total_car; i++)
	{
		while (number[i] != 0 && number[current_South_id] > number[i]);
	}
	usleep(1000);
	pthread_mutex_lock(&mutex_dir);
	number[current_South_id] = 0;
	if (remain == 0 && dir == South) {
		pthread_cond_signal(&deadlock);
		pthread_cond_wait(&deadlock_solve, &mutex_dir);
		printf("car %d from S left\n", current_South_id);
		car_has_dealed++;
		resource++;
		South_for_resource1 = 0;
		active_Car();
		pthread_mutex_unlock(&mutex_dir);
		return NULL;
	}
	else if (East_for_resource1) {
		if (is_deadlock) {
			printf("car %d from S left\n", current_South_id);
			car_has_dealed++;
			resource++;
			if (dir == West) {
				pthread_cond_signal(&deadlock_solve);
			}
			else pthread_cond_signal(&GoWest);
			South_for_resource1 = 1;
			pthread_mutex_unlock(&mutex_dir);
			return NULL;
		}
		else {
			South_for_resource1 = 0;
			printf("car %d from S left\n", current_South_id);
			car_has_dealed++;
			resource++;
			if (West_for_resource1) pthread_cond_signal(&GoWest);
			if (count(car_South) > 0) {
				current_South_id = pop(car_South);
				pthread_cond_signal(&firstSouth);
			}
			pthread_mutex_unlock(&mutex_dir);
			return NULL;
		}
	}
	else {
		printf("car %d from S left\n", current_South_id);
		car_has_dealed++;
		resource++;
		South_for_resource1 = 0;
		if (West_for_resource1) pthread_cond_signal(&GoWest);
		if (count(car_South) > 0) {
			current_South_id = pop(car_South);
			pthread_cond_signal(&firstSouth);
		}
	}
	pthread_mutex_unlock(&mutex_dir);
	return NULL;
}
void * car_EasttoWest(void *arg)
{
	int remain;
	pthread_mutex_lock(&wait_East);
	pthread_cond_wait(&firstEast, &wait_East);
	pthread_mutex_unlock(&wait_East);
	pthread_mutex_lock(&mutex_dir);
	dir = East;
	East_for_resource1 = 1;
		remain = --resource;
	int tmp = 0;
	for (int i = 0; i < total_car; i++)
	{
		if (tmp < number[i]) tmp = number[i];
		number[current_East_id] = tmp + 1;
	}
	pthread_mutex_unlock(&mutex_dir);
	printf("car %d from E arrives\n", current_East_id);
	for (int i = 0; i < total_car; i++)
	{
		while (number[i] != 0 && number[current_East_id] > number[i]);
	}
	usleep(1000);
	pthread_mutex_lock(&mutex_dir);
	number[current_East_id] = 0;
	if (remain == 0 && dir == East) {
		pthread_cond_signal(&deadlock);
		pthread_cond_wait(&deadlock_solve, &mutex_dir);
		printf("car %d from E left\n", current_East_id);
		car_has_dealed++;
		resource++;
		East_for_resource1 = 0;
		active_Car();
		pthread_mutex_unlock(&mutex_dir);
		return NULL;
	}
	//
	else if (North_for_resource1) {
		if (is_deadlock) {
			printf("car %d from E left\n", current_East_id);
			car_has_dealed++;
			resource++;
			if (dir == South) {
				pthread_cond_signal(&deadlock_solve);
			}
			else pthread_cond_signal(&GoSouth);
			East_for_resource1 = 1;
			pthread_mutex_unlock(&mutex_dir);
			return NULL;
		}
		else {
			East_for_resource1 = 0;
			printf("car %d from E left\n", current_East_id);
			car_has_dealed++;
			resource++;
			if (South_for_resource1) pthread_cond_signal(&GoSouth);
			if (count(car_East) > 0) {
				current_East_id = pop(car_East);
				pthread_cond_signal(&firstEast);
			}
			pthread_mutex_unlock(&mutex_dir);
			return NULL;
		}
	}
	else {
		printf("car %d from E left\n", current_East_id);
		car_has_dealed++;
		resource++;
		East_for_resource1 = 0;
		if (East_for_resource1) pthread_cond_signal(&GoSouth);
		if (count(car_East) > 0) {
			current_East_id = pop(car_East);
			pthread_cond_signal(&firstEast);
		}
	}
	pthread_mutex_unlock(&mutex_dir);
	return NULL;
}
void * car_WesttoEast(void*arg)
{
	int remain;
	pthread_mutex_lock(&wait_West);
	pthread_cond_wait(&firstWest, &wait_West);
	pthread_mutex_unlock(&wait_West);
	pthread_mutex_lock(&mutex_dir);
	dir = West;
	West_for_resource1 = 1;
		remain = --resource;
	int tmp = 0;
	for (int i = 0; i < total_car; i++)
	{
		if (tmp < number[i]) tmp = number[i];
		number[current_West_id] = tmp + 1;
	}
	pthread_mutex_unlock(&mutex_dir);
	printf("car %d from W arrives\n", current_West_id);
	for (int i = 0; i < total_car; i++)
	{
		while (number[i] != 0 && number[current_West_id] > number[i]);
	}
	usleep(1000);
	pthread_mutex_lock(&mutex_dir);
	number[current_West_id] = 0;
	if (remain == 0 && dir == West) {
		pthread_cond_signal(&deadlock);
		pthread_cond_wait(&deadlock_solve, &mutex_dir);
		printf("car %d from W left\n", current_West_id);
		car_has_dealed++;
		resource++;
		West_for_resource1 = 0;
		active_Car();
		pthread_mutex_unlock(&mutex_dir);
		return NULL;
	}
	//
	else if (South_for_resource1) {
		if (is_deadlock) {
			printf("car %d from W left\n", current_West_id);
			car_has_dealed++;
			resource++;
			if (dir == North) {
				pthread_cond_signal(&deadlock_solve);
			}
			else pthread_cond_signal(&GoNorth);
			West_for_resource1 = 1;
			pthread_mutex_unlock(&mutex_dir);
			return NULL;
		}
		else {
			West_for_resource1 = 0;
			printf("car %d from W left\n", current_West_id);
			car_has_dealed++;
			resource++;
			if (North_for_resource1) pthread_cond_signal(&GoNorth);
			if (count(car_West) > 0) {
				current_West_id = pop(car_West);
				pthread_cond_signal(&firstWest);
			}
			pthread_mutex_unlock(&mutex_dir);
			return NULL;
		}
	}
	else {
		printf("car %d from W left\n", current_West_id);
		car_has_dealed++;
		resource++;
		West_for_resource1 = 0;
		if (West_for_resource1) pthread_cond_signal(&GoNorth);
		if (count(car_West) > 0) {
			current_West_id = pop(car_West);
			pthread_cond_signal(&firstWest);
		}
	}
	pthread_mutex_unlock(&mutex_dir);
	return NULL;
}

int main() {
	pthread_cond_init(&deadlock, NULL);
	pthread_cond_init(&deadlock_solve, NULL);
	pthread_cond_init(&firstWest, NULL);
	pthread_cond_init(&firstEast, NULL);
	pthread_cond_init(&firstNorth, NULL);
	pthread_cond_init(&firstSouth, NULL);
	pthread_cond_init(&GoWest, NULL);
	pthread_cond_init(&GoEast, NULL);
	pthread_cond_init(&GoNorth, NULL);
	pthread_cond_init(&GoSouth, NULL);
	pthread_mutex_init(&wait_West, NULL);
	pthread_mutex_init(&wait_East, NULL);
	pthread_mutex_init(&wait_North, NULL);
	pthread_mutex_init(&wait_South, NULL);
	pthread_mutex_init(&mutex_car, NULL);
	int total_car = 0;
	char ch[MAX];
	scanf("%s", ch);
	total_car = strlen(ch);
	printf("total: %d\n", total_car);
		int i = 0;
		for (int id = 0; id < total_car; id++)
		{
			switch (ch[id]) {
				printf("%c", ch[id]);
			case 'n': {
				Cars_id[i++] = id;
				push(car_North, id);
				pthread_create(&thread_pool[id], NULL, car_NorthtoSouth, NULL);
				break;
			}
			case 's': {
				Cars_id[i++] = id;
				push(car_South, id);
				pthread_create(&thread_pool[id], NULL, car_SouthtoNorth, NULL);
				break;
			}
			case 'e': {
				Cars_id[i++] = id;
				push(car_East, id);
				pthread_create(&thread_pool[id], NULL, car_EasttoWest, NULL);
				break;
			}
			case 'w': {
				Cars_id[i++] = id;
				push(car_West, id);
				pthread_create(&thread_pool[id], NULL, car_WesttoEast, NULL);
				break;
			}
			default: break;
			}
		}
	pthread_create(&check, NULL, check_deadlock, NULL);
	for (int i = 0; i < total_car; i++)
	{
		pthread_join(thread_pool[i], NULL);
	}
	pthread_cond_destroy(&deadlock);
	pthread_cond_destroy(&deadlock_solve);
	pthread_cond_destroy(&firstWest);
	pthread_cond_destroy(&firstEast);
	pthread_cond_destroy(&firstNorth);
	pthread_cond_destroy(&firstSouth);
	pthread_cond_destroy(&GoWest);
	pthread_cond_destroy(&GoEast);
	pthread_cond_destroy(&GoNorth);
	pthread_cond_destroy(&GoSouth);
	pthread_mutex_destroy(&wait_West);
	pthread_mutex_destroy(&wait_East);
	pthread_mutex_destroy(&wait_North);
	pthread_mutex_destroy(&wait_South);
	pthread_mutex_destroy(&mutex_car);
}
