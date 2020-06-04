#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <semaphore.h> 
#include <time.h>

#define CAPACITY 3
#define LOOP 100



//Create bathroom struct
typedef struct restroom {
    int men_count;      //count of men 
    int women_count;    //count od women 
    sem_t men;          //men mutex
    sem_t women;        //women mutex
    sem_t n_men;        //men multiplex var (n.men allowed)
    sem_t n_women;      //women multiplex var(n.women allowed)
    sem_t avialable;    //if bathroom is avilable or not    
    sem_t turnstile;    //to prevent starvation  
}bathroom_t;

//Initialize the bathroom struct
static bathroom_t unisex_bathroom ={

    .men_count = 0,
    .women_count = 0, 
    .men = 1,
    .women = 1,
    .n_men = CAPACITY,
    .n_women = CAPACITY,
    .avialable = 1,
    .turnstile = 1
};


void
man_wants_to_enter(bathroom_t *unisex_bathroom)
{
    sem_wait(&unisex_bathroom->turnstile);
    sem_wait(&unisex_bathroom->men);
    unisex_bathroom->men_count++;
    if(unisex_bathroom->men_count==1)
        sem_wait(&unisex_bathroom->avialable);
    sem_post(&unisex_bathroom->men);
    sem_post(&unisex_bathroom->turnstile);
    sem_wait(&unisex_bathroom->n_men);
 
}

void 
man_leaves(bathroom_t *unisex_bathroom)
{
    sem_post(&unisex_bathroom->n_men);
    sem_wait(&unisex_bathroom->men);
    unisex_bathroom->men_count--;
    if(unisex_bathroom->men_count==0){
        sem_post(&unisex_bathroom->avialable);
        printf("\n\nEmpty");
    }
    sem_post(&unisex_bathroom->men);
 
}

void
woman_wants_to_enter(bathroom_t *unisex_bathroom)
{
    sem_wait(&unisex_bathroom->turnstile);
    sem_wait(&unisex_bathroom->women);
    unisex_bathroom->women_count++;
    if(unisex_bathroom->women_count==1)
        sem_wait(&unisex_bathroom->avialable);
    sem_post(&unisex_bathroom->women);
    sem_post(&unisex_bathroom->turnstile);
    sem_wait(&unisex_bathroom->n_women);
}

void 
woman_leaves(bathroom_t *unisex_bathroom)
{
    sem_post(&unisex_bathroom->n_women);
    sem_wait(&unisex_bathroom->women);
    unisex_bathroom->women_count--;
    if(unisex_bathroom->women_count==0){

        sem_post(&unisex_bathroom->avialable);
        printf("\n\nEmpty");
    }
    sem_post(&unisex_bathroom->women);
}

//men thread function 
static void*
men (void *restroom)
{
    bathroom_t *unisex_bathroom = (bathroom_t *) restroom; 
    for (int i = 0; i < LOOP ; i++){
        man_wants_to_enter(unisex_bathroom);
        printf("\n\nmen present");
        //go_caca(unisex_bathroom); 
        man_leaves(unisex_bathroom); 
    }
    pthread_exit(0);  
}

//women thread function
static void*
women (void *restroom)
{
    bathroom_t *unisex_bathroom = (bathroom_t *) restroom; 
    for (int i = 0; i < LOOP ; i++){
        woman_wants_to_enter(unisex_bathroom);
        printf("\n\nwomen present");
        //go_caca(unisex_bathroom); 
        woman_leaves(unisex_bathroom); 
    }
    pthread_exit(0);  
}
static int 
executioner (int n_men, int n_women)
{
   int total = n_men + n_women; 
   int errnum; // pthread_create() returns 0 if success, an error number otherwise; it will be used to print the error if any
   pthread_t threads[total]; //array of thread IDs

   for (int i = 0; i < total; i++)
   {
       errnum = pthread_create(&threads[i], NULL,
                i < n_men ? men : women, &unisex_bathroom); 
       if (errnum) {
           fprintf (stderr, "UNISEX_BATHROOM: proplem creating thread no. %d: %s\n",
                    i , strerror(errnum)); 
            return EXIT_FAILURE; 
       }
   }
   //randomize(threads, total);
   for (int i = 0 ; i < total; i++)
   {
       if (threads[i]) {
           errnum = pthread_join(threads[i], NULL); 
           if (errnum) {
               fprintf (stderr, "UNISEX_BATHROOM: proplem joining thread no. %d: %s\n",
                        i , strerror(errnum)); 
            return EXIT_FAILURE;
           }
       }
   }

   return EXIT_SUCCESS; 

}

int 
main(int argc, char *argv[])
{
    int c, n_men = 1, n_women = 1;
    while ((c = getopt(argc, argv, "w:m:h")) > -1) {
        switch (c)
        {
        case 'w':
            n_women = atoi(optarg); 
            if (n_women <= 0){
                fprintf(stderr, "Enter number of women greater than 0\n");
                exit(EXIT_FAILURE);    
            }
            break;
        case 'm':
            n_men = atoi(optarg); 
            if (n_men <= 0){
                fprintf(stderr, "Enter number of men greater than 0\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'h':
            printf("Usage:[-w women] [-m men] [-h help]"); 
            exit(EXIT_SUCCESS);
        }
    }

    return executioner(n_men, n_women);

}