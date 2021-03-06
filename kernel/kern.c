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
void kernel_end();

void icu_init();

// IO functions
#define UART_FAST 115200 
#define UART_SLOW 2400

/* http://www.marklin.com/tech/digital1/components/commands.html
 baud rate = 2400 
 start bits = 1 
 stop bits = 2 
 parity = none 
 word size = 8 bits 
 these settings are reflected in: 
                                       | no fifo
 0b 0000 0000 0000 0000 0000 0000 0 11 0 1 0 0 0 
                             8 bits ||   | stop bits 
*/
#define COM1_SETTINGS 0x68 
void io_init();
int io_set_connection( int channel, int speed);

#define TIMER_PER_SEC 508469
#define TIMER_INIT_VAL (TIMER_PER_SEC / 100) // 1 tick = 10 ms
void clock_init();


// ASM
extern void asm_cache_on();
extern void asm_cache_off();
extern void asm_kern_entry();
extern void asm_kern_exit(Task* active, Request** request);
extern void irq_entry();

// statistics
inline int get_time_passed(volatile int* timer_val, int* timer_last) {
    int timer_now = *timer_val;
    int ret = (timer_now > *timer_last) ? (TIMER_INIT_VAL - timer_now) + *timer_last : *timer_last - timer_now;
    *timer_last = timer_now;

    return ret;
}

int main() {
    Task_Scheduler task_scheduler;
    Request* request;

    kernel_init(&task_scheduler);

    volatile int* timer_val = (volatile int*)(TIMER3_BASE + VAL_OFFSET);

    unsigned int total_time_passed = 0;
    unsigned int total_idle_time_passed = 0;

    /*
    unsigned int total_swi_time_passed = 0;
    unsigned int total_irq_time_passed = 0;
    unsigned int total_task_time_passed = 0;
    unsigned int total_bwio_time_passed = 0;
    */

    int timer_last = TIMER_INIT_VAL;
    unsigned int last_printed_time = 0;
    int time_passed;
    FOREVER {
        scheduler_next(&task_scheduler);
        if (task_scheduler.active == 0) {
            break;
        }
        
        //DEBUG_MSG("activiate task %d %d %d %d\n\r", task_scheduler.active->tid, task_scheduler.active->lr, task_scheduler.active->sp, task_scheduler.active->spsr);
        total_time_passed += get_time_passed(timer_val, &timer_last);
        /*
        time_passed = get_time_passed(timer_val, &timer_last);
        total_time_passed += time_passed;

        if (task_scheduler.active->last_request == 0) {
            total_irq_time_passed += time_passed;
        } else {
            total_swi_time_passed += time_passed;
        }
        */

        // DEBUG_MSG("exit kernel \n\r");
        asm_kern_exit(task_scheduler.active, &request);

        time_passed = get_time_passed(timer_val, &timer_last);
        total_time_passed += time_passed;
        if (task_scheduler.active->priority == 31) {
            total_idle_time_passed += time_passed;
        } 
        /*
        else {
            total_task_time_passed += time_passed;
        }
        */
        
        // print percentage usage for idle task
        if (total_time_passed - last_printed_time > (TIMER_PER_SEC * 1)) {
            /* DEBUG_MSG("time percentage: %d/%d=%d, task: %d, swi: %d, hwi: %d, bwio: %d\n\r",
                total_idle_time_passed, total_time_passed,
                total_idle_time_passed * 100 / total_time_passed,
                total_task_time_passed, total_swi_time_passed,
                total_irq_time_passed, total_bwio_time_passed);
            */

            if (task_scheduler.events[KERNEL_STATS].wait_task != 0) {
                return_to_task(total_idle_time_passed * 1000 / total_time_passed, task_scheduler.events[KERNEL_STATS].wait_task, &task_scheduler);
                task_scheduler.events[KERNEL_STATS].wait_task = 0;
            }


            total_time_passed = 0;
            total_idle_time_passed = 0;

            /*
            total_task_time_passed = 0;
            total_swi_time_passed = 0;
            total_irq_time_passed = 0;
            total_bwio_time_passed = 0;
            */

            last_printed_time = total_time_passed;            
        }

        if (task_scheduler.halt == 2) {
            kernel_end();
        }

        /*

        time_passed = get_time_passed(timer_val, &timer_last);
        total_time_passed += time_passed;
        total_bwio_time_passed += time_passed;
        */

        handle(request, &task_scheduler);
    }

    kernel_end();
    
    return 0;
}



