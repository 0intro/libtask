/* Copyright (c) 2005-2006 Russ Cox, MIT; see COPYRIGHT */

#if defined(__FreeBSD__) && defined(__i386__) && __FreeBSD__ < 5
#define NEEDX86CONTEXT 1
#define SET setmcontext
#define GET getmcontext
#endif

#if defined(__OpenBSD__) && defined(__i386__)
#define NEEDX86CONTEXT 1
#define SET setmcontext
#define GET getmcontext
#endif

#if defined(__APPLE__)
#if defined(__aarch64__) || defined(__arm64__)
#define NEEDARM64CONTEXT 1
#define SET __setmcontext
#define GET __getmcontext
#elif defined(__i386__)
#define NEEDX86CONTEXT 1
#define SET _setmcontext
#define GET _getmcontext
#elif defined(__x86_64__)
#define NEEDAMD64CONTEXT 1
#define SET _setmcontext
#define GET _getmcontext
#else
#define NEEDPOWERCONTEXT 1
#define SET __setmcontext
#define GET __getmcontext
#endif
#endif

#if defined(__linux__) && defined(__amd64__)
#define NEEDAMD64CONTEXT 1
#define SET setmcontext
#define GET getmcontext
#endif

#if defined(__linux__) && defined(__arm__)
#define NEEDARMCONTEXT 1
#define SET setmcontext
#define GET getmcontext
#endif

#if defined(__linux__) && defined(__mips__)
#define NEEDMIPSCONTEXT 1
#define SET setmcontext
#define GET getmcontext
#endif

#ifdef NEEDARM64CONTEXT

.text
.p2align 2

