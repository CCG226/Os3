#include <sys/types.h>
#include <stdio.h>
//represents shared memory Operating system clock
struct Sys_Time {
int seconds;//current second
int nanoseconds; //current nanosecond
int rate;//increment rate
};
//process table
struct PCB {
	int state;//is or is not working (1/0)
	pid_t pid; //worker process id
	int startSeconds;//second launched
	int startNano; //nanosecond launched
};
//message data sent through message queue
typedef struct msgbuffer {
long mtype;//who to send to
int Data;//data to send (status information) (0 means worker is complete, 1 means worker is still working)
} msgbuffer;
//help information
void Help();

//parses arguments to get -n value (workerAmount), -s value (workerSimLimit), -t value for (workerTimeLimit), -f value for logfile name 
void ArgumentParser(int argc, char** argv, int* workerAmount,int* workerSimLimit, int* workerTimeLimit,char** fileName);

//validates argument input
int ValidateInput(int workerAmount, int workerSimLimit, int workerTimeLimit, char* fileName);

//sets up shared memory location and ties the address to are os's system clock
int StartSystemClock(struct Sys_Time **Clock);

//detaches shared memory from systemc clock and removes shared memory location
void StopSystemClock(struct Sys_Time *Clock, int sharedMemoryId);

//increments clock
void RunSystemClock(struct Sys_Time *Clock);

//generates random time workers will work 
//seconds will be between 0 - t
//nanoseconds will be random value = (0 - 1e-9)
//sec and nanosec passed by reference using ptrs so no return value
//timeLimit = -t value
void GenerateWorkTime(int timeLimit, int* sec, int* nanosec);

//handles how many workers are laucnhed, tracks the works in process table, and controls how long workers work
//workAmount = n value, workerSimLimit = -s value, workerTimeLimit = -t value, logFile = -f value, OsClock = shared memory clock, table is process pcb table 
void WorkerHandler(int workerAmount, int workerSimLimit, int workerTimeLimit,char* logFile, struct Sys_Time* OsClock, struct PCB table[]);

//logs a particular message to logfile dependent on msgType value using logger
//if msgType is 'Terminating' print message about worker going to terminate soon
//if msgType is 'Sending' print message about worker sending status message to os
//if msgType is 'Recieving' print message about os sending request for status update to particular worker
void LogMessage(FILE** logger,const char* msgType,int workerIndex,pid_t workerId,int curSec,int curNano);

//nonblocking await to see if a worker is done
//return 0 if no workers done
//returns id of worker done if a worker is done
int AwaitWorker(pid_t worker_Id);

//launches worker processes
//amount launched at once is based on simLimit, adds workers id & start clock time  to process table post launch, randomly generates time to work with timeLimit 
void WorkerLauncher(int simLimit,int timeLimit, struct PCB table[], struct Sys_Time* clock);
//starts alarm clock for 60 seconds
void Begin_OS_LifeCycle();
//kills os after 60 second event signal is triggered
void End_OS_LifeCycle();
//adds worker data to process table  after worker is launched including its pid and time it was laucnhed
void AddWorkerToProcessTable(struct PCB table[], pid_t workerId, int secondsCreated, int nanosecondsCreated);
//sets worker who has finished occupied value to 0 indicating worker is done
void UpdateWorkerStateInProcessTable(struct PCB table[], pid_t workerId, int state);
//gets the index value of a particular woker from the process table using workers pid
int GetWorkerIndexFromProcessTable(struct PCB table[], pid_t workerId);
//prints process table
void PrintProcessTable(struct PCB processTable[],int curTimeSeconds, int curTimeNanoseconds);
//constructor for process table
void BuildProcessTable(struct PCB table[]);
//returns 0 if 1/2 second has passed, else it return 1
int HasHalfSecPassed(int nanoseconds);
//creates message queue and returns message queues id
int ConstructMsgQueue();
//destroys message queue via id 
void DestructMsgQueue(int msqid);
//sends request to specific worker using workers pid via message queue
//and recieves a response about workers status to see if worker is done or not
int SendAndRecieveStatusMsg(int msqid, pid_t worker_id);
//determines next worket os will message in the system
//using the process table and a index value passed by reference to track previous worker messaged
pid_t GetNxtWorkerToMsg(struct PCB processTable[], int* curIndex);

//proccess table state values, 1 means process is running, 0 means process is terminated
const int STATE_RUNNING = 1;

const int STATE_TERMINATED = 0;

const int TABLE_SIZE = 20;

const int MSG_SYSTEM_KEY = 5303;

const int SYS_TIME_SHARED_MEMORY_KEY = 63131;
