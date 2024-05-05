#include "termutil.h"
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include "agents/negamax.h"

struct termios orig_term;

void hide_cursor()
{
    printf("\33[?25l");
}

void show_cursor()
{
    printf("\33[?25h");
}

void disable_raw_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term) == -1)
        printf("Failed to disable raw mode");
}

void enable_raw_mode()
{
    if (tcgetattr(STDIN_FILENO, &orig_term) == -1)
        printf("Failed to get current terminal state");
    printf("\x1b[?47h");
    struct termios raw = orig_term;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN /*| ISIG*/);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        printf("Failed to set raw mode");
}

void clear_screen()
{
    printf("\e[2J");
}

void clear_line()
{
    printf("\x1b[2K");
}

void move_cursor_to_begin()
{
    printf("\e[1;1H");
}

void back_space()
{
    printf("\b");
}