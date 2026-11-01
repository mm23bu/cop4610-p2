#ifndef _ELEVATOR_H
#define _ELEVATOR_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>

/* states represented directly as simple integers */
#define OFFLINE 0
#define IDLE    1
#define LOADING 2
#define UP      3
#define DOWN    4

// Project Limits
#define NUM_FLOORS 5
#define MAX_PETS 5
#define MAX_WEIGHT 50

// Pet types
enum pet_type {
    CHIHUAHUA = 0,
    PUG,
    PUGHUAHUA,
    DACHSHUND,
    PET_TYPE_COUNT
};

// Passenger Request
struct pet {
    int start_floor;
    int dest_floor;
    int type;
    int weight;

    // Linked list
    struct list_head node;
};

struct elevator_data {
    int state;
    int current_floor;
    int current_weight;
    int current_pets;

    int total_pets_serviced;
    int total_weight_serviced;
    int total_waiting;

    int stop_requested;
    int floor_wait_count[NUM_FLOORS];
    struct mutex lock;

    // Linked list
    
    // Number of passengers waiting on each floor
    struct list_head waiting[NUM_FLOORS];
    // Linked list for passengers inside elevator
    struct list_head onboard;

    // Kernel thread for elevator movement
    struct task_struct *thread;

};

extern struct elevator_data elevator;

/* system call stub pointer declarations from syscalls.c */
extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

/* functions implemented in elevator.c for the stubs */
int elevator_start(void);
int elevator_issue_request(int start_floor, int dest_floor, int type);
int elevator_stop(void);

#endif 
