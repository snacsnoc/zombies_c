#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-msc50-cpp"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h> //used for distance()

#define MIN_DISTANCE 5 //min distance to place goal vs player

#define VERSION "0.1.3"
#define ZOMBIE_MOVE_TIME 250000
#define MONITOR_INTERVAL 10000
#define MAP_WIDTH 40
#define MAP_HEIGHT 30
#define WALL_CHAR '#'
#define PLAYER_CHAR 'P'
#define END_CHAR 'E'
#define ZOMBIE_CHAR 'z'
#define BIG_ZOMBIE_CHAR 'Z'
#define ITEM_CHAR '$'
#define MAX_ZOMBIES 10
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
int move_counter = 0;
int game_over = 0;
int game_win = 0;
// Keep track of the time of the last player move and the last zombie move
time_t last_player_move_time = 0;
time_t last_zombie_move_time = 0;
time_t current_time = 0;

int zombie_thread_running;

// Thread for any zombie/player movements
// TODO: check for all/any movements
//pthread_cond_t movement_cond;
//pthread_mutex_t movement_mutex;


pthread_t zombie_thread;
pthread_mutex_t map_mutex;

typedef struct {
    char type;   // Type of point on the map ('#' for wall, 'P' for player, etc.)
    int visited; // Flag indicating whether the point has been visited by the
// player
} Point;

typedef struct {
    Point points[MAP_HEIGHT][MAP_WIDTH];
    int player_x;
    int player_y;
    int goal_x;
    int goal_y;
    int num_zombies;
    int zombie_x[MAX_ZOMBIES];
    int zombie_y[MAX_ZOMBIES];
    int num_big_zombies;
    int big_zombies[MAX_ZOMBIES];
} Map;

// Create a blank map
void init_map(Map *map) {
    int i, j;
    for (i = 0; i < MAP_HEIGHT; i++) {
        for (j = 0; j < MAP_WIDTH; j++) {
            map->points[i][j].type = EMPTY_CHAR;
            map->points[i][j].visited = 0;
        }
    }

    map->num_zombies = 0;
    map->num_big_zombies = 0;
}


// Display game map
void print_map(Map *map) {
    clear(); // Clear the screen

    // Print the score in the top-left corner
    move(0, 0);
    printw("Score: %d | Moves: %d  |  Zombies: %d |  Big Zombies: %d | gameover: %d,gamewin %d\n", score,
           move_counter, map->num_zombies, map->num_big_zombies, game_over, game_win);

    init_pair(1, COLOR_WHITE, COLOR_BLACK);  // white walls
    init_pair(2, COLOR_GREEN, COLOR_BLACK);  // green zombies
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // yellow end
    init_pair(4, COLOR_BLUE, COLOR_BLACK);   // blue player


    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            Point point = map->points[i][j];
            if (i == map->player_x && j == map->player_y) {
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
            } else if (point.type == BIG_ZOMBIE_CHAR) {
                attron(COLOR_PAIR(2));
                printw("%c", BIG_ZOMBIE_CHAR);
                attroff(COLOR_PAIR(2));
            } else if (point.type == TRAIL_CHAR) {
                attron(COLOR_PAIR(4));
                printw("%c", TRAIL_CHAR);
                attroff(COLOR_PAIR(4));
            } else {
                printw("%c", EMPTY_CHAR);
            }
        }
        printw("\n"); // Move to the next row
    }
    refresh(); // Update the screen
}

// Print map when no changes are being made
void safe_print_map(Map *map) {
    pthread_mutex_lock(&map_mutex);
    print_map(map);
    pthread_mutex_unlock(&map_mutex);
}

// Place walls in and around map
void place_walls(Map *map) {
    int i, j;

    for (i = 0; i < MAP_HEIGHT; i++) {
        map->points[i][0].type = WALL_CHAR;
        map->points[i][MAP_WIDTH - 1].type = WALL_CHAR;
    }

    for (i = 0; i < MAP_WIDTH; i++) {
        map->points[0][i].type = WALL_CHAR;
        map->points[MAP_HEIGHT - 1][i].type = WALL_CHAR;
    }

    // Add random walls in center of map
    for (i = 0; i < MAP_HEIGHT / 2; i++) {
        j = rand() % (MAP_WIDTH - 2) + 1;
        map->points[i][j].type = WALL_CHAR;
        map->points[MAP_HEIGHT - i - 1][MAP_WIDTH - j - 1].type = WALL_CHAR;
    }
}

// Randomly place player in map
void place_player(Map *map) {
    int x, y;
    do {
        x = rand() % (MAP_WIDTH - 2) + 1;
        y = rand() % (MAP_HEIGHT - 2) + 1;
    } while (map->points[x][y].type != ' ');
    map->player_x = x;
    map->player_y = y;
    map->points[x][y].type = PLAYER_CHAR;
}

// calculate magnitude
double distance(int x1, int y1, int x2, int y2) {
    int dx = x1 - x2;
    int dy = y1 - y2;
    return sqrt(dx * dx + dy * dy);
}

// place goal in relation to player start point
void place_goal(Map *map) {
    int x, y;

    // Find empty coords to place the end goal
    do {
        x = rand() % (MAP_WIDTH - 2) + 1;
        y = rand() % (MAP_HEIGHT - 2) + 1;
    } while (map->points[x][y].type != ' ' || distance(x, y, map->player_x, map->player_y) < MIN_DISTANCE);
    map->goal_x = x;
    map->goal_y = y;
    map->points[x][y].type = END_CHAR;
}


// Initialize zombies on map
void place_zombies(Map *map, int num_zombies) {
    int i, x, y;
    //Prevent garbage values on initialization
    for (i = 0; i < num_zombies; i++) {
        map->zombie_x[i] = -1;
        map->zombie_y[i] = -1;
    }
    for (i = 0; i < num_zombies; i++) {
        do {
            x = rand() % (MAP_WIDTH - 2) + 1;
            y = rand() % (MAP_HEIGHT - 2) + 1;
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

    if (new_x < 0 || new_x >= MAP_WIDTH || new_y < 0 || new_y >= MAP_HEIGHT) {
        return 0; // Can't move outside the map
    }

    if (map->points[new_x][new_y].type == WALL_CHAR) {
        return 0; // Can't move into a wall
    }

    map->points[map->player_x][map->player_y].type =
            TRAIL_CHAR; // set the old point to TRAIL_CHAR
    map->player_x = new_x;
    map->player_y = new_y;
    map->points[new_x][new_y].type = PLAYER_CHAR;
    map->points[map->player_x][map->player_y].visited = 1;
    return 1;
}

// Move all the zombies on the map one step closer to the player
// Zombies eat one another if on the same x y and change characters
void move_zombies(Map *map) {
    int eaten_zombies[map->num_zombies];
    int eaten_zombie_count = 0;

    for (int i = 0; i < map->num_zombies; i++) {
        int old_x = map->zombie_x[i];
        int old_y = map->zombie_y[i];
        int new_x = old_x;
        int new_y = old_y;
        int eaten_zombie = -1;
        // Check if the zombie is a big zombie
        int is_big_zombie = (map->points[old_x][old_y].type == BIG_ZOMBIE_CHAR);

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

        // Calculate distance from zombie to player
        int dist = distance(old_x, old_y, map->player_x, map->player_y);



        // Zombies chase the player if within a magnitude of 10
        if (dist <= 10) {
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


        } else {
            // Wander code
            int dir = rand() % 4; // Choose a random direction (0: up, 1: right, 2: down, 3: left)
            switch (dir) {
                case 0: // up
                    new_x--;
                    break;
                case 1: // right
                    new_y++;
                    break;
                case 2: // down
                    new_x++;
                    break;
                case 3: // left
                    new_y--;
                    break;
                default:
                    new_x++;
                    new_y--;
                    break;
            }
        }
        /* Remove this comment to forbid zombies to eat the player
                if (new_x == map->player_x && new_y == map->player_y) {
                    continue; // Don't move onto the player's square
                }
        */
        // Check if the new X position is inside a wall
        if (map->points[new_x][old_y].type == WALL_CHAR) {
            new_x = old_x; // Reset the X position if it's inside a wall
        }

        // Check if the new Y position is inside a wall
        if (map->points[old_x][new_y].type == WALL_CHAR) {
            new_y = old_y; // Reset the Y position if it's inside a wall
        }
        if (new_x < 0 || new_x >= MAP_WIDTH || new_y < 0 || new_y >= MAP_HEIGHT) {
            continue; // Can't move outside the map
        }
        // TODO: when zombies move off of trail char, trail char gets erased
        // This mostly works
        if (map->points[new_x][new_y].type == TRAIL_CHAR) {
            map->points[old_x][old_y].type = TRAIL_CHAR;
        } else {
            map->points[old_x][old_y].type = EMPTY_CHAR;
        }
        map->points[old_x][old_y].visited = 1;

        if (map->points[new_x][new_y].type == END_CHAR) {
            continue; // Can't move into end goal
        }
        map->zombie_x[i] = new_x;
        map->zombie_y[i] = new_y;


        // If this zombie ate another zombie, remove the eaten zombie from the map
        // Big zombies cannot be eaten
        // TODO: zombie can get frozen somehow after eating
        // TODO: only one big zombie displays sometimes
        if (eaten_zombie != -1 && is_big_zombie == 0) {
            map->points[map->zombie_x[eaten_zombie]][map->zombie_y[eaten_zombie]]
                    .type = EMPTY_CHAR;
            map->zombie_x[eaten_zombie] = -1;
            map->zombie_y[eaten_zombie] = -1;
            map->num_zombies--;
            map->num_big_zombies++;
            map->points[map->zombie_x[i]][map->zombie_y[i]].type = BIG_ZOMBIE_CHAR;
            map->big_zombies[map->num_big_zombies - 1] = i;
            eaten_zombies[eaten_zombie_count++] = eaten_zombie;

        } else {
            // Show applicable zombie type
            map->points[new_x][new_y].type =
                    is_big_zombie ? BIG_ZOMBIE_CHAR : ZOMBIE_CHAR;
        }
    }
    // Update eaten zombies' positions
    for (int i = 0; i < eaten_zombie_count; i++) {
        int eaten_zombie = eaten_zombies[i];
        map->zombie_x[eaten_zombie] = -1;
        map->zombie_y[eaten_zombie] = -1;
    }

    // Remove eaten zombies from the map
    if (eaten_zombie_count > 0) {
        int new_num_zombies = 0;
        for (int i = 0; i < map->num_zombies; i++) {
            if (map->zombie_x[i] != -1 && map->zombie_y[i] != -1) {
                map->zombie_x[new_num_zombies] = map->zombie_x[i];
                map->zombie_y[new_num_zombies++] = map->zombie_y[i];
            }
        }
        map->num_zombies = new_num_zombies;

    }

}

// Read from buffer for player input
int get_arrow_keys() {
    char buf = 0;
    int direction = 0;

    if (read(STDIN_FILENO, &buf, 1) == -1) {
        perror("read");
        exit(1);
    }

    // Escape
    if (buf == ESC_CHAR && read(STDIN_FILENO, &buf, 1) == -1) {
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

int check_item(Map *map) {
    return map->points[map->player_x][map->player_y].type == ITEM_CHAR;
}

// Check if the player has reached the end of the map
int check_goal(Map *map) {
    return map->player_x == map->goal_x && map->player_y == map->goal_y;
}

int display_menu() {
    int choice;
    while (1) {
        clear();
        printw("zombies | v%s \n\n", VERSION);
        printw("Select difficulty level:\n");
        printw("1. Easy\n");
        printw("2. Medium\n");
        printw("3. Hard\n");
        printw("4. Extreme\n");
        choice = getch() - '0';
        if (choice >= 1 && choice <= 3) {
            return choice;
        }
    }
}

int display_win_screen() {
    int choice;
    while (1) {
        clear();
        printw("YOU WIN!\n");
        printw("1. Play again\n");
        printw("2. Back to main menu\n");
        printw("3. Quit\n");
        choice = getch() - '0';
        if (choice >= 1 && choice <= 3) {
            return choice;
        }
    }
}

int display_lose_screen() {
    int choice;
    while (1) {
        clear();
        printw("YOU LOSE!\n");
        printw("1. Try again\n");
        printw("2. Back to main menu\n");
        printw("3. Quit\n");
        choice = getch() - '0'; // str to int
        if (choice >= 1 && choice <= 3) {
            return choice;
        }
    }
}

// Set number of zombies based on difficulty
int diffset_zombies(int difficulty) {
    switch (difficulty) {
        case 1: // easy
            return 3;
        case 2: // medium
            return 6;
        case 3: // hard
            return 10;
        case 4: // extreme
            return 16;
        default:
            return 0;
    }
}

// Free the memory used by the map
void free_map(Map *map) {
    for (int i = 0; i < MAP_WIDTH; i++) {
        free(map->points[i]);
    }
    free(map->points);
}

void exit_game(void) {
    endwin();
    zombie_thread_running = 0;
    exit(1);
}


// Independent zombie movement
void *zombie_movement(void *arg) {
    Map *map = (Map *) arg;
    while (zombie_thread_running) {
        pthread_mutex_lock(&map_mutex);
        move_zombies(map);
        pthread_mutex_unlock(&map_mutex);
        // Check for game over condition
        if (check_collision(map)) {
            game_over = 1;
            //exit_game();
            break;
        }


        safe_print_map(map);

        usleep(ZOMBIE_MOVE_TIME); // Wait 500ms before moving the zombies again
        last_zombie_move_time = time(NULL);
    }
    pthread_exit(NULL);
}

// Setup ncurses modes we need for the game
void initialize_game() {
    initscr();
    // Enable non-blocking mode
    nodelay(stdscr, TRUE);
    start_color();
    // Do not echo user input
    noecho();
    // Make cursor invisible
    curs_set(0);
    cbreak();
    // Seed the random number generator
    srand(time(NULL));
    timeout(0); // set non-blocking input mode

}

// Set up the playing area
void setup_map(Map *map, int difficulty) {
    init_map(map);
    place_walls(map);
    place_player(map);
    place_goal(map);

    // Set number of zombies based on difficulty
    int num_zombies = diffset_zombies(difficulty);
    place_zombies(map, num_zombies);
    print_map(map);
}

// Reinitialize map and reset counters
void reset_game(Map *map, int difficulty) {
    game_over = 0;
    game_win = 0;
    move_counter = 0;
    score = 0;
    setup_map(map, difficulty);
}

// Thread clean up
void t_cleanup() {
    pthread_join(zombie_thread, NULL);
    pthread_mutex_destroy(&map_mutex);
}

// Collision and goal checking monitor
void *monitor_game_state(void *arg) {
    Map *map = (Map *) arg;

    while (1) {
        pthread_mutex_lock(&map_mutex);

        // Check for game over condition
        if (check_collision(map)) {
            game_over = 1;
        }

        // Check for game win condition
        if (check_goal(map)) {
            game_win = 1;
        }

        pthread_mutex_unlock(&map_mutex);

        // Sleep for a certain time before checking the game state again
        usleep(MONITOR_INTERVAL);
    }

    return NULL;
}


/* This is where the action is
 * a thread is created for zombie movement, so
 * zombies can move independently of the player.
 *
 * We lock the mutex when the map is updated and
 * unlock it when we're updating with it. This prevents race conditions
 * and weird errors.
 */
void game_loop(Map *map) {
    while (!game_over && !game_win) {

        // Read input and move the player
        int direction = get_arrow_keys();
        // lock it if we are updating the map
        pthread_mutex_lock(&map_mutex);
        // Move the player
        if (direction != 0) {
            int result = move_player(map, direction);
            // Count player moves, must fix counting moving into walls
            // TODO: implement character move limit
            if (result) {
                move_counter++;
                // Update the time of the last player move
                last_player_move_time = current_time;
            }

//            if (check_goal(map)) {
//                game_win = 1;
//                break;
//            }
        }
        pthread_mutex_unlock(&map_mutex);

        // Redraw the map after player movement
        safe_print_map(map);
        // Check for game over condition
        if (check_collision(map)) {
            game_over = 1;
            break;
        }

        // Check for game win condition
        if (check_goal(map)) {
            game_win = 1;
            break;
        }
        if (direction == 'q') {
            game_over = 1;
            break;
        }
    }

    if (game_over) {
        score--;
        int result = display_lose_screen();

        switch (result) {
            case 1: //try again
                break;
            case 2: // main menu
                //fix this
                break;
            case 3: //quit
                exit_game();
                break;
        }
    } else if (game_win) {
        score++;
        int result = display_win_screen();
        switch (result) {
            case 1: //try again
                break;
            case 2: // main menu
                //fix this
                break;
            case 3: //quit
                exit_game();
                break;
        }
    }

    // Reset the game_over and game_win flags and start a new game
    game_over = 0;
    game_win = 0;
}

int main() {

    initialize_game();
    // Display menu and get user's choice
    int difficulty = display_menu();

    Map map;
    // Start a new thread for the zombie movement
    zombie_thread_running = 1;
    pthread_create(&zombie_thread, NULL, &zombie_movement, &map);
    pthread_mutex_init(&map_mutex, NULL);
//    pthread_t monitor_thread;
//    pthread_create(&monitor_thread, NULL, &monitor_game_state, &map);


    // Main game loop
    while (1) {
        reset_game(&map, difficulty);
        game_loop(&map);
    }
    t_cleanup();

}


#pragma clang diagnostic pop