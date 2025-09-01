#Create 32-bit executable for fib.c by using the gcc flags as mentioned in the PDF

#Provide the command for cleanup

all: fib

fib: fib.c
	# 32-bit, no CRT/libc, entry=_start, non-PIE
	gcc -m32 -nostartfiles -nostdlib -no-pie -Wl,-e,_start -o fib fib.c

clean:
	rm -f fib
