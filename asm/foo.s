### This file is a compilation of foo.c using the following gcc command:
###  $ gcc -m32 -O0 -S foo.c
### This file is a library for the driver.c program. Assemble this file and build the
### driver like so:
###  $ as --32 foo.s -ofoo.o
###  $ gcc -m32 driver.c foo.o
### The assembly code here is for a 32-bit environment (i386, Intel 80386 instruction set).
### This code is almost compatible between GCC platforms. On Windows, names are mangled with
### a leading underscore. The MinGW variant can be viewed in 'foo-mingw.s'.

        .file	"foo.c"
	.text
### function `foo`
	.globl	foo             # must define foo as a global symbol so linker can find it
	.type	foo, @function  # define foo as a function symbol
foo:
	pushl	%ebp            # push base pointer onto stack to save old stack frame
	movl	%esp, %ebp      # make base pointer have stack pointer value to build new stack frame
	subl	$16, %esp       # allocate 4 words (longs) on the stack (it only uses 1 at -4(%ebp))
	movl	12(%ebp), %eax  # move function arguments into registers (argument 'b' from C source code)
	movl	8(%ebp), %edx   # (argument 'a' from C source code)
	addl	%edx, %eax      # add the arguments
	movl	%eax, -4(%ebp)  # store result in variable ('c' from C source code)
	movl	-4(%ebp), %eax  # move value back into EAX as the function return value (I turned off optimizations, so this is redundant)
	leave                   # undo the stack frame (leave stack frame); same as 'mov %ebp, %esp' followed by 'pop %ebp'
	ret                     # return from procedure (jumps to return address on stack)
        
### function `bar`
	.globl	bar             # must define bar as a global symbol so linker can find it
	.type	bar, @function  # define bar as a function symbol
bar:
	pushl	%ebp            # this function is the exact same except it does a 'subl' instruction on the arguments
	movl	%esp, %ebp
	subl	$16, %esp
	movl	12(%ebp), %eax  # remember that arguments are stored in reverse order: this is argument 'b'
	movl	8(%ebp), %edx   # argument 'a'
	movl	%edx, %ecx      # because subtraction is not associative, the value is swapped to ECX
	subl	%eax, %ecx      # subtract the arguments (a - b)
	movl	%ecx, %eax
	movl	%eax, -4(%ebp)
	movl	-4(%ebp), %eax
	leave
	ret
