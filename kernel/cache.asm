	.file	"cache.c"
	.text
	.align	2
	.global	cache_on
	.type	cache_on, %function
cache_on:
    # Read Control Register
    mrc p15, 0, r0, c1, c0, 0;

    # 12 - instruction cache, 2 - data cache
    orr r0, r0, #0x1000;
    orr r0, r0, #0x4;

    # Write to control register
    mcr p15, 0, r0, c1, c0, 0;

    mov pc, lr;
cache_off:
    # Read Control Register
    mrc p15, 0, r0, c1, c0, 0;

    # 12 - instruction cache, 2 - data cache
    bic r0, r0, #0x1000;
    bic r0, r0, #0x4;

    # Write to control register
    mcr p15, 0, r0, c1, c0, 0;

    mov pc, lr;
