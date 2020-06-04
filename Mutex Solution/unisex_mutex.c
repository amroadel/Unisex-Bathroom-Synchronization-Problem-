#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#ifndef _REENTRANT
#define _REENTRANT
#endif

#define CAPACITY 4
#define FREE 3
#define LOOP 100

//Create bathroom struct
typedef struct restroom { 
    int turn; 
    int men_inside;             // number of men intside the bathroom 
    int women_inside;           // number of women intside the bathroom
    int men_waiting;            // number of men waiting outside the bathroom            
    int women_waiting;          // number of women waiting outside the bathroom
    pthread_mutex_t lock;      // mutex to provide synchronization
    pthread_cond_t occupied[2];    //condiion variable to know when the bathroom if full
}bathroom_t;

//Initialize the bathroom struct
static bathroom_t unisex_bathroom = {
    .turn = FREE,
    .men_inside = 0, 
    .women_inside = 0, 
    .men_waiting = 0, 
    .women_waiting = 0, 
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .occupied[0] = PTHREAD_COND_INITIALIZER,
    .occupied[1] = PTHREAD_COND_INITIALIZER
};  


void swap (pthread_t *a , pthread_t *b)
{
    pthread_t temp = *a; 
    *a = *b; 
    *b = temp; 
}

void randomize (pthread_t threads[], int total)
{
    srand(time(NULL));
    for (int i = total-1 ; i > 0 ; i--){
        int r = rand() % (i+1);
        swap(&threads[i], &threads[r]);
    }
}

int random()
{
    srand(time(NULL));
    return  rand() % 2;
}
void print_status(bathroom_t *unisex_bathroom)
{ 
    printf("\n\nstart status");
    printf("\n\nMen inside : %i", unisex_bathroom->men_inside);
    printf("\n\nMen waiting : %i ", unisex_bathroom->men_waiting);
    printf("\n\nWomen inside : %i", unisex_bathroom->women_inside);
    printf("\n\nWomen waiting : %i", unisex_bathroom->women_waiting);
    printf("\n\nEnd\n\n");
}
static void 
go_caca(bathroom_t *unisex_bathroom)
{
    //check bathroom is updated correctly
    assert(unisex_bathroom->men_inside >= 0 || unisex_bathroom->women_inside >= 0); 

    //make sure we stick to traditions and abide by capacity
    assert((unisex_bathroom->men_inside == 0 && unisex_bathroom->women_inside <= CAPACITY)
         ||(unisex_bathroom->women_inside == 0 && unisex_bathroom->men_inside <= CAPACITY)); 
}
void
man_wants_to_enter(bathroom_t *unisex_bathroom)
{
    
    pthread_mutex_lock(&unisex_bathroom->lock); // lock the mutex for cirtical area 
    int men = 1; 
    int women = 0; 
    unisex_bathroom-> men_waiting++; 

    //while the bathroom is full or there are women inside, wait.
    while(unisex_bathroom->men_inside == CAPACITY || unisex_bathroom->women_inside > 0 
            || (unisex_bathroom->turn == women && unisex_bathroom->women_waiting > 0 )){
                //wait for the bathroom to be empty
                pthread_cond_wait(&unisex_bathroom->occupied[men], &unisex_bathroom->lock);    
            }
        

    unisex_bathroom->men_waiting--;  
    unisex_bathroom->men_inside++; //man enters
    unisex_bathroom->turn = men; 
    pthread_mutex_unlock(&unisex_bathroom->lock); //release the mutex 
    go_caca(unisex_bathroom);

    
}

void 
man_leaves(bathroom_t *unisex_bathroom)
{
    const int men = 1;
    const int women = 0; 
    pthread_mutex_lock(&unisex_bathroom->lock); 

    if(unisex_bathroom->men_inside > 0){
        unisex_bathroom->men_inside--; //man leavs
    }
    
    if (unisex_bathroom->men_inside == 0){
        printf("\n\nEmpty");
        if (unisex_bathroom->women_waiting > 0 ){
            unisex_bathroom->turn = women; 
            pthread_cond_broadcast(&unisex_bathroom->occupied[women]); //broadcast a signal that the bathroom is now avilable
        }
        else {
            unisex_bathroom->turn = FREE; 
            pthread_cond_broadcast(&unisex_bathroom->occupied[women]); //x
            pthread_cond_broadcast(&unisex_bathroom->occupied[men]);
        }
    }

    pthread_mutex_unlock(&unisex_bathroom->lock); 
}

void
woman_wants_to_enter(bathroom_t *unisex_bathroom)
{
    pthread_mutex_lock(&unisex_bathroom->lock); // lock the mutex for cirtical area 
    const int men = 1; 
    const int women = 0; 
    unisex_bathroom-> women_waiting++;

    //while the bathroom is full or there are men inside, wait. 
    while(unisex_bathroom->women_inside == CAPACITY || unisex_bathroom->men_inside > 0 || (unisex_bathroom->turn == men && unisex_bathroom->men_waiting > 0))
        pthread_cond_wait(&unisex_bathroom->occupied[women], &unisex_bathroom->lock); //wait for the bathroom to be empty

    unisex_bathroom->women_waiting--;
    unisex_bathroom->women_inside++; //woman enters
    unisex_bathroom->turn = women; 
    pthread_mutex_unlock(&unisex_bathroom->lock); //release the mutex 
    go_caca(unisex_bathroom);
    
}

void 
woman_leaves(bathroom_t *unisex_bathroom)
{
    pthread_mutex_lock(&unisex_bathroom->lock); 
    const int men = 1;
    const int women = 0;
    
    if (unisex_bathroom->women_inside > 0){
        unisex_bathroom->women_inside--; //woman leavs
    }

    if (unisex_bathroom->women_inside == 0){
        printf("\n\nEmpty\n");  
        if (unisex_bathroom->men_waiting > 0 ){
            unisex_bathroom->turn = men; 
            pthread_cond_broadcast(&unisex_bathroom->occupied[men]); //broadcast a signal that the bathroom is now avilable
        }
        else {
            unisex_bathroom->turn = FREE; 
            pthread_cond_broadcast(&unisex_bathroom->occupied[men]); //x
            pthread_cond_broadcast(&unisex_bathroom->occupied[women]);
        }
    }
    /*else {
        pthread_cond_broadcast(&unisex_bathroom->occupied[women]);
    }*/
    pthread_mutex_unlock(&unisex_bathroom->lock); 
}

//men thread function 
static void*
men (void *restroom)
{
    bathroom_t *unisex_bathroom = (bathroom_t *) restroom; 
    for (int i = 0; i < LOOP ; i++){

        man_wants_to_enter(unisex_bathroom);
        printf("\n\nMen present"); 
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
        printf("\n\nWomen present");
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
            printf("Usage:[-w women] [-m men] [-h help]\n"); 
            exit(EXIT_SUCCESS);
        }
    }
    
    return executioner(n_men, n_women); 


}