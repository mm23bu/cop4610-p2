#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/err.h>
#include "elevator.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Elevator module scaffold");

#define ENTRY_NAME "elevator"
#define PERMS 0644

// Time travel one floor and load/unload passenger
#define TRAVEL_MS 2000
#define LOADING_MS 1000

// Pet weights and letters correspond to enum in .h
static const int pet_weights[PET_TYPE_COUNT] = {3, 14, 10, 16};
static const char pet_letters[PET_TYPE_COUNT] = {'C', 'P', 'H', 'D'};

struct elevator_data elevator;
static struct proc_dir_entry *elevator_entry;

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

// LINKED LIST FUNCTIONS

// Check if there pets waiting on any floor
static bool has_waiting_pets(void) {
    int i;
    for (i = 0; i < NUM_FLOORS; i++)
        if (!list_empty(&elevator.waiting[i]))
            return true;
    return false;
}

// Remove pets that have reached destination
static bool unload_pets(void) {
    struct pet *p;
    struct pet *tmp;
    bool unloaded = false;

    list_for_each_entry_safe(p, tmp, &elevator.onboard, node) {
        if (p->dest_floor == elevator.current_floor) {
            // Remove pet from onboard list
            list_del(&p->node);
            // Update elevator info
            elevator.current_pets--;
            elevator.current_weight -= p->weight;
            elevator.total_pets_serviced++;
            elevator.total_weight_serviced += p->weight;
            // Free dynamic alloc memy
            kfree(p);

            unloaded = true;
        }
    }
    return unloaded;
}

// Move pets from current floor waiting list into elevator onboard list (FIFO)
static bool board_pets(void) {
    int index = elevator.current_floor - 1;
    struct pet *p;
    bool boarded = false;

    while (!list_empty(&elevator.waiting[index])) {
        // Elevator cannot carry more than 5 pets
        if (elevator.current_pets >= MAX_PETS)
            break;

        // Get first pet in waiting list
        p = list_first_entry(&elevator.waiting[index], struct pet, node);

        // Elevator cannot exceed 50 pounds
        if (elevator.current_weight + p->weight > MAX_WEIGHT)
            break;

        // Move pet to onbaord list
        list_move_tail(&p->node, &elevator.onboard);
        // Update waiting info
        elevator.floor_wait_count[index]--;
        elevator.total_waiting--;
        // Update current elevator load
        elevator.current_pets++;
        elevator.current_weight += p->weight;

        boarded = true;
    }
    return boarded;
}

// Find next floor for thread to visit
// TEMP , will change for scheduling alogirthm
static int choose_next_floor(void) {
    struct pet *p;
    int i;

    if (!list_empty(&elevator.onboard)) {
        p = list_first_entry(&elevator.onboard, struct pet, node);
        return p->dest_floor;
    }

    for (i = 0; i < NUM_FLOORS; i++) {
        if (!list_empty(&elevator.waiting[i]))
            return i + 1;
    }
    return elevator.current_floor;
}


// KERNEL THREAD

