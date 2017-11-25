all: vm-pgfault

vm-pgfault: vm-pgfault.c vm-pgfault-tracer.c
	gcc -o faultpages vm-pgfault.c -lpthread
	gcc -o tracer vm-pgfault-tracer.c -lpthread

clean:
	@rm -f vm-pagefault
	@rm -f tracer
