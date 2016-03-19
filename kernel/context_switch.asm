	.file	"kernel_switch.c"
	.text
	.align 4
	.global	asm_kern_exit
	.type	asm_kern_exit, %function
asm_kern_exit:
    # store kernel registers
    # store TD and Request, r0 = TD, r1 = Request
    stmfd   sp!, {r0, r1, r4-r12, lr};

    # move TD to r3
    mov r3, r0;

    # restore spsr
    ldr r2, [r3, #16];
    msr spsr, r2;

    # set lr_svc = pc_before_swi
    ldr lr, [r3, #12];

    # check if last_request is hardware interrupt
    ldr r2, [r3, #40];
    cmp r2, #0;

    # if not hardware, put the return value
    ldrne r0, [r3, #20];
    # if not hardware, put in the sp in r4
    ldrne r5, [r3, #8]; 
    # if not hardware, we do not go to system mode and restore anything
    bne _not_hardware;     

    # hardware switch
    msr     CPSR_c, #0xDF; # change to system mode, interrupt off

    # restore sp_user
    ldr sp, [r3, #8];
    ldmfd sp!, {r0-r12, lr}; # restore user space registers

    _not_hardware:
    msr CPSR_c, #0xD3; # change to supervisor mode, interrupt off
    
    # exit to user space, pc = lr_svc
    movs pc, lr;

	.size	asm_kern_exit, .-asm_kern_exit
	.align	4
	.global	irq_entry
	.type	irq_entry, %function
irq_entry:
    # manually shift lr back because of hardware interrupt
    sub lr, lr, #4;

    MSR     CPSR_c, #0xDF; # change to system mode, interrupt off

    stmfd sp!, {r0-r12,lr}; # save on user stack

    mov r3, sp;
    mov r0, #0; #indicate it's a hardware interrupt

    msr CPSR_c, #0xD2; #irq mode, interrupt off


    mov r1, lr; # let r1 = lr_irq
    mrs r2, spsr; # let r2 = spsr_irq

    msr CPSR_c, #0xD3; # change to supervisor mode, interrupt off

    #Get TD and Request
    ldmfd sp!, {r4};

    # save sp to TD
    str r3, [r4, #8];

    # save spsr to TD
    str r2, [r4, #16];

    # save lr_svc to TD
    str r1, [r4, #12];

    #Fill Request
    ldmfd sp!, {r4};
    str r0, [r4, #0];

    #popped TD and request
    #restore kernel register
    ldmfd sp!, {r4-r12, lr};

    mov pc, lr;
	.size	irq_entry, .-irq_entry
    .align 2
	.ident	"GCC: (GNU) 4.0.2"
	.global	asm_kern_entry
	.type	asm_kern_entry, %function
asm_kern_entry:
    mov r1, lr; # let r1 = lr_svc
    mrs r2, spsr; # let r2 = spsr_irq
    # r5 holds sp when entered

    #Get TD and Request
    ldmfd sp!, {r4};

    # save sp to TD
    str r5, [r4, #8];

    # save spsr to TD
    str r2, [r4, #16];

    # save lr_svc to TD
    str r1, [r4, #12];

    #Fill Request
    ldmfd sp!, {r4};
    str r0, [r4, #0];

    #popped TD and request
    #restore kernel register
    ldmfd sp!, {r4-r12, lr};

    mov pc, lr;
	.size	asm_kern_entry, .-asm_kern_entry
	.align 4
	.global	swi_jump
	.type	swi_jump, %function
swi_jump:
    mov ip, sp;
    stmfd sp!, {ip, lr};

    # store user registers
    stmfd sp!, {r4-r12};

    mov r5, sp;

    #r0 holds value of syscall
    swi;

    mov sp, r5;

    ldmfd sp!, {r4-r12};
    ldmfd sp, {sp, pc};

    .align 4
	.size	swi_jump, .-swi_jump
	.ident	"GCC: (GNU) 4.0.2"
