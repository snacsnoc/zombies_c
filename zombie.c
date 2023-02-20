#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>


#define MAP_SIZE 32
#define WALL_CHAR '#'
#define PLAYER_CHAR 'P'
#define END_CHAR 'E'
#define ZOMBIE_CHAR 'Z'
#define ITEM_CHAR '!'
#define MAX_ZOMBIES 10
#define NUM_ZOMBIES 12
#define EMPTY_CHAR ' '
#define DIRECTION_UP 4
#define DIRECTION_RIGHT 1
#define DIRECTION_DOWN 2
#define DIRECTION_LEFT 3
#define ESC_CHAR 27
#define LEFT_CHAR 68
#define RIGHT_CHAR 67
#define UP_CHAR 65
#define DOWN_CHAR 66

int score = 0;

typedef struct {
    char type;   // Type of point on the map ('#' for wall, 'P' for player, etc.)
    int visited; // Flag indicating whether the point has been visited by the
    // player
} Point;

typedef struct {
    Point points[MAP_SIZE][MAP_SIZE];
    int player_x;
    int player_y;
    int goal_x;
    int goal_y;
    int num_zombies;
    int zombie_x[MAX_ZOMBIES];
    int zombie_y[MAX_ZOMBIES];
} Map;


void init_map(Map *map) {
    int i, j;
    for (i = 0; i < MAP_SIZE; i++) {
        for (j = 0; j < MAP_SIZE; j++) {
            map->points[i][j].type = ' ';
            map->points[i][j].visited = 0;
        }
    }

    map->num_zombies = 0;
}

void print_map(Map *map) {
    clear(); // Clear the screen
    // Print the score at the top-left corner
    move(0, 0);
    printw("Score: %d\n", score);

    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
            Point point = map->points[i][j];
            if (i == map->player_x && j == map->player_y) {
                attron(A_STANDOUT);
                printw("%c", PLAYER_CHAR);
                attroff(A_STANDOUT);
            } else if (point.type == WALL_CHAR) {
                printw("%c", WALL_CHAR);
            } else if (point.type == END_CHAR) {
                printw("%c", END_CHAR);
            } else if (point.type == ZOMBIE_CHAR) {
                attron(A_VERTICAL);
                printw("%c", ZOMBIE_CHAR);
                attroff(A_VERTICAL);
            } else {
                printw("%c", EMPTY_CHAR);
            }
        }
        printw("\n"); // Move to the next row
    }
    refresh(); // Update the screen
}

void place_walls(Map *map) {
    int i, j;

    for (i = 0; i < MAP_SIZE; i++) {
        map->points[i][0].type = WALL_CHAR;
        map->points[i][MAP_SIZE - 1].type = WALL_CHAR;
        map->points[0][i].type = WALL_CHAR;
        map->points[MAP_SIZE - 1][i].type = WALL_CHAR;
    }

    // Add random walls in center of map
    for (i = 0; i < MAP_SIZE / 2; i++) {
        j = rand() % (MAP_SIZE - 2) + 1;
        map->points[i][j].type = WALL_CHAR;
        map->points[MAP_SIZE - i - 1][MAP_SIZE - j - 1].type = WALL_CHAR;
    }
}

void place_player(Map *map) {
    int x, y;
    do {
        x = rand() % (MAP_SIZE - 2) + 1;
        y = rand() % (MAP_SIZE - 2) + 1;
    } while (map->points[x][y].type != ' ');
    map->player_x = x;
    map->player_y = y;
    map->points[x][y].type = PLAYER_CHAR;
}

void place_goal(Map *map) {
    int x, y;
    do {
        x = rand() % (MAP_SIZE - 2) + 1;
        y = rand() % (MAP_SIZE - 2) + 1;
    } while (map->points[x][y].type != ' ');
    map->goal_x = x;
    map->goal_y = y;
    map->points[x][y].type = END_CHAR;
}

void place_zombies(Map *map, int num_zombies) {
    int i, x, y;
    for (i = 0; i < num_zombies; i++) {
        do {
            x = rand() % (MAP_SIZE - 2) + 1;
            y = rand() % (MAP_SIZE - 2) + 1;
        } while (map->points[x][y].type != ' ');
        map->zombie_x[i] = x;
        map->zombie_y[i] = y;
        map->points[x][y].type = ZOMBIE_CHAR;
    }

    map->num_zombies = num_zombies;
}

// Move the player in the specified direction, if possible
// Returns 1 if the move was successful, 0 otherwise
int move_player(Map *map, int direction) {
    int new_x = map->player_x;
    int new_y = map->player_y;

    switch (direction) {
        case DIRECTION_UP:
            new_x--;
            break;
        case DIRECTION_RIGHT:
            new_y++;
            break;
        case DIRECTION_DOWN:
            new_x++;
            break;
        case DIRECTION_LEFT:
            new_y--;
            break;
        default:
            return 0;
    }

    if (new_x < 0 || new_x >= MAP_SIZE || new_y < 0 || new_y >= MAP_SIZE) {
        return 0; // Can't move outside the map
    }

    if (map->points[new_x][new_y].type == WALL_CHAR) {
        return 0; // Can't move into a wall
    }

    map->points[map->player_x][map->player_y].type = EMPTY_CHAR;
    map->player_x = new_x;
    map->player_y = new_y;
    map->points[new_x][new_y].type = PLAYER_CHAR;

    return 1;
}

