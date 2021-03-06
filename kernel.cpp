#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "llist.h"
#include "prioll.h"
#include "kernel.h"

/*

   STUDENT 1 NUMBER:
   STUDENT 1 NAME:

   STUDENT 2 NUMBER:
   STUDENT 2 NAME:

   STUDENT 3 NUMBER:
   STUDENT 3 NAME:

 */

/* OS variables */

// Current number of processes
int procCount;

// Current timer tick
int timerTick=0;
int currProcess, currPrio;

#if SCHEDULER_TYPE == 0

/* Process Control Block for LINUX scheduler*/
typedef struct tcb
{
	int procNum;
	int prio;
	int quantum;
	int timeLeft;
} TTCB;

#elif SCHEDULER_TYPE == 1

/* Process Control Block for RMS scheduler*/

typedef struct tcb
{
	int procNum;
	int timeLeft;
	int deadline;
	int c;
	int p;
} TTCB;

#endif


TTCB processes[NUM_PROCESSES];

#if SCHEDULER_TYPE == 0

// Lists of processes according to priority levels.
TNode *queueList1[PRIO_LEVELS];
TNode *queueList2[PRIO_LEVELS];

// Active list and expired list pointers
TNode **activeList = queueList1;
TNode **expiredList = queueList2;

#elif SCHEDULER_TYPE == 1

// Ready queue and blocked queue
TPrioNode *readyQueue, *blockedQueue;

// This stores the data for pre-empted processes.
TPrioNode *suspended; // Suspended process due to pre-emption

// Currently running process
TPrioNode *currProcessNode; // Current process
#endif


#if SCHEDULER_TYPE == 0

// Searches the active list for the next priority level with processes
int findNextPrio(int currPrio)
{
	int i;

	for(i=0; i<PRIO_LEVELS; i++)
		if(activeList[i] != NULL)
			return i;

	return -1; 

}
int linuxScheduler()
{
	//for the first timerTick
	if (timerTick == 0) {

		return currProcess;

	//if currProcess is on last quantum
	} else if (processes[currProcess].quantum < 2) {
		//if there are no more processes in active list
		if (findNextPrio(currPrio) == -1) {
			
			//restore the quantum of the process and insert it into the expired list
			processes[currProcess].quantum = ((PRIO_LEVELS - 1) - processes[currProcess].prio) * QUANTUM_STEP + QUANTUM_MIN;
			insert(&expiredList[processes[currProcess].prio],currProcess,processes[currProcess].quantum);
			destroy(&activeList[currPrio]);

			//make the expired list the active list
			TNode **temp = activeList;
			activeList = expiredList;
			expiredList = temp;
			int nextPrio = findNextPrio(0);

			//clear the new expired list
			int i;
			for(i=0; i<PRIO_LEVELS; i++)
				expiredList[i]=NULL;

			printf("\n****** SWAPPED LISTS ****\n\n");

			//update nextPID and currPrio
			int nextPID = remove(&activeList[nextPrio]);
			currPrio = nextPrio;
			return nextPID;

		//if the active list is not yet empty
		// set new process from active list
		} else {
			//update nextPID
			int nextPrio = findNextPrio(currPrio);
			int nextPID = remove(&activeList[nextPrio]);

			// no more processes with priority == currPrio
			if (nextPrio != currPrio)
				destroy(&activeList[currPrio]);

			//restore the quantum of the current process and insert it into the expired list
			processes[currProcess].quantum = ((PRIO_LEVELS - 1) - processes[currProcess].prio) * QUANTUM_STEP + QUANTUM_MIN;
			insert(&expiredList[processes[currProcess].prio],currProcess,processes[currProcess].quantum);

			// update currPrio
			currPrio = nextPrio;
			return nextPID;
		}
	//currProcess still has quantum to run
	} else {
		//decrements the quantum
		processes[currProcess].quantum--;
		return currProcess;
	}
	return 0;
}
#elif SCHEDULER_TYPE == 1

