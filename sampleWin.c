#include <ncurses.h>

int main(int argc, char *argv[])
{
    WINDOW *win1, *win2;

    initscr();
    noecho();
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    refresh();
    win1 = newwin(4, 4, 5, 5);
    win2 = newwin(7, 7, 7, 8);
    wbkgd(win1, COLOR_PAIR(1));
    mvwprintw(win1, 0, 0, "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRST");
    mvwprintw(win2, 0, 0, "0123456789001234567890012345678900123456789012");
    overlay(win1, win2);
    //// overlayによりwin1が上になる
    wrefresh(win1);
    wrefresh(win2);
    getch();
    delwin(win1);
    delwin(win2);
    endwin();
    return 0;
}