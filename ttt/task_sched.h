#pragma once
typedef void(task_callback_t)(void *arg);

void task_init(void);
void timer_init(void);
// void task_add(task_callback_t *func, void *param);
void task_add(task_callback_t *func, void *param);
int sched_start(void);
void preempt_disable(void);
void preempt_enable(void);
void schedule(void);

#define task_printf(...)     \
    ({                       \
        preempt_disable();   \
        printf(__VA_ARGS__); \
        preempt_enable();    \
    })
