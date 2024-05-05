/* task_sched: preemptive multitasking in userspace based on SIGALRM signal.
 *
 * This program starts 3 sorting routines, execution of each is preempted by
 * SIGALRM signal, simulating an OS timer interrupt.  Each routine is an
 * execution context, which can do a voluntary scheduling (calling schedule()
 * directly) or be preempted by a timer, and in that case nonvoluntary
 * scheduling occurs.
 *
 * The default time slice is 10ms, that means that each 10ms SIGALRM fires and
 * next context is scheduled by round robin algorithm.
 */

#define _GNU_SOURCE
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

#include "../list.h"
#include "task_sched.h"

static int preempt_count = 0;
void preempt_disable(void)
{
    preempt_count++;
}
void preempt_enable(void)
{
    preempt_count--;
}

static void local_irq_save(sigset_t *sig_set)
{
    sigset_t block_set;
    sigfillset(&block_set);
    sigdelset(&block_set, SIGINT);
    sigprocmask(SIG_BLOCK, &block_set, sig_set);
}

static void local_irq_restore(sigset_t *sig_set)
{
    sigprocmask(SIG_SETMASK, sig_set, NULL);
}

struct task_struct {
    struct list_head list;
    ucontext_t context;
    void *stack;
    task_callback_t *callback;
    void *arg;
    bool reap_self;
};

static struct task_struct *task_current, task_main;
static LIST_HEAD(task_reap);

void task_init(void)
{
    INIT_LIST_HEAD(&task_main.list);
    task_current = &task_main;
}

static struct task_struct *task_alloc(task_callback_t *func, void *arg)
{
    struct task_struct *task = calloc(1, sizeof(*task));
    task->stack = calloc(1, 1 << 20);
    task->callback = func;
    task->arg = arg;
    return task;
}

static void task_destroy(struct task_struct *task)
{
    list_del(&task->list);
    free(task->stack);
    free(task);
}

static void task_switch_to(struct task_struct *from, struct task_struct *to)
{
    task_current = to;
    swapcontext(&from->context, &to->context);
}

void schedule(void)
{
    sigset_t set;
    local_irq_save(&set);

    struct task_struct *next_task =
        list_first_entry(&task_current->list, struct task_struct, list);
    if (next_task) {
        if (task_current->reap_self)
            list_move(&task_current->list, &task_reap);
        task_switch_to(task_current, next_task);
    }
    struct task_struct *task, *tmp;
    list_for_each_entry_safe (task, tmp, &task_reap, list) /* clean reaps */
        task_destroy(task);

    local_irq_restore(&set);
}

union task_ptr {
    void *p;
    int i[2];
};

static void local_irq_restore_trampoline(struct task_struct *task)
{
    sigdelset(&task->context.uc_sigmask, SIGALRM);
    local_irq_restore(&task->context.uc_sigmask);
}

__attribute__((noreturn)) static void task_trampoline(int i0, int i1)
{
    union task_ptr ptr = {.i = {i0, i1}};
    struct task_struct *task = ptr.p;

    /* We switch to trampoline with blocked timer.  That is safe.
     * So the first thing that we have to do is to unblock timer signal.
     * Paired with task_add().
     */
    local_irq_restore_trampoline(task);
    task->callback(task->arg);
    task->reap_self = true;
    schedule();

    __builtin_unreachable(); /* shall not reach here */
}

void task_add(task_callback_t *func, void *param)
{
    struct task_struct *task = task_alloc(func, param);
    if (getcontext(&task->context) == -1)
        abort();

    task->context.uc_stack.ss_sp = task->stack;
    task->context.uc_stack.ss_size = 1 << 20;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = NULL;

    union task_ptr ptr = {.p = task};
    makecontext(&task->context, (void (*)(void)) task_trampoline, 2, ptr.i[0],
                ptr.i[1]);

    /* When we switch to it for the first time, timer signal must be blocked.
     * Paired with task_trampoline().
     */
    sigaddset(&task->context.uc_sigmask, SIGALRM);

    preempt_disable();
    list_add_tail(&task->list, &task_main.list);
    preempt_enable();
}

static void timer_handler(int signo, siginfo_t *info, ucontext_t *ctx)
{
    if (preempt_count) /* once preemption is disabled */
        return;

    /* We can schedule directly from sighandler because Linux kernel cares only
     * about proper sigreturn frame in the stack.
     */
    schedule();
}

void timer_init(void)
{
    struct sigaction sa = {
        .sa_handler = (void (*)(int)) timer_handler,
        .sa_flags = SA_SIGINFO,
    };
    sigfillset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);
}

static void timer_create(unsigned int usecs)
{
    ualarm(usecs, usecs);
}
static void timer_cancel(void)
{
    ualarm(0, 0);
}

static void timer_wait(void)
{
    sigset_t mask;
    sigprocmask(0, NULL, &mask);
    sigdelset(&mask, SIGALRM);
    sigsuspend(&mask);
}

int sched_start()
{
    preempt_disable();
    timer_create(10 * 1000); /* 10 ms */

    while (!list_empty(&task_main.list) || !list_empty(&task_reap)) {
        preempt_enable();
        timer_wait();
        preempt_disable();
    }

    preempt_enable();
    timer_cancel();
    return 0;
}