void kernel_init(Task_Scheduler* task_scheduler) {
    // turn cache on
    asm_cache_on();

    // initialize io
    io_init();
    
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

    // enable UART 1 General interrupt
    *((int*)(VIC2_BASE + VIC_INT_ENABLE)) |= (0x1u << 20);

    // enable UART 2 General interrupt
    *((int*)(VIC2_BASE + VIC_INT_ENABLE)) |= (0x1u << 22);
}

void clock_init() {
    volatile int* timer_loader = (int*)(TIMER3_BASE + LDR_OFFSET);
    volatile int* timer_control = (int*)(TIMER3_BASE + CTRL_OFFSET);

    *timer_loader = TIMER_INIT_VAL;
    *timer_control = CLKSEL_MASK | MODE_MASK | ENABLE_MASK;
}

void io_init() {
    // UART 2
    io_set_connection(COM2, UART_FAST);
    *((volatile int *)(UART2_BASE + UART_CTLR_OFFSET)) |= (RIEN_MASK);

    // UART 1
    io_set_connection(COM1, UART_SLOW);
    *((volatile int *)(UART1_BASE + UART_CTLR_OFFSET)) |= (RIEN_MASK);
}

/*
 * The UARTs are initialized by RedBoot to the following state
 * 	115,200 bps
 * 	8 bits
 * 	no parity
 * 	fifos enabled
 */
int io_set_connection( int channel, int speed) {
	int *high, *low, *mid;
    

	switch( channel ) {
	case COM1:
		high = (int *)( UART1_BASE + UART_LCRH_OFFSET );
        mid = (int *)( UART1_BASE + UART_LCRM_OFFSET );
		low = (int *)( UART1_BASE + UART_LCRL_OFFSET );
	        break;
	case COM2:
		high = (int *)( UART2_BASE + UART_LCRH_OFFSET );
        mid = (int *)( UART2_BASE + UART_LCRM_OFFSET );
		low = (int *)( UART2_BASE + UART_LCRL_OFFSET );
	        break;
	default:
	        return -1;
	        break;
	}

	switch( speed ) {
	    case UART_FAST:
            *high = *high & ~FEN_MASK; // fifo off
		    *mid = 0x0;
		    *low = 0x3;
		    return 0;
	    case UART_SLOW:
		    *high = (*high & 0xffffff00) | COM1_SETTINGS; // turns fifo off as well
            // if train doesn't work, try this instead:
            //*high = (*high | STP2_MASK) & ~PE_MASK & ~FEN_MASK;
            *mid = 0x0;
            *low = 0xbf;
		    return 0;
	    default:
		    return -1;
	}
}

void kernel_end() {
    // disble ICU
    *((int*)(VIC1_BASE + VIC_INT_ENABLE)) = 0x0;
    *((int*)(VIC2_BASE + VIC_INT_ENABLE)) = 0x0;
 
    // disable clock   
    *((int*)(TIMER3_BASE + CTRL_OFFSET)) &= ~ENABLE_MASK;

    // disable io interrupt
    *((volatile int *)(UART1_BASE + UART_CTLR_OFFSET)) &= ~(TIEN_MASK | RIEN_MASK | MSIEN_MASK);
    *((volatile int *)(UART2_BASE + UART_CTLR_OFFSET)) &= ~(TIEN_MASK | RIEN_MASK);

}
