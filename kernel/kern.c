#include <bwio.h>
#include <task.h>

#define FOREVER for(;;)

void init_kernel();
void kernel_exit(Task* active);
void handle();

int main() {
    Task task_pool[TASK_POOL_SIZE];

    init_kernel(task_pool);

    FOREVER {
        
        // active = schedule( tds );
        // kerxit( active, req );
        // handle( tds, req );
    }
    
    return 0;
}

void init_kernel(Task* task_pool) {
    init_task_pool(task_pool, TASK_POOL_SIZE);
    
    // add first task
}

