#include <locale.h>
#include <string.h>
#include <curses.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>


void init_curses(void)
{
    setlocale(LC_ALL, "");

    // Initializes program, sets input type to 'raw' (disables ctrl+c/z 
    // behavior), and turns off echo
    initscr();
    raw();
    noecho();

    // Idk, does some kind of initialization. I copied it from the manpage
    nonl();
    intrflush(stdscr, FALSE);

    // Enables reading of F1, F2, arrow keys, etc
    keypad(stdscr, TRUE);

    // Hide cursor
    curs_set(0);

    // non-blocking io
    timeout(0);
}

// Main loop:
// - Check for user input
// - Update game state based on input + time passed
// - Re-render map
//
// Other thread: check for user input, append to global state
// 



void clearscr(int row, int col)
{
    for (int i = 0; i < row; i++)
    {
        for (int j = 0; j < col; j++)
        {
            mvaddch(i, j, ' ');
        }
    }
}

void move_up(int *row_ptr, int col, struct timespec time)
{
    int row = *row_ptr;
    mvaddch(row, col, ' ');
    if (row > 0) row--;
    mvaddch(row, col, '*');
    *row_ptr = row;
    refresh();
    nanosleep(&time, NULL);
}

void move_down(int *row_ptr, int col, struct timespec time, int row_max)
{
    int row = *row_ptr;
    mvaddch(row, col, ' ');
    if (row < row_max - 1) row++;
    mvaddch(row, col, '*');
    *row_ptr = row;
    refresh();
    nanosleep(&time, NULL);
}

void do_jump_animation(int row, int col, int row_max)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 30000000L;
    move_up(&row, col, time);
    move_up(&row, col, time);
    move_up(&row, col, time);
    move_down(&row, col, time, row_max);
    move_down(&row, col, time, row_max);
    move_down(&row, col, time, row_max);
}

int main(void)
{
    init_curses();
    printw("press 'q' to quit");

    int row_max, col_max;
    getmaxyx(stdscr, row_max, col_max);

    clearscr(row_max, col_max);

    // Set up time for sleep
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 500000L;

    int row = 10;
    int col = 10;
    bool quit = false;
    move(row, col);
    addch('*');
    int ch;
    while (!quit)
    {
        ch = getch();
        mvaddch(row, col, ' ');
        switch (ch)
        {
            case KEY_UP:
                if (row > 0) row--;
                break;
            case KEY_DOWN:
                if (row < row_max - 1) row++;
                break;
            case KEY_LEFT:
                if (col > 0) col--;
                break;
            case KEY_RIGHT:
                if (col < col_max - 1) col++;
                break;
            case ' ':
                do_jump_animation(row, col, row_max);
                break;
            case 'q':
                quit = true;
                break;
            case KEY_RESIZE:
                getmaxyx(stdscr, row_max, col_max);
                clearscr(row_max, col_max);
                if (col >= col_max) col = col_max - 1;
                if (row >= row_max) row = row_max - 1;
                break;
            default:
                break;
        }
        mvaddch(row, col, '*');
        refresh();
        nanosleep(&time, NULL);
    }

    clearscr(row, col);
    endwin();
    return 0;
}
