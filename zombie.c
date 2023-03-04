#include <stdio.h>

#include <stdlib.h>

#include <time.h>

#include <unistd.h>


#define MAP_SIZE 24
#define WALL_CHAR '#'
#define PLAYER_CHAR 'P'
#define END_CHAR 'E'
#define ZOMBIE_CHAR 'z'
#define BIG_ZOMBIE_CHAR 'Z'
#define ITEM_CHAR '!'
#define MAX_ZOMBIES 10
#define NUM_ZOMBIES 6
#define EMPTY_CHAR ' '
#define TRAIL_CHAR '.'
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
    char type; // Type of point on the map ('#' for wall, 'P' for player, etc.)
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
// Determine how we're printing to screen
#ifdef USE_NCURSES
#include <ncurses.h>

#define PRINT(msg) printw(msg)
void print_map(Map * map) {
  clear(); // Clear the screen
  // Print the score at the top-left corner
  move(0, 0);
  printw("Score: %d\n", score);

  init_pair(1, COLOR_WHITE, COLOR_BLACK); // white walls
  init_pair(2, COLOR_GREEN, COLOR_BLACK); // green zombies
  init_pair(3, COLOR_YELLOW, COLOR_BLACK); // yellow end
  init_pair(4, COLOR_BLUE, COLOR_BLACK); // blue player

  for (int i = 0; i < MAP_SIZE; i++) {
    for (int j = 0; j < MAP_SIZE; j++) {
      Point point = map -> points[i][j];
      if (i == map -> player_x && j == map -> player_y) {
        attron(A_STANDOUT);
        attron(COLOR_PAIR(4));
        printw("%c", PLAYER_CHAR);
        attron(COLOR_PAIR(4));
        attroff(A_STANDOUT);
      } else if (point.type == WALL_CHAR) {
        attron(A_BOLD);
        attron(COLOR_PAIR(1));
        printw("%c", WALL_CHAR);
        attroff(COLOR_PAIR(1));
        attroff(A_BOLD);
      } else if (point.type == END_CHAR) {
        attron(COLOR_PAIR(3));
        printw("%c", END_CHAR);
        attroff(COLOR_PAIR(3));
      } else if (point.type == ZOMBIE_CHAR) {
        attron(COLOR_PAIR(2));
        printw("%c", ZOMBIE_CHAR);
        attroff(COLOR_PAIR(2));
      } else {
        printw("%c", EMPTY_CHAR);
      }
    }
    printw("\n"); // Move to the next row
  }
  refresh(); // Update the screen
}
#else
#define PRINT(msg) printf(msg)

void print_map(Map *map) {
    int i, j;
    for (i = 0; i < MAP_SIZE; i++) {
        for (j = 0; j < MAP_SIZE; j++) {
            printf("%c", map->points[i][j].type);
        }

        printf("\n");
    }
}

#endif

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

    map->points[map->player_x][map->player_y].type = TRAIL_CHAR; // set the old point to TRAIL_CHAR
    map->player_x = new_x;
    map->player_y = new_y;
    map->points[new_x][new_y].type = PLAYER_CHAR;

    return 1;
}

// Move all the zombies on the map one step closer to the player
// Zombies eat one another if on the same x y
void move_zombies(Map *map) {
    for (int i = 0; i < map->num_zombies; i++) {
        int old_x = map->zombie_x[i];
        int old_y = map->zombie_y[i];
        int new_x = old_x;
        int new_y = old_y;
        int eaten_zombie = -1;

        // Check if this zombie collides with another zombie
        for (int j = 0; j < map->num_zombies; j++) {
            if (i == j) {
                continue;
            }
            if (map->zombie_x[j] == new_x && map->zombie_y[j] == new_y) {
                eaten_zombie = j;
                break;
            }
        }

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

        if (map->points[new_x][new_y].type == END_CHAR) {
            continue; // Can't move into end goal
        }
        map->points[old_x][old_y].type = EMPTY_CHAR;
        map->zombie_x[i] = new_x;
        map->zombie_y[i] = new_y;

        // If this zombie ate another zombie, remove the eaten zombie from the map
        //TODO: zombie can get frozen somehow after eating
        if (eaten_zombie != -1) {
            map->points[map->zombie_x[eaten_zombie]][map->zombie_y[eaten_zombie]].type = EMPTY_CHAR;
            map->zombie_x[eaten_zombie] = -1;
            map->zombie_y[eaten_zombie] = -1;
            map->num_zombies--;
            // Change zombie to big zombie
            //TODO: fix this
            map->points[map->zombie_x[i]][map->zombie_y[i]].type = BIG_ZOMBIE_CHAR;
        } else {
            map->points[new_x][new_y].type = ZOMBIE_CHAR;

        }
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
#ifdef USE_NCURSES
    endwin();
#endif
    exit(1);
}

int main() {
#ifdef USE_NCURSES
    initscr();
  start_color();
#endif

    // Seed the random number generator
    srand(time(NULL));

    // Main game loop
    while (1) {
        // Print instructions and set up the map
        PRINT("Use arrow keys to move, q to quit\n\n");
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
                    PRINT("Invalid move\n");
                }

                // Check if the game is over
                if (check_goal(&map)) {
                    score++;
                    PRINT("You win!\n");

                #ifdef USE_NCURSES
                    PRINT("Play again? (y/n)\n");
                    int play_again = getch();
                    if (play_again == 'y') {
                    // Free the map and break out of inner loop to restart game
                    // free_map(&map);
                      break;
                    } else {
                      exit_game();
                    }
                #else
                    char play_again;
                    PRINT("Play again? (y/n)\n");
                    scanf("%c", &play_again);
                    if (play_again == 'y') {
                        // Free the map and break out of inner loop to restart game
                        // free_map(&map);
                        break;
                    } else {
                        exit_game();
                    }
                #endif
                }

                // Move the zombies
                move_zombies(&map);

                // Print the map
                print_map(&map);

                if (check_collision(&map)) {
                    score--;
                    PRINT("You lose!\n");

                    // Prompt user to play again
#ifdef USE_NCURSES
                    PRINT("Play again? (y/n)\n");
          int play_again = getch();
          if (play_again == 'y') {
            // Free the map and break out of inner loop to restart game
            // free_map(&map);
            break;
          } else {
            exit_game();
          }
#else
                    char play_again;
                    PRINT("Play again? (y/n)\n");
                    scanf("%c", &play_again);
                    if (play_again == 'y') {
                        // Free the map and break out of inner loop to restart game
                        // free_map(&map);
                        break;
                    } else {
                        exit_game();
                    }
#endif

                }
            }

            // Quit the game if q is pressed
            if (direction == 'q') {
                PRINT("Quitting game...\n");
                exit_game();
            }
        }

    }
}