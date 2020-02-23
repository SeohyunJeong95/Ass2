/*
 * alarm_mutex.c
 *
 * This is an enhancement to the alarm_thread.c program, which
 * created an "alarm thread" for each alarm command. This new
 * version uses a single alarm thread, which reads the next
 * entry in a list. The main thread places new requests onto the
 * list, in order of absolute expiration time. The list is
 * protected by a mutex, and the alarm thread sleeps for at
 * least 1 second, each iteration, to ensure that the main
 * thread can lock the mutex to add new work to the list.
 */
/*
*   time_t: time_t represents the clock time as integer which 
*   is a part of the calendar time
*   
*   the function time() returns the calendar-time equivalent using
*   data type time_t
*   you can pass in a pointer to a time_t object that time will fill
*   up with the current time (and the return value is hte same one that
*    *you pointed to)    the time is in SECONDS
*
*
*/


#include <pthread.h>
#include <time.h>
#include "errors.h"

/*
 * The "alarm" structure now contains the time_t (time since the
 * Epoch, in seconds) for each alarm, so that they can be
 * sorted. Storing the requested number of seconds would not be
 * enough, since the "alarm thread" cannot tell how long it has
 * been on the list.
 */
typedef struct alarm_tag {              // recall that structs are very much like objects in java
    struct alarm_tag    *link;    //  the pointer variable link points to an alarm_tag struct. you're trying to link different structs
    int                 seconds;        
    time_t              time;   /* seconds from EPOCH */            
    char                message[64];        
} alarm_t;                              //subtle difference between having and not having this tag
                                        // before the semicolon is that this tag helps "define" the 
                                        //variables where if you didn't have the tag, you would only 
                                        //be declaring the struct's variables.

// in cases where default mutex attributes are appropriate, the macro
// PTHREAD_MUTEX_INITIALIZER can be used to initialize mutexes that 
// are staically allocated. The effect shall be equivalent to dynamic
//  initialization by a call to pthread_mutex_init()  with parameter
//  attr specified as NULL, except that no error checks are performed
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;    
alarm_t *alarm_list = NULL;         //this instance of alarm_t is called alarm_list and is pointing to NULL because it is empty
                                    //Do these structs get added to the memory like arrays?



/*
 * The alarm thread's start routine.
 * This function is run as a thread, and proceses each alarm
 * request in order from the list alarm_list. The thread never
 * terminates-- when main returns, the thread simply "evaporates"..
 * The only consequence of this is that any remaining alarms will 
 * not be delivered-- the thread maintains no state that can be 
 * seen outside the process.........
 * 
 * 
 */                                 // (void) for the argument would have indicated that there are no arguments for the function but in this case, it indicates that the argument is a a generic pointer to a location.
