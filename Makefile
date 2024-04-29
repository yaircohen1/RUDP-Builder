CC=gcc
FLAGS=-Wall -g

all: RUDP_Sender RUDP_Receiver 

RUDP_Sender: RUDP_Sender.o RUDP_API.o
	$(CC) $(FLAGS) -o RUDP_Sender RUDP_Sender.o RUDP_API.o

RUDP_Sender.o: RUDP_Sender.c RUDP_API.h
	$(CC) $(FLAGS) -c RUDP_Sender.c

RUDP_Receiver: RUDP_Receiver.o RUDP_API.o LinkedList.o 
	$(CC) $(FLAGS) -o RUDP_Receiver RUDP_Receiver.o RUDP_API.o LinkedList.o

RUDP_Receiver.o: RUDP_Receiver.c LinkedList.h RUDP_API.h
	$(CC) $(FLAGS) -c RUDP_Receiver.c

RUDP_API.o: RUDP_API.c RUDP_API.h
	$(CC) $(FLAGS) -c RUDP_API.c

LinkedList.o: LinkedList.c LinkedList.h
	$(CC) $(FLAGS) -c LinkedList.c

.PHONY: clean

clean:
	rm -f *.o *txt RUDP_Sender RUDP_Receiver 