// Move all the zombies on the map one step closer to the player
void move_zombies(Map *map) {
    for (int i = 0; i < map->num_zombies; i++) {
        int old_x = map->zombie_x[i];
        int old_y = map->zombie_y[i];
        int new_x = old_x;
        int new_y = old_y;

        // Move the zombie closer to the player's X position
        if (map->player_x < old_x) {
            new_x--;
        } else if (map->player_x > old_x) {
            new_x++;
        }

        // Move the zombie closer to the player's Y position
        if (map->player_y < old_y) {
            new_y--;
        } else if (map->player_y > old_y) {
            new_y++;
        }

        if (new_x == map->player_x && new_y == map->player_y) {
            continue; // Don't move onto the player's square
        }

        if (new_x < 0 || new_x >= MAP_SIZE || new_y < 0 || new_y >= MAP_SIZE) {
            continue; // Can't move outside the map
        }

        if (map->points[new_x][new_y].type == WALL_CHAR) {
            continue; // Can't move into a wall
        }

        map->points[old_x][old_y].type = EMPTY_CHAR;
        map->zombie_x[i] = new_x;
        map->zombie_y[i] = new_y;
        map->points[new_x][new_y].type = ZOMBIE_CHAR;
    }
}


int get_arrow_keys() {
    char buf = 0;
    int direction = 0;

    if (read(STDIN_FILENO, &buf, 1) == -1) {
        perror("read");
        exit(1);
    }

    // Escape
    if (buf == 27 && read(STDIN_FILENO, &buf, 1) == -1) {
        perror("read");
        exit(1);
    }
    // [
    if (buf == 91 && read(STDIN_FILENO, &buf, 1) == -1) {
        perror("read");
        exit(1);
    }

    // Move the player, otherwise return the buffer
    if (buf == UP_CHAR) {
        direction = DIRECTION_UP;
    } else if (buf == RIGHT_CHAR) {
        direction = DIRECTION_RIGHT;
    } else if (buf == DOWN_CHAR) {
        direction = DIRECTION_DOWN;
    } else if (buf == LEFT_CHAR) {
        direction = DIRECTION_LEFT;
    } else {
        direction = (unsigned char) buf;
    }

    return direction;
}


// Check if the player has collided with any zombies
int check_collision(Map *map) {
    for (int i = 0; i < map->num_zombies; i++) {
        if (map->player_x == map->zombie_x[i] &&
            map->player_y == map->zombie_y[i]) {
            return 1;
        }
    }

    return 0;
}

void check_item(Map *map) {
    //return map->points[map->player_x][map->player_y].type == ITEM_CHAR;
}

// Check if the player has reached the end of the map
int check_goal(Map *map) {
    return map->player_x == map->goal_x && map->player_y == map->goal_y;
}

// Free the memory used by the map
void free_map(Map *map) {
    for (int i = 0; i < MAP_SIZE; i++) {
        free(map->points[i]);
    }
    free(map->points);
}


void exit_game(void) {
    endwin();
    exit(1);
}


int main() {
    initscr();
    start_color();

    // Seed the random number generator
    srand(time(NULL));

    // Main game loop
    while (1) {
        // Print instructions and set up the map
        printw("Use arrow keys to move, q to quit\n\n");
        Map map;
        init_map(&map);
        place_walls(&map);
        place_player(&map);
        place_goal(&map);
        place_zombies(&map, NUM_ZOMBIES);
        print_map(&map);

        // Game loop
        while (1) {
            // Read input
            int direction = get_arrow_keys();

            // Move the player
            if (direction != 0) {
                int result = move_player(&map, direction);
                if (result == 0) {
                    printw("Invalid move\n");
                }

                // Check if the game is over
                if (check_goal(&map)) {
                    score = score + 1;
                    printw("You win!\n");

                    // Prompt user to play again
                    printw("Play again? (y/n)\n");
                    int play_again = getch();
                    if (play_again == 'y') {
                        // Free the map and break out of inner loop to restart game
                        // free_map(&map);
                        break;
                    } else {
                        exit_game();
                    }
                }

                // Move the zombies
                move_zombies(&map);

                // Print the map
                print_map(&map);

                if (check_collision(&map)) {
                    score = score - 1;
                    printw("You lose!\n");

                    // Prompt user to play again
                    printw("Play again? (y/n)\n");
                    int play_again = getch();
                    if (play_again == 'y') {
                        // Free the map and break out of inner loop to restart game
                        // free_map(&map);
                        break;
                    } else {
                        exit_game();
                    }
                }
            }

            // Quit the game if q is pressed
            if (direction == 'q') {
                printw("Quitting game...\n");
                exit_game();
            }
        }

    }

    endwin();
    return 0;
}

