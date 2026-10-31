#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include "elevator.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Elevator module scaffold");

#define ENTRY_NAME "elevator"
#define PERMS 0644

struct elevator_data elevator;

/* state helper */ 
static const char *get_state_string(int state)
{
    switch (state) {
    case OFFLINE: return "OFFLINE";
    case IDLE:    return "IDLE";
    case LOADING: return "LOADING";
    case UP:      return "UP";
    case DOWN:    return "DOWN";
    default:      return "UNKNOWN";
    }
}

/* Stub implementations */

int elevator_start(void)
{
    mutex_lock(&elevator.lock);

    /* if not OFFLINE, cannot start; return 1 */
    if (elevator.state != OFFLINE) {
        pr_info("elevator: start called, but state is %s (returning 1)\n",
                get_state_string(elevator.state));
        mutex_unlock(&elevator.lock);
        return 1;
    }

    elevator.state = IDLE;
    elevator.stop_requested = 0;
    pr_info("elevator: started successfully. State: %s\n",
            get_state_string(elevator.state));

    mutex_unlock(&elevator.lock);
    return 0;
}

int elevator_stop(void)
{
    mutex_lock(&elevator.lock);

    /* if already OFFLINE or stop already requested, return 1 */
    if (elevator.state == OFFLINE || elevator.stop_requested) {
        pr_info("elevator: stop called, but state is %s / stop_req=%d (returning 1)\n",
                get_state_string(elevator.state), elevator.stop_requested);
        mutex_unlock(&elevator.lock);
        return 1;
    }

    elevator.stop_requested = 1;
    /* without the thread loop running yet, transition to OFFLINE directly for testing */
    elevator.state = OFFLINE;
    pr_info("elevator: stop requested. State: %s\n",
            get_state_string(elevator.state));

    mutex_unlock(&elevator.lock);
    return 0;
}

/*PROCFS Operations */

static int elevator_proc_show(struct seq_file *m, void *v)
{
    int i;

    mutex_lock(&elevator.lock);

    seq_printf(m, "Elevator state: %s\n", get_state_string(elevator.state));
    seq_printf(m, "Current floor: %d\n", elevator.current_floor);
    seq_printf(m, "Current load: %d lbs\n", elevator.current_weight);
    seq_printf(m, "Elevator status: \n\n");

    /* print floors descending: 5 down to 1 */
    for (i = NUM_FLOORS; i >= 1; i--) {
        char marker = (elevator.current_floor == i) ? '*' : ' ';
        seq_printf(m, "[%c] Floor %d: %d\n", marker, i, elevator.floor_wait_count[i - 1]);
    }

    seq_printf(m, "\nNumber of pets serviced: %d\n", elevator.total_pets_serviced);
    seq_printf(m, "Total weight serviced: %d lbs\n", elevator.total_weight_serviced);

    mutex_unlock(&elevator.lock);
    return 0;
}

static int elevator_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, elevator_proc_show, NULL);
}

static const struct proc_ops elevator_proc_ops = {
    .proc_open    = elevator_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* MODULE LIFECYCLE */

static int __init elevator_init(void)
{
    int i;

    mutex_init(&elevator.lock);
    elevator.state = OFFLINE;
    elevator.current_floor = 1;
    elevator.current_weight = 0;
    elevator.total_pets_serviced = 0;
    elevator.total_weight_serviced = 0;
    elevator.stop_requested = 0;

    for (i = 0; i < NUM_FLOORS; i++){
        elevator.floor_wait_count[i] = 0;
    }

    /* register /proc/elevator */
    if (!proc_create(ENTRY_NAME, PERMS, NULL, &elevator_proc_ops)) {
        pr_err("elevator: failed to create /proc/%s\n", ENTRY_NAME);
        return -ENOMEM;
    }

    /* hook the start and stop stubs */
    STUB_start_elevator = elevator_start;
    STUB_stop_elevator  = elevator_stop;
    STUB_issue_request  = NULL;

    pr_info("elevator: module loaded. State: %s\n",
            get_state_string(elevator.state));
    return 0;
}

static void __exit elevator_exit(void)
{
    /* clear stubs so syscalls return -ENOSYS */
    STUB_start_elevator = NULL;
    STUB_stop_elevator  = NULL;
    STUB_issue_request  = NULL;

    mutex_destroy(&elevator.lock);
    pr_info("elevator: module unloaded\n");
}

module_init(elevator_init);
module_exit(elevator_exit);
