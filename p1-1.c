#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/unistd.h>
#include <stdbool.h>
#define MAX 100  //define max car numbers
typedef struct queue queue;
typedef enum { N_D, E_D, S_D, W_D } dir;
struct queue {
	int front;  //head
	int rear;  //rear
	int count;  //number of cars
	int num[MAX];  //Car id in this way
};
void init(queue* q) {
	q->front = 0;
	q->rear = 0;
	q->count = 0;
}
void enqueue(queue* q, int i) {
	q->rear = (q->rear + 1) % MAX;
	q->count++;
	q->num[q->rear] = i;
}
int pop(queue* q) {
	q->front = (q->front + 1) % MAX;
	q->count--;
	return q->num[q->front];  //return the head of a queue
}
bool Empty(queue* q) {
	if (q->count)
		return false;
	else
		return true;
}

queue North, South, East, West;//four queues for four dirs to store cars

pthread_mutex_t NorthMutex, SouthMutex, EastMutex, WestMutex;//mutex for updating four queues above			

pthread_cond_t NorthCond, SouthCond, EastCond, WestCond;  //conditional variables for activing the queue thread

bool First_North, First_South, First_East, First_West;//bool variables preventing starvation

pthread_mutex_t FirstNorthMutex, FirstSouthMutex, FirstEastMutex, FirstWestMutex;//mutex for updating four bool variables above	

pthread_cond_t FirstNorth, FirstSouth, FirstEast, FirstWest; //conditional variables for activing the waiting thread

pthread_mutex_t Mutex_SE, Mutex_NE, Mutex_NW, Mutex_SW;//mutex for cross:
													   /*
													   ——|        |——
													        NW   NE
													   ———————
													        SW    SE
													   ——|        |——
													   */
int CurID_North = 0, CurID_South = 0, CurID_East = 0, CurID_West = 0; //car's ID crossing the road in four directions

pthread_mutex_t CurNorthMutex, CurSouthMutex, CurEastMutex, CurWestMutex; //mutex for updating four integer variables above

bool NorthHasCar = false, SouthHasCar = false, EastHasCar = false, WestHasCar = false;//four bool variables to represent whether existing cars in every dirs

int Resources = 4;//Cross Resources

pthread_mutex_t EmptyMutex;//mutex for updating integer variable Resources

bool DeadlockOver; //bool variable to represent whether the deadlock is over

pthread_mutex_t DeadlockOverMutex; //mutex for updating bool variable DeadlockOver

pthread_cond_t DeadlockOverCond; //conditional variables for activing the waiting thread


void CrossOpen()  //active the whole program
{
	if (!Empty(&North))  //if north has car, pop a car from queue
	{
		CurID_North = pop(&North);
		pthread_cond_broadcast(&NorthCond);  //active the waiting thread(car)
	}
	if (!Empty(&South))  //same with north
	{
		CurID_South = pop(&South);
		pthread_cond_broadcast(&SouthCond);
	}
	if (!Empty(&East))  //same with north
	{
		CurID_East = pop(&East);
		pthread_cond_broadcast(&EastCond);
	}
	if (!Empty(&West))  //same with north
	{
		CurID_West = pop(&West);
		pthread_cond_broadcast(&WestCond);
	}
}

