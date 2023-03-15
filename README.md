# zombies_c

[![gameplay.gif](https://i.postimg.cc/htMd4VSk/gameplay.gif)](https://postimg.cc/cvgHXgth)

A simple, yet challenging text-based game where the player has to avoid zombies in a 2D matrix map. 
The game is written in C using ncurses for terminal manipulation and pthreads for handling zombie movements concurrently.

## Features

- A player represented by the 'P' symbol, who can move in four directions using arrow keys
- Zombies, represented by 'z' and 'Z', move autonomously and chase the player
- Walls are represented by '#' and block movement
- A goal, represented by 'E', that the player has to reach to win the game
- A main menu for selecting difficulty levels and starting the game
- Score tracking and display at the end of each game

## Prerequisites

- A C compiler (such as GCC)
- ncurses library
- pthreads library

## Compilation

To compile the game, first make sure you have the required libraries (ncurses and pthreads) installed on your system. Then, use the following command to compile the game:

### Build
```bash
make all
```
## Contributing
Contributions are welcome! Feel free to submit issues or pull requests to help improve the game.