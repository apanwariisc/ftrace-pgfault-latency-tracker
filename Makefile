all: vm-pgfault

vm-pgfault: vm-pgfault.c
	gcc -o faultpages vm-pgfault.c -lpthread

clean:
	@rm -f vm-pagefault
