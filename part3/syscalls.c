#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/linkage.h>
#include <linux/export.h>

/* stub function pointers initialized to NULL.
   elevator.ko assigns these pointers when loaded and clears them when unloaded.
 */
int (*STUB_start_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_start_elevator);

int (*STUB_issue_request)(int, int, int) = NULL;
EXPORT_SYMBOL(STUB_issue_request);

int (*STUB_stop_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_stop_elevator);

/* system call 548: start_elevator */
SYSCALL_DEFINE0(start_elevator)
{
    if (STUB_start_elevator)
        return STUB_start_elevator();
    return -ENOSYS;
}

/* system call 549: issue_request */
SYSCALL_DEFINE3(issue_request, int, start_floor, int, destination_floor, int, type)
{
    if (STUB_issue_request)
        return STUB_issue_request(start_floor, destination_floor, type);
    return -ENOSYS;
}

/* system call 550: stop_elevator */
SYSCALL_DEFINE0(stop_elevator)
{
    if (STUB_stop_elevator)
        return STUB_stop_elevator();
    return -ENOSYS;
}