/* OFFLINE   Wait until elevator started
   IDLE      Check requests and decide what to do
   LOADING   Unload and board pets at the floor
   UP        Travel up one floor
   DOWN      Travel down one floor

   Handle Movement
   elevator.lock for protection
*/
static int elevator_thread(void *unused) {
   while (!kthread_should_stop()) {
       int target;
       bool changed = false;
       int delay_ms = 100;
       
       mutex_lock(&elevator.lock);

       switch(elevator.state) {
        case OFFLINE:
            // Stay inactive until elevator_start(), change state to IDLE
            delay_ms = 100;
            break;
        
        case IDLE:
        case LOADING:
            // Process pets at current floor, both cases use same

            // Unload first
            if (unload_pets())
                changed = true;

            // Do not board new pets while stopping
            if (!elevator.stop_requested) {
                if (board_pets())
                    changed = true;
            }

            // If boading or unloaded occured, stay LOADING for 1 sec, change to IDLE after delay
            if (changed) {
                elevator.state = LOADING;

                mutex_unlock(&elevator.lock);
                msleep_interruptible(LOADING_MS);

                if (kthread_should_stop())
                    return 0;
                mutex_lock(&elevator.lock);

                // Only worker tranitions away from LOADING during normal service
                if (elevator.state == LOADING)
                    elevator.state = IDLE;

                mutex_unlock(&elevator.lock);
                continue;
            }

            // Complete a stop only after every onboard pet has been delivered
            if (elevator.stop_requested && list_empty(&elevator.onboard)) {
                elevator.state = OFFLINE;
                elevator.stop_requested = 0;
                break;
            }

            // Become IDLE when there is no work
            if (list_empty(&elevator.onboard) && !has_waiting_pets()) {
                elevator.state = IDLE;
                delay_ms = 100;
                break;
            }

            // Choose next destination
            target = choose_next_floor();
            if (target > elevator.current_floor)
                elevator.state = UP;
            else if (target < elevator.current_floor)
                elevator.state = DOWN;
            else
                elevator.state = IDLE;

            delay_ms = 100;
            break;

        
        case UP:
            mutex_unlock(&elevator.lock);
            msleep_interruptible(TRAVEL_MS);
            
            if (kthread_should_stop())
                return 0;
            mutex_lock(&elevator.lock);
            
            if (elevator.current_floor < NUM_FLOORS)
                elevator.current_floor++;
            // New floor
            elevator.state = LOADING;
            mutex_unlock(&elevator.lock);
            continue;
        
        
        case DOWN:
            mutex_unlock(&elevator.lock);
            msleep_interruptible(TRAVEL_MS);
            
            if (kthread_should_stop())
                return 0;
            mutex_lock(&elevator.lock);
            
            if (elevator.current_floor > 1)
                elevator.current_floor--;
            // New floor
            elevator.state = LOADING;
            mutex_unlock(&elevator.lock);
            continue;
        
        default:
            elevator.state = IDLE;
            delay_ms = 100;
            break;
       }
       mutex_unlock(&elevator.lock);
       msleep_interruptible(delay_ms);
   }
   return 0;
}


/* Stub implementations */

int elevator_start(void)
{
    mutex_lock(&elevator.lock);

    /* if not OFFLINE, cannot start; return 1 */
    if (elevator.state != OFFLINE) {
        //pr_info("elevator: start called, but state is %s (returning 1)\n",
        //        get_state_string(elevator.state));
        mutex_unlock(&elevator.lock);
        return 1;
    }

    elevator.current_floor = 1;
    elevator.stop_requested = 0;
    elevator.state = IDLE;
    //pr_info("elevator: started successfully. State: %s\n",
    //        get_state_string(elevator.state));

    mutex_unlock(&elevator.lock);
    return 0;
}


// Create new pet request 
int elevator_issue_request(int start_floor, int dest_floor, int type) {
    struct pet *p;

    // Validate the floor numbers and pet type
    if (start_floor < 1 || start_floor > NUM_FLOORS ||
        dest_floor < 1 || dest_floor > NUM_FLOORS ||
        start_floor == dest_floor || type < 0 || type >= PET_TYPE_COUNT) {

        return 1;
    }

    // Allocate memory for one new pet
    p = kmalloc(sizeof(*p), GFP_KERNEL);
    if (!p)
        return -ENOMEM;

    // Initialize pet
    p->start_floor = start_floor;
    p->dest_floor = dest_floor;
    p->type = type;
    p->weight = pet_weights[type];
    INIT_LIST_HEAD(&p->node);
    mutex_lock(&elevator.lock);

    // Reject new requests while elevaotr is in process of stopping
    if (elevator.stop_requested) {
        mutex_unlock(&elevator.lock);
        kfree(p);
        return 1;
    }

    // Add pet to end of waiting list of belonging to its start floor
    list_add_tail(&p->node, &elevator.waiting[start_floor - 1]);

    elevator.floor_wait_count[start_floor - 1]++;
    elevator.total_waiting++;

    mutex_unlock(&elevator.lock);
    return 0;

}

int elevator_stop(void)
{
    mutex_lock(&elevator.lock);

    /* if already OFFLINE or stop already requested, return 1 */
    if (elevator.state == OFFLINE || elevator.stop_requested) {
        //pr_info("elevator: stop called, but state is %s / stop_req=%d (returning 1)\n",
        //        get_state_string(elevator.state), elevator.stop_requested);
        mutex_unlock(&elevator.lock);
        return 1;
    }

    elevator.stop_requested = 1;
    /* without the thread loop running yet, transition to OFFLINE directly for testing */
    //elevator.state = OFFLINE;
    //pr_info("elevator: stop requested. State: %s\n",
    //        get_state_string(elevator.state));

    mutex_unlock(&elevator.lock);
    return 0;
}

/*PROCFS Operations */

