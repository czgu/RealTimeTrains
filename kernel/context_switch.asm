	.file	"kernel_switch.c"
	.text
	.align	2
	.global	asm_kern_exit
	.type	asm_kern_exit, %function
asm_kern_exit:
    # store kernel registers
    # store TD and Request, r0 = TD, r1 = Request
    stmfd   sp!, {r0, r1, r4-r12, lr};

    msr     CPSR_c, #31; # change to system mode

    # move TD to r3
    mov r3, r0;

    # put the return value
    ldr r2, [r3, #20];
    mov r0, r2; 
    
    # restore sp_user
    ldr r2, [r3, #8];
    mov sp, r2;

    ldmfd sp!, {r4-r12, lr}; # restore user space registers

    msr CPSR_c, #19; # change to supervisor mode
  
    # restore spsr
    ldr r2, [r3, #16];
    msr spsr, r2;

    # set lr_svc = pc_before_swi
    ldr r2, [r3, #12];
    mov lr, r2;

    # exit to user space
    movs pc, lr;

	.size	asm_kern_exit, .-asm_kern_exit
	.align	2
	.global	asm_kern_entry
	.type	asm_kern_entry, %function
asm_kern_entry:
    MSR     CPSR_c, #31; # change to system mode

    # store user registers
    stmfd sp!, {r4-r12, lr};

    mov r2, sp; # acqure sp of the active task

    msr CPSR_c, #19; # change to supervisor mode

    mov r3, r0; # move request value to r3

    #Get TD and Request
    ldmfd sp!, {r0, r1};

    # save sp to TD
    str r2, [r0, #8];

    # save spsr to TD
    mrs r2, spsr;
    str r2, [r0, #16];

    # save lr to TD
    str lr, [r0, #12];

    #Fill Request
    str r3, [r1, #0];

    #pop TD and request
    #restore kernel register
    ldmfd sp!, {r4-r12, lr};

    mov pc, lr;
	.size	asm_kern_entry, .-asm_kern_entry
	.align	2
	.global	swi_jump
	.type	swi_jump, %function


swi_jump:
    mov ip, sp;
    stmfd sp!, {ip, lr};
    #r0 holds value of syscall
    swi;

    ldmfd sp, {sp, pc};

	.size	swi_jump, .-swi_jump
	.ident	"GCC: (GNU) 4.0.2"
