#pragma once
#define CTRL_(k) ((k) & (0x1f))
#define BACK_SPACE 127
#define DEL 8
#define ENTER 13

void disable_raw_mode();
void enable_raw_mode();
void hide_cursor();
void show_cursor();
void clear_screen();
void clear_line();
void ctrl_key_monitor();
void move_cursor_to_begin();
void back_space();