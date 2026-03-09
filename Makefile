all: titlefreq titlefreq-debug

titlefreq : titlefreq.c
	@mpicc titlefreq.c -o titlefreq -Wall

titlefreq-debug : titlefreq.c
	@mpicc -g titlefreq.c -o titlefreq-debug -Wall

clean:
	@rm -f titlefreq titlefreq-debug