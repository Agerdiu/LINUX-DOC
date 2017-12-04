#include <stdio.h>
#include <pthread.h>
#include <sys/unistd.h>
#include <string.h>
#include <stdbool.h>

#define MAX 100  //define max car numbers
typedef enum { NORTH, EAST, SOUTH, WEST } dir;
typedef struct queue queue;
struct queue {
	int front;  //index of the head of a queue
	int rear;  //index of the rear of a queue
	int count;  //number of cars in a queue
	int num[MAX];  //array to store car id
};

void init(queue* q) {
	q->front = 0;
	q->rear = 0;
	q->count = 0;
}

void push(queue* q, int i) {
	q->rear = (q->rear + 1) % MAX;
	q->count++;
	q->num[q->rear] = i;
}

int pop(queue* q) {
	q->front = (q->front + 1) % MAX;
	q->count--;
	return q->num[q->front];  //return the head of a queue
}

bool is_empty(queue* q) {
	if (q->count)
		return false;
	else
		return true;
}

queue North, South, East, West;//four queues for four dirs to store cars
pthread_mutex_t NorthMutex, SouthMutex, EastMutex, WestMutex;//mutex for updating four queues above

															 //conditional variables for activing the queue thread
pthread_cond_t NorthCond, SouthCond, EastCond, WestCond;

bool FirstNorth, FirstSouth, FirstEast, FirstWest;//bool variables preventing starvation
pthread_mutex_t FirstNorthMutex, FirstSouthMutex, FirstEastMutex, FirstWestMutex;//mutex for updating four bool variables above

																				 //conditional variables for activing the waiting thread
pthread_cond_t FirstNorthCond, FirstSouthCond, FirstEastCond, FirstWestCond;

//mutex for a.b.c.d road
pthread_mutex_t Mutex_a, Mutex_b, Mutex_c, Mutex_d;

//four integer variables, the car id crossing the road for every dir
int CurNorth = 0, CurSouth = 0, CurEast = 0, CurWest = 0;
//mutex for updating four integer variables above
pthread_mutex_t CurNorthMutex, CurSouthMutex, CurEastMutex, CurWestMutex;

//four bool variables to represent whether existing cars in every dirs
bool NorthHasCar = false, SouthHasCar = false, EastHasCar = false, WestHasCar = false;

//Empty number  a b c d
int Empty = 4;
//mutex for updating integer variable Empty
pthread_mutex_t EmptyMutex;

//bool variable to represent whether the deadlock is over
bool DeadlockOver;
//mutex for updating bool variable DeadlockOver
pthread_mutex_t DeadlockOverMutex;
//conditional variables for activing the waiting thread
pthread_cond_t DeadlockOverCond;


void CrossOpen()  //active the whole program
{
	if (!is_empty(&North))  //if north has car, pop a car from queue
	{
		CurNorth = pop(&North);
		pthread_cond_broadcast(&NorthCond);  //active the waiting thread(car)
	}
	if (!is_empty(&South))  //same with north
	{
		CurSouth = pop(&South);
		pthread_cond_broadcast(&SouthCond);
	}
	if (!is_empty(&East))  //same with north
	{
		CurEast = pop(&East);
		pthread_cond_broadcast(&EastCond);
	}
	if (!is_empty(&West))  //same with north
	{
		CurWest = pop(&West);
		pthread_cond_broadcast(&WestCond);
	}
}

int enterTheCrossing(dir Dir, int CarNumber)  //when a car comes to the crossing
{
	pthread_mutex_t* Road;
	switch (Dir)  //judge which Empty the car needs
	{
	case NORTH:  //the car from north needs c firstly
		Road = &Mutex_c;
		break;
	case SOUTH:  //the car from south needs a firstly
		Road = &Mutex_a;
		break;
	case EAST:  //the car from east needs b firstly
		Road = &Mutex_b;
		break;
	case WEST:  //the car from west needs d firstly
		Road = &Mutex_d;
		break;
	}
	pthread_mutex_lock(Road);  //lock or wait to lock it
	switch (Dir)  //update HasCar variables because the car is crossing
	{
	case NORTH:
                printf("car %d from N arrives at crossing\n", CarNumber);
		NorthHasCar = true;
		break;
	case SOUTH:
                printf("car %d from S arrives at crossing\n", CarNumber);
		SouthHasCar = true;
		break;
	case EAST:
                printf("car %d from E arrives at crossing\n", CarNumber);
		EastHasCar = true;
		break;
	case WEST:
                printf("car %d from W arrives at crossing\n", CarNumber);
		WestHasCar = true;
		break;
	}

	//update the number of Empty
	pthread_mutex_lock(&EmptyMutex);
	int Remainder = --Empty;  //record the remaind Empty to judge deadlock
	pthread_mutex_unlock(&EmptyMutex);

	return Remainder;
}

