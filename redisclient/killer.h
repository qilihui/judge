#ifndef __KILLER__
#define __KILLER__

struct timeout_killer_args {
    int pid;
    int timeout;
};

int kill_pid(pid_t pid);

void *timeout_killer(void *timeout_killer_args);

#endif
