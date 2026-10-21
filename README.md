# COP4610-Project 2 (group 38)
Project 2 focuses on working with system calls, kernel modules, and modifying the Linux kernel. The project begins with tracing system calls and creating a timer kernel module. It then involves adding new system calls to the Linux kernel, compiling and installing the modified kernel, and implementing an elevator system using kernel threads, linked lists, mutexes, and a scheduling algorithm.

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
│   └── Makefile
├── part3/
│   ├── src/
│   ├── tests/
│   ├── Makefile
│   └── sys_call.c
├── Makefile
└── README.md

```
# How to Compile & Execute

### Requirements
- **Operating System:** Linux (Ubuntu VM)
- **Compiler:** GCC
- **Build Tools:** Make
- **Kernel Version:** Linux 6.16.12
- **Dependencies:** Linux kernel headers, kernel build tools, and `strace`
- **Additional Requirements:** Root privileges (`sudo`) for loading and removing kernel modules.

## Part 1
### Compilation
```bash
cd part1/
make
```
### Execution
```bash
strace -o empty.trace ./empty
strace -o part1.trace ./part1
wc -l empty.trace part1.trace
```


## Part 2

### Compilation
```bash
cd part2/
make
```
### Execution
```bash
sudo insmod my_timer.ko
cat /proc/timer
sleep 1
cat /proc/timer
sleep 3
cat /proc/timer
sleep 5
cat /proc/timer
sudo rmmod my_timer
```


## Part 3
### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in ...
### Execution
```bash
make run
```
This will run the program ...

## Development Log
Each member records their contributions here.

### Rodney Carey

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |

### Dhruv Patel

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |


### Mahir Masud

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |


## Meetings
We had three major meetings on Discord and all members attended.

### Meeting 1: October 13, 2026  
We discussed the project requirements and the different parts that needed to be completed. We went over who would work on each part and how to divide the responsibilities among the group. By the end of the meeting, we had a division of labor so everyone knew which parts they were responsible for.




## Bugs
- **Bug 1**: This is bug 1.
- **Bug 2**: This is bug 2.
- **Bug 3**: This is bug 3.

## Considerations
