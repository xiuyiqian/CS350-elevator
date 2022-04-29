/* elevator_null.c
   Null solution for the elevator threads lab.
   Jim Plank
   CS560
   Lab 2
   January, 2009
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "elevator.h"

typedef struct PList{
    Dllist people;
    pthread_mutex_t* lock;
    pthread_cond_t* blockEle;
} *pList;

void initialize_simulation(Elevator_Simulation *es)
{
    pList pl = (pList) malloc(sizeof (struct PList));
    pl->people = new_dllist();
    pl->blockEle = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pl->lock = (pthread_mutex_t*) malloc(sizeof (pthread_mutex_t));
    if (pthread_mutex_init(pl->lock, NULL) != 0) { perror("Mutex init"); exit(1); }
    if (pthread_cond_init(pl->blockEle, NULL) != 0) { perror("Cond init"); exit(1); }
    es->v = pl;// global variable
}

void initialize_elevator(Elevator *e)
{
}

void initialize_person(Person *e)
{
}

void wait_for_elevator(Person *p)
{
    pList pl = p->es->v;
    //add new person to the elevator waiting list
    // lock elevator
    if (pthread_mutex_lock(p->es->lock)!= 0){perror("Mutex lock"); exit(1);}
    pthread_cond_signal(pl->blockEle); // the elevator now should move to people's floor level
    dll_append(pl->people, new_jval_v(p));
    //printf("the people elevator is going to serve; %s %s\n",p->fname, p->lname);
    //signal the elevator
    if (pthread_mutex_unlock(p->es->lock)!= 0){perror("Mutex unlock"); exit(1);}

    //printf("end of addition of people \n");

    if (pthread_mutex_lock(p->lock)!= 0){perror("Mutex lock"); exit(1);}
    pthread_cond_wait(p->cond, p->lock);  // wait until the elevator reaches
    if (pthread_mutex_unlock(p->lock)!= 0){perror("Mutex unlock"); exit(1);}

}

void wait_to_get_off_elevator(Person *p)
{
    if (pthread_mutex_lock(p->e->lock)!= 0){perror("Mutex lock"); exit(1);}
    pthread_cond_signal(p->e->cond); // notify the elevator to move　
    if (pthread_mutex_unlock(p->e->lock)!= 0){perror("Mutex unlock"); exit(1);}

    //block on the person’s condition variable.
    if (pthread_mutex_lock(p->lock)!= 0){perror("Mutex lock"); exit(1);}
    pthread_cond_wait(p->cond, p->lock);  // wait until the elevator reaches
    if (pthread_mutex_unlock(p->lock)!= 0){perror("Mutex unlock"); exit(1);}

}

void person_done(Person *p)
{
    //Unblock the elevator’s condition variable.
    if (pthread_mutex_lock(p->e->lock)!= 0){perror("Mutex lock"); exit(1);}
    pthread_cond_signal(p->e->cond); // notify the elevator to open the door or close the door
    if (pthread_mutex_unlock(p->e->lock)!= 0){perror("Mutex unlock"); exit(1);}

}

void *elevator(void *arg)
{
    Elevator * ele = (Elevator*)(arg);
    pList pl = (pList)ele->es->v;
    while(1){
        //check if empty
        if (pthread_mutex_lock(pl->lock)!= 0){perror("Mutex lock"); exit(1);}
        while(dll_empty(pl->people)){
            // wait until someone is going to use the elevator
            //printf("waiting\n");
            pthread_cond_wait(pl->blockEle, pl->lock);
        }
        if (pthread_mutex_unlock(pl->lock)!= 0){perror("Mutex unlock"); exit(1);}

        //printf("end of the simulation waiting\n");
        if (pthread_mutex_lock(ele->es->lock)!= 0){perror("Mutex lock"); exit(1);}
        Person *ppl = (Person*) jval_v(dll_val(dll_first(pl->people)));
        //printf("the people elevator is going to serve; %s %s\n",ppl->fname, ppl->lname);
        dll_delete_node(dll_first(pl->people));
        if (pthread_mutex_unlock(ele->es->lock)!= 0){perror("Mutex unlock"); exit(1);}

        //move to the floor
        //printf("elevator moving to the destination %d\n", ppl->from);
        while(ele->onfloor != ppl->from){
            move_to_floor(ele,ppl->from);
        }
        
        if (ele -> door_open == 0) {
            open_door(ele);
        }
        
        //printf("elevator finish opening the door\n");
        //It puts itself into the person’s e field, then signals the person and blocks until the person wakes it up.
        if (pthread_mutex_lock(ppl->lock)!= 0){perror("Mutex lock"); exit(1);}
        ppl->e = ele;
        pthread_cond_signal(ppl->cond);
        if (pthread_mutex_unlock(ppl->lock)!= 0){perror("Mutex unlock"); exit(1);}

        if (pthread_mutex_lock(ele->lock)!= 0){perror("Mutex lock"); exit(1);}
        pthread_cond_wait(ele->cond,ele->lock); // wait for person to wake it up  enter the ele
        if (pthread_mutex_unlock(ele->lock)!= 0){perror("Mutex unlock"); exit(1);}


        if (ele -> door_open > 0) {
             close_door(ele);
        }

    
     // move to the destination
        while(ele->onfloor != ppl->to){
            move_to_floor(ppl->e,ppl->to);
        }
    // wake people up and block the elevator
        if (ele -> door_open == 0) {
            open_door(ele);
        }
        if (pthread_mutex_lock(ppl->lock)!= 0){perror("Mutex lock"); exit(1);}
        //signal the people
        pthread_cond_signal(ppl->cond);
        if (pthread_mutex_unlock(ppl->lock)!= 0){perror("Mutex unlock"); exit(1);}

        if (pthread_mutex_lock(ele->lock)!= 0){perror("Mutex lock"); exit(1);}
        pthread_cond_wait(ele->cond,ele->lock); // wait for person to get off
        if (pthread_mutex_unlock(ele->lock)!= 0){perror("Mutex unlock"); exit(1);}

        //after people leave
        if (ele -> door_open > 0) {
             close_door(ele);
        }
    }
}
