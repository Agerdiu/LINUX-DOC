#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX 100

typedef enum { west, north, south, east, unknown } dir_t;

typedef struct Car {
    int id;
    dir_t dir;
    pthread_t thread;
    int state;
    int indexInQueue;
} Car;

typedef struct Queue {
    int front;
    int rear;
    int count;
    dir_t dir;
    Car cars[MAX];
} queue, *pQueue;

// queues of cars from four direction
queue car_from_north_queue;
queue car_from_west_queue;
queue car_from_south_queue;
queue car_from_east_queue;

// threads of all cars
pthread_t car_threads[MAX];
// total number of cars from all directions
int total = 0;

// mutex for a,b,c,d
pthread_mutex_t mutex_a;
pthread_mutex_t mutex_b;
pthread_mutex_t mutex_c;
pthread_mutex_t mutex_d;
pthread_mutex_t mutex_deadlock;

// mutex for cars waiting in queue
pthread_mutex_t mutex_north;
pthread_mutex_t mutex_west;
pthread_mutex_t mutex_south;
pthread_mutex_t mutex_east;

// mutex wait right
pthread_mutex_t mutex_north_wait_right;
pthread_mutex_t mutex_west_wait_right;
pthread_mutex_t mutex_south_wait_right;
pthread_mutex_t mutex_east_wait_right;

// mutex wait left
pthread_mutex_t mutex_north_wait_left;
pthread_mutex_t mutex_west_wait_left;
pthread_mutex_t mutex_south_wait_left;
pthread_mutex_t mutex_east_wait_left;

// cond for wake up cars waiting in queue
pthread_cond_t cond_north_broadcast = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_west_broadcast = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_south_broadcast = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_east_broadcast = PTHREAD_COND_INITIALIZER;

// cond of go ahead
pthread_cond_t cond_north = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_west = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_south = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_east = PTHREAD_COND_INITIALIZER;

// cond of pass
pthread_cond_t cond_north_pass = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_west_pass = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_south_pass = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_east_pass = PTHREAD_COND_INITIALIZER;

// Is there a car from the direction
sem_t isNorth;
sem_t isWest;
sem_t isSouth;
sem_t isEast;

int min(int a, int b, int c, int d) {
    if(a<=b && a<=c && a<=d) {
        return a;
    }
    else if(b<=a && b<=c && b<=d) {
        return b;
    }
    else if(c<=a && c<=b && c<=d) {
        return c;
    }
    else if(d<=a && d<=c && d<=d) {
        return d;
    }
    else {
        return -1;
    }
}

