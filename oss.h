#include <sys/types.h>
//represents shared memory Operating system clock
struct Sys_Time {
int seconds;//current second
int nanoseconds; //current nanosecond
int rate;//increment rate
};
//process table
struct PCB {
	int occupied;//is or is not working (1/0)
	pid_t pid; //worker process id
	int startSeconds;//second launched
	int startNano; //nanosecond launched
};
typedef struct msgbuffer {
long mtype;
int Data;
} msgbuffer;
//help information
void Help();

//parses arguments to get -n value (workerAmount), -s value (workerSimLimit), it value for (workerTimeLimit) 
void ArgumentParser(int argc, char** argv, int* workerAmount,int* workerSimLimit, int* workerTimeLimit);

//validates to make sure arg input is grater than 0
int ValidateInput(int workerAmount, int workerSimLimit, int workerTimeLimit);

//sets up shared memory location and ties the address to are os's system clock
int StartSystemClock(struct Sys_Time **Clock);

//detaches shared memory from systemc clock and removes shared memory location
void StopSystemClock(struct Sys_Time *Clock, int sharedMemoryId);

//increments clock
void RunSystemClock(struct Sys_Time *Clock);

//generates random time workers will work 
//seconds will be between 0 - t
//nanoseconds will be 0, 1/4, 1/2, or 3/4 a second
void GenerateWorkTime(int timeLimit, int* sec, int* nanosec);

//handles how many workers are laucnhed, tracks the works in process table, and controls how long workers work
void WorkerHandler(int workerAmount, int workerSimLimit, int workerTimeLimit, struct Sys_Time* OsClock, struct PCB table[]);

//nonblocking await to see if a worker is done
//return 0 if no workers done
//returns id of worker done if a worker is done
int AwaitWorker();

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
void UpdateFinishedWorkerInProcessTable(struct PCB table[], pid_t workerId);
//prints process table
void PrintProcessTable(struct PCB processTable[],int curTimeSeconds, int curTimeNanoseconds);
//constructor for process table
void BuildProcessTable(struct PCB table[]);
//returns 0 if 1/2 second has passed, else it return 1
int HasHalfSecPassed(int nanoseconds);

int ConstructMsgQueue();

void DestructMsgQueue(int msqid);

int SendAndRecieveStatusMsg(int msqid, pid_t worker_id);

pid_t GetNxtWorkerToMsg(struct PCB processTable[], int* curIndex);

const int TABLE_SIZE = 20;

const int MSG_SYSTEM_KEY = 5303;

const int SYS_TIME_SHARED_MEMORY_KEY = 63131;
