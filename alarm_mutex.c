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
typedef struct alarm_tag {
    struct alarm_tag    *link;
    int                 id;
    int                 seconds;
    time_t              time;   /* seconds from EPOCH */
    char                message[128];
} alarm_t;

typedef struct display_struct{
  int thread_num;
  alarm_t *alarm_list;
  alarm_t * latest_request;
}display_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t display_mutex = PTHREAD_MUTEX_INITIALIZER;
alarm_t *alarm_list = NULL;

//Insert to the list, sorted by smallest id;
void Insert(alarm_t  **last, alarm_t *new){

  alarm_t *old = *last;

  while (*last != NULL) {
      //if alarm id is smaller, append alarm in front of next
      if (old->id >= new->id) {
          new->link = old;
          *last = new;
          break;
      }
      last = &new->link;
      new = new->link;
  }
  if(*last == NULL)
  {
    *last = new;
  }
}

void Change(alarm_t **old,alarm_t *new){

  alarm_t *alarm = *old;

  while(alarm != NULL){
    if (alarm->id == new->id){
      strcpy(alarm->message, new->message);
      alarm->seconds = new->seconds;
      alarm->time = time (NULL) + new->seconds;
      printf("Alarm(%d) Changed at <%d>: %s\n", alarm->id, alarm->time, alarm->message);
      break;
    }
    alarm = alarm->link;
  }
}
/*
 * The alarm thread's start routine.
 */
void *alarm_thread (void *arg)
{
    alarm_t *alarm;
    int sleep_time;
    time_t now;
    int status;
    pthread_t thred;
    /*
     * Loop forever, processing commands. The alarm thread will
     * be disintegrated when the process exits.
     */
    while (1) {
        status = pthread_mutex_lock (&alarm_mutex);
        if (status != 0)
            err_abort (status, "Lock mutex");

        alarm = alarm_list;

        if (alarm == NULL) //iterating through list
            sleep_time = 1;
        else {
            alarm_list = alarm->link;
            now = time (NULL);
            if (alarm->time <= now)
                sleep_time = 0;
            else
                sleep_time = alarm->time - now;

            }
        /*
         * Unlock the mutex before waiting, so that the main
         * thread can lock it to insert a new alarm request. If
         * the sleep_time is 0, then call sched_yield, giving
         * the main thread a chance to run if it has been
         * readied by user input, without delaying the message
         * if there's no input.
         */
        status = pthread_mutex_unlock (&alarm_mutex);
        if (status != 0)
            err_abort (status, "Unlock mutex");
        if (sleep_time > 0)
            sleep (sleep_time);
        else
            sched_yield ();
        /*
         * If a timer expired, print the message and free the
         * structure.
         */
        if (alarm != NULL) {
            free (alarm);
        }
    }
}

int main (int argc, char *argv[])
{
    int status, id, seconds;
    char line[128], message[128];
    alarm_t *alarm, **last, *next;
    pthread_t thread;

    status = pthread_create (
        &thread, NULL, alarm_thread, NULL);
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
         * (%128[^\n]), consisting of up to 128 characters
         * separated from the seconds by whitespace.
         */

        if ((sscanf (line, "Start(%d) %d %128[^\n]", &alarm->id, &alarm->seconds, alarm->message) < 3)
            && (sscanf (line, "Change(%d) %d %128[^\n]", &alarm->id, &alarm->seconds, alarm->message) < 3))
            {
            fprintf (stderr, "Bad command\n");
            free (alarm);
            continue;
        } else if (!(sscanf (line, "Start(%d) %d %128[^\n]", &alarm->id, &alarm->seconds, alarm->message) < 3))
          { //Start_Alarm call
            status = pthread_mutex_lock (&alarm_mutex);
            if (status != 0)
                err_abort (status, "Lock mutex");

            alarm->time = time (NULL) + alarm->seconds;
            printf ("Alarm(%d) Inserted by Main Thread Into %d Alarm list at %d: [\"%s\"]\n", alarm->id, pthread_self(),alarm->time,alarm->message);

            /*
             * Insert the new alarm into the list of alarms,
             * sorted by their id.
             */
             alarm_list = alarm;
            Insert(&alarm, alarm_list);
            printf("%d\n",alarm_list->id);

            status = pthread_mutex_unlock(&alarm_mutex);
            if (status != 0)
                  err_abort(status, "Unlock_mutex");


        }
        else { //Change_Alarm Call
              status = pthread_mutex_lock (&alarm_mutex);
              if (status != 0)
                  err_abort (status, "Lock mutex");

              Change(&alarm_list, alarm);


              status = pthread_mutex_unlock(&alarm_mutex);
              if (status != 0)
                    err_abort(status, "Unlock_mutex");
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