void *alarm_thread (void *arg)      // this specifies that the pointer can point to 
                                    // any variable that is not declared with the const or volatile keyword
                                    // void* function_name indicates that this
                                    // function returns a pointer to some memory location of unspecified type.
{
    alarm_t *alarm;     // this instance of alarm_t is called alarm
    int sleep_time;
    time_t now;
    int status;

    /*
     * Loop forever, processing commands. The alarm thread will
     * be disintegrated when the process exits.
     */
    while (1) {
        status = pthread_mutex_lock (&alarm_mutex);  //status is equal to pthread_mutex_lock (address of alarm_mutex) (recall that alarm_mutex was initialized using PTHREAD_MUTEX_INITIALIZER previously) 
                                                    //(this call results in the object mutex getting locked. if the object mutex is already locked, then the calling thread will be block...)
                                                    // if successful, the pthread_mutex_lock() will return 0. otherwise, an error number will be returned indicating an error
        if (status != 0)                //aborting with the error message "Lock mutex" if the status of pthread_mutex_lock is not 0
            err_abort (status, "Lock mutex");
        alarm = alarm_list;     //alarm_list which is NULL is assigned to alarm...
                                //Q: wouldn't this just make alarm NULL anyway?
        /*
         * If the alarm list is empty, wait for one second. This
         * allows the main thread to run, and read another
         * command. If the list is not empty, remove the first
         * item. Compute the number of seconds to wait -- if the
         * result is less than 0 (the time has passed), then set
         * the sleep_time to 0.
         */
        if (alarm == NULL)
            sleep_time = 1;
        else {          //alarm is not null...
            alarm_list = alarm->link;   // the arrow operator allows access to elements within a struct (used with a pointer variable). the variable we're accessing is a pointer variable to another alarm_tag struct. 
                                        // this is saying that if alarm is not null, then alarm's link is assigned to alarm_list
                                        // is this assignemnt supposed to create a singly linked list?
            now = time (NULL);          // if you pass in NULL, it returns a new time_t object that represents the current time
            if (alarm->time <= now)     // if the alarm's time is less than or equal to current time, sleep_time = 0 ;
                sleep_time = 0;
            else
                sleep_time = alarm->time - now;     // if the alarm's time is greater than current time, then sleep time is alarm's time minus current time
#ifdef DEBUG
            printf ("[waiting: %d(%d)\"%s\"]\n", alarm->time,
                sleep_time, alarm->message);
#endif
            }

        /*
         * Unlock the mutex before waiting, so that the main
         * thread can lock it to insert a new alarm request. If
         * the sleep_time is 0, then call sched_yield, giving
         * the main thread a chance to run if it has been
         * readied by user input, without delaying the message
         * if there's no input.
         */
        // if successful, the pthread_mutex_lock() will return 0. otherwise, an error number will be returned indicating an error
        // pthread_mutex_unlock() function shall release the mutex object referenced by mutex
        // if there are threads blocked on the mutex object referenced by mutex
        // when the pthread_mutex_unlock() is called, resulting in the 
        // mutex becoming unavailable, the scheduling policy shall determine
        // which thread shall acquire the mutex.
        status = pthread_mutex_unlock (&alarm_mutex);   
        if (status != 0)    //unlock is not successful
            err_abort (status, "Unlock mutex"); 
        if (sleep_time > 0)         
            sleep (sleep_time);
        else
            sched_yield ();    // this yields the processor to a thread that is 
                                // ready to run, but will return immediately if
                                // there are no ready threads.
                                // in this case, the main thread will be allowed to process
                                // a user command if there's input waiting-but if the user hasn't 
                                // entered a command, sched_yield will return immediately

        /*
         * If a timer expired, print the message and free the
         * structure.
         */
        if (alarm != NULL) {
            printf ("(%d) %s\n", alarm->seconds, alarm->message);
            free (alarm);
        }
    }
}

int main (int argc, char *argv[])
{
    int status;
    char line[128];
    alarm_t *alarm, **last, *next;
    pthread_t thread;

    status = pthread_create (
        &thread, NULL, alarm_thread, NULL);     //status is a variable that contains the value yielded by pthread_create with the thread id equal to the address of thread)
    if (status != 0)
        err_abort (status, "Create alarm thread");
    while (1) {
        printf ("alarm> ");
        if (fgets (line, sizeof (line), stdin) == NULL) exit (0);
        if (strlen (line) <= 1) continue;
        alarm = (alarm_t*)malloc (sizeof (alarm_t));
        if (alarm == NULL)
            errno_abort ("Allocate alarm");

        /*
         * Parse input line into seconds (%d) and a message
         * (%64[^\n]), consisting of up to 64 characters
         * separated from the seconds by whitespace.
         */
        if (sscanf (line, "%d %64[^\n]", 
            &alarm->seconds, alarm->message) < 2) {
            fprintf (stderr, "Bad command\n");
            free (alarm);
        } else {
            status = pthread_mutex_lock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");
            alarm->time = time (NULL) + alarm->seconds;

            /*
             * Insert the new alarm into the list of alarms,
             * sorted by expiration time.
             */
            last = &alarm_list; //last is equal to the address of alarm_list
            next = *last;       // next is the content of last since * is an indirection operator.
            printf("&last: %d\n",*last);
            printf("&alarm_list: %d\n",&alarm_list);
            printf("next: %d\n",next);
            while (next != NULL) {
                if (next->time >= alarm->time) {
                    alarm->link = next;
                    *last = alarm;
                    break;
                }
                last = &next->link;
                next = next->link;
            }
            /*
             * If we reached the end of the list, insert the new
             * alarm there. ("next" is NULL, and "last" points
             * to the link field of the last item, or to the
             * list header).
             */
            if (next == NULL) {
                *last = alarm;
                alarm->link = NULL;
            }

            next=alarm_list;
            while(next!=NULL){
                printf("next id: %d\n",next->seconds);
                next=next->link;
            }
#ifdef DEBUG
            printf ("[list: ");
            for (next = alarm_list; next != NULL; next = next->link)
                printf ("%d(%d)[\"%s\"] ", next->time,
                    next->time - time (NULL), next->message);
            printf ("]\n");
#endif
            status = pthread_mutex_unlock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Unlock mutex");
        }
    }
}