void detectDeadlock(dir Dir, int Remainder)  //detect the deadlock
{
	if (Remainder != 0) return;  //if a b c and d aren't used up, it must be not deadlock
	pthread_mutex_t* Road;
	pthread_cond_t* First;
	switch (Dir)  //record who detect the deadlock and the left car
	{
	case NORTH:  //the car from north lock c and will signal its left(east) car
		Road = &Mutex_c;
		First = &FirstEastCond;
		break;
	case SOUTH:  //same with north
		Road = &Mutex_a;
		First = &FirstWestCond;
		break;
	case EAST:  //same with north
		Road = &Mutex_b;
		First = &FirstSouthCond;
		break;
	case WEST:  //same with north
		Road = &Mutex_d;
		First = &FirstNorthCond;
		break;
	}
	printf("DEADLOCK car jam detected. signal %s to go\n", Dir == 0 ? "East" : Dir == 1 ? "South" : Dir == 2 ? "West" : "North");
	DeadlockOver = false;  //the deadlock is to be solved
	pthread_mutex_unlock(Road);  //the car who detect deadlock unlock its Empty
	switch (Dir)  //then the dir of the car must not have car
	{
	case NORTH:
		NorthHasCar = false;
		break;
	case SOUTH:
		SouthHasCar = false;
		break;
	case EAST:
		EastHasCar = false;
		break;
	case WEST:
		WestHasCar = false;
		break;
	}
	pthread_cond_signal(First);  //then signal to its left car, because it needn't wait

								 //wait for deadlock over, when some car cross
	pthread_mutex_lock(&DeadlockOverMutex);
	while (DeadlockOver == false)
	{
		pthread_cond_wait(&DeadlockOverCond, &DeadlockOverMutex);
	}
	pthread_mutex_unlock(&DeadlockOverMutex);

	pthread_mutex_lock(Road);  //when deadlock is solved, the car will get the Empty again
	switch (Dir)  //then update the HasCar in its dir
	{
	case NORTH:
		NorthHasCar = true;
		break;
	case SOUTH:
		SouthHasCar = true;
		break;
	case EAST:
		EastHasCar = true;
		break;
	case WEST:
		WestHasCar = true;
		break;
	}
}

void judgeRight(dir Dir)  //judge whether there is car crossing on its right
{
	pthread_mutex_t* FirstMutex;
	pthread_cond_t* FirstCond;
	bool* HasCar;
	switch (Dir)  //record the car's right
	{
	case NORTH:  //the car in the right need wait for the west
		FirstMutex = &FirstNorthMutex;
		FirstCond = &FirstNorthCond;
		HasCar = &WestHasCar;
		break;
	case SOUTH:  //same with north
		FirstMutex = &FirstSouthMutex;
		FirstCond = &FirstSouthCond;
		HasCar = &EastHasCar;
		break;
	case EAST:  //same with north
		FirstMutex = &FirstEastMutex;
		FirstCond = &FirstEastCond;
		HasCar = &NorthHasCar;
		break;
	case WEST:  //same with north
		FirstMutex = &FirstWestMutex;
		FirstCond = &FirstWestCond;
		HasCar = &SouthHasCar;
		break;
	}

	//when existing car crossing in the right, wait for the signal
	pthread_mutex_lock(FirstMutex);
	while (*HasCar)
	{
		pthread_cond_wait(FirstCond, FirstMutex);
	}
	pthread_mutex_unlock(FirstMutex);
}

void leave(dir Dir, int CarNumber)  //leave the crossing
{
	pthread_mutex_t* Road1;  //the first Empty
	pthread_mutex_t* Road2;  //the second Empty
	pthread_mutex_t* FirstMutex;
	pthread_cond_t* FirstCond;
	bool* First;
	bool* HasCar;

	switch (Dir)  //record
	{
	case NORTH:  //record two Emptys the car needs
		Road1 = &Mutex_c;
		Road2 = &Mutex_d;
		HasCar = &NorthHasCar;
		First = &FirstEast;
		FirstCond = &FirstEastCond;
		FirstMutex = &FirstEastMutex;
		break;
	case SOUTH:  //same with north
		Road1 = &Mutex_a;
		Road2 = &Mutex_b;
		HasCar = &SouthHasCar;
		First = &FirstWest;
		FirstCond = &FirstWestCond;
		FirstMutex = &FirstWestMutex;
		break;
	case EAST:  //same with north
		Road1 = &Mutex_b;
		Road2 = &Mutex_c;
		HasCar = &EastHasCar;
		First = &FirstSouth;
		FirstCond = &FirstSouthCond;
		FirstMutex = &FirstSouthMutex;
		break;
	case WEST:  //same with north
		Road1 = &Mutex_d;
		Road2 = &Mutex_a;
		HasCar = &WestHasCar;
		First = &FirstNorth;
		FirstCond = &FirstNorthCond;
		FirstMutex = &FirstNorthMutex;
		break;
	}
	pthread_mutex_lock(Road2);  //lock the secend Empty
	pthread_mutex_unlock(Road1);  //unlock the first Empty
	printf("car %d from %s leaving crossing\n", CarNumber, Dir == 0 ? "North" : Dir == 1 ? "East" : Dir == 2 ? "South" : "West");
	//update the number of Empty
	pthread_mutex_unlock(Road2);  //unlock the second Empty
	pthread_mutex_lock(&EmptyMutex);
	Empty++;
	pthread_mutex_unlock(&EmptyMutex);

	DeadlockOver = true;  //deadlock must be over because there is car crossing
	pthread_cond_signal(&DeadlockOverCond);  //active the car waiting for deadlock

											 //pthread_mutex_unlock(Road2);  //unlock the second Empty

	*HasCar = false;  //now the car has left and the dir doesn't have car crossing

					  //the left car can go to prevent starvation
	pthread_mutex_lock(FirstMutex);
	*First = true;
	pthread_mutex_unlock(FirstMutex);

	pthread_cond_signal(FirstCond);  //active the left car to cross
}

void afterLeave(dir Dir)  //when a car has left, it needs active the cars after it
{
	queue* q;
	pthread_mutex_t* QueueMutex;
	pthread_cond_t* QueueCond;
	pthread_mutex_t* CurMutex;
	int* Cur;
	switch (Dir)  //record
	{
	case NORTH:  //record the dir queue and Cur car id
		q = &North;
		QueueMutex = &NorthMutex;
		QueueCond = &NorthCond;
		Cur = &CurNorth;
		CurMutex = &CurNorthMutex;
		break;
	case SOUTH:  //same with north
		q = &South;
		QueueMutex = &SouthMutex;
		QueueCond = &SouthCond;
		Cur = &CurSouth;
		CurMutex = &CurSouthMutex;
		break;
	case EAST:  //same with north
		q = &East;
		QueueMutex = &EastMutex;
		QueueCond = &EastCond;
		Cur = &CurEast;
		CurMutex = &CurEastMutex;
		break;
	case WEST:  //same with north
		q = &West;
		QueueMutex = &WestMutex;
		QueueCond = &WestCond;
		Cur = &CurWest;
		CurMutex = &CurWestMutex;
		break;
	}

	//lock the queue and Cur varialbe and update them
	pthread_mutex_lock(QueueMutex);
	pthread_mutex_lock(CurMutex);

	*Cur = pop(q);  //the next car in this dir queue will be the crossing car

	pthread_mutex_unlock(CurMutex);
	pthread_mutex_unlock(QueueMutex);

	//sleep to wait all cars have been pushed into queue, then it will get broadcast
	usleep(100);

	pthread_cond_broadcast(QueueCond);  //broadcast to all cars in the queue

}

