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
typedef struct alarm_tag
{
    struct alarm_tag *link;
    int id;
    int seconds;
    time_t time; /* seconds from EPOCH */
    char message[128];
    int Changed;
} alarm_t;

pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t d1_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t d2_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t d3_cond = PTHREAD_COND_INITIALIZER;
alarm_t *alarm_list = NULL;
alarm_t *current_alarm = NULL; //current alarm

//Insert to the list, sorted by smallest id;
void Insert(alarm_t **last, alarm_t *new)
{

    alarm_t *old = *last;

    while (*last != NULL)
    {
        //if alarm id is smaller, append alarm in front of next
        if (old->id >= new->id)
        {
            new->link = old;
            *last = new;
            new->Changed = 0;
            break;
        }
        last = &new->link;
        new = new->link;
    }
    if (*last == NULL)
    {
        *last = new;
    }

    new->time = time(NULL) + new->seconds;
    printf("Alarm(%d) Inserted by Main Thread Into %d Alarm list at %d: [\"%s\"]\n",
           new->id, pthread_self(), new->time, new->message);
}

void Change(alarm_t **old, alarm_t *new)
{

    alarm_t *alarm = *old;

    while (alarm != NULL)
    {
        if (alarm->id == new->id)
        {
            strcpy(alarm->message, new->message);
            alarm->seconds = new->seconds;
            alarm->time = time(NULL) + new->seconds;
            new->Changed = 1;
            printf("Alarm(%d) Changed at <%d>: %s\n", alarm->id, alarm->time, alarm->message);
            break;
        }
        alarm = alarm->link;
    }
}
/*
* The alarm thread's start routine.
*/
void *alarm_thread(void *arg)
{
    alarm_t *alarm;
    int sleep_time;
    time_t now;
    int status;
    pthread_t thread;
    /*
        * Loop forever, processing commands. The alarm thread will
        * be disintegrated when the process exits.
        */
    while (1)
    {
        status = pthread_mutex_lock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Lock mutex");

        alarm = alarm_list;

        if (alarm == NULL)
        {
            sleep_time = 1;
        } //iterating through list

        else
        {
            alarm_list = alarm->link;
            current_alarm = alarm;
            printf("Alarm Thread Created New Display Alarm Thread %d For Alarm(%d) at %d:%s\n", pthread_self(), alarm->id, alarm->time, alarm->message);
            status = pthread_cond_signal(&d1_cond);
            if (status != 0)
                err_abort(status, "Signal cond");
        }

        status = pthread_mutex_unlock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Unlock mutex");
        sleep(sleep_time);
    }
}

void *display_thread(void *arg)
{
    alarm_t *alarm;
    int status;
    time_t now;
    while (1)
    {
        status = pthread_mutex_lock(&alarm_mutex);
        if (status != 0)
        {
            err_abort(status, "Lock mutex");
        }
        status = pthread_cond_wait(&d1_cond, &alarm_mutex);
        if (status != 0)
        {
            err_abort(status, "wait on cond");
        }
        alarm = current_alarm;
        while (alarm->time > time(NULL))
        {
            printf("Alarm(%d) Printed by Alarm Display Thread %d at %d : %s \n",
                   alarm->id,
                   pthread_self(),
                   time(NULL),
                   alarm->message);
            sleep(5);
        }
        printf("Alarm Thread Removed Alarm(%d) at %d: %s\n",
            alarm->id,time(NULL),alarm->message);
        status = pthread_mutex_unlock(&alarm_mutex);
        if (status != 0)
        {
            err_abort(status, "unlock mutex");
        }
        free(alarm);
    }
}
void *display_thread_two(void *arg)
{
    alarm_t *alarm;
    int status;
    time_t now;
    while (1)
    {
        status = pthread_mutex_lock(&alarm_mutex);
        if (status != 0)
        {
            err_abort(status, "Lock mutex");
        }
        status = pthread_cond_wait(&d2_cond, &alarm_mutex);
        if (status != 0)
        {
            err_abort(status, "wait on cond");
        }
        alarm = current_alarm;
        while (alarm->time > time(NULL))
        {
            printf("Alarm(%d) Printed by Alarm Display Thread %d at %d : %s \n",
                   alarm->id,
                   pthread_self(),
                   time(NULL),
                   alarm->message);
            sleep(5);
        }
        printf("Alarm Thread Removed Alarm(%d) at %d: %s\n",
            alarm->id,time(NULL),alarm->message);
        status = pthread_mutex_unlock(&alarm_mutex);
        if (status != 0)
        {
            err_abort(status, "unlock mutex");
        }
        free(alarm);
    }
}
void *display_thread_three(void *arg)
{
    alarm_t *alarm;
    int status;
    time_t now;
    while (1)
    {
        status = pthread_mutex_lock(&alarm_mutex);
        if (status != 0)
        {
            err_abort(status, "Lock mutex");
        }
        status = pthread_cond_wait(&d3_cond, &alarm_mutex);
        if (status != 0)
        {
            err_abort(status, "wait on cond");
        }
        alarm = current_alarm;
        while (alarm->time > time(NULL))
        {
            printf("Alarm(%d) Printed by Alarm Display Thread %d at %d : %s \n",
                   alarm->id,
                   pthread_self(),
                   time(NULL),
                   alarm->message);
            sleep(5);
        }
        printf("Alarm Thread Removed Alarm(%d) at %d: %s\n",
            alarm->id,time(NULL),alarm->message);
        status = pthread_mutex_unlock(&alarm_mutex);
        if (status != 0)
        {
            err_abort(status, "unlock mutex");
        }
        free(alarm);
    }
}

