#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMELTS 16384

int prime(int n)
{
	int ret=1, i;

	for(i=2; i<=(int) sqrt(n) && ret; i++)
		ret=n % i;

	return ret;
}

int main()
{
	int data[NUMELTS]; 
	
	//create pipe
	int fd[2];
	pipe(fd);

	//number of prime number from child
	int numPrimeChild;

	//generate random numbers
	srand (time(NULL));

	for(int i=0; i<NUMELTS; i++)
		data[i]=(int) (((double) rand() / (double) RAND_MAX) * 10000);

	if (fork() != 0) { //parent

		//number of prime number from parent
		int numPrimeParent = 0;

		//total number of prime number
		int totalPrime = 0;

		//calculate number of prime from parent
		for(int i=0; i<NUMELTS/2; i++) {
			if (prime(data[i])) 
				numPrimeParent++;
		}	


		close(fd[1]);//parent will read from pipe

		while(1) {
			//if pipe is not empty
			if (read(fd[0], &numPrimeChild, sizeof(numPrimeChild)) > 0) {
				//print the results
				printf("Number of prime number from parent is %d\n", numPrimeParent);
				printf("Number of prime number from child is %d\n", numPrimeChild);
				totalPrime = numPrimeParent + numPrimeChild;//update total prime
				printf("Total number of prime number is %d\n",totalPrime);
				break;
			}
		}

		close(fd[1]);

	} else {//child
		close(fd[0]);

		//calculate number of prime from child
		for(int i=NUMELTS/2; i<NUMELTS; i++) {
			if(prime(data[i]))
				numPrimeChild++;
		}	

		//write number of prime from child to pipe
		write(fd[1], &numPrimeChild, sizeof(numPrimeChild));

		close(fd[1]);
	}

}
