# COP4610-Project 2 (group 38)
### Slack Days Used: 3
Project 2 focuses on working with system calls, kernel modules, and modifying the Linux kernel. The project begins with tracing system calls and creating a timer kernel module. It then involves adding new system calls to the Linux kernel, compiling and installing the modified kernel, and implementing an elevator system using kernel threads, linked lists, mutexes, and a scheduling algorithm.

## Table of Contents
1. [Group Members](#group-members)
2. [Division of Labor](#division-of-labor)
3. [File Listing](#file-listing)
4. [How to Compile & Execute](#how-to-compile--execute)
5. [Development Log](#development-log)
6. [Meetings](#meetings)
7. [Bugs](#bugs)
8. [Considerations](#considerations)
9. [Use of AI](#use-of-ai)

## Group Members
- **Rodney Carey**: rc24n@fsu.edu
  - Github ID: rodneycd
- **Dhruv Patel**: drp24@fsu.edu
  - Github ID: DhruvP16
- **Mahir Asef Bin Masud**: mm23bu@fsu.edu
  - Github ID: mm23bu

## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: This part introduces system call tracing using `strace`. An empty C program is compared with another C program containing exactly five additional system calls. The trace files are compared to verify that the additional system calls were executed.
- **Assigned to**: Dhruv Patel, Mahir Masud

### Part 2: Timer Kernel Module
- **Responsibilities**: This part involves creating a Linux kernel module that uses the `/proc` filesystem to track elapsed time. The timer module reports the time between reads of `/proc/timer`.
- **Assigned to**: Mahir Masud, Rodney Carey

### Part 3a: Adding System Calls
- **Responsibilities**: This part involves adding three custom system calls to the Linux kernel to communicate with the elevator module.
- **Assigned to**: Mahir Masud, Rodney Carey

### Part 3b: Kernel Compilation
- **Responsibilities**: This part involves configuring, compiling, and installing the modified Linux kernel. The kernel must include the three new system calls and successfully boot in the virtual machine.
- **Assigned to**: Mahir Masud, Rodney Carey

### Part 3c: Threads
- **Responsibilities**: This part involves implementing a kernel thread to manage the elevator's operation. The thread controls elevator movement and updates its state while the module is running.
- **Assigned to**: Dhruv Patel, Mahir Masud

### Part 3d: Linked List
- **Responsibilities**: This part uses Linux kernel linked lists to store and manage passengers waiting for the elevator. Passenger requests are added and removed as the elevator picks up and drops off passengers.
- **Assigned to**: Dhruv Patel, Mahir Masud

### Part 3e: Mutexes
- **Responsibilities**: This part uses mutexes to protect shared elevator data and prevent race conditions when multiple operations access or modify passenger requests and elevator information.
- **Assigned to**: Dhruv Patel, Mahir Masud, Rodney Carey

### Part 3f: Scheduling Algorithm
- **Responsibilities**: This part implements the elevator scheduling algorithm. The algorithm determines how the elevator moves between floors, picks up passengers, drops them off, and handles incoming requests.
- **Assigned to**: Dhruv Patel, Mahir Masud, Rodney Carey

## File Listing
```
elevator/
├── Makefile
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
│   └── Makefile
├── part2/
│   ├── src/
│   │   └── timer_core.c
│   └── Makefile
├── part3/
│   ├── src/
│   │   ├── elevator.c
│   │   └── elevator.h
│   ├── tests/
│   ├── Makefile
│   └── sys_call.c
├── Makefile
└── README.md

```

### File Descriptions

**Root Directory**
- `Makefile` – Provides build commands for the project.
- `README.md` – Contains project information, instructions, contributions, and documentation.

**Part 1**
- `empty.c` – A basic C program used as the baseline for system call tracing.
- `part1.c` – A C program that performs five additional system calls.
- `empty.trace` – Stores the system call trace generated from `empty.c`.
- `part1.trace` – Stores the system call trace generated from `part1.c`.
- `Makefile` – Compiles the Part 1 programs and generates trace files.

**Part 2**
- `src/timer_core.c` – Implements the timer kernel module, which displays the current time and elapsed time through `/proc/timer`.
- `Makefile` – Compiles the timer kernel module into `my_timer.ko`.

**Part 3**
- `src/elevator.c` – Implements the elevator's operations, including passenger management, kernel threading, synchronization, and scheduling.
- `src/elevator.h` – Contains definitions, structures, and function declarations used by the elevator implementation.
- `tests/elevator/` – Contains the producer and consumer testing programs used to generate passenger requests and control the elevator.
- `tests/syscalls/` – Contains tests used to verify that the custom elevator system calls are available and working.
- `sys_call.c` – Contains code associated with the custom elevator system calls.
- `Makefile` – Compiles the Part 3 kernel module.


# How to Compile & Execute

### Requirements
- **Operating System:** Linux (Ubuntu VM)
- **Compiler:** GCC
- **Build Tools:** Make
- **Kernel Version:** Linux 6.16.12
- **Dependencies:** Linux kernel headers, kernel build tools, and `strace`
- **Additional Requirements:** Root privileges (`sudo`) for loading and removing kernel modules.

### Part 1: System Call Tracing
**Compilation**
```bash
cd part1/
make
```
This compiles `empty.c` and `part1.c`.

**Execution**

```bash
make trace
wc -l empty.trace part1.trace
```
The `make trace` command runs both programs using `strace` and generates their trace files. The `wc -l` command compares the number of lines in the trace files to verify the five additional system calls.

**Cleanup**
```bash
make clean
```


### Part 2: Timer Kernel Module

**Compilation**

```bash
cd part2/
make
```

This compiles the timer kernel module and generates `my_timer.ko`.

**Execution**

Load the kernel module:

```bash
sudo insmod my_timer.ko
```

Read the timer file with different delays:

```bash
cat /proc/timer
sleep 1
cat /proc/timer
sleep 3
cat /proc/timer
sleep 5
cat /proc/timer
```

The first read displays the current time. Subsequent reads display the current time and elapsed time since the previous read.

**Removing the Module**

```bash
sudo rmmod my_timer
```

**Cleanup**

```bash
make clean
```


### Part 3: Elevator Kernel Module

Part 3 uses three custom system calls:

- `548` – `start_elevator()`
- `549` – `issue_request()`
- `550` – `stop_elevator()`

**Compilation**

```bash
cd part3/
make
```

This compiles the elevator kernel module and generates `elevator.ko`.

**Loading the Module**

```bash
sudo insmod elevator.ko
```

**Execution and Testing**

Open two terminal sessions.

**Terminal 1: Monitor the Elevator**

```bash
watch -n1 cat /proc/elevator
```

This continuously displays the elevator's current state, floor, passengers, and waiting requests.

**Terminal 2: Run the Tests**

Navigate to the elevator testing directory:

```bash
cd part3/tests/elevator/
make
```

Start the elevator:

```bash
./consumer --start
```

Generate passenger requests:

```bash
./producer 10
```

The elevator should begin transporting passengers according to its scheduling algorithm. Additional passenger requests can be generated by running the producer again.

To stop the elevator:

```bash
./consumer --stop
```

The elevator must finish dropping off passengers already inside before stopping. Passengers still waiting on floors will not be picked up after the stop request.

The elevator can be restarted using:

```bash
./consumer --start
```

**Removing the Module**

Return to the Part 3 directory and remove the elevator module:

```bash
sudo rmmod elevator
```

**Cleanup**

```bash
make clean
```


## Development Log
Each member records their contributions here.

### Rodney Carey

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-10-18 | Worked on Part 2 timer kernel module  |
| 2026-11-01 | Worked on Part 3 scheduling algorithm  |

### Dhruv Patel

| Date | Work Completed / Notes |
|-----|-----|
| 2026-10-15 | Worked on Part 1 system call tracing and tested the additional system calls using `strace`. |
| 2026-10-19 | Worked on Part 2 timer kernel module and tested the `/proc/timer` functionality. |
| 2026-10-21 | Compiled, installed, and tested Linux kernel 6.16.12, including verifying the custom system calls. |
| 2026-10-30 | Added mutex synchronization to the Part 2 timer module.|
| 2026-10-31 | Worked on implementing the kernel thread and linked lists for Part 3 to manage elevator operations and passenger requests. |
| 2026-10-31 | Added a temporary `next_floor` function to allow the elevator to move between floors before the final scheduling algorithm was implemented. |


### Mahir Masud

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-10-08 | initialized Github repository and basic file architecture.  |
| 2026-10-28 | implemented syscalls.c with stubs for syscalls and added elevator module scaffold. |
| 2026-10-29 | added elevator start/stop stubs with OFFLINE/IDLE state tracking and mutex protection. |
| 2026-10-30 | added proc/elevator interface showing elevator state, current floor, load, and waiting counts. |


## Meetings
We had three major meetings on Discord and all members attended.

### Meeting 1: October 13, 2026  
We discussed the project requirements and the different parts that needed to be completed. We went over who would work on each part and how to divide the responsibilities among the group. By the end of the meeting, we had a division of labor so everyone knew which parts they were responsible for.

### Meeting 2: October 18, 2026

We discussed accessing and setting up the virtual machine for the project. We also went over starting Parts 1 and 2, including system call tracing and the timer kernel module. Additionally, we discussed downloading, compiling, and installing the modified Linux kernel needed for Part 3.

### Meeting 3: October 28, 2026

We discussed the division of work for Part 3, including kernel threads, linked lists, mutexes, and the scheduling algorithm. We reviewed different approaches to managing elevator movement and passenger requests, including which functions and scheduling methods would work best. We also discussed the file structure and how to organize the implementation and testing files.

## Bugs

No confirmed bugs have been documented at this time.

## Considerations

- The elevator operates between floors 1 and 5 and cannot move directly from floor 1 to floor 5 or vice versa.
- The elevator can carry a maximum of 5 pets with a combined weight of no more than 50 lbs.
- The elevator takes 2 seconds to move between adjacent floors and 1 second for loading or unloading.
- The `/proc/elevator` file can be assumed to have a maximum length of 10,000 characters.
- Pets will not request to travel to the same floor where they entered.
- The elevator must finish dropping off its current passengers when stopped, without picking up additional passengers.

## Use of AI

ChatGPT was used as a supporting tool during installation of the kernel for the project, mainly for understanding instructions, organizing steps, and troubleshooting errors.

During the Linux kernel installation, the provided PowerPoint instructions were uploaded to ChatGPT to help organize the installation process into clear, step-by-step instructions. These steps were then followed to download, configure, compile, and install the modified Linux kernel.

ChatGPT was also used to help understand and troubleshoot errors encountered during the kernel installation and compilation process, including identifying possible causes and suggesting solutions.