.globl GET
GET:
    /* Save callee-saved general-purpose registers */
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]

    /* Save frame pointer and link register */
    stp x29, x30, [x0, #80]

    /* Save stack pointer */
    mov x9, sp
    str x9, [x0, #96]

    /* Return 1 when this context is restored */
    mov x9, #1
    str x9, [x0, #104]
    str x1, [x0, #112]

    /* Save callee-saved floating-point registers */
    stp d8, d9, [x0, #120]
    stp d10, d11, [x0, #136]
    stp d12, d13, [x0, #152]
    stp d14, d15, [x0, #168]

    /* First getcontext call returns 0 */
    mov w0, #0
    ret


.globl SET
SET:
    /* x0 points to mcontext */
    mov x9, x0

    /* Restore general-purpose registers */
    ldp x19, x20, [x9, #0]
    ldp x21, x22, [x9, #16]
    ldp x23, x24, [x9, #32]
    ldp x25, x26, [x9, #48]
    ldp x27, x28, [x9, #64]

    /* Restore frame pointer and return address */
    ldp x29, x30, [x9, #80]

    /* Restore stack pointer */
    ldr x10, [x9, #96]

    /* Restore floating-point registers */
    ldp d8, d9, [x9, #120]
    ldp d10, d11, [x9, #136]
    ldp d12, d13, [x9, #152]
    ldp d14, d15, [x9, #168]

    mov sp, x10

    /* Restore return value / task arguments */
    ldr x0, [x9, #104]
    ldr x1, [x9, #112]

    ret

#endif

#ifdef NEEDX86CONTEXT
.globl SET
SET:
	movl	4(%esp), %eax

	movl	8(%eax), %fs
	movl	12(%eax), %es
	movl	16(%eax), %ds
	movl	76(%eax), %ss
	movl	20(%eax), %edi
	movl	24(%eax), %esi
	movl	28(%eax), %ebp
	movl	36(%eax), %ebx
	movl	40(%eax), %edx
	movl	44(%eax), %ecx

	movl	72(%eax), %esp
	pushl	60(%eax)	/* new %eip */
	movl	48(%eax), %eax
	ret

.globl GET
GET:
	movl	4(%esp), %eax

	movl	%fs, 8(%eax)
	movl	%es, 12(%eax)
	movl	%ds, 16(%eax)
	movl	%ss, 76(%eax)
	movl	%edi, 20(%eax)
	movl	%esi, 24(%eax)
	movl	%ebp, 28(%eax)
	movl	%ebx, 36(%eax)
	movl	%edx, 40(%eax)		 
	movl	%ecx, 44(%eax)

	movl	$1, 48(%eax)	/* %eax */
	movl	(%esp), %ecx	/* %eip */
	movl	%ecx, 60(%eax)
	leal	4(%esp), %ecx	/* %esp */
	movl	%ecx, 72(%eax)

	movl	44(%eax), %ecx	/* restore %ecx */
	movl	$0, %eax
	ret
#endif

#ifdef NEEDAMD64CONTEXT
.globl SET
SET:
	movq	16(%rdi), %rsi
	movq	24(%rdi), %rdx
	movq	32(%rdi), %rcx
	movq	40(%rdi), %r8
	movq	48(%rdi), %r9
	movq	56(%rdi), %rax
	movq	64(%rdi), %rbx
	movq	72(%rdi), %rbp
	movq	80(%rdi), %r10
	movq	88(%rdi), %r11
	movq	96(%rdi), %r12
	movq	104(%rdi), %r13
	movq	112(%rdi), %r14
	movq	120(%rdi), %r15
	movq	184(%rdi), %rsp
	pushq	160(%rdi)	/* new %eip */
	movq	8(%rdi), %rdi
	ret

.globl GET
GET:
	movq	%rdi, 8(%rdi)
	movq	%rsi, 16(%rdi)
	movq	%rdx, 24(%rdi)
	movq	%rcx, 32(%rdi)
	movq	%r8, 40(%rdi)
	movq	%r9, 48(%rdi)
	movq	$1, 56(%rdi)	/* %rax */
	movq	%rbx, 64(%rdi)
	movq	%rbp, 72(%rdi)
	movq	%r10, 80(%rdi)
	movq	%r11, 88(%rdi)
	movq	%r12, 96(%rdi)
	movq	%r13, 104(%rdi)
	movq	%r14, 112(%rdi)
	movq	%r15, 120(%rdi)

	movq	(%rsp), %rcx	/* %rip */
	movq	%rcx, 160(%rdi)
	leaq	8(%rsp), %rcx	/* %rsp */
	movq	%rcx, 184(%rdi)

	movq	32(%rdi), %rcx	/* restore %rcx */
	movq	$0, %rax
	ret
#endif

#ifdef NEEDPOWERCONTEXT
/* get FPR and VR use flags with sc 0x7FF3 */
/* get vsave with mfspr reg, 256 */

.text
.align 2

.globl GET
GET:				/* xxx: instruction scheduling */
	mflr	r0
	mfcr	r5
	mfctr	r6
	mfxer	r7
	stw	r0, 0*4(r3)
	stw	r5, 1*4(r3)
	stw	r6, 2*4(r3)
	stw	r7, 3*4(r3)

	stw	r1, 4*4(r3)
	stw	r2, 5*4(r3)
	li	r5, 1			/* return value for setmcontext */
	stw	r5, 6*4(r3)

	stw	r13, (0+7)*4(r3)	/* callee-save GPRs */
	stw	r14, (1+7)*4(r3)	/* xxx: block move */
	stw	r15, (2+7)*4(r3)
	stw	r16, (3+7)*4(r3)
	stw	r17, (4+7)*4(r3)
	stw	r18, (5+7)*4(r3)
	stw	r19, (6+7)*4(r3)
	stw	r20, (7+7)*4(r3)
	stw	r21, (8+7)*4(r3)
	stw	r22, (9+7)*4(r3)
	stw	r23, (10+7)*4(r3)
	stw	r24, (11+7)*4(r3)
	stw	r25, (12+7)*4(r3)
	stw	r26, (13+7)*4(r3)
	stw	r27, (14+7)*4(r3)
	stw	r28, (15+7)*4(r3)
	stw	r29, (16+7)*4(r3)
	stw	r30, (17+7)*4(r3)
	stw	r31, (18+7)*4(r3)

	li	r3, 0			/* return */
	blr

.globl SET
SET:
	lwz	r13, (0+7)*4(r3)	/* callee-save GPRs */
	lwz	r14, (1+7)*4(r3)	/* xxx: block move */
	lwz	r15, (2+7)*4(r3)
	lwz	r16, (3+7)*4(r3)
	lwz	r17, (4+7)*4(r3)
	lwz	r18, (5+7)*4(r3)
	lwz	r19, (6+7)*4(r3)
	lwz	r20, (7+7)*4(r3)
	lwz	r21, (8+7)*4(r3)
	lwz	r22, (9+7)*4(r3)
	lwz	r23, (10+7)*4(r3)
	lwz	r24, (11+7)*4(r3)
	lwz	r25, (12+7)*4(r3)
	lwz	r26, (13+7)*4(r3)
	lwz	r27, (14+7)*4(r3)
	lwz	r28, (15+7)*4(r3)
	lwz	r29, (16+7)*4(r3)
	lwz	r30, (17+7)*4(r3)
	lwz	r31, (18+7)*4(r3)

	lwz	r1, 4*4(r3)
	lwz	r2, 5*4(r3)

	lwz	r0, 0*4(r3)
	mtlr	r0
	lwz	r0, 1*4(r3)
	mtcr	r0			/* mtcrf 0xFF, r0 */
	lwz	r0, 2*4(r3)
	mtctr	r0
	lwz	r0, 3*4(r3)
	mtxer	r0

	lwz	r3,	6*4(r3)
	blr
#endif

#ifdef NEEDARMCONTEXT
.globl GET
GET:
	str	r1, [r0,#4]
	str	r2, [r0,#8]
	str	r3, [r0,#12]
	str	r4, [r0,#16]
	str	r5, [r0,#20]
	str	r6, [r0,#24]
	str	r7, [r0,#28]
	str	r8, [r0,#32]
	str	r9, [r0,#36]
	str	r10, [r0,#40]
	str	r11, [r0,#44]
	str	r12, [r0,#48]
	str	r13, [r0,#52]
	str	r14, [r0,#56]
	/* store 1 as r0-to-restore */
	mov	r1, #1
	str	r1, [r0]
	/* return 0 */
	mov	r0, #0
	mov	pc, lr

.globl SET
SET:
	ldr	r1, [r0,#4]
	ldr	r2, [r0,#8]
	ldr	r3, [r0,#12]
	ldr	r4, [r0,#16]
	ldr	r5, [r0,#20]
	ldr	r6, [r0,#24]
	ldr	r7, [r0,#28]
	ldr	r8, [r0,#32]
	ldr	r9, [r0,#36]
	ldr	r10, [r0,#40]
	ldr	r11, [r0,#44]
	ldr	r12, [r0,#48]
	ldr	r13, [r0,#52]
	ldr	r14, [r0,#56]
	ldr	r0, [r0]
	mov	pc, lr
#endif

#ifdef NEEDMIPSCONTEXT
.globl GET
GET:
	sw	$4, 24($4)
	sw	$5, 28($4)
	sw	$6, 32($4)
	sw	$7, 36($4)

	sw	$16, 72($4)
	sw	$17, 76($4)
	sw	$18, 80($4)
	sw	$19, 84($4)
	sw	$20, 88($4)
	sw	$21, 92($4)
	sw	$22, 96($4)
	sw	$23, 100($4)

	sw	$28, 120($4)	/* gp */
	sw	$29, 124($4)	/* sp */
	sw	$30, 128($4)	/* fp */
	sw	$31, 132($4)	/* ra */

	xor	$2, $2, $2
	j	$31
	nop

.globl SET
SET:
	lw	$16, 72($4)
	lw	$17, 76($4)
	lw	$18, 80($4)
	lw	$19, 84($4)
	lw	$20, 88($4)
	lw	$21, 92($4)
	lw	$22, 96($4)
	lw	$23, 100($4)

	lw	$28, 120($4)	/* gp */
	lw	$29, 124($4)	/* sp */
	lw	$30, 128($4)	/* fp */

	/* 
	 * If we set $31 directly and j $31, 
	 * we would loose the outer return address.
	 * Use a temporary register, then.
	 */
	lw	$8, 132($4)		/* ra */

	/*  bug: not setting the pc causes a bus error */
	lw	$25, 132($4)	/* pc */

	lw	$5, 28($4)
	lw	$6, 32($4)
	lw	$7, 36($4)
	lw	$4, 24($4)

	j	$8
	nop
#endif