int RMSScheduler()
{	
	//first move everything that are ready to the ready queue
	TPrioNode *node = checkReady(blockedQueue, timerTick);
	TTCB *process;

	while(node != NULL) {
		process = &processes[node->procNum];
		process->deadline = process->deadline + process->p;
		process->timeLeft = process->c;
		prioRemoveNode(&blockedQueue, node);
		prioInsertNode(&readyQueue, node);
		node = checkReady(blockedQueue, timerTick);
	}


	//first node in ready queue
	TPrioNode *firstInReadyNode = peek(readyQueue);
	
	//if currProcessNode is not NULL (meaning previous round was not idle)
	if (currProcessNode != NULL) { 
		//get the current process
		TTCB *currProcess = &processes[currProcessNode->procNum];

		//if deadline was missed
		if (timerTick >= currProcess->deadline) {
			//if time left is not extended yet, extend time left by c
			if (currProcess->timeLeft < currProcess->c) {
				currProcess->timeLeft = currProcess->timeLeft + currProcess->c;
			}
			
			//if time left decreases to c(previous deadline was cleared), update deadline
			if (currProcess->timeLeft == currProcess->c) {
				currProcess->deadline = currProcess->deadline + currProcess->p;
			}

		}

		//if ready queue is not empty
		if (firstInReadyNode != NULL) {

			//if current runing process has finished running for the current run
			if (currProcess->timeLeft <= 0) {

				prioInsertNode(&blockedQueue, currProcessNode);
				currProcessNode = prioRemoveNode(&readyQueue, firstInReadyNode);


			//if the first process in the ready queue has higher priority than current process
			} else if (firstInReadyNode->prio < currProcessNode->prio) {

				prioInsertNode(&readyQueue, currProcessNode);
				currProcessNode = prioRemoveNode(&readyQueue, firstInReadyNode);
				suspended = currProcessNode;
				printf("\n--- PRE-EMPTION ---\n\n");
			}
			currProcess = &processes[currProcessNode->procNum];
			currProcess->timeLeft = currProcess->timeLeft - 1;
			return currProcessNode->procNum;
	
		//if ready queue is emtpy
		//if current process still have timeLeft
		} else if (currProcess->timeLeft > 0) {
			
			currProcess->timeLeft = currProcess->timeLeft - 1;
			return currProcessNode->procNum;
		
		//if current process has finished running for the current run
		} else if (currProcess->timeLeft <= 0) {
			prioInsertNode(&blockedQueue, currProcessNode);
			currProcessNode = NULL;
			return -1;
		}

	//if previous round was idle
	} else {
		
		//if ready queue is no longer empty
		if (firstInReadyNode != NULL ) {
			currProcessNode = prioRemoveNode(&readyQueue, firstInReadyNode);
			TTCB *currProcess = &processes[currProcessNode->procNum];
			currProcess->timeLeft = currProcess->timeLeft -1;
			return currProcessNode->procNum;
		}
		return -1;
	}
}

#endif

void timerISR()
{

#if SCHEDULER_TYPE == 0
	currProcess = linuxScheduler();
#elif SCHEDULER_TYPE == 1
	currProcess = RMSScheduler();
#endif

#if SCHEDULER_TYPE == 0
	static int prevProcess=-1;

	// To avoid repetitiveness for hundreds of cycles, we will only print when there's
	// a change of processes
	if(currProcess != prevProcess)
	{

		// Print process details for LINUX scheduler
		printf("Time: %d Process: %d Prio Level: %d Quantum : %d\n", timerTick, processes[currProcess].procNum+1,
				processes[currProcess].prio, processes[currProcess].quantum);
		prevProcess=currProcess;
	}
#elif SCHEDULER_TYPE == 1

	// Print process details for RMS scheduler

	printf("Time: %d ", timerTick);
	if(currProcess == -1)
		printf("---\n");
	else
	{
		// If we have busted a processe's deadline, print !! first
		int bustedDeadline = (timerTick >= processes[currProcess].deadline);

		if(bustedDeadline)
			printf("!! ");

		printf("P%d Deadline: %d", currProcess+1, processes[currProcess].deadline);

		if(bustedDeadline)
			printf(" !!\n");
		else
			printf("\n");
	}

#endif

	// Increment timerTick. You will use this for scheduling decisions.
	timerTick++;
}

