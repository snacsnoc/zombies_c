all:
	$(CC) zombie.c -DUSE_NCURSES -lcurses -o zombie.out

no-curses:
	$(CC) zombie.c -o zombie.out