void* northCar(void* arg)  //the function north car will run
{
	int CarNumber = *((int*)arg);  //get the car id

	pthread_mutex_lock(&NorthMutex);
	while (CurNorth != CarNumber)
	{
		pthread_cond_wait(&NorthCond, &NorthMutex);  //wait until the car id equals Cur id(should cross)
	}
	pthread_mutex_unlock(&NorthMutex);

	int Remainder = enterTheCrossing(NORTH, CarNumber);  //try to get the Empty
	detectDeadlock(NORTH, Remainder);  //detect the deadlock
	judgeRight(NORTH);  //judge whether it needs to let the right car go firstly
	leave(NORTH, CarNumber);  //then go
	afterLeave(NORTH);  //active the car after it(the next car in the queue)
	return NULL;
}

void* southCar(void* arg)  //same with northCar
{
	int CarNumber = *((int*)arg);
	pthread_mutex_lock(&SouthMutex);
	while (CurSouth != CarNumber)
	{
		pthread_cond_wait(&SouthCond, &SouthMutex);
	}
	pthread_mutex_unlock(&SouthMutex);

	int Remainder = enterTheCrossing(SOUTH, CarNumber);
	detectDeadlock(SOUTH, Remainder);
	judgeRight(SOUTH);
	leave(SOUTH, CarNumber);
	afterLeave(SOUTH);
	return NULL;
}

void* eastCar(void* arg)  //same with northCar
{
	int CarNumber = *((int*)arg);
	pthread_mutex_lock(&EastMutex);
	while (CurEast != CarNumber)
	{
		pthread_cond_wait(&EastCond, &EastMutex);
	}
	pthread_mutex_unlock(&EastMutex);

	int Remainder = enterTheCrossing(EAST, CarNumber);
	detectDeadlock(EAST, Remainder);
	judgeRight(EAST);
	leave(EAST, CarNumber);
	afterLeave(EAST);
	return NULL;
}

void* westCar(void* arg)  //same with northCar
{
	int CarNumber = *((int*)arg);
	pthread_mutex_lock(&WestMutex);
	while (CurWest != CarNumber)
	{
		pthread_cond_wait(&WestCond, &WestMutex);
	}
	pthread_mutex_unlock(&WestMutex);

	int Remainder = enterTheCrossing(WEST, CarNumber);
	detectDeadlock(WEST, Remainder);
	judgeRight(WEST);
	leave(WEST, CarNumber);
	afterLeave(WEST);
	return NULL;
}

