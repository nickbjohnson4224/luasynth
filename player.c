#include <curses.h>
#include "player_state.h"

void create_interface();
void create_border();
void create_buttons();
void create_menu();

void player() {
    initscr();
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    attron(COLOR_PAIR(1));

    int width, height;
    getmaxyx(stdscr, height, width);

    //mvhline(1, 1, '*', width - 2);
    //mvhline(height - 1, 0, '*', width - 2);

    attroff(COLOR_PAIR(1));

    int ch = ' ';
    while(ch != 'q') {
        ch = getch();
        pthread_mutex_lock (&pause_tex);
        switch(ch) {
            case ' ':
                paused = !paused;
                pause_flag = 1;
                pthread_mutex_unlock (&pause_tex);
            default:
                break;
        }
    }
    quit = 1;
    endwin();
}

