#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include "elevator.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Elevator module scaffold");

#define ENTRY_NAME "elevator"
#define PERMS 0644

// Time travel one floor and load/unload passenger
#define TRAVEL_MS 2000
#define LOADING_MS 1000
#define IDLE_POLL_MS 100

// Pet weights and letters correspond to enum in .h
static const int pet_weights[PET_TYPE_COUNT] = {3, 14, 10, 16};
static const char pet_letters[PET_TYPE_COUNT] = {'C', 'P', 'H', 'D'};

struct elevator_data elevator;
static struct proc_dir_entry *elevator_entry;

// 1 = up, -1 = down, 0 = no direction
static int travel_direction;

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

// Remove pets that have reached destination
static bool unload_pets(void) {
    struct pet *p;
    struct pet *tmp;
    bool unloaded = false;

    // Safe, allow pets to be removed
    list_for_each_entry_safe(p, tmp, &elevator.onboard, node) {
        if (p->dest_floor != elevator.current_floor)
            continue;

        // Remove pet from onboard list
        list_del(&p->node);

        // Update elevator info adn counters
        elevator.current_pets--;
        elevator.current_weight -= p->weight;
        elevator.total_pets_serviced++;
        elevator.total_weight_serviced += p->weight;
        
        // Free dynamic alloc memy
        kfree(p);
        unloaded = true;
    }
    return unloaded;
}


// Move pets from current floor waiting list into elevator onboard list (FIFO)
static bool board_pets(void) {
    struct list_head *queue = &elevator.waiting[elevator.current_floor - 1];
    struct pet *p;
    bool boarded = false;

    // Max carry is 5 pets
    while (!list_empty(queue) && elevator.current_pets < MAX_PETS) {

        // Get first pet in waiting list
        p = list_first_entry(queue, struct pet, node);

        // Elevator cannot exceed 50 pounds
        if (elevator.current_weight + p->weight > MAX_WEIGHT)
            break;

        // Move pet to onbaord list
        list_move_tail(&p->node, &elevator.onboard);
        
        // Update waiting info and onboard counts
        elevator.floor_wait_count[elevator.current_floor - 1]--;
        elevator.total_waiting--;
        elevator.current_pets++;
        elevator.current_weight += p->weight;

        boarded = true;
    }
    return boarded;
}

// Check if a floor has pets to unload or room for a pickup
static bool floor_needs_stop(int floor) {
    struct pet *p;
    struct list_head *queue;

    // Check if any onboard pet needs to get off
    list_for_each_entry(p, &elevator.onboard, node) {
        if (p->dest_floor == floor)
            return true;
    }

    // Do not pick up new pets during shutdown or full
    if (elevator.stop_requested || elevator.current_pets >= MAX_PETS)
        return false;

    queue = &elevator.waiting[floor - 1];
    if (list_empty(queue))
        return false;

    // Check the first pet to preserve FIFO
    p = list_first_entry(queue, struct pet, node);

    return elevator.current_weight + p->weight <= MAX_WEIGHT;
}

// LOOK scheduling, find nearest useful stop in current direction, reverse when no stops remain ahead
static int choose_next_floor(void) {
    int floor, distance;
    int here = elevator.current_floor;

    if (travel_direction > 0) {
        // Continue upward
        for (floor = here + 1; floor <= NUM_FLOORS; floor++) {
            if (floor_needs_stop(floor))
                return floor;
        }

        // Reverse downward if no stops remain above
        for (floor = here - 1; floor >= 1; floor--) {
            if (floor_needs_stop(floor))
                return floor;
        }
    }else if (travel_direction < 0) {
        // Continue downward
        for (floor = here - 1; floor >= 1; floor--) {
            if (floor_needs_stop(floor))
                return floor;
        }

        // Reverse upward if no stop remain below
        for (floor = here + 1; floor <= NUM_FLOORS; floor++) {
            if (floor_needs_stop(floor))
                return floor;
        }
    } else {
        // Initially choose the closest available stop
        for (distance = 1; distance < NUM_FLOORS; distance++) {
            floor = here + distance;

            if (floor <= NUM_FLOORS && floor_needs_stop(floor))
                return floor;
            floor = here - distance;

            if (floor >= 1 && floor_needs_stop(floor))
                return floor;
        }
    }
    // Remain on current floor if no stop is found
    return here;
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
       int target, direction;
       bool changed, moving;
       
       mutex_lock(&elevator.lock);

       // Wait for the start signal
       if(elevator.state == OFFLINE) {
           mutex_unlock(&elevator.lock);
           msleep(IDLE_POLL_MS);
           continue;
       }

       // Move one floor in the current direction
       if (elevator.state == UP || elevator.state == DOWN) {
           direction = (elevator.state == UP) ? 1 : -1;

           // Release lock while moving
           mutex_unlock(&elevator.lock);
           msleep(TRAVEL_MS);
           if (kthread_should_stop())
               return 0;
           mutex_lock(&elevator.lock);

           // Keep elevator within floor 1-5
           if( (direction > 0 && elevator.current_floor < NUM_FLOORS) ||
                (direction < 0 && elevator.current_floor > 1 )) {
               elevator.current_floor += direction;
           }

           elevator.state = IDLE;
           mutex_unlock(&elevator.lock);
           continue;
       }

       // Unload first to free capacity
       changed = unload_pets();

       // Do not board more pets after a stop request
       if (!elevator.stop_requested && board_pets())
           changed = true;

       // One sec for loading/unloading, required delay if pets enter/exit
       if (changed) {
           elevator.state = LOADING;
           mutex_unlock(&elevator.lock);
           msleep(LOADING_MS);
           if (kthread_should_stop())
               return 0;
           mutex_lock(&elevator.lock);
           elevator.state = IDLE;
           mutex_unlock(&elevator.lock);
           continue;
       }

       // Stop only when elevator is empty, pets waiting on floor remain in queues
       if (elevator.stop_requested && list_empty(&elevator.onboard)) {
           elevator.state = OFFLINE;
           elevator.stop_requested = 0;
           travel_direction = 0;
           mutex_unlock(&elevator.lock);
           continue;
       }

       // Remain idle if no requests exist
       if (list_empty(&elevator.onboard) && elevator.total_waiting == 0) {
           elevator.state = IDLE;
           travel_direction = 0;
           mutex_unlock(&elevator.lock);
           msleep(IDLE_POLL_MS);
           continue;
       }

       // Choose next floor using LOOK scheduling
       target = choose_next_floor();

       // Set direction towrd next stop
       if (target > elevator.current_floor) {
           travel_direction = 1;
           elevator.state = UP;
       } else if (target < elevator.current_floor) {
           travel_direction = -1;
           elevator.state = DOWN;
       } else {
           elevator.state = IDLE;
       }

       moving = target != elevator.current_floor;
       mutex_unlock(&elevator.lock);
       if (!moving)
           msleep(IDLE_POLL_MS);
   }
   return 0;
}