int main(int argc, char *argv[])
{
    int status;
    char line[128];
    alarm_t *alarm, **last, *next;
    pthread_t thread;
    pthread_t d1_thread;
    pthread_t d2_thread;
    pthread_t d3_thread;

    status = pthread_create(
        &thread, NULL, alarm_thread, NULL);
    if (status != 0)
        err_abort(status, "Create alarm thread");
    status = pthread_create(
        &d1_thread, NULL, display_thread, NULL);
    if (status != 0)
        err_abort(status, "display thread one");
    status = pthread_create(
        &d2_thread, NULL, display_thread_two, NULL);
    if (status != 0)
        err_abort(status, "display thread two");
    status = pthread_create(
        &d3_thread, NULL, display_thread_three, NULL);
    if (status != 0)
        err_abort(status, "display thread three");
    while (1)
    {
        printf("alarm> ");
        if (fgets(line, sizeof(line), stdin) == NULL)
            exit(0);
        if (strlen(line) <= 1)
            continue;
        alarm = (alarm_t *)malloc(sizeof(alarm_t));
        if (alarm == NULL)
            errno_abort("Allocate alarm");

        /*
            * Parse input line into seconds (%d) and a message
            * (%128[^\n]), consisting of up to 128 characters
            * separated from the seconds by whitespace.
            */

        if ((sscanf(line, "Start(%d) %d %128[^\n]", &alarm->id, &alarm->seconds, alarm->message) < 3) && (sscanf(line, "Change(%d) %d %128[^\n]", &alarm->id, &alarm->seconds, alarm->message) < 3))
        {
            fprintf(stderr, "Bad command\n");
            free(alarm);
            continue;
        }
        else if (!(sscanf(line, "Start(%d) %d %128[^\n]", &alarm->id, &alarm->seconds, alarm->message) < 3))
        { //Start_Alarm call
            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Lock mutex");

            /*
                * Insert the new alarm into the list of alarms,
                * sorted by their id.
                */
            Insert(&alarm_list, alarm);
            
            status = pthread_mutex_unlock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Unlock_mutex");
        }
        else
        { //Change_Alarm Call
            status = pthread_mutex_lock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Lock mutex");

            Change(&alarm_list, alarm);

            status = pthread_mutex_unlock(&alarm_mutex);
            if (status != 0)
                err_abort(status, "Unlock_mutex");
        }

#ifdef DEBUG
        printf("[list: ");
        for (next = alarm_list; next != NULL; next = next->link)
            printf("%d(%d)[\"%s\"] ", next->time,
                   next->time - time(NULL), next->message);
        printf("]\n");
#endif
        status = pthread_mutex_unlock(&alarm_mutex);
        if (status != 0)
            err_abort(status, "Unlock mutex");
    }
}
