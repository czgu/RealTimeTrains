#include <bwio.h>
#include <task.h>
#include <pqueue.h>
#include <memory.h>
#include <ts7200.h>

#include <k_syscall.h>
#include <user_task.h>

#define FOREVER for(;;)

// Initialization functions
void kernel_init(Task_Scheduler* scheduler);
void icu_init();

#define TIMER_PER_SEC 508469
#define TIMER_INIT_VAL (TIMER_PER_SEC / 100) // 1 tick = 10 ms
void clock_init();

// ASM
extern void asm_cache_on();
extern void asm_cache_off();
extern void asm_kern_entry();
extern void asm_kern_exit(Task* active, Request** request);
extern void irq_entry();

int main() {
    Task_Scheduler task_scheduler;
    Request* request;

    kernel_init(&task_scheduler);

    //volatile int* timer_val = (volatile int*)(TIMER3_BASE + VAL_OFFSET);
    //long long time_passed = 0;
    //long long idle_time_passed = 0;

    FOREVER {
        scheduler_next(&task_scheduler);
        if (task_scheduler.active == 0) {
            break;
        }
        // DEBUG_MSG("activiate task %d %d %d %d\n\r", task_scheduler.active->tid, task_scheduler.active->lr, task_scheduler.active->sp, task_scheduler.active->spsr);

        int time_now = *timer_val;

        asm_kern_exit(task_scheduler.active, &request);

        handle(request, &task_scheduler);
    }
    
    return 0;
}

void kernel_init(Task_Scheduler* task_scheduler) {
    // turn cache on
    asm_cache_on();

    // initialize io
    bwsetfifo(COM2, OFF);
    bwsetspeed(COM2, 115200);
    
    // initialize low memory
    *((unsigned int*)SWI_JUMP_TABLE) = (unsigned int)&asm_kern_entry;
    *((unsigned int*)SWI_FIRST_INSTRUCTION_LOCATION) = SWI_JUMP_INSTRUCTION;

    *((unsigned int*)IRQ_JUMP_TABLE) = (unsigned int)&irq_entry;
    *((unsigned int*)IRQ_FIRST_INSTRUCTION_LOCATION) = IRQ_JUMP_INSTRUCTION;

    // initialize hardware
    clock_init();

    // initialize ICU
    icu_init();

    // initialize scheduler
    scheduler_init(task_scheduler);

    // initialize first task
    k_create(15, (void *)first_task, task_scheduler);
}

void icu_init() {
    // set all interrupt to irq
    *((int*)(VIC1_BASE + VIC_INT_SELECT)) = 0x0;
    *((int*)(VIC2_BASE + VIC_INT_SELECT)) = 0x0;

    // disable all interrupts
    *((int*)(VIC1_BASE + VIC_INT_ENABLE)) = 0x0;
    *((int*)(VIC2_BASE + VIC_INT_ENABLE)) = 0x0;

    // enable Timer 3 interrupt
    *((int*)(VIC2_BASE + VIC_INT_ENABLE)) |= (0x1u << 19);
}

void clock_init() {
    unsigned int timer_base = TIMER3_BASE;
    volatile int* timer_loader = (int*)(timer_base + LDR_OFFSET);
    volatile int* timer_control = (int*)(timer_base + CTRL_OFFSET);

    *timer_loader = TIMER_INIT_VAL;
    *timer_control = CLKSEL_MASK | MODE_MASK | ENABLE_MASK;
}
