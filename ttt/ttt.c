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

#include "../queue.h"
#include "agents/mcts.h"
#include "agents/negamax.h"
#include "game.h"
#include "task_sched.h"
#include "termutil.h"

#define INPUT_BUF_SIZE 16

static int move_record[N_GRIDS];
static int move_count = 0;
bool keep_ttt = 0;
char input[INPUT_BUF_SIZE] = "";
bool run_ttt = true;
bool has_input = false;
bool render_screen = true;
bool recordOK = true;
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

struct list_head *move_list;

void q_print_record_list(struct list_head *head, game_arg *g_args)
{
    if (!head || list_empty(head))
        return;

    char rec_path[] = "ttt_rec";
    FILE *rec_file = fopen(rec_path, "w");
    element_t *ele = NULL;
    list_for_each_entry (ele, head, list) {
        printf("%s\n", ele->value);
        fprintf(rec_file, "%s\n", ele->value);
    }
    printf("%s:%d %s:%d draw:%d\n", g_args->pX_name, g_args->pX_score,
           g_args->pO_name, g_args->pO_score, g_args->draw);
    fprintf(rec_file, "%s:%d %s:%d draw:%d\n", g_args->pX_name,
            g_args->pX_score, g_args->pO_name, g_args->pO_score, g_args->draw);
    fclose(rec_file);
    printf("Record has been written to %s\n", rec_path);
}

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
static void record_move_list(char *winner)
{
    // A1 -> A2 -> B2 -> B3
    // ' -> ' need 4 char space, at most (N_GRIDS - 1)'s ' -> ' in a draw game
    // each move need 2 char, at most N_GRIDS's move will be record
    // 1 extra char for '\0'
    // total is 4 * (N_GRIDS - 1) + 2 * N_GRIDS + 1
    // = 6 * N_GRIDS - 3
    // Prepend 32 byte for record winners
    if (!recordOK)
        return;
    char moves[32 + 6 * N_GRIDS - 3];
    int offset = 0;
    if (!strcmp(winner, "draw")) {
        offset += snprintf(moves, 32, "%10s | ", "draw");
    } else {
        offset += snprintf(moves, 32, "%10s win | ", winner);
    }
    snprintf(moves + offset, 8, "Moves: ");
    for (int i = 0; i < move_count; i++) {
        offset +=
            snprintf(moves + offset, 3, "%c%d", 'A' + GET_COL(move_record[i]),
                     1 + GET_ROW(move_record[i]));
        if (i < move_count - 1) {
            offset += snprintf(moves + offset, 5, " -> ");
        }
    }
    recordOK = q_insert_tail(move_list, moves);
    if (!recordOK) {
        preempt_disable();
        printf("out of memory, stop record further moves\r\n");
        preempt_enable();
    }
}

static int parse_input()
{
    int x, y;
    int parseX = 1;
    if (input_offset < 2)
        return -1;
    x = 0;
    y = 0;
    parseX = 1;
    for (int i = 0; i < (input_offset); i++) {
        if (isalpha(input[i]) && parseX) {
            x = x * 26 + (tolower(input[i]) - 'a' + 1);
            if (x > BOARD_SIZE) {
                preempt_disable();
                printf("\r\nInvalid operation: index exceeds board size\r\n");
                preempt_enable();
                return -1;
            }
            continue;
        }
        if (x == 0) {
            preempt_disable();
            printf("\r\nInvalid operation: No leading alphabet\r\n");
            preempt_enable();
            return -1;
        }
        parseX = 0;
        if (isdigit(input[i])) {
            y = y * 10 + input[i] - '0';
            if (y > BOARD_SIZE) {
                preempt_disable();
                printf("\r\nInvalid operation: index exceeds board size\r\n");
                preempt_enable();
                return -1;
            }
            continue;
        }
        preempt_disable();
        printf("\r\nInvalid operation\r\n");
        preempt_enable();
        return -1;
    }
    x -= 1;
    y -= 1;
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
            preempt_disable();
            clear_line();
            run_ttt = false;
            preempt_enable();
            break;
        }
        case CTRL_('p'): {
            if (!echo) {  // echo will be false in ai vs ai mode, ctrl p only
                          // worked in this mode
                render_screen = !render_screen;
            }
            break;
        }
        case ENTER: {
            if (echo) {
                preempt_disable();
                clear_line();
                clear_screen();
                fflush(stdout);
                has_input = input_offset;
                preempt_enable();
            }
            break;
        }
        case '\0': {
            break;
        }
        case BACK_SPACE: {
            if (input_offset) {
                input_offset -= 1;
                input[input_offset] = '\0';
                printf("\b \b");
                // move cursor back,
                // then print a space (to erase/overwrite last char)
                // and move cursor back again (because space let cursor move for
                // ward)
                fflush(stdout);
            }
            break;
        }
        default: {
            if (echo) {
                if ((c & 0x1f) == c) {  // press ctrl
                    if (input_offset <
                        INPUT_BUF_SIZE - 2) {  // 2 char, one for '^'
                        printf("^%c", c | 0x40);
                        snprintf(input + input_offset, 3, "^%c", c | 0x40);
                        input_offset += 2;
                    }
                } else {
                    if (input_offset < INPUT_BUF_SIZE - 1) {
                        printf("%c", c);
                        snprintf(input + input_offset, 2, "%c", c);
                        input_offset += 1;
                    }
                }
                fflush(stdout);
            }
            // printf("default: %c\r\n", c);
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

int human_play(char *table, char mark)
{
    if (!has_input) {
        return -1;
    }
    int move = parse_input();
    memset(input, ' ', INPUT_BUF_SIZE);  // clr input buffer
    input[INPUT_BUF_SIZE - 1] = '\r';
    has_input = false;
    input_offset = 0;
    if (move != -1 && table[move] != ' ') {
        move = -1;
        preempt_disable();
        printf(
            "\r\nInvalid operation: the position has been "
            "marked\r\n");
        preempt_enable();
    }
    return move;
}

void print_time()
{
    time_t mytime = time(NULL);
    char *time_str = ctime(&mytime);
    time_str[strlen(time_str) - 1] = '\0';
    printf("%s\r\nctrl-Q: quit, ctrl-P: pause(only work in ai vs ai mode)\r\n",
           time_str);
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
            }
            record_move_list("Draw");
            memset(table, ' ', N_GRIDS);
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
            record_move_list((win == 'X') ? g_args->pX_name : g_args->pO_name);
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
        if (g_args->echo)
            printf("O>%s", input);
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

void free_game_arg(game_arg *g_args)
{
    if (!g_args)
        return;

    if (g_args->pO_name)
        free(g_args->pO_name);
    if (g_args->pX_name)
        free(g_args->pX_name);
    free(g_args);
}

// rst ttt global vars
void rst_ttt()
{
    keep_ttt = 0;
    run_ttt = true;
    has_input = false;
    render_screen = true;
    recordOK = true;
    input_offset = 0;
    memset(input, ' ', INPUT_BUF_SIZE);
    input[0] = '\0';
}

int ttt(int ai2)
{
    srand(time(NULL));
    move_list = malloc(sizeof(struct list_head));
    INIT_LIST_HEAD(move_list);
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
    printf("\r\nLeaving ttt...\r\n");
    show_cursor();
    disable_raw_mode();
    q_print_record_list(move_list, g_args);
    q_free(move_list);
    free_game_arg(g_args);
    rst_ttt();

    return 0;
}
