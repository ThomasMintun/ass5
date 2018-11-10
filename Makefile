CC = gcc

all: oss user 

oss: oss.c
	$(CC) -o oss  oss.c -lrt

user: user.c
	$(CC) -o user  user.c

clean:
	rm *.o oss user