void startTimer()
{
	// In an actual OS this would make hardware calls to set up a timer
	// ISR, start an actual physical timer, etc. Here we will simulate a timer
	// by calling timerISR every millisecond

	int i;

#if SCHEDULER_TYPE==0
	int total = processes[currProcess].quantum;

	for(i=0; i<PRIO_LEVELS; i++)
	{
		total += totalQuantum(activeList[i]);
	}

	for(i=0; i<NUM_RUNS * total; i++)
	{
		timerISR();
		usleep(1000);
	}
#elif SCHEDULER_TYPE==1

	// Find LCM of all periods
	int lcm = prioLCM(readyQueue);

	for(i=0; i<NUM_RUNS*lcm; i++)
	{
		timerISR();
		usleep(1000);
	}
#endif
}

void startOS()
{
#if SCHEDULER_TYPE == 0
	// There must be at least one process in the activeList
	currPrio = findNextPrio(0);

	if(currPrio < 0)
	{
		printf("ERROR: There are no processes to run!\n");
		return;
	}

	// set the first process
	currProcess = remove(&activeList[currPrio]);

#elif SCHEDULER_TYPE == 1
	currProcessNode = prioRemove(&readyQueue);
	currProcess = currProcessNode->procNum;
#endif

	// Start the timer
	startTimer();

#if SCHEDULER_TYPE == 0
	int i;

	for(i=0; i<PRIO_LEVELS; i++)
		destroy(&activeList[i]);

#elif SCHEDULER_TYPE == 1
	prioDestroy(&readyQueue);
	prioDestroy(&blockedQueue);

	if(suspended != NULL)
		free(suspended);
#endif
}

void initOS()
{
	// Initialize all variables
	procCount=0;
	timerTick=0;
	currProcess = 0;
	currPrio = 0;
#if SCHEDULER_TYPE == 0
	int i;

	// Set both queue lists to NULL
	for(i=0; i<PRIO_LEVELS; i++)
	{
		queueList1[i]=NULL;
		queueList2[i]=NULL;
	}
#elif SCHEDULER_TYPE == 1

	// Set readyQueue and blockedQueue to NULL
	readyQueue=NULL;
	blockedQueue=NULL;

	// The suspended variable is used to store
	// which process was pre-empted.
	suspended = NULL;
#endif

}

#if SCHEDULER_TYPE == 0

// Returns the quantum in ms for a particular priority level.
int findQuantum(int priority)
{
	return ((PRIO_LEVELS - 1) - priority) * QUANTUM_STEP + QUANTUM_MIN;
}

// Adds a process to the process table
int addProcess(int priority)
{
	if(procCount >= NUM_PROCESSES)
		return -1;

	// Insert process data into the process table
	processes[procCount].procNum = procCount;
	processes[procCount].prio = priority;
	processes[procCount].quantum = findQuantum(priority);
	processes[procCount].timeLeft = processes[procCount].quantum;

	// Add to the active list
	insert(&activeList[priority], processes[procCount].procNum, processes[procCount].quantum);
	procCount++;
	return 0;
}
#elif SCHEDULER_TYPE == 1

// Adds a process to the process table
int addProcess(int p, int c)
{
	if(procCount >= NUM_PROCESSES)
		return -1;

	// Insert process data into the process table
	processes[procCount].p = p;
	processes[procCount].c = c;
	processes[procCount].timeLeft=c;
	processes[procCount].deadline = p;

	// And add to the ready queue.
	prioInsert(&readyQueue, procCount, p, p);
	procCount++;
	return 0;
}


#endif


