#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Kernel module that displays current and elasped time");

#define ENTRY_NAME "timer"
#define PERMS 0644
#define PARENT NULL

// Pointer to proc file
static struct proc_dir_entry* timer_entry;

// Store previous time and track the first read
static struct timespec64 last_time;
static int first_read = 1;

// Called whenever /proc/timer is read
static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct timespec64 current_time;
    struct timespec64 elapsed;
    char buf[256];
    int len = 0;

    // Prevent multiple reads from updating the time again
    if (*ppos > 0) {
        return 0;
    }

    // Get current time in seconds and nanoseconds
    ktime_get_real_ts64(&current_time);

    // Store formatted current time in buffer
    len += scnprintf(buf + len,  sizeof(buf) - len, "current time: %lld.%09ld\n", (long long)current_time.tv_sec, current_time.tv_nsec);

    // Calculate elapsed time only if a previous time exists
    if (!first_read) {
        elapsed.tv_sec = current_time.tv_sec - last_time.tv_sec;
        elapsed.tv_nsec = current_time.tv_nsec - last_time.tv_nsec;

        // Adjustr if nanoseconds diff is negative
        if (elapsed.tv_nsec < 0) {
            elapsed.tv_sec--;
            elapsed.tv_nsec += 1000000000L;
        }

        // Add elapsed time to buffer
        len += scnprintf(buf + len,  sizeof(buf) - len, "elapsed time: %lld.%09ld\n", (long long)elapsed.tv_sec, elapsed.tv_nsec);
    }

    // Save current time for next read
    last_time = current_time;
    first_read = 0;

    // Send buffer contents to user
    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
}

// Connect proc file to read func
static const struct proc_ops timer_fops = {
    .proc_read = timer_read,
};

// Called when module is loaded
static int __init timer_init(void)
{
    first_read = 1;

    // Create /proc/timer
    timer_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &timer_fops);
    if (!timer_entry) {
        return -ENOMEM;
    }
    return 0;
}

// Called when module is unloaded
static void __exit timer_exit(void)
{
    // Remove /proc/timer
    proc_remove(timer_entry);
}

// Register module initializzation and exit funcs
module_init(timer_init);
module_exit(timer_exit);
