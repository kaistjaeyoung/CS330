#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

#include "threads/synch.h"

struct lock syscall_lock;

#endif /* userprog/syscall.h */
