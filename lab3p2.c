#include <stdio.h>
#include <pthread.h>

int ctr=0;
pthread_t thread[10];

void *child(void *t)
{
	printf("I am child %d. Ctr=%d\n", t, ctr);
	ctr++;
	//printf("here");
	pthread_exit(NULL);
}

int main()
{
	int i;

	ctr=0;

	for(i=0; i<10; i++) {
		int err =pthread_create(&(thread[i]), NULL, child, (void *)i);
		//printf("%d\n",err);
	}

	for (i=0; i<10; i++)
		pthread_join(thread[i], NULL);

	printf("Value of ctr=%d\n", ctr);
	return 0;
}
