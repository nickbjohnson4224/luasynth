#include <curses.h>

void create_interface();
void create_border();
void create_buttons();
void create_menu();

int main() {
    initscr();
    start_color();
    init_pair(1, COLOR_MAGENTA, COLOR_BLACK);
    attron(COLOR_PAIR(1));

    int width, height;
    getmaxyx(stdscr, height, width);

    mvhline(1, 1, '*', width - 2);
    mvhline(height - 1, 1, '*', width - 2);

    attroff(COLOR_PAIR(1));

    int ch;
    while( (ch = getch()) != 'q') {
        ch = getch();
    }
    endwin();
    return 0;
}