/* Stub implementations */

// Start elevator
int elevator_start(void)
{
    mutex_lock(&elevator.lock);

    // Prevent starting an already active elevator
    if (elevator.state != OFFLINE) {
        mutex_unlock(&elevator.lock);
        return 1;
    }

    // Preserve queued pets from before start or after stop
    elevator.current_weight = 0;
    elevator.current_pets = 0;

    travel_direction = 0;
    elevator.stop_requested = 0;
    elevator.state = IDLE;

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

    // Add pet to end of waiting list of belonging to its start floor
    list_add_tail(&p->node, &elevator.waiting[start_floor - 1]);

    elevator.floor_wait_count[start_floor - 1]++;
    elevator.total_waiting++;

    mutex_unlock(&elevator.lock);
    return 0;

}

// Request deactivation after all onboard pets are delivered
int elevator_stop(void)
{
    mutex_lock(&elevator.lock);

    // Reject duplicate stop request
    if (elevator.stop_requested) {
        mutex_unlock(&elevator.lock);
        return 1;
    }

    // Let thread finish delivering onboard pets
    if (elevator.state != OFFLINE) {
        elevator.stop_requested = 1;
    }

    mutex_unlock(&elevator.lock);
    return 0;
}

/*PROCFS Operations */

// Display elevator information in /proc/elevator
static int elevator_proc_show(struct seq_file *m, void *v)
{
    struct pet *p;
    int i;

    // Lock shared data while reading info
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

    seq_printf(m, "\nNumber of pets: %d\n", elevator.current_pets);
    seq_printf(m, "Number of pets waiting: %d\n", elevator.total_waiting);
    seq_printf(m, "Number of pets serviced: %d\n", elevator.total_pets_serviced);

    mutex_unlock(&elevator.lock);
    return 0;
}

// Open the proc file
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

    // Remve and free each remaining pet
    list_for_each_entry_safe(p, tmp, head, node) {
        list_del(&p->node);
        kfree(p);
    }
}

// Initialize the elevator module
static int __init elevator_init(void)
{
    int i;

    mutex_init(&elevator.lock);

    // Set the intial elevator state and info
    elevator.state = OFFLINE;
    elevator.current_floor = 1;
    elevator.current_weight = 0;
    elevator.current_pets = 0;

    elevator.total_pets_serviced = 0;
    elevator.total_waiting = 0;
    elevator.stop_requested = 0;
    elevator.thread = NULL;
    travel_direction = 0;
    INIT_LIST_HEAD(&elevator.onboard);

    // Init the onboard and floor linked lists
    for (i = 0; i < NUM_FLOORS; i++){
        INIT_LIST_HEAD(&elevator.waiting[i]);
        elevator.floor_wait_count[i] = 0;
    }
   
    // Create the /proc/elevator entry
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
    STUB_issue_request  = elevator_issue_request;
    STUB_stop_elevator  = elevator_stop;

    return 0;
}

// Remove the elevator module
static void __exit elevator_exit(void)
{
    int i;

    /* Disconnect system call pointers */
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
    for (i = 0; i < NUM_FLOORS; i++)
        free_pets(&elevator.waiting[i]);
    mutex_unlock(&elevator.lock);
    mutex_destroy(&elevator.lock);
}

module_init(elevator_init);
module_exit(elevator_exit);
