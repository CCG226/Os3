struct Sys_Time {
int seconds;
int nanoseconds;
int rate;

};
//work printing status task done here
void Task(int workerSeconds, int workerNanoseconds);
//accesses shared memory resource for system clock
struct Sys_Time* AccessSystemTime();

//detaches worker from shared memory system clock resource after terminating 
void DisposeAccessToShm(struct Sys_Time* clock);

//parses through arguments sent to worker (random time seoconds and nanoseconds value)
void ArgumentParser(int argc, char** argv, int* seconds, int* nanoseconds);

//calculates Termination time of worker based on randomly generated parameter time plus the current system clock time
//results stored in termSecond and termNanosecond which are passed by ref
void GenerateTerminationTime(int currentSecond, int currentNanosecond, int* termSecond, int* termNanosecond, int secondsToRun, int nanosecondsToRun);

const int SYS_TIME_SHARED_MEMORY_KEY = 63131;
