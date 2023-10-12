#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "worker.h"
#include <errno.h>

//Author: Connor Gilmore
//Purpose: takes in a numeric argument
//Program will take in two arguments that represent a random time in seconds and nanoseconds
//are program will then work for that amoount of time by printing status messages
//are worker will constantly check the os's system clock to see if it has reached the end of its work time, if it has it will terminate

int main(int argc, char** argv)
{

	int seconds = 0;
	int nanoseconds = 0;

	//get rand time arguments and store them in seconds and nanoseconds vars using pass by refernce
	ArgumentParser(argc, argv, &seconds, &nanoseconds);

	Task(seconds, nanoseconds);//do worker task


	return EXIT_SUCCESS;
}
void ArgumentParser(int argc, char** argv, int* seconds, int* nanoseconds)
{//parse the random time arguments 
	if(argc != 3)
	{
		printf("\nWorker Process Should Only Be Launched By Os!\n");
		exit(1);
	}
	*(seconds) = atoi(argv[1]);

	*(nanoseconds) = atoi(argv[2]);

	return;

}
int AccessMsgQueue()
{
	//access message queue create by workinh using key constant
	int msqid = -1;
	if((msqid = msgget(MSG_SYSTEM_KEY, 0777)) == -1)
	{
		perror("Failed To Join Os's Message Queue.\n");
		exit(1);
	}
	return msqid;
}
struct Sys_Time* AccessSystemTime()
{//access system clock from shared memory (read-only(
	int shm_id = shmget(SYS_TIME_SHARED_MEMORY_KEY, sizeof(struct Sys_Time), 0444);
	if(shm_id == -1)
	{
		perror("Failed To Access System Clock");

		exit(1);	
	}
	return (struct Sys_Time*)shmat(shm_id, NULL, 0);

}
void DisposeAccessToShm(struct Sys_Time* clock)
{//detach system clock from shared memory
	if(shmdt(clock) == -1)
	{
		perror("Failed To Release Shared Memory Resources.\n");
		exit(1);
	}

}
void GenerateTerminationTime(int currentSecond, int currentNanosecond, int* termSecond, int* termNanosecond, int secondsToRun, int nanosecondsToRun)
{
	//calculates termination time by adding starting time to random amount of time worker must rin adn storing results in termSecond and termNanosecond ptrs
	*(termSecond) = currentSecond + secondsToRun;

	if(currentNanosecond + nanosecondsToRun >= 1000000000)
	{
		*(termNanosecond) = (currentNanosecond + nanosecondsToRun) - 1000000000;
		*(termSecond) = *(termSecond) + 1;
	}
	else {
		*(termNanosecond) = currentNanosecond + nanosecondsToRun;
	}


}	
//workers console print task
void Task(int workerSeconds, int workerNanoseconds)
{
	int msqid = AccessMsgQueue();
	//get parent and worker ids to print
	pid_t os_id = getppid();
	pid_t worker_id = getpid();
	//access os's system clock via shared memory and read from it
	struct Sys_Time* Clock = AccessSystemTime();
	//stores termination time
	int termSecond = 0;
	int termNanosecond = 0;
	//calculates termination time, passes above variables by ref
	GenerateTerminationTime(Clock->seconds, Clock->nanoseconds, &termSecond, &termNanosecond, workerSeconds, workerNanoseconds);
	//print just starting msh
	printf("WORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d \n--Just Starting  \n",worker_id,os_id,Clock->seconds,Clock->nanoseconds, termSecond, termNanosecond);

	//stores current time at start, to determine if a second goes by
	int initialNanoseconds = Clock->nanoseconds;
	int prevSecond = Clock->seconds;

	//determines of worker can keep working, if 0 worker must terminate
	int status = 1;

	//tracks how many seconds to go by
	int secondCounter = 0;

	msgbuffer msg;

	//while status is NOT 0, keep working
	while(status != 0)
	{ 

		//a second has gone by on the system clock	
		if(Clock->seconds == prevSecond + 1 && Clock->nanoseconds >= initialNanoseconds)	
		{
			prevSecond = Clock->seconds;
			secondCounter++;
			printf("WORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d \n--%d seconds have passsed since starting  \n",worker_id,os_id,Clock->seconds,Clock->nanoseconds, termSecond, termNanosecond, secondCounter);
		}
		//wait for os to send a request for status information on wheather or not this worker is done working
		AwaitOsRequestForStatusMsg(msqid, &msg);
		//if system clock surpasses termination time
		if(Clock->seconds >= termSecond && Clock->nanoseconds >= termNanosecond)
		{//set status to 0 meaning work is done and needs to terminate
			status = 0;

		}
		else
		{//else worker still has time to worker
			status = 1;	
		}
		//send status varable indicating status of worker back to os
		SendStatusResponseMsg(msqid, &msg, status);

	}
	//worker saying its done
	printf("WORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d \n--Terminating  \n",worker_id,os_id,Clock->seconds,Clock->nanoseconds, termSecond, termNanosecond);
	//detach workers read only system clock from shared memory
	DisposeAccessToShm(Clock);

}
void AwaitOsRequestForStatusMsg(int msqid, msgbuffer *msg)
{
	//blocking wait for message from os requesting status info
	//os communicates to this worker via its pid (4th param)	
	if(msgrcv(msqid, msg, sizeof(msgbuffer), getpid(), 0) == -1)
	{
		printf("Failed To Get Message Request From Os. Worker: %d\n", getpid());
		fprintf(stderr, "errno: %d\n", errno);
		exit(1);
	}

}
void SendStatusResponseMsg(int msqid, msgbuffer *msg, int status)
{//send status update via integer value Data about if this worker is gonna terminate
	//status == 0 if worker is gonna terminate
	msg->Data = status;
	msg->mtype = 1;
	//send message back to os
	if(msgsnd(msqid, msg, sizeof(msgbuffer), 0) == -1) {
		perror("Failed To Generate Response Message Back To Os.\n");
		exit(1);
	}

}
