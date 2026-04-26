#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

/* カーソルの構造体　*/
typedef struct {
    double px, py; //Position(位置)
    double vx, vy; //Velocity(速度) 
} Cobj;            // cursor object(カーソルの場所)
/* カーソルの初期化 */
void InitCobj(Cobj *obj, double px,double py,double vx,double vy)
{
    obj->px = px; obj->py = py;
    obj->vx = vx; obj->vy = vy;
}
/* カーソルの構造体情報制御 キー入力　*/
int ControlCursor(Cobj *obj)
{
    int key;
    key = getch();
    obj->vx = obj->vy = 0.0;
    switch(key){
        case KEY_UP : obj->vy = -1.0; break;
        case KEY_DOWN : obj->vy = 1.0; break;
        case KEY_LEFT : obj->vx = -1.0; break;
        case KEY_RIGHT : obj->vx = 1.0; break;
        case 'q': case 'Q': case'\e': return ('q'); break;
        default : break;

    }
    return (key);
}
/* カーソルの移動制御　*/
/* ここでUI情報の最大値最小値受け取ったら移動範囲を制限できる　*/
void MoveCursor(Cobj *obj)
{
	int	w, h;
	getmaxyx(stdscr, h, w);
    if((obj->px + obj->vx >= 0) && (obj->px + obj->vx <= w-1 )) obj->px += obj->vx;
    if((obj->py + obj->vy >= 0) && (obj->py + obj->vy <= h-1 )) obj->py += obj->vy;
}
/* カーソルの表示　*/
void DrawCursor(Cobj *obj)
{
    move((int)(obj->py),(int)(obj->px));
    addch('>');
}

/* 引数で左上のxyと幅，高さを受け取る　*/
void DrawUI(int y, int x, int h, int w)
{
    int widthLine = w - 2;
    int heightLine = h -2;
   
    /* 枠　*/
    //左上
   move(y,x);
   addch('+');
   //右上まで
   for(int i =0; i < widthLine;i++)
   {
   addch('-');
   }
   //右上
   addch('+');
   // 左右の'|'を下まで
   for(int j=0; j < heightLine; j++)
   {
    move(y+1+j,x);
    addch('|');
    move(y+1+j, x+w-1);
    addch('|');
   }
   //左下
   move(y+1+heightLine,x);
   addch('+');
   //右下まで
   for(int i =0; i < widthLine;i++)
   {
   addch('-');
   }
   //右下
   addch('+');
}

void MainScreen()
{
    Cobj c;
    int w,h;

    //初期設定
    getmaxyx(stdscr, h, w);
    InitCobj(&c,(double)w/2.0, (double)h/2.0, 0.0, 0.0);
    timeout(0);
    while(1){
        erase();
        refresh();
        DrawUI(0,0,h,w);
        DrawCursor(&c);

        // キー入力
        if(ControlCursor(&c) == 'q') break;
        MoveCursor(&c);

        // 動作速度調節
        usleep(20000);
    }

}


int main(void)
{
    /* curses の設定 */
	initscr();
	curs_set(0);		// カーソルを表示しない
	noecho();		// 入力されたキーを表示しない
	cbreak();		// 入力バッファを使わない(Enter 不要の入力)
	keypad(stdscr, TRUE);	// カーソルキーを使用可能にする

    addstr("the position of this text is 2,5");

    /* 本体　*/
    MainScreen();
    /* 終了 */
    endwin();
    return 0;
}