#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "oss.h"
//Author: Connor Gilmore
//Purpose: Program task is to handle and launch x amount of worker processes 
//The Os will terminate no matter what after 60 seconds
//the OS operates its own system clock it stores in shared memory to control worker runtimes
//the OS stores worker status information in a process table
//takes arguments: 
//-h for help,
//-n for number of child processes to run
//-s for number of child processes to run simultanously
//-t max range value for random amount of time workers will work for 
void Begin_OS_LifeCycle()
{
	printf("\n\nBooting Up Operating System...\n\n");	

	signal(SIGALRM, End_OS_LifeCycle);

	alarm(60);

}

void End_OS_LifeCycle()
{
	printf("\n\nOS Has Reached The End Of Its Life Cycle And Has Terminated.\n\n");	
	exit(1);

}	

int main(int argc, char** argv)
{
	srand(time(NULL));

	//set 60 second timer	
	Begin_OS_LifeCycle();

	//process table
	struct PCB processTable[20];

	//assigns default values to processtable elements (constructor)
	BuildProcessTable(processTable);

	//stores shared memory system clock info (seconds passed, nanoseconds passed, clock speed(rate))
	struct Sys_Time *OS_Clock;

	//default values for argument variables are 0
	int workerAmount = 0;//-n proc value (number of workers to launch)
	int simultaneousLimit = 0;//-s simul value (number of workers to run in sync)
	int timeLimit = 0;//-t iter value (number to pass to worker as parameter)

	//stored id to access shared memory for system clock
	int shm_ClockId = -1;

	//pass workerAmount (-n), simultaneousLimit (-s),timeLimit(-t) by reference and assigns coresponding argument values from the command line to them
	ArgumentParser(argc, argv,&workerAmount, &simultaneousLimit, &timeLimit);

	//'starts' aka sets up shared memory system clock and assigns default values for clock 
	shm_ClockId = StartSystemClock(&OS_Clock);

	//handles how os laumches, tracks, and times workers
	WorkerHandler(workerAmount, simultaneousLimit, timeLimit, OS_Clock, processTable);

	//removes system clock from shared mem
	StopSystemClock(OS_Clock ,shm_ClockId);
	printf("\n\n\n");
	return EXIT_SUCCESS;

}
int StartSystemClock(struct Sys_Time **Clock)
{
	//sets up shared memory location for system clock
	int shm_id = shmget(SYS_TIME_SHARED_MEMORY_KEY, sizeof(struct Sys_Time), 0777 | IPC_CREAT);
	if(shm_id == -1)
	{
		printf("Failed To Create Shared Memory Location For OS Clock.\n");

		exit(1);
	}
	//attaches system clock to shared memory
	*Clock = (struct Sys_Time *)shmat(shm_id, NULL, 0);
	if(*Clock == (struct Sys_Time *)(-1)) {
		printf("Failed to connect OS Clock To A Slot In Shared Memory\n");
		exit(1);
	}
	//set default clock values
	//clock speed is 5000 nanoseconds per 'tick'
	(*Clock)->rate = 5000;
	(*Clock)->seconds = 0;
	(*Clock)->nanoseconds = 0;

	return shm_id;
}
void StopSystemClock(struct Sys_Time *Clock, int sharedMemoryId)
{ //detach shared memory segement from our system clock
	if(shmdt(Clock) == -1)
	{
		printf("Failed To Release Shared Memory Resources..");
		exit(1);
	}
	//remove shared memory location
	if(shmctl(sharedMemoryId, IPC_RMID, NULL) == -1) {
		printf("Error Occurred When Deleting The OS Shared Memory Segmant");
		exit(1);
	}
}
void RunSystemClock(struct Sys_Time *Clock) {
	//handles clock iteration / ticking
	if(Clock->nanoseconds < 1000000000)
	{
		Clock->nanoseconds += Clock->rate;	
	}
	if(Clock->nanoseconds == 1000000000)
	{ //nanoseconds reack 1,000,000,000 then reset nanoseconds and iterate clock
		Clock->seconds++;
		Clock->nanoseconds = 0;	 
	}

}


