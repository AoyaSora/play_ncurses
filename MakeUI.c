#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
/* カーソルの構造体　*/
typedef struct {
    double px, py; //Position(位置)
    double vx, vy; //Velocity(速度) 
} Cobj;            // cursor object(カーソルの場所)

/* UIの部品 外枠 */
typedef struct {
    int x, y; // top left position
    int w, h; // width, height 
} UIobj;
/* カーソルの初期化 */
void InitCobj(Cobj *obj, double px,double py,double vx,double vy)
{
    obj->px = px; obj->py = py;
    obj->vx = vx; obj->vy = vy;
}
/* UIの外枠の初期化　*/
void InitUIobj(UIobj * obj, int x, int y, int w, int h)
{
    obj->x = x; obj->y = y;
    obj->w = w; obj->h = h;

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
    if((obj->px + obj->vx >= 0) && (obj->px + obj->vx <= w-1 ) && (mvinch(obj->py,obj->px + obj->vx)& A_CHARTEXT) == ' ') obj->px += obj->vx;
    if((obj->py + obj->vy >= 0) && (obj->py + obj->vy <= h-1 ) &&( mvinch(obj->py + obj->vy, obj->px) & A_CHARTEXT) == ' ') obj->py += obj->vy;
}
/* カーソルの表示　*/
void DrawCursor(Cobj *obj)
{
    move((int)(obj->py),(int)(obj->px));
    addch('>');
}

/* 引数で左上のxyと幅，高さを受け取る　*/
void DrawUI(UIobj *obj)
{
    int widthLine = obj->w - 2;
    int heightLine = obj->h -2;
   
    /* 枠　*/
    //左上
   move(obj->y,obj->x);
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
    move( obj->y + 1 + j, obj->x );
    addch('|');
    move( obj->y + 1 + j, obj->x + obj->w - 1);
    addch('|');
   }
   //左下
   move( obj->y + 1 + heightLine , obj->x );
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
    UIobj menu;
    int w,h;

    //初期設定
    getmaxyx(stdscr, h, w);
    InitCobj(&c,(double)w/2.0, (double)h/2.0, 0.0, 0.0);
    InitUIobj(&menu,0,0,w,h);
    timeout(0);
    while(1){
        erase();
        refresh();
        DrawUI(&menu);
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