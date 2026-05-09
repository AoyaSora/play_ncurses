#include <ncurses.h>

int main(int argc, char *argv[])
{
    WINDOW *local_win;
    WINDOW *second_win;
    initscr();
    noecho();
    cbreak();
    local_win = newwin(5, 10, 2, 3);
    second_win = newwin(20,40,2,2);
    box(local_win, 0 , '-');
    mvwprintw(local_win, 1, 1, "01234567890");
    refresh();              // 何も書いてないが元スクリーンもrefreshする必要がある
    wrefresh(local_win);    // ウィンドウを更新
    getch();
    delwin(local_win);

    box(second_win,'|','-');
    mvwprintw(local_win, 1, 1, "01234567890");
    refresh();              // 何も書いてないが元スクリーンもrefreshする必要がある
    wrefresh(second_win);
    getch();
    delwin(second_win);

    endwin();
    return 0;
}