void DeadlockDealing(dir Dir)  //detect the deadlock
{
	if (Resource != 0) return;  //if a b c and d aren't used up, it must be not deadlock
	pthread_mutex_t* Road;
	pthread_cond_t* First;
	switch (Dir)  //record who detect the deadlock and the left car
	{
	case N_D:  //the car from north lock c and will signal its left(east) car
		Road = &Mutex_NW;
		First = &FirstEast;
		break;
	case S_D:  //same with north
		Road = &Mutex_SE;
		First = &FirstWest;
		break;
	case E_D:  //same with north
		Road = &Mutex_NE;
		First = &FirstSouth;
		break;
	case W_D:  //same with north
		Road = &Mutex_SW;
		First = &FirstNorth;
		break;
	}
	printf("DEADLOCK car jam detected. signal %s to go\n", Dir == 0 ? "East" : Dir == 1 ? "South" : Dir == 2 ? "West" : "North");
	DeadlockOver = false;  //the deadlock is to be solved
	pthread_mutex_unlock(Road);  //the car who detect deadlock unlock its Resources
	switch (Dir)  //then the dir of the car must not have car
	{
	case N_D:
		NorthHasCar = false;
		break;
	case S_D:
		SouthHasCar = false;
		break;
	case E_D:
		EastHasCar = false;
		break;
	case W_D:
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

	pthread_mutex_lock(Road);  //when deadlock is solved, the car will get the Resources again
	switch (Dir)  //then update the HasCar in its dir
	{
	case N_D:
		NorthHasCar = true;
		break;
	case S_D:
		SouthHasCar = true;
		break;
	case E_D:
		EastHasCar = true;
		break;
	case W_D:
		WestHasCar = true;
		break;
	}
}


void* CarFromN(void* arg)  //the function north car will run
{
	int CarNumber = *((int*)arg);  //get the car id
	pthread_mutex_lock(&NorthMutex);
	while (CurID_North != CarNumber)
	{
		pthread_cond_wait(&NorthCond, &NorthMutex);  //wait until the car id equals Cur id(should cross)
	}
	pthread_mutex_unlock(&NorthMutex);
	//try to get the Resources
	pthread_mutex_lock(&Mutex_NW);  //lock or wait to lock it
	printf("car %d from N arrives at crossing\n", CarNumber);
	NorthHasCar = true;
	//update the number of Resources
	pthread_mutex_lock(&EmptyMutex);
	Resources--;  //record the remaind Resources to judge deadlock
	pthread_mutex_unlock(&EmptyMutex);
	//entered
	DeadlockDealing(N_D);  //detect the deadlock
									   //when existing car crossing in the right, wait for the signal
	pthread_mutex_lock(&FirstNorthMutex);
	while (WestHasCar)		pthread_cond_wait(&FirstNorth, &FirstNorthMutex);
	pthread_mutex_unlock(&FirstNorthMutex);
	//leaving
	pthread_mutex_lock(&Mutex_SW);  //lock the secend Resources
	pthread_mutex_unlock(&Mutex_NW);  //unlock the first Resources
	printf("car %d from N leaving crossing\n", CarNumber);
	pthread_mutex_unlock(&Mutex_SW);  //unlock the second Resources
	pthread_mutex_lock(&EmptyMutex);
	Resources++;  //update the number of Resources
	pthread_mutex_unlock(&EmptyMutex);
	DeadlockOver = true;  //deadlock must be over because there is car crossing
	pthread_cond_signal(&DeadlockOverCond);  //active the car waiting for deadlock
	NorthHasCar = false;  //now the car has left and the dir doesn't have car crossing				
	pthread_mutex_lock(&FirstEastMutex); //the left car can go to prevent starvation
	First_East = true;
	pthread_mutex_unlock(&FirstEastMutex);
	pthread_cond_signal(&FirstEast);  //active the left car to cross
	int* Cur;
	Cur = &CurID_North;
	//lock the queue and Cur varialbe and update them
	pthread_mutex_lock(&NorthMutex);
	pthread_mutex_lock(&CurNorthMutex);
	*Cur = pop(&North);  //the next car in this dir queue will be the crossing car
	pthread_mutex_unlock(&CurNorthMutex);
	pthread_mutex_unlock(&NorthMutex);
	//sleep to wait all cars have been pushed into queue, then it will get broadcast
	usleep(100);
	pthread_cond_broadcast(&NorthCond);  //broadcast to all cars in the queue
	return NULL;
}

void* CarFromS(void* arg)  //same with CarFromS
{
	int CarNumber = *((int*)arg);  //get the car id
	pthread_mutex_lock(&SouthMutex);
	while (CurID_South != CarNumber)
	{
		pthread_cond_wait(&SouthCond, &SouthMutex);  //wait until the car id equals Cur id(should cross)
	}
	pthread_mutex_unlock(&SouthMutex);
	//try to get the Resources
	pthread_mutex_lock(&Mutex_SE);  //lock or wait to lock it
	printf("car %d from S arrives at crossing\n", CarNumber);
	SouthHasCar = true;
	//update the number of Resources
	pthread_mutex_lock(&EmptyMutex);
	Resources--;  //record the remaind Resources to judge deadlock
	pthread_mutex_unlock(&EmptyMutex);
	//entered
	DeadlockDealing(S_D);  //detect the deadlock
									   //when existing car crossing in the right, wait for the signal
	pthread_mutex_lock(&FirstSouthMutex);
	while (EastHasCar)		pthread_cond_wait(&FirstSouth, &FirstSouthMutex);
	pthread_mutex_unlock(&FirstSouthMutex);
	//leaving
	pthread_mutex_lock(&Mutex_NE);  //lock the secend Resources
	pthread_mutex_unlock(&Mutex_SE);  //unlock the first Resources
	printf("car %d from S leaving crossing\n", CarNumber);
	pthread_mutex_unlock(&Mutex_NE);  //unlock the second Resources
	pthread_mutex_lock(&EmptyMutex);
	Resources++;  //update the number of Resources
	pthread_mutex_unlock(&EmptyMutex);
	DeadlockOver = true;  //deadlock must be over because there is car crossing
	pthread_cond_signal(&DeadlockOverCond);  //active the car waiting for deadlock
	SouthHasCar = false;  //now the car has left and the dir doesn't have car crossing				
	pthread_mutex_lock(&FirstWestMutex); //the left car can go to prevent starvation
	First_West = true;
	pthread_mutex_unlock(&FirstWestMutex);
	pthread_cond_signal(&FirstWest);  //active the left car to cross
	int* Cur;
	Cur = &CurID_South;
	//lock the queue and Cur varialbe and update them
	pthread_mutex_lock(&SouthMutex);
	pthread_mutex_lock(&CurSouthMutex);
	*Cur = pop(&South);  //the next car in this dir queue will be the crossing car
	pthread_mutex_unlock(&CurSouthMutex);
	pthread_mutex_unlock(&SouthMutex);
	//sleep to wait all cars have been pushed into queue, then it will get broadcast
	usleep(100);
	pthread_cond_broadcast(&SouthCond);  //broadcast to all cars in the queue
	return NULL;
}

void* CarFromE(void* arg)  //same with CarFromE
{
	int CarID = *((int*)arg);  //get the car id
	pthread_mutex_lock(&EastMutex);
	while (CurID_East != CarID)
	{
		pthread_cond_wait(&EastCond, &EastMutex);  //wait until the car id equals Cur id(should cross)
	}
	pthread_mutex_unlock(&EastMutex);
	//try to get the Resources
	pthread_mutex_lock(&Mutex_NE);  //lock or wait to lock it
	printf("car %d from E arrives at crossing\n", CarID);
	EastHasCar = true;
	//update the number of Resources
	pthread_mutex_lock(&EmptyMutex);
	Resources--;  //record the remaind Resources to judge deadlock
	pthread_mutex_unlock(&EmptyMutex);
	//entered
	DeadlockDealing(E_D);  //detect the deadlock
									   //when existing car crossing in the right, wait for the signal
	pthread_mutex_lock(&FirstEastMutex);
	while (NorthHasCar)		pthread_cond_wait(&FirstEast, &FirstEastMutex);
	pthread_mutex_unlock(&FirstEastMutex);
	//leaving
	pthread_mutex_lock(&Mutex_NW);  //lock the secend Resources
	pthread_mutex_unlock(&Mutex_NE);  //unlock the first Resources
	printf("car %d from E leaving crossing\n", CarID);
	pthread_mutex_unlock(&Mutex_NW);  //unlock the second Resources
	pthread_mutex_lock(&EmptyMutex);
	Resources++;  //update the number of Resources
	pthread_mutex_unlock(&EmptyMutex);
	DeadlockOver = true;  //deadlock must be over because there is car crossing
	pthread_cond_signal(&DeadlockOverCond);  //active the car waiting for deadlock
	EastHasCar = false;  //now the car has left and the dir doesn't have car crossing				
	pthread_mutex_lock(&FirstSouthMutex); //the left car can go to prevent starvation
	First_East = true;
	pthread_mutex_unlock(&FirstSouthMutex);
	pthread_cond_signal(&FirstSouth);  //active the left car to cross
	int* Cur;
	Cur = &CurID_East;
	//lock the queue and Cur varialbe and update them
	pthread_mutex_lock(&EastMutex);
	pthread_mutex_lock(&CurEastMutex);
	*Cur = pop(&East);  //the next car in this dir queue will be the crossing car
	pthread_mutex_unlock(&CurEastMutex);
	pthread_mutex_unlock(&EastMutex);
	//sleep to wait all cars have been pushed into queue, then it will get broadcast
	usleep(100);
	pthread_cond_broadcast(&EastCond);  //broadcast to all cars in the queue
	return NULL;
}

void* CarFromW(void* arg)  //same with CarFromN
{
	int CarID = *((int*)arg);  //get the car id
	pthread_mutex_lock(&WestMutex);
	while (CurID_West != CarID)
	{
		pthread_cond_wait(&WestCond, &WestMutex);  //wait until the car id equals Cur id(should cross)
	}
	pthread_mutex_unlock(&WestMutex);
	//try to get the Resources
	pthread_mutex_lock(&Mutex_SW);  //lock or wait to lock it
	printf("car %d from W arrives at crossing\n", CarID);
	WestHasCar = true;
	//update the number of Resources
	pthread_mutex_lock(&EmptyMutex);
	Resources--;  //record the remaind Resources to judge deadlock
	pthread_mutex_unlock(&EmptyMutex);
	//entered
	DeadlockDealing(W_D);  //detect the deadlock
									  //when existing car crossing in the right, wait for the signal
	pthread_mutex_lock(&FirstWestMutex);
	while (SouthHasCar)		pthread_cond_wait(&FirstWest, &FirstWestMutex);
	pthread_mutex_unlock(&FirstWestMutex);
	//leaving
	pthread_mutex_lock(&Mutex_SE);  //lock the secend Resources
	pthread_mutex_unlock(&Mutex_SW);  //unlock the first Resources
	printf("car %d from W leaving crossing\n", CarID);
	pthread_mutex_unlock(&Mutex_SE);  //unlock the second Resources
	pthread_mutex_lock(&EmptyMutex);
	Resources++;  //update the number of Resources
	pthread_mutex_unlock(&EmptyMutex);
	DeadlockOver = true;  //deadlock must be over because there is car crossing
	pthread_cond_signal(&DeadlockOverCond);  //active the car waiting for deadlock
	WestHasCar = false;  //now the car has left and the dir doesn't have car crossing				
	pthread_mutex_lock(&FirstNorthMutex); //the left car can go to prevent starvation
	First_West = true;
	pthread_mutex_unlock(&FirstNorthMutex);
	pthread_cond_signal(&FirstNorth);  //active the left car to cross
	int* Cur;
	Cur = &CurID_West;
	//lock the queue and Cur varialbe and update them
	pthread_mutex_lock(&WestMutex);
	pthread_mutex_lock(&CurWestMutex);
	*Cur = pop(&West);  //the next car in this dir queue will be the crossing car
	pthread_mutex_unlock(&CurWestMutex);
	pthread_mutex_unlock(&WestMutex);
	//sleep to wait all cars have been pushed into queue, then it will get broadcast
	usleep(100);
	pthread_cond_broadcast(&WestCond);  //broadcast to all cars in the queue
	return NULL;
}

int main()
{
	char input[MAX];
	printf("Please input cars:\n");
	scanf("%s", input);
	int CarNumber = strlen(input);
	if (CarNumber == 0)
	{
		printf("No input...\n");
		return 0;
	}
	else printf("Total Car Number:%d\n", CarNumber);
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
	pthread_cond_init(&FirstNorth, NULL);
	pthread_cond_init(&FirstSouth, NULL);
	pthread_cond_init(&FirstEast, NULL);
	pthread_cond_init(&FirstWest, NULL);
	pthread_mutex_init(&Mutex_SE, NULL);
	pthread_mutex_init(&Mutex_NE, NULL);
	pthread_mutex_init(&Mutex_NW, NULL);
	pthread_mutex_init(&Mutex_SW, NULL);
	pthread_mutex_init(&CurNorthMutex, NULL);
	pthread_mutex_init(&CurSouthMutex, NULL);
	pthread_mutex_init(&CurEastMutex, NULL);
	pthread_mutex_init(&CurWestMutex, NULL);

	pthread_mutex_init(&EmptyMutex, NULL);

	pthread_mutex_init(&DeadlockOverMutex, NULL);
	pthread_cond_init(&DeadlockOverCond, NULL);
	//initialize all the queue

	pthread_t Cars[CarNumber];  //store pid for all cars
	init(&North);init(&South);init(&East);init(&West);
	int i;
	int id[MAX + 1];
	for (i = 1; i<MAX; i++)	id[i] = i;
	//create thread for each car
	for (i = 1; i <= CarNumber + 1; i++)
	{
		switch (input[i - 1])
		{
		case 'n':  //enqueue into North queue , create thread;
			enqueue(&North, i);
			pthread_create(&Cars[i], NULL, CarFromN, (void*)(&id[i]));
			usleep(1);  //improve coCur
			break;
		case 's':  //enqueue into South queue , create thread;
			enqueue(&South, i);
			pthread_create(&Cars[i], NULL, CarFromS, (void*)(&id[i]));
			usleep(1);
			break;
		case 'e':  //enqueue into East queue , create thread;
			enqueue(&East, i);
			pthread_create(&Cars[i], NULL, CarFromE, (void*)(&id[i]));
			usleep(1);
			break;
		case 'w':  //enqueue into West queue , create thread;
			enqueue(&West, i);
			pthread_create(&Cars[i], NULL, CarFromW, (void*)(&id[i]));
			usleep(1);
			break;
		case 'N':  //same to 'n'
			enqueue(&North, i);
			pthread_create(&Cars[i], NULL, CarFromN, (void*)(&id[i]));
			usleep(1);  //improve coCur
			break;
		case 'S':  //same to 's'
			enqueue(&South, i);
			pthread_create(&Cars[i], NULL, CarFromS, (void*)(&id[i]));
			usleep(1);
			break;
		case 'E':  //same to 'e'
			enqueue(&East, i);
			pthread_create(&Cars[i], NULL, CarFromE, (void*)(&id[i]));
			usleep(1);
			break;
		case 'W':  //same to 'w'
			enqueue(&West, i);
			pthread_create(&Cars[i], NULL, CarFromW, (void*)(&id[i]));
			usleep(1);
			break;
		default:
			if (input[i - 1] == '\0') break;
			printf("%c is an Invaild direction!\n", input[i - 1]);
			return 0;
		}
	}

	CrossOpen();  //enable the crossroad system

				  //wait for all sub thread ends
	for (i = 1; i <= CarNumber; i++)  	pthread_join(Cars[i], NULL);

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
	pthread_cond_destroy(&FirstNorth);
	pthread_cond_destroy(&FirstSouth);
	pthread_cond_destroy(&FirstEast);
	pthread_cond_destroy(&FirstWest);
	pthread_mutex_destroy(&Mutex_SE);
	pthread_mutex_destroy(&Mutex_NE);
	pthread_mutex_destroy(&Mutex_NW);
	pthread_mutex_destroy(&Mutex_SW);
	pthread_mutex_destroy(&CurNorthMutex);
	pthread_mutex_destroy(&CurSouthMutex);
	pthread_mutex_destroy(&CurEastMutex);
	pthread_mutex_destroy(&CurWestMutex);
	pthread_mutex_destroy(&EmptyMutex);
	pthread_mutex_destroy(&DeadlockOverMutex);
	pthread_cond_destroy(&DeadlockOverCond);

}