int main()
{
	char input[MAX];
        printf("Please input cars:\n");
	scanf("%s", input);
        int CarNumber = strlen(input);
	if (CarNumber==0)
	{
		printf("No input...\n");
		return 0;
	}
        else printf("Total Car Number:%d\n",CarNumber);
	//initialize all the mutex and conditional variables
	pthread_mutex_init(&NorthMutex, NULL);
	pthread_mutex_init(&SouthMutex, NULL);
	pthread_mutex_init(&EastMutex, NULL);
	pthread_mutex_init(&WestMutex, NULL);
	pthread_cond_init(&NorthCond, NULL);
	pthread_cond_init(&NorthCond, NULL);
	pthread_cond_init(&NorthCond, NULL);
	pthread_cond_init(&NorthCond, NULL);

	pthread_mutex_init(&FirstNorthMutex, NULL);
	pthread_mutex_init(&FirstSouthMutex, NULL);
	pthread_mutex_init(&FirstEastMutex, NULL);
	pthread_mutex_init(&FirstWestMutex, NULL);
	pthread_cond_init(&FirstNorthCond, NULL);
	pthread_cond_init(&FirstSouthCond, NULL);
	pthread_cond_init(&FirstEastCond, NULL);
	pthread_cond_init(&FirstWestCond, NULL);

	pthread_mutex_init(&Mutex_a, NULL);
	pthread_mutex_init(&Mutex_b, NULL);
	pthread_mutex_init(&Mutex_c, NULL);
	pthread_mutex_init(&Mutex_d, NULL);

	pthread_mutex_init(&CurNorthMutex, NULL);
	pthread_mutex_init(&CurSouthMutex, NULL);
	pthread_mutex_init(&CurEastMutex, NULL);
	pthread_mutex_init(&CurWestMutex, NULL);

	pthread_mutex_init(&EmptyMutex, NULL);

	pthread_mutex_init(&DeadlockOverMutex, NULL);
	pthread_cond_init(&DeadlockOverCond, NULL);
	//initialize all the queue

        pthread_t Cars[CarNumber];  //store all cars' thread
	init(&North);
	init(&South);
	init(&East);
	init(&West);	
	int i;
	int id[MAX + 1];
	for (i = 1; i<MAX; i++)	id[i] = i;
	//create thread for each car
	for (i = 1; i <= CarNumber+1; i++)
	{
		switch (input[i-1])
		{
		case 'n':  //push into North cars , create thread;
			push(&North, i);
			pthread_create(&Cars[i], NULL, northCar, (void*)(&id[i]));
			usleep(1);  //improve coCur
			break;
		case 's':  //push into South cars , create thread;
			push(&South, i);
			pthread_create(&Cars[i], NULL, southCar, (void*)(&id[i]));
			usleep(1);
			break;
		case 'e':  //push into East cars , create thread;
			push(&East, i);
			pthread_create(&Cars[i], NULL, eastCar, (void*)(&id[i]));
			usleep(1);
			break;
		case 'w':  //push into West cars , create thread;
			push(&West, i);
			pthread_create(&Cars[i], NULL, westCar, (void*)(&id[i]));
			usleep(1);
			break;
                case 'N':  //same to 'n'
			push(&North, i);
			pthread_create(&Cars[i], NULL, northCar, (void*)(&id[i]));
			usleep(1);  //improve coCur
			break;
		case 'S':  //same to 's'
			push(&South, i);
			pthread_create(&Cars[i], NULL, southCar, (void*)(&id[i]));
			usleep(1);
			break;
		case 'E':  //same to 'e'
			push(&East, i);
			pthread_create(&Cars[i], NULL, eastCar, (void*)(&id[i]));
			usleep(1);
			break;
		case 'W':  //same to 'w'
			push(&West, i);
			pthread_create(&Cars[i], NULL, westCar, (void*)(&id[i]));
			usleep(1);
			break;
                default:
                       if(input[i-1]=='\0') break;
                       printf("%c is an Invaild direction!\n",input[i-1]);
                       return 0;
		}
	}

	CrossOpen();  //enable the crossroad system

	for (i = 1; i <= CarNumber; i++)  //join all cars' thread
	{
		pthread_join(Cars[i], NULL);
	}


	//destroy all mutex and conditional variables
	pthread_mutex_destroy(&NorthMutex);
	pthread_mutex_destroy(&SouthMutex);
	pthread_mutex_destroy(&EastMutex);
	pthread_mutex_destroy(&WestMutex);
	pthread_cond_destroy(&NorthCond);
	pthread_cond_destroy(&SouthCond);
	pthread_cond_destroy(&EastCond);
	pthread_cond_destroy(&WestCond);

	pthread_mutex_destroy(&FirstNorthMutex);
	pthread_mutex_destroy(&FirstSouthMutex);
	pthread_mutex_destroy(&FirstEastMutex);
	pthread_mutex_destroy(&FirstWestMutex);
	pthread_cond_destroy(&FirstNorthCond);
	pthread_cond_destroy(&FirstSouthCond);
	pthread_cond_destroy(&FirstEastCond);
	pthread_cond_destroy(&FirstWestCond);

	pthread_mutex_destroy(&Mutex_a);
	pthread_mutex_destroy(&Mutex_b);
	pthread_mutex_destroy(&Mutex_c);
	pthread_mutex_destroy(&Mutex_d);

	pthread_mutex_destroy(&CurNorthMutex);
	pthread_mutex_destroy(&CurSouthMutex);
	pthread_mutex_destroy(&CurEastMutex);
	pthread_mutex_destroy(&CurWestMutex);

	pthread_mutex_destroy(&EmptyMutex);

	pthread_mutex_destroy(&DeadlockOverMutex);

	pthread_cond_destroy(&DeadlockOverCond);

}