void ArgumentParser(int argc, char** argv, int* workerAmount, int* workerSimLimit, int* workerTimeLimit) {
	//assigns argument values to workerAmount, simultaneousLimit, workerArg variables in main using ptrs 
	int option;
	//getopt to iterate over CL options
	while((option = getopt(argc, argv, "hn:s:t:")) != -1)
	{
		switch(option)
		{//junk values (non-numerics) will default to 0 with atoi which is ok
			case 'h'://if -h
				Help();
				break;
			case 'n'://if -n int_value
				*(workerAmount) = atoi(optarg);
				break;
			case 's'://if -s int_value
				*(workerSimLimit) = atoi(optarg);
				break;
			case 't'://if t int_value
				*(workerTimeLimit) = atoi(optarg);
				break;
			default://give help info
				Help();
				break; 
		}

	}
	//check if arguments are valid 
	int isValid = ValidateInput(*workerAmount, *workerSimLimit, *workerTimeLimit);

	if(isValid == 0)
	{//valid arguments 
		return;	
	}
	else
	{
		exit(1);
	}
}
int ValidateInput(int workerAmount, int workerSimLimit, int workerTimeLimit)
{
	//acts a bool	
	int isValid = 0;
	//arguments cant be negative 
	if(workerAmount < 0 || workerSimLimit < 0 || workerTimeLimit < 0)
	{
		printf("\nInput Arguments Cannot Be Negative Number!\n");	 
		isValid = 1;	
	}
	//args cant be zero
	if(workerAmount < 1)
	{
		printf("\nNo Workers To Launch!\n");
		isValid = 1;
	}
	if(workerAmount > 20)
	{
		printf("\nTo many Workers!\n");
		isValid = 1;
	}
	if(workerSimLimit < 1)
	{
		printf("\nWe Need To Be Able To Launch At Least 1 Worker At A Time!\n");
		isValid = 1;
	}
	if(workerTimeLimit < 1)
	{
		printf("\nWorkers Need At Least A Second To Complete Their Task!\n");
		isValid = 1;
	}
	return isValid;

}
void WorkerHandler(int workerAmount, int workerSimLimit,int workerTimeLimit, struct Sys_Time* OsClock, struct PCB processTable[])
{

	//tracks amount of workers finished with task
	int workersComplete = 0;

	//tracks amount of workers left to be launched
	int workersLeft = 0;

	if(workerSimLimit > workerAmount)
	{
		//if (-s) values is greater than (-n) value, then launch all workers at once	
		WorkerLauncher(workerAmount,workerTimeLimit, processTable, OsClock);
		//no more workers left to be launched
		workersLeft = 0;

	}
	else
	{

		//workerAmount (-n) is greater than or equal to  WorkerSimLimit (-s), so launch (-s)  amount of workers	
		WorkerLauncher(workerSimLimit, workerTimeLimit, processTable, OsClock);	

		//subtract simultanoues limit from amount of workers (n) to get amount of workers left to launch
		workersLeft = workerAmount - workerSimLimit;

	}
	//keep looping until all workers (-n) have finished working
	while(workersComplete !=  workerAmount)
	{
		int WorkerFinishedId = AwaitWorker();//check to see if a worker is done, returns 0 if none are done, returns id of worker done if a worker is done

		if(WorkerFinishedId != 0)//if a 0 is returned, then no workers are currently done at them moement
		{//if a worker is finished
			workersComplete++;
			//worker no longer occupied
			UpdateFinishedWorkerInProcessTable(processTable, WorkerFinishedId);

			if(workersLeft != 0)
			{ //if we are allowed to, launch another worker
				WorkerLauncher(1,workerTimeLimit,processTable,OsClock);
				workersLeft--;
			}
		}	
		//increment clock
		RunSystemClock(OsClock);
		//if 1/2 second passed, print process table
		if(HasHalfSecPassed(OsClock->nanoseconds) == 0)
		{
			PrintProcessTable(processTable, OsClock->seconds, OsClock->nanoseconds);
		}	

	}


}
void GenerateWorkTime(int timeLimit, int* sec, int* nanosec)
{
	//randomly generate second (1 - t)
  	//randomly generate nanoseconds to be 0, 1/4 of a sec, 1/2 a sec, or 3/4 of a sec
     
	*(sec) = (rand() % timeLimit) + 1; // 1 - t
	*(nanosec) = (rand() % 4); // 0 -3

	//nanoseoncd will be a values 0 to 3
	//use that value to assign real nanoseconds value
	if(*(nanosec) == 0)
	{
		*(nanosec) = 0;
	}	
	if(*(nanosec) == 1)
	{
		*(nanosec) = 250000000;
	}	
	if(*(nanosec) == 2)
	{
		*(nanosec) = 500000000;
	}	
	if(*(nanosec) == 3)
	{
		*(nanosec) = 750000000;
	}	




}	