static int elevator_proc_show(struct seq_file *m, void *v)
{
    struct pet *p;
    int i;

    mutex_lock(&elevator.lock);

    seq_printf(m, "Elevator state: %s\n", get_state_string(elevator.state));
    seq_printf(m, "Current floor: %d\n", elevator.current_floor);
    seq_printf(m, "Current load: %d lbs\n", elevator.current_weight);
    seq_puts(m, "Elevator status:");

    list_for_each_entry(p, &elevator.onboard, node) {
        seq_printf(m, " %c%d", pet_letters[p->type], p->dest_floor);
    }
    seq_puts(m, "\n\n");

    /* print floors descending: 5 down to 1 */
    for (i = NUM_FLOORS; i >= 1; i--) {
        char marker = (elevator.current_floor == i) ? '*' : ' ';
        seq_printf(m, "[%c] Floor %d: %d", marker, i, elevator.floor_wait_count[i - 1]);
        list_for_each_entry(p, &elevator.waiting[i - 1], node) {
            seq_printf(m, " %c%d", pet_letters[p->type], p->dest_floor);
        }
        seq_putc(m, '\n');
    }

    seq_printf(m, "\nNumber of passengers: %d\n", elevator.current_pets);
    seq_printf(m, "Number of passengers waiting: %d\n", elevator.total_waiting);
    seq_printf(m, "Number of passengers serviced: %d\n", elevator.total_pets_serviced);

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

// Free every remaining pet from one linked list
static void free_pets(struct list_head *head) {
    struct pet *p;
    struct pet *tmp;

    list_for_each_entry_safe(p, tmp, head, node) {
        list_del(&p->node);
        kfree(p);
    }
}

static int __init elevator_init(void)
{
    int i;

    mutex_init(&elevator.lock);

    elevator.state = OFFLINE;
    elevator.current_floor = 1;
    elevator.current_weight = 0;
    elevator.current_pets = 0;

    elevator.total_pets_serviced = 0;
    elevator.total_waiting = 0;
    //elevator.total_weight_serviced = 0;
    elevator.stop_requested = 0;
    elevator.thread = NULL;
    INIT_LIST_HEAD(&elevator.onboard);

    for (i = 0; i < NUM_FLOORS; i++){
        INIT_LIST_HEAD(&elevator.waiting[i]);
        elevator.floor_wait_count[i] = 0;
    }

    /* register /proc/elevator */
    //if (!proc_create(ENTRY_NAME, PERMS, NULL, &elevator_proc_ops)) {
    //    pr_err("elevator: failed to create /proc/%s\n", ENTRY_NAME);
    //    return -ENOMEM;
    //}

    elevator_entry = proc_create(ENTRY_NAME, PERMS, NULL, &elevator_proc_ops);
    if (!elevator_entry) {
        mutex_destroy(&elevator.lock);
        return -ENOMEM;
    }

    // Start kernel thread
    elevator.thread = kthread_run(elevator_thread, NULL, "pet_elevator");

    if (IS_ERR(elevator.thread)) {
        int error = PTR_ERR(elevator.thread);
        elevator.thread = NULL;
        proc_remove(elevator_entry);
        mutex_destroy(&elevator.lock);
        return error;
    }

    /* hook the start and stop stubs */
    STUB_start_elevator = elevator_start;
    STUB_stop_elevator  = elevator_stop;
    STUB_issue_request  = elevator_issue_request;

    //pr_info("elevator: module loaded. State: %s\n",
    //        get_state_string(elevator.state));
    pr_info("elevator: module loaded\n");
    return 0;
}

static void __exit elevator_exit(void)
{
    /* clear stubs so syscalls return -ENOSYS */
    STUB_start_elevator = NULL;
    STUB_stop_elevator  = NULL;
    STUB_issue_request  = NULL;

    // Stop thread before freeing shared data
    if (elevator.thread)
        kthread_stop(elevator.thread);
    
    proc_remove(elevator_entry);
    mutex_lock(&elevator.lock);
    // Free pets inside elevator
    free_pets(&elevator.onboard);
    // Free pets still waiting on floor
    for (int i = 0; i < NUM_FLOORS; i++)
        free_pets(&elevator.waiting[i]);
    mutex_unlock(&elevator.lock);

    mutex_destroy(&elevator.lock);

    pr_info("elevator: module unloaded\n");
}

module_init(elevator_init);
module_exit(elevator_exit);
