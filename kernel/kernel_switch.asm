	.file	"kernel_switch.c"
	.text
	.align	2
	.global	dummy
	.type   dummy, %function
dummy:
    stmfd   sp!, {r4-r12};

    # SYSTEM MODE
    msr     CPSR_c, #31; # change to system mode
    # SUPERVISOR MODE
    msr     CPSR_c, #19; # change to svc mode

    #restore kernel register
    ldmfd sp!, {r4-r12};
    mov pc, lr;

	.global	swi_kern_exit
	.type	swi_kern_exit, %function
swi_kern_exit:
    # store kernel registers
    stmfd   sp!, {r4-r12, lr};
    msr     CPSR_c, #31; # change to system mode

    # load TD
    ldr r3, [fp, #-20] 
    
    # put the return value
    ldr r2, [r3, #20];
    mov r0, r2; 
    
    # restore sp_user
    ldr r2, [r3, #8];
    mov sp, r2;

    ldmfd sp!, {r4-r12}; # restore user space registers

    msr CPSR_c, #19; # change to supervisor mode
  
    # restore spsr
    ldr r2, [r3, #16];
    msr spsr, r2;

    # set lr_svc = lr_user
    ldr r2, [r3, #12];
    mov lr, r2;

    # exit to user space
    movs pc, lr;
    #b swi_kern_entry;


	.size	swi_kern_exit, .-swi_kern_exit
	.align	2
	.global	swi_kern_entry
	.type	swi_kern_entry, %function
swi_kern_entry:
    #mov r0, #1;
    #mov r1, sp;
    #bl bwputr;

    mov r1, lr; # 2nd line of swi_jump

    MSR     CPSR_c, #31; # change to system mode

    # store user registers
    stmfd sp!, {r4-r12};

    mov r2, sp; # acqure sp of the active task

    msr CPSR_c, #19; # change to supervisor mode

    #restore kernel register
    ldmfd sp!, {r4-r12, lr};

    #Get TD
    ldr r3, [fp, #-16]; 

    # save sp
    str r2, [r3, #8];

    # save spsr
    mrs r2, spsr;
    str r2, [r3, #16];

    # save lr
    str r1, [r3, #12];

    #TODO: Fill Request

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
