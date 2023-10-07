GCC = gcc
CFLAGS = -g -Wall -Wshadow

oss: oss.o
	$(GCC) $(CFLAGS) oss.o -o oss

oss.o: oss.c oss.h
	$(GCC) $(CFLAGS) -c oss.c oss.h

worker: worker.o
	$(GCC) $(CFLAGS) worker.o -o worker

worker.o: worker.c worker.h
	$(GCC) $(CFLAGS) worker.c worker.h

clean:
	rm oss.o worker.o