void *car_from_north_thread_fun(int *_indexInQueue) {
    // get the information of the car from the car queue
    int indexInQueue = *_indexInQueue;
    int id = car_from_north_queue.cars[indexInQueue].id;
    dir_t dir = car_from_north_queue.cars[indexInQueue].dir;

    // variables to get the value of the semaphores
    int isRight;
    int isLeft;

    pthread_mutex_lock(&mutex_north);
    // waiting in the queue if the car is not the first car in the queue
    while(car_from_north_queue.front != indexInQueue) {
      pthread_cond_wait(&cond_north_broadcast, &mutex_north);
    }

    printf("car %d from North arrives at cross.\n", id);

    // state 0: the car arrives but doesn't occupy any resource
    car_from_north_queue.cars[indexInQueue].state = 0;
    
    sem_wait(&isNorth);

    sem_getvalue(&isWest, &isRight);
    sem_getvalue(&isEast, &isLeft);

    // isRight = isWest = 0
    // if there is a car on the right that have arrived the cross but not occupy its first resource
    // we must ensure the car on the right go first, so this car will wait until the car on the right occupy c
    // once the car on the right has reached state 1, it is ensured that this car can't pass the cross before it
    // so there won't be any waste time or unnecessary wait
    if (isRight==0 &&
        car_from_west_queue.cars[car_from_east_queue.front].state==0) {
        // the signal is issued by the car on the right which cause the waiting of this car
        pthread_mutex_lock(&mutex_north_wait_right);
        pthread_cond_wait(&cond_north, &mutex_north_wait_right);
        pthread_mutex_unlock(&mutex_north_wait_right);
    }
    // if there is a car on the left, and that car is passing the cross (state = 1 or state = 2)
    // wait until the car leaves
    if (isLeft==0 &&
       (car_from_east_queue.cars[car_from_east_queue.front].state==1 ||
        car_from_east_queue.cars[car_from_east_queue.front].state==2)) {
        pthread_mutex_lock(&mutex_north_wait_left);
        pthread_cond_wait(&cond_east_pass, &mutex_north_wait_left);
        pthread_mutex_unlock(&mutex_north_wait_left);
    }

    usleep(1);

    // occupy c, the first resource need by the car from north to pass the cross
    pthread_mutex_lock(&mutex_c);
    // and meanwhile, set the state of the car as 1
    car_from_north_queue.cars[indexInQueue].state = 1;
    // issue a signal to the car on the left to go ahead
    pthread_cond_signal(&cond_east);
    printf("car %d from North has occupied its first resource.\n", id);

    usleep(1);

    // occupy d, the second resource need by the car from north to pass the cross
    pthread_mutex_lock(&mutex_d);
    // and meanwhile, set the state of the car as 1
    car_from_north_queue.cars[indexInQueue].state = 2;

    sem_post(&isNorth);
    // wake up the next car in the queue
    car_from_north_queue.front++;

    printf("car %d from North has passed the cross.\n", id);

    // issue a signal of the car from north has passed the cross
    pthread_cond_signal(&cond_north_pass);
    pthread_cond_broadcast(&cond_north_broadcast);

    pthread_mutex_unlock(&mutex_north);

    usleep(1);

    // release the resources
    pthread_mutex_unlock(&mutex_c);
    pthread_mutex_unlock(&mutex_d);

    // exit the thread
    pthread_exit(NULL);
}

void *car_from_west_thread_fun(int *_indexInQueue) {
    // get the information of the car from the car queue
    int indexInQueue = *_indexInQueue;
    int id = car_from_west_queue.cars[indexInQueue].id;
    dir_t dir = car_from_west_queue.cars[indexInQueue].dir;

    // variables to get the value of the semaphores
    int isRight;
    int isLeft;

    pthread_mutex_lock(&mutex_west);

    // waiting in the queue if the car is not the first car in the queue
    while(car_from_west_queue.front != indexInQueue) {
      pthread_cond_wait(&cond_west_broadcast, &mutex_west);
    }

    printf("car %d from West arrives at cross.\n", id);

    // state 0: the car arrives but doesn't occupy any resource
    car_from_west_queue.cars[indexInQueue].state = 0;
    
    sem_wait(&isWest);

    sem_getvalue(&isSouth, &isRight);
    sem_getvalue(&isNorth, &isLeft);
    
    // isRight = isSouth = 0
    // if there is a car on the right that have arrived the cross but not occupy its first resource
    // we must ensure the car on the right go first, so this car will wait until the car on the right occupy c
    // once the car on the right has reached state 1, it is ensured that this car can't pass the cross before it
    // so there won't be any waste time or unnecessary wait
    if (isRight==0 &&
        car_from_south_queue.cars[car_from_south_queue.front].state==0) {
        // the signal is issued by the car on the right which cause the waiting of this car
        pthread_mutex_lock(&mutex_west_wait_right);
        pthread_cond_wait(&cond_west, &mutex_west_wait_right);
        pthread_mutex_unlock(&mutex_west_wait_right);
    }
    // if there is a car on the left, and that car is passing the cross (state = 1 or state = 2)
    // wait until the car leaves
    if (isLeft==0 &&
       (car_from_north_queue.cars[car_from_north_queue.front].state==1 ||
        car_from_north_queue.cars[car_from_north_queue.front].state==2)) {
        pthread_mutex_lock(&mutex_west_wait_left);
        pthread_cond_wait(&cond_north_pass, &mutex_west_wait_left);
        pthread_mutex_unlock(&mutex_west_wait_left);
    }

    usleep(1);

    // occupy c, the first resource need by the car from north to pass the cross
    pthread_mutex_lock(&mutex_d);
    // and meanwhile, set the state of the car as 1
    car_from_west_queue.cars[indexInQueue].state = 1;
    // issue a signal to the car on the left to go ahead
    pthread_cond_signal(&cond_north);
    printf("car %d from West has occupied its first resource.\n", id);

    usleep(1);

    // occupy a, the second resource need by the car from north to pass the cross
    pthread_mutex_lock(&mutex_a);
    // and meanwhile, set the state of the car as 1
    car_from_west_queue.cars[indexInQueue].state = 2;

    sem_post(&isWest);
    // wake up the next car in the queue
    car_from_west_queue.front++;

    printf("car %d from West has passed the cross.\n", id);

    // issue a signal of the car from north has passed the cross
    pthread_cond_signal(&cond_west_pass);
    pthread_cond_broadcast(&cond_west_broadcast);

    pthread_mutex_unlock(&mutex_west);

    usleep(1);

    // release the resources
    pthread_mutex_unlock(&mutex_d);
    pthread_mutex_unlock(&mutex_a);

    // exit the thread
    pthread_exit(NULL);
}

void *car_from_south_thread_fun(int *_indexInQueue) {
    // get the information of the car from the car queue
    int indexInQueue = *_indexInQueue;
    int id = car_from_south_queue.cars[indexInQueue].id;
    dir_t dir = car_from_south_queue.cars[indexInQueue].dir;

    // variables to get the value of the semaphores
    int isRight;
    int isLeft;

    // get the control right of the car from south thread
    pthread_mutex_lock(&mutex_south);

    // waiting in the queue if the car is not the first car in the queue
    while(car_from_south_queue.front != indexInQueue) {
      pthread_cond_wait(&cond_south_broadcast, &mutex_south);
    }

    printf("car %d from South arrives at cross.\n", id);

    // state 0: the car arrives but doesn't occupy any resource
    car_from_south_queue.cars[indexInQueue].state = 0;
    
    sem_wait(&isSouth);
    
    sem_getvalue(&isEast, &isRight);
    sem_getvalue(&isWest, &isLeft);
    
    // isRight = isEast = 0
    // if there is a car on the right that have arrived the cross but not occupy its first resource
    // we must ensure the car on the right go first, so this car will wait until the car on the right occupy c
    // once the car on the right has reached state 1, it is ensured that this car can't pass the cross before it
    // so there won't be any waste time or unnecessary wait
    if (isRight==0 &&
        car_from_east_queue.cars[car_from_east_queue.front].state==0) {
        // the signal is issued by the car on the right which cause the waiting of this car
        pthread_mutex_lock(&mutex_south_wait_right);
        pthread_cond_wait(&cond_south, &mutex_south_wait_right);
        pthread_mutex_unlock(&mutex_south_wait_right);
    }
    // if there is a car on the left, and that car is passing the cross (state = 1 or state = 2)
    // wait until the car leaves
    if (isLeft==0 &&
       (car_from_west_queue.cars[car_from_west_queue.front].state==1 ||
        car_from_west_queue.cars[car_from_west_queue.front].state==2)) {
        pthread_mutex_lock(&mutex_south_wait_left);
        pthread_cond_wait(&cond_west_pass, &mutex_south_wait_left);
        pthread_mutex_unlock(&mutex_south_wait_left);
    }

    usleep(1);

    // occupy a, the first resource need by the car from north to pass the cross
    pthread_mutex_lock(&mutex_a);
    // and meanwhile, set the state of the car as 1
    car_from_south_queue.cars[indexInQueue].state = 1;
    // issue a signal to the car on the left to go ahead
    pthread_cond_signal(&cond_west);
    printf("car %d from South has occupied its first resource.\n", id);

    usleep(1);

    // occupy b, the second resource need by the car from north to pass the cross
    pthread_mutex_lock(&mutex_b);
    // and meanwhile, set the state of the car as 1
    car_from_south_queue.cars[indexInQueue].state = 2;
    
    sem_post(&isSouth);
    // wake up the next car in the queue
    car_from_south_queue.front++;

    printf("car %d from South has passed the cross.\n", id);

    // issue a signal of the car from north has passed the cross
    pthread_cond_signal(&cond_south_pass);
    pthread_cond_broadcast(&cond_south_broadcast);

    pthread_mutex_unlock(&mutex_south);

    usleep(1);

    // release the resources
    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);

    // exit the thread
    pthread_exit(NULL);
}

void *car_from_east_thread_fun(int *_indexInQueue) {
    // get the information of the car from the car queue
    int indexInQueue = *_indexInQueue;
    int id = car_from_east_queue.cars[indexInQueue].id;
    dir_t dir = car_from_east_queue.cars[indexInQueue].dir;

    // variables to get the value of the semaphores
    int isRight;
    int isLeft;

    // waiting in the queue if the car is not the first car in the queue
    pthread_mutex_lock(&mutex_east);
    while(car_from_east_queue.front != indexInQueue) {
      pthread_cond_wait(&cond_east_broadcast, &mutex_east);
    }
    
    printf("car %d from East arrives at cross.\n", id);

    // state 0: the car arrives but doesn't occupy any resource
    car_from_east_queue.cars[indexInQueue].state = 0;
    
    sem_wait(&isEast);
    
    sem_getvalue(&isNorth, &isRight);
    sem_getvalue(&isSouth, &isLeft);
    
    // isRight = isNorth = 0
    // if there is a car on the right that have arrived the cross but not occupy its first resource
    // we must ensure the car on the right go first, so this car will wait until the car on the right occupy c
    // once the car on the right has reached state 1, it is ensured that this car can't pass the cross before it
    // so there won't be any waste time or unnecessary wait
    if (isRight==0 &&
        car_from_north_queue.cars[car_from_north_queue.front].state==0) {
        // the signal is issued by the car on the right which cause the waiting of this car
        pthread_mutex_lock(&mutex_east_wait_right);
        pthread_cond_wait(&cond_east, &mutex_east_wait_right);
        pthread_mutex_unlock(&mutex_east_wait_right);
    }
    // if there is a car on the left, and that car is passing the cross (state = 1 or state = 2)
    // wait until the car leaves
    if (isLeft==0 &&
       (car_from_south_queue.cars[car_from_south_queue.front].state==1 ||
        car_from_south_queue.cars[car_from_south_queue.front].state==2)) {
        pthread_mutex_lock(&mutex_east_wait_left);
        pthread_cond_wait(&cond_south_pass, &mutex_east_wait_left);
        pthread_mutex_unlock(&mutex_east_wait_left);
    }

    usleep(1);

    // occupy b, the first resource need by the car from north to pass the cross
    pthread_mutex_lock(&mutex_b);
    // and meanwhile, set the state of the car as 1
    car_from_east_queue.cars[indexInQueue].state = 1;
    // issue a signal to the car on the left to go ahead
    pthread_cond_signal(&cond_south);
    printf("car %d from Esst has occupied its first resource.\n", id);

    usleep (1);

    // occupy c, the second resource need by the car from north to pass the cross
    pthread_mutex_lock(&mutex_c);
    // and meanwhile, set the state of the car as 2
    car_from_east_queue.cars[indexInQueue].state = 2;
    
    sem_post(&isEast);
    // wake up the next car in the queue
    car_from_east_queue.front++;

    printf("car %d from East has passed the cross.\n", id);

    // issue a signal of the car from north has passed the cross
    pthread_cond_signal(&cond_east_pass);
    pthread_cond_broadcast(&cond_east_broadcast);

    pthread_mutex_unlock(&mutex_east);

    usleep(1);

    // release the resources
    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_c);

    // exit the thread
    pthread_exit(NULL);
}


void queue_init(pQueue pQ, dir_t dir) {
    pQ->front = -1;
    pQ->rear = 0;
    pQ->count = 0;
    pQ->dir = dir;
    int i;
    for(i=0; i<MAX; i++) {
        pQ->cars[i].id = 0;
        pQ->cars[i].dir = unknown;
        pQ->cars[i].state = -1;
    }
}

void queue_enqueue(pQueue pQ, dir_t dir, int id) {
    pQ->rear++;
    pQ->cars[pQ->count].id = id;
    pQ->cars[pQ->count].dir = dir;
    pQ->cars[pQ->count].state = 0;
    pQ->cars[pQ->count].indexInQueue = pQ->count;
    int res;
    switch (dir) {
        case north:
            pthread_create(&(pQ->cars[pQ->count].thread), NULL, car_from_north_thread_fun, &(pQ->cars[pQ->count].indexInQueue));
            break;
        case west:
            pthread_create(&(pQ->cars[pQ->count].thread), NULL, car_from_west_thread_fun, &(pQ->cars[pQ->count].indexInQueue));
            break;
        case south:
            pthread_create(&(pQ->cars[pQ->count].thread), NULL, car_from_south_thread_fun, &(pQ->cars[pQ->count].indexInQueue));
            break;
        case east:
            pthread_create(&(pQ->cars[pQ->count].thread), NULL, car_from_east_thread_fun, &(pQ->cars[pQ->count].indexInQueue));
            break;
        default:
            break;
    }
    car_threads[total++] = pQ->cars[pQ->count].thread;
    pQ->count++;
}

int main(int argc, char** argv) {
    queue_init(&car_from_north_queue, north);
    queue_init(&car_from_east_queue, east);
    queue_init(&car_from_south_queue, south);
    queue_init(&car_from_west_queue, west);
    memset(car_threads, 0, sizeof(pthread_t)*MAX);

    pthread_mutex_init(&mutex_north, NULL);
    pthread_mutex_init(&mutex_east, NULL);
    pthread_mutex_init(&mutex_south, NULL);
    pthread_mutex_init(&mutex_west, NULL);

    pthread_mutex_init(&mutex_a, NULL);
    pthread_mutex_init(&mutex_b, NULL);
    pthread_mutex_init(&mutex_c, NULL);
    pthread_mutex_init(&mutex_d, NULL);

    // mutex wait right
    pthread_mutex_init(&mutex_north_wait_right, NULL);
    pthread_mutex_init(&mutex_east_wait_right, NULL);
    pthread_mutex_init(&mutex_south_wait_right, NULL);
    pthread_mutex_init(&mutex_west_wait_right, NULL);

    // mutex wait left
    pthread_mutex_init(&mutex_north_wait_left, NULL);
    pthread_mutex_init(&mutex_east_wait_left, NULL);
    pthread_mutex_init(&mutex_south_wait_left, NULL);
    pthread_mutex_init(&mutex_west_wait_left, NULL);
    
    sem_init(&isNorth, 0, 1);
    sem_init(&isEast, 0, 1);
    sem_init(&isSouth, 0, 1);
    sem_init(&isWest, 0, 1);

    char cars[MAX];
    scanf("%s", cars);

    int i=0;
    for(i=0; i<strlen(cars); i++) {
        switch(cars[i]) {
            case 'n':
                queue_enqueue(&car_from_north_queue, north, i);
                break;
            case 'e':
                queue_enqueue(&car_from_east_queue, east, i);
                break;
            case 's':
                queue_enqueue(&car_from_south_queue, south, i);
                break;
            case 'w':
                queue_enqueue(&car_from_west_queue, west, i);
                break;
            default:
                break;
        }
    }

    // wake up the first car in each queue
    pthread_mutex_lock(&mutex_north);
    pthread_mutex_lock(&mutex_east);
    pthread_mutex_lock(&mutex_south);
    pthread_mutex_lock(&mutex_west);
    car_from_north_queue.front++;
    pthread_cond_broadcast(&cond_north_broadcast);
    car_from_east_queue.front++;
    pthread_cond_broadcast(&cond_east_broadcast);
    car_from_south_queue.front++;
    pthread_cond_broadcast(&cond_south_broadcast);
    car_from_west_queue.front++;
    pthread_cond_broadcast(&cond_west_broadcast);
    pthread_mutex_unlock(&mutex_north);
    pthread_mutex_unlock(&mutex_east);
    pthread_mutex_unlock(&mutex_south);
    pthread_mutex_unlock(&mutex_west);

    //join the thread
    for (i=0; i<total; i++) {
      pthread_join(car_threads[i], NULL);
    }

    return 0;
}
