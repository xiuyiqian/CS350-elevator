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
    Dllist peopleWaiting;
    pthread_mutex_t* lock;
    pthread_cond_t* arrived;
    pthread_cond_t* blockEle;
} *pList;

void initialize_simulation(Elevator_Simulation *es)
{
    //printf("initialization\n");
    pList pl = (pList) malloc(sizeof (struct PList));
    pl->peopleWaiting = new_dllist();
    //printf("here1\n");
    pl->blockEle = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pl->arrived = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pl->lock = (pthread_mutex_t*) malloc(sizeof (pthread_mutex_t));
    //sprintf("here2\n");
    if(pthread_cond_init(pl->arrived, NULL)){perror("Cond init"); exit(1);};
    if (pthread_mutex_init(pl->lock, NULL) != 0) { perror("Mutex init"); exit(1); }
    if (pthread_cond_init(pl->blockEle, NULL) != 0) { perror("Cond init"); exit(1); }
    es->v = pl;// global variable
    //printf("here\n");
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
    //printf("waiting for elevator\n");
    if (pthread_mutex_lock(p->es->lock)!= 0){perror("Mutex lock"); exit(1);}
    //printf("the people elevator is going to serve; %s %s\n",p->fname, p->lname);
    //signal the elevator
    //printf("adding new person\n");
    pthread_cond_signal(pl->blockEle); // the elevator now should move to people's floor level
    dll_append(pl->peopleWaiting, new_jval_v(p));
    if (pthread_mutex_unlock(p->es->lock)!= 0){perror("Mutex unlock"); exit(1);}

    //printf("end of addition of people \n");

    if (pthread_mutex_lock(p->lock)!= 0){perror("Mutex lock"); exit(1);}
    pthread_cond_wait(p->cond, p->lock);  // wait until the elevator reaches
    if (pthread_mutex_unlock(p->lock)!= 0){perror("Mutex unlock"); exit(1);}

}

void wait_to_get_off_elevator(Person *p)
{   //printf("wait to get off\n");
    if (pthread_mutex_lock(p->e->lock)!= 0){perror("Mutex lock"); exit(1);}
    pthread_cond_signal(p->e->cond); // notify the elevator to remove people
    //printf("siginal elevator to move\n");
    if (pthread_mutex_unlock(p->e->lock)!= 0){perror("Mutex unlock"); exit(1);}

    //block on the person’s condition variable.
    if (pthread_mutex_lock(p->lock)!= 0){perror("Mutex lock"); exit(1);}
    //printf("%s %s try to block the people\n",p->fname, p->lname);
    pthread_cond_wait(p->cond, p->lock);  // wait until the elevator reaches
    //printf("block the people on the elevator \n");
    if (pthread_mutex_unlock(p->lock)!= 0){perror("Mutex unlock"); exit(1);}

}

void person_done(Person *p)
{
    //Unblock the elevator’s condition variable.
    if (pthread_mutex_lock(p->e->lock)!= 0){perror("Mutex lock"); exit(1);}
    pthread_cond_signal(p->e->cond); // notify the elevator to open the door/close the door
    if (pthread_mutex_unlock(p->e->lock)!= 0){perror("Mutex unlock"); exit(1);}

}

void pre_load_people(int direction, Elevator* ele, pList pl, Dllist peopleLoading){
    Dllist itr = (dll_first(pl->peopleWaiting));
    while(!dll_empty(pl->peopleWaiting) && itr != pl->peopleWaiting){
        Dllist next = dll_next(itr);
        Person* ppl = (Person*) jval_v(dll_val(itr));
        // if(pthread_mutex_lock(ppl->lock)){perror("Mutex lock"); exit(1);}
        int customer_direction = 1; // when 1 go up
        if(ppl->from > ppl->to){
            //go down
            customer_direction = 0;
        }
        if (ele->onfloor == ppl->from && direction == customer_direction){
            dll_append(peopleLoading,itr->val);
            dll_delete_node(itr);// itr is on the peoplWaiting
        }
        itr = next;
    }
}

void load_people(Elevator* ele, pList pl,Dllist peopleLoading){
    Dllist itr = dll_first(peopleLoading);
    while(!dll_empty(peopleLoading) && itr!= peopleLoading){
        Dllist next = dll_next(itr);
        Person* ppl = (Person*) jval_v(dll_val(itr));
        if(ele->door_open == 0){
            open_door(ele);
        }
        // let people get on ele

        if (pthread_mutex_lock(ppl->lock)!= 0){perror("Mutex lock"); exit(1);}
        ppl->e = ele;
        pthread_cond_signal(ppl->cond);  // signal people get on
        pthread_mutex_lock(ele->lock);
        if (pthread_mutex_unlock(ppl->lock)!= 0){perror("Mutex lock"); exit(1);}
        //wait for person done
        pthread_cond_wait(ele->cond,ele->lock);
        pthread_mutex_unlock(ele->lock);
        dll_delete_node(itr);

        itr = next;
        //printf("finish loading\n");
    }
}

void unload_people(Elevator* ele, pList pl){
    Dllist itr = dll_first(ele->people);
    while(!dll_empty(ele->people) && itr != ele->people){
        Dllist next = dll_next(itr);
        Person* ppl = (Person*) jval_v(dll_val(itr));
        if (ppl->to == ele->onfloor) {
            if (!ele->door_open) {
                open_door(ele);
            }
            if(pthread_mutex_lock(ppl->lock) != 0 ){perror("Mutex lock"); exit(1);}
            pthread_cond_signal(ppl->cond); //to get off
            if(pthread_mutex_lock(ele->lock) != 0) {perror("Mutex Unlock"); exit(1);}
            if(pthread_mutex_unlock(ppl->lock) != 0) {perror("Mutex Unlock"); exit(1);}
            pthread_cond_wait(ele->cond, ele->lock); // wait people to get off
            if(pthread_mutex_unlock(ele->lock) != 0) {perror("Mutex Unlock"); exit(1);}
        }

        itr = next;
    }
    if (ele->door_open) {
        close_door(ele);
    }
}



void *elevator(void *arg)
{
    //printf("elevator function\n");
    Elevator * ele = (Elevator*)(arg);
    //printf("here\n");
    pList pl = (pList)ele->es->v;
    Dllist peopleLoading = new_dllist();
    int direction=1; //go up
    while(1){
        /*
            if (pthread_mutex_lock(pl->lock)!= 0){perror("Mutex lock"); exit(1);}
            //printf("here\n");
            while(dll_empty(pl->peopleWaiting)){
                // wait until someone is going to use the elevator
                //printf("waiting\n");
                pthread_cond_wait(pl->blockEle, pl->lock);
            }
            if (pthread_mutex_unlock(pl->lock)!= 0){perror("Mutex unlock"); exit(1);}
            */

        if(ele->onfloor == 1){
            direction = 1;
        }
        if (ele->onfloor == ele->es->nfloors){
            direction = 0;
        }
        //printf("here1\n");
        //only need one elevator if two elevatos are on the same floor
        pthread_mutex_lock(pl->lock);
        pre_load_people(direction,ele,pl,peopleLoading);
        pthread_mutex_unlock(pl->lock);
        //printf("here2\n");
        load_people(ele,pl,peopleLoading);

        if(ele->door_open){
            close_door(ele);
        }
        if(direction == 1){
            move_to_floor(ele, ele->onfloor+1);
        }
        else{
            move_to_floor(ele, ele->onfloor-1);
        }
        //printf("here3\n");
        unload_people(ele, ele->v);
    }
    return NULL;
}
