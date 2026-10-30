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

struct elevator_data {
    int state;
    int stop_requested;
    struct mutex lock;
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
