CC = gcc
CFLAGS = -Wall -O2

all : argusd argus

argusd: argusd.c
	$(CC) $(CFLAGS) -o argusd argusd.c executa.c SmartArray.c

argus: argus.c
	$(CC) $(CFLAGS) -o argus argus.c

clean:
	rm -f PID*
	rm -f fifoF1
	rm -f *.txt
	rm -rf *.o $(objects) argus
	rm -rf *.o $(objects) argusd