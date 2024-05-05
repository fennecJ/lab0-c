#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "agents/mcts.h"
#include "agents/negamax.h"
#include "game.h"
#include "task_sched.h"
#include "termutil.h"

static int move_record[N_GRIDS];
static int move_count = 0;
bool keep_ttt = 0;
bool run_ttt = true;
bool has_input = false;
bool render_screen = true;
int input_offset = 0;
typedef int(game_agent_t)(char *table, char mark);
typedef struct {
    char turn;
    char table[N_GRIDS];
    bool game_play;
    game_agent_t *agentX;
    game_agent_t *agentO;
    int pX_score;
    int pO_score;
    int draw;
    char *pX_name;
    char *pO_name;
    bool echo;
} game_arg;

static void record_move(int move)
{
    move_record[move_count++] = move;
}

static void print_moves()
{
    printf("Moves: ");
    for (int i = 0; i < move_count; i++) {
        printf("%c%d", 'A' + GET_COL(move_record[i]),
               1 + GET_ROW(move_record[i]));
        if (i < move_count - 1) {
            printf(" -> ");
        }
    }
    printf("\r\n");
}

static int get_input(char player)
{
    char *line = NULL;
    size_t line_length = 0;
    int parseX = 1;

    int x = -1, y = -1;
    while (x < 0 || x > (BOARD_SIZE - 1) || y < 0 || y > (BOARD_SIZE - 1)) {
        printf("%c> ", player);
        int r = getline(&line, &line_length, stdin);
        if (r == -1)
            exit(1);
        if (r < 2)
            continue;
        x = 0;
        y = 0;
        parseX = 1;
        for (int i = 0; i < (r - 1); i++) {
            if (isalpha(line[i]) && parseX) {
                x = x * 26 + (tolower(line[i]) - 'a' + 1);
                if (x > BOARD_SIZE) {
                    // could be any value in [BOARD_SIZE + 1, INT_MAX]
                    x = BOARD_SIZE + 1;
                    printf("Invalid operation: index exceeds board size\n");
                    break;
                }
                continue;
            }
            // input does not have leading alphabets
            if (x == 0) {
                printf("Invalid operation: No leading alphabet\n");
                y = 0;
                break;
            }
            parseX = 0;
            if (isdigit(line[i])) {
                y = y * 10 + line[i] - '0';
                if (y > BOARD_SIZE) {
                    // could be any value in [BOARD_SIZE + 1, INT_MAX]
                    y = BOARD_SIZE + 1;
                    printf("Invalid operation: index exceeds board size\n");
                    break;
                }
                continue;
            }
            // any other character is invalid
            // any non-digit char during digit parsing is invalid
            // TODO: Error message could be better by separating these two cases
            printf("Invalid operation\n");
            x = y = 0;
            break;
        }
        x -= 1;
        y -= 1;
    }
    free(line);
    return GET_INDEX(y, x);
}

void listen_keyboard(void *arg)
{
    game_arg *g_args = (game_arg *) arg;
    bool echo = g_args->echo;
    struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
    nfds_t nfds = 1;
    char c = '\0';

    while (run_ttt) {
        poll(&pfd, nfds, 0);
        if (!(pfd.revents & POLLIN))
            schedule();
        // prepend (void)! to make gcc happy
        // otherwise it will throw Werror = unused-result
        // but here we don't need to take care of read() return value
        (void) !read(STDIN_FILENO, &c, 1);
        switch (c) {
        case CTRL_('q'): {
            run_ttt = false;
            break;
        }
        case CTRL_('p'): {
            if (!echo)
                render_screen = !render_screen;
            break;
        }
        }
        c = '\0';
    }
}

void player_X_move(void *arg)
{
    game_arg *g_args = (game_arg *) arg;
    while (run_ttt) {
        bool game_play = g_args->game_play;
        char *table = g_args->table;
        char turn = g_args->turn;
        if (turn == 'X' && game_play) {
            int move = g_args->agentX(table, 'X');
            if (move != -1) {
                table[move] = 'X';
                record_move(move);
                g_args->turn = 'O';
            }
        } else {
            schedule();
        }
    }
}

void player_O_move(void *arg)
{
    game_arg *g_args = (game_arg *) arg;
    while (run_ttt) {
        bool game_play = g_args->game_play;
        char *table = g_args->table;
        char turn = g_args->turn;
        if (turn == 'O' && game_play) {
            int move = g_args->agentO(table, 'O');
            if (move != -1) {
                table[move] = 'O';
                record_move(move);
                g_args->turn = 'X';
            }
        } else {
            schedule();
        }
    }
}

void print_time()
{
    time_t mytime = time(NULL);
    char *time_str = ctime(&mytime);
    time_str[strlen(time_str) - 1] = '\0';
    printf("%s\r\n", time_str);
}

int negamax_wrapper(char *table, char mark)
{
    return negamax_predict(table, mark).move;
}

void game_manager(void *arg)
{
    while (run_ttt) {
        preempt_disable();
        game_arg *g_args = (game_arg *) arg;
        char *table = g_args->table;
        char win = check_win(table);
        if (win == 'D') {
            if (render_screen) {
                clear_line();
                fflush(stdout);
                printf("It is a draw!\r\n");
                memset(table, ' ', N_GRIDS);
            }
            g_args->draw += 1;
            move_count = 0;
            // break;
        } else if (win != ' ') {
            if (render_screen) {
                printf("\r\n");
                clear_line();
                fflush(stdout);
                printf("%s won!\r\n",
                       (win == 'X') ? g_args->pX_name : g_args->pO_name);
            }
            memset(table, ' ', N_GRIDS);
            g_args->pX_score += (win == 'X');
            g_args->pO_score += (win == 'O');
            move_count = 0;
            // break;
        }
        preempt_enable();
        schedule();
    }
}

void update_screen(void *arg)
{
    game_arg *g_args = (game_arg *) arg;
    char *table = g_args->table;
    while (run_ttt) {
        preempt_disable();
        move_cursor_to_begin();
        print_time();
        printf("%s:%d %s:%d draw:%d\r\n", g_args->pX_name, g_args->pX_score,
               g_args->pO_name, g_args->pO_score, g_args->draw);
        if (render_screen) {
            draw_board(table);
            clear_line();
            fflush(stdout);
            print_moves();
        }
        preempt_enable();
        schedule();
    }
}

void init_game_arg(game_arg *g_args,
                   game_agent_t *g1,
                   game_agent_t *g2,
                   char *g1_name,
                   char *g2_name)
{
    g_args->turn = 'X';
    memset(g_args->table, ' ', N_GRIDS);
    g_args->game_play = true;
    g_args->agentX = g1;
    g_args->agentO = g2;
    g_args->pX_score = 0;
    g_args->pO_score = 0;
    g_args->draw = 0;
    g_args->pX_name = strdup(g1_name);
    g_args->pO_name = strdup(g2_name);
}

int human_play(char *table, char mark)
{
    // TODO
    get_input(mark);
    return -1;
}

int ttt(int ai2)
{
    srand(time(NULL));
    game_arg *g_args = malloc(sizeof(game_arg));
    if (ai2)
        init_game_arg(g_args, negamax_wrapper, mcts, "negamax", "mcts");
    else
        init_game_arg(g_args, negamax_wrapper, human_play, "negamax", "human");

    g_args->echo = !ai2;
    negamax_init();
    clear_screen();

    timer_init();
    task_init();
    hide_cursor();
    enable_raw_mode();

    task_add(listen_keyboard, g_args);
    task_add(player_X_move, g_args);
    task_add(update_screen, g_args);
    task_add(game_manager, g_args);
    task_add(player_O_move, g_args);
    task_add(update_screen, g_args);
    task_add(game_manager, g_args);

    sched_start();
    show_cursor();
    disable_raw_mode();
    printf("\nLeaving ttt...\n");
    return 0;
}
