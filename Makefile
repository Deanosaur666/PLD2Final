all: titlefreq

titlefreq : titlefreq.c
	@mpicc titlefreq.c -o titlefreq -Wall

clean:
	@rm -f titlefreq