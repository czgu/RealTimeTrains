#define SWI_FIRST_INSTRUCTION_LOCATION 0x08
#define SWI_JUMP_INSTRUCTION 0xe59ff018 // ldr pc, [pc, 0x18]
#define SWI_JUMP_TABLE 0x28

#define IRQ_FIRST_INSTRUCTION_LOCATION 0x18
#define IRQ_JUMP_INSTRUCTION 0xe59ff018 // ldr pc, [pc, 0x18]
#define IRQ_JUMP_TABLE 0x38

//#define TASK_BASE_SP    0x00400000
#define TASK_BASE_SP    0x00280000
//#define TASK_STACK_SIZE 524288      // 0.5 MB
#define TASK_STACK_SIZE 262144