void WorkerLauncher(int simLimit, int timeLimit, struct PCB table[], struct Sys_Time* clock)
{
	//stores random time argument values workers will work	
	//	int secondsArg = 0;
	//iint nanosecondsArg = 0;

	//keep launching workers until limit (is reached
	//to create workers, first create a new copy process (child)
	//then replace that child with a worker program using execlp

	for(int i = 0 ; i < simLimit; i++)
	{
		
			//stores random time argument values workers will work	
		int secondsArg = 0;
		int nanosecondsArg = 0;


		//assign random time value to secondsArg and nanosecondsArg by passing by ref
		//nanoseconds will be 0, 1/4, 1/2, 3/4
		//seconds will be 0 - workerTimeLimit (-t)
		GenerateWorkTime(timeLimit, &secondsArg, &nanosecondsArg);


		//converting random nanosecond & second value to a string using sprintf method 	
		char secondsArgString[10];
		char nanosecondsArgString[20];
		sprintf(secondsArgString, "%d", secondsArg);
		sprintf(nanosecondsArgString, "%d", nanosecondsArg);


		pid_t newProcess = fork();

		if(newProcess < 0)
		{
			printf("\nFailed To Create New Process\n");
			exit(1);
		}
		if(newProcess == 0)
		{//child process (workers launching and running their software)

			execlp("./worker", "./worker", secondsArgString,nanosecondsArgString, NULL);

			printf("\nFailed To Launch Worker\n");
			exit(1);
		}
		//add worker launched to process table 
		AddWorkerToProcessTable(table,newProcess, clock->seconds, clock->nanoseconds);
	}	
	return;
}

int AwaitWorker()
{
	int stat = 0;	
	//nonblocking await to check if any workers are done
	int pid = waitpid(-1,&stat, WNOHANG);

	if(WIFEXITED(stat)) {

		if(WEXITSTATUS(stat) != 0)
		{
			exit(1);
		}	
	}
	//return pid of finished worker
	//pid will equal 0 if no workers done
	return pid;
}

int HasHalfSecPassed(int nanoseconds)
{//return 0, aka true if 1/2 second has passed
	if(nanoseconds == 500000000 || nanoseconds == 0)
	{
		return 0;
	}	
	return 1;
}

void AddWorkerToProcessTable(struct PCB table[], pid_t workerId, int secondsCreated, int nanosecondsCreated)
{
	for(int i = 0; i < TABLE_SIZE; i++)
	{
		//look for empty slot of process table bt checking each rows pid	
		if(table[i].pid == 0)
		{

			table[i].pid = workerId;
			table[i].startSeconds = secondsCreated;
			table[i].startNano = nanosecondsCreated;
			table[i].occupied = 1;
			//break out of loop, prevents assiginged this worker to every empty row
			break;
		}

	}	
}

void UpdateFinishedWorkerInProcessTable(struct PCB table[], pid_t workerId)
{ //using pid of finished worker, set its occupied to 0, to tell os worker is done
	
	for(int i = 0; i < TABLE_SIZE;i++)
	{
		if(table[i].pid == workerId)
		{
			table[i].occupied = 0;
		}
	}
}	
void BuildProcessTable(struct PCB table[])
{ 
	for(int i = 0; i < TABLE_SIZE;i++)
	{ //give default values to process table so it doesnt output junk
		table[i].pid = 0;
		table[i].occupied = 0;
		table[i].startSeconds = 0;
		table[i].startNano = 0;
	}	

}	


void PrintProcessTable(struct PCB processTable[],int curTimeSeconds, int curTimeNanoseconds)
{ //print to console 
	int os_id = getpid();	
	printf("\nOSS PID:%d SysClockS: %d SysclockNano: %d\n",os_id, curTimeSeconds, curTimeNanoseconds);
	printf("Process Table:\n");
	printf("Entry Occupied PID          StartS  StartN\n");
	for(int i = 0; i < TABLE_SIZE; i++)
	{
		printf("%d      %d        %d          %d        %d\n", i, processTable[i].occupied, processTable[i].pid, processTable[i].startSeconds, processTable[i].startNano);
	}
}

//help info
void Help() {
	printf("When executing this program, please provide three numeric arguments");
	printf("Set-Up: oss [-h] [-n proc] [-s simul] [-t time]");
	printf("The [-h] argument is to get help information");
	printf("The [-n int_value] argument to specify the amount of workers to launch");
	printf("The [-s int_value] argument to specify how many workers are allowed to run simultaneously");
	printf("The [-t int_value] argument will be used to generate a random time in seconds (1 - t) that the workers will work for");
	printf("Example: ./oss -n 5 -s 3 -t 7");
	printf("\n\n\n");
	exit(1);
}
