#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <string.h>

#define VERSION "0.1"
#define MAXOPTIONS 4


struct termios saved_attributes;

// Reset terminal to the state it had before
void reset_input_mode(void) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    write(STDOUT_FILENO, "\x1b[?25h", 6);
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}


// Set terminal to raw mode
void set_raw_mode(void) {
    struct termios tattr;

    if (!isatty (STDIN_FILENO)) {
        fprintf(stderr, "Not a terminal.\n");
        exit(EXIT_FAILURE);
    }

    tcgetattr(STDIN_FILENO, &saved_attributes);
    atexit(reset_input_mode);

    tcgetattr(STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON|ECHO);
    tattr.c_cc[VMIN] = 0;
    tattr.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
    setvbuf(stdout, NULL, _IONBF, 0);
}

// Clear screen and position cursor at top left
void clear_screen(void) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}



void get_window_size(int *rows, int *cols) {
    struct winsize ws;
  
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *cols = ws.ws_col;
    *rows = ws.ws_row;
}


// get arrow keys and return direction
int read_key(int current_dir) {
    char c;
    char seq[2];

    read(STDIN_FILENO, &c, 1);
    
    if (c == '\x1b') {
        read(STDIN_FILENO, &seq[0], 1);
        if (seq[0] == '[') {
            read(STDIN_FILENO, &seq[1], 1);
            switch (seq[1]) {
                case 'A': if (current_dir != 2) return 1; break; // up
                case 'B': if (current_dir != 1) return 2; break; // down
                case 'C': if (current_dir != 4) return 3; break; // right
                case 'D': if (current_dir != 3) return 4; break; // left
                default: return current_dir; break; // anything else
            }
        }
        else {
            reset_input_mode();
            exit(0);
        }
    }

    return current_dir;
}


// Display game over message
void game_over(int rows, int cols) {
    int escseq_len;
    char escseq[10] = {0};
    char c;

    escseq_len = sprintf(escseq, "\x1b[%d;%dH", rows / 2 - 1, cols / 2 - 12);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("                         ");
    escseq_len = sprintf(escseq, "\x1b[%d;%dH", rows / 2, cols / 2 - 12);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("        GAME OVER        ");
    escseq_len = sprintf(escseq, "\x1b[%d;%dH", rows / 2 + 1, cols / 2 - 12);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("  Press any key to quit  ");
    escseq_len = sprintf(escseq, "\x1b[%d;%dH", rows / 2 + 2, cols / 2 - 12);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("                        " );
    
    while(read(STDIN_FILENO, &c, 1) != 1);
    exit(0);
}


// Game loop
void play(int rows, int cols) {
    char board[rows][cols];
    char snake[rows][cols];
    int head_x = cols / 2;
    int head_y = rows / 2;
    int tail_x = cols / 2 + 3;
    int tail_y = rows / 2;
    char escseq[10] = {0};
    int escseq_len;
    char dir = 4;
    int alive = 1;
    char tail_dir = 'U';

    memset(snake, 0, sizeof(snake));
    memset(board, ' ', sizeof(board));

    board[head_y][head_x] = 'Q';
    board[head_y][head_x + 1] = 'O';
    board[head_y][head_x + 2] = 'O';
    board[head_y][head_x + 3] = 'O';

    snake[head_y][head_x] = 'H';
    snake[head_y][head_x + 1] = 4;
    snake[head_y][head_x + 2] = 4;
    snake[head_y][head_x + 3] = 'U';

    while (1) {
        dir = read_key(dir);
        snake[head_y][head_x] = dir;

        switch (dir) {
            case 1: snake[head_y - 1][head_x] = 'H'; break;
            case 2: snake[head_y + 1][head_x] = 'H'; break;
            case 3: snake[head_y][head_x + 1] = 'H'; break;
            case 4: snake[head_y][head_x - 1] = 'H'; break;
        }

        for (int i = 0; i < rows && alive; i++) {
            for (int j = 0; j < cols && alive; j++) {
                if (snake[i][j] > 'Q') {
                    tail_dir = snake[i][j];
                    snake[i][j] = 0;
                    board[i][j] = ' ';
                    switch (tail_dir) {
                        case 'R': snake[i - 1][j] += 'Q'; break;
                        case 'S': snake[i + 1][j] += 'Q'; break;
                        case 'T': snake[i][j + 1] += 'Q'; break;
                        case 'U': snake[i][j - 1] += 'Q'; break;
                    }
                    continue;
                }
                else if (snake[i][j] == 'H') {
                    head_y = i;
                    head_x = j;
                    if (i == 0 || j == 0 || i == rows - 1 || j == cols - 1 || board[i][j] == 'O') {
                        game_over(rows, cols);
                        alive = 0;
                    }
                    board[i][j] = 'Q';
                }
                else if (snake[i][j])
                    board[i][j] = 'O';
                else
                    board[i][j] = ' ';
            }
        }

        memset(board[0], '#', sizeof(board[0]));
        memset(board[rows - 1], '#', sizeof(board[0]));
        
        for (int i = 0; i < rows; i++) {
            board[i][0] = '#';
            board[i][cols - 1] = '#';
            board[i][cols] = 0;
        }

        for (int i = 0; i < rows; i++) {
            escseq_len = sprintf(escseq, "\x1b[%d;1H", i + 1);
            write(STDOUT_FILENO, escseq, escseq_len);
            printf("%s", board[i]);
        }

        usleep(50000);
    }
}


// Init screen and show welcome message
void init_game(int rows, int cols) {
    char board[rows][cols + 1];
    char escseq[10] = {0};
    char c = 0;
    int escseq_len;

    set_raw_mode();
    clear_screen();
    memset(board, ' ', sizeof(board));
    memset(board[0], '#', sizeof(board[0]));
    memset(board[rows - 1], '#', sizeof(board[0]));

    for (int i = 1; i < rows - 1; i++) {
        board[i][0] = '#';
        board[i][cols - 1] = '#';
        board[i][cols] = '\n';
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%c", board[i][j]);
        }
    }

    write(STDOUT_FILENO, "\x1b[2;3H", 6);
    printf("Version " VERSION);
    escseq_len = sprintf(escseq, "\x1b[%d;%dH", rows / 3, cols / 2 - 9);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("Welcome to Snake_C!" );
    escseq_len = sprintf(escseq, "\x1b[%d;%dH", 1 + rows / 3, cols / 2 - 18);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("Press Space key to start, q to quit.");
    escseq_len = sprintf(escseq, "\x1b[%d;%dH", 5 + rows / 3, cols / 2 - 35);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("How to play:");
    escseq_len = sprintf(escseq, "\x1b[%d;%dH", 6 + rows / 3, cols / 2 - 35);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("-> Use the arrow keys to control the direction of the snake.");
    escseq_len = sprintf(escseq, "\x1b[%d;%dH", 7 + rows / 3, cols / 2 - 35);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("-> If the head collides with a wall or any part of the snake: game over.");
    escseq_len = sprintf(escseq, "\x1b[%d;%dH", 8 + rows / 3, cols / 2 - 35);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("-> The snake will get longer when it eats @'s.");
    escseq_len = sprintf(escseq, "\x1b[%d;%dH", 9 + rows / 3, cols / 2 - 35);
    write(STDOUT_FILENO, escseq, escseq_len);
    printf("-> While playing, press Esc to quit.");


    write(STDOUT_FILENO, "\x1b[H", 3);
    write(STDOUT_FILENO, "\x1b[?25l", 6);

    while (c != ' ' && c != 'q' && c != 'Q')
        read(STDIN_FILENO, &c, 1);

    if (c == ' ') {
        //clear_screen();
        play(rows, cols);
    }
    else {
        reset_input_mode();
        exit(0);
    }
}


int main(int argc, char **argv) {
    int rows, cols;

    srand(time(NULL)); 
    get_window_size(&rows, &cols);
    init_game(rows, cols);
    
    return EXIT_SUCCESS;
}
