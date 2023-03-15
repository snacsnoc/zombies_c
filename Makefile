all:
	$(CC) -Wall zombie.c -lcurses -lpthread -o zombie.out