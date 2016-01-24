	.file	"kernel_switch.c"
	.text
	.align	2
	.global	swi_kern_exit
	.type	swi_kern_exit, %function
swi_kern_exit:
    # store kernel registers
    stmfd   sp!, {r4-r12, lr};

    # store TD and Request, r0 = TD, r1 = Request
    stmfd sp!, {r0, r1};

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

	.size	swi_kern_exit, .-swi_kern_exit
	.align	2
	.global	swi_kern_entry
	.type	swi_kern_entry, %function
swi_kern_entry:
    mov r1, lr; # pc when we go back to user

    MSR     CPSR_c, #31; # change to system mode

    # store user registers
    stmfd sp!, {r4-r12, lr};

    mov r2, sp; # acqure sp of the active task

    msr CPSR_c, #19; # change to supervisor mode

    #Get TD
    ldr r3, [sp, #0];

    # save sp
    str r2, [r3, #8];

    # save spsr
    mrs r2, spsr;
    str r2, [r3, #16];

    # save lr
    str r1, [r3, #12];

    #Fill Request
    ldr r3, [sp, #4];
    str r0, [r3, #0];

    #pop TD and request
    ldmfd sp!, {r0, r1}

    #restore kernel register
    ldmfd sp!, {r4-r12, lr};

    mov pc, lr;
	.size	swi_kern_entry, .-swi_kern_entry
	.align	2
	.global	swi_jump
	.type	swi_jump, %function


swi_jump:
    mov ip, sp;
    stmfd sp!, {fp, ip, lr, pc};
    #r0 holds value of syscall
    swi;

    ldmfd sp, {fp, sp, pc};

	.size	swi_jump, .-swi_jump
	.ident	"GCC: (GNU) 4.0.2"
