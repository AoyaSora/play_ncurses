#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
/* カーソルの構造体　*/
typedef struct {
    double px, py; //Position(位置)
    double vx, vy; //Velocity(速度) 
} Cobj;            // cursor object(カーソルの場所)

/* UIの部品 外枠 */
typedef struct {
    int x, y; // top left position
    int w, h; // width, height 
    // button関係
    int buttonNum,textMax;
    char buttonUI;
    char text[][];
    // ivent
    // '*'の場所とイベントを関連づける．どこかの判定式で場所を参照してイベントを発生させたい
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
        case ' ' : return ('s'); break;
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
    if((obj->px + obj->vx >= 0) && (obj->px + obj->vx <= w-1 ) && (mvinch(obj->py, obj->px + obj->vx) & A_CHARTEXT) == ' ' || (mvinch(obj->py, obj->px + obj->vx) & A_CHARTEXT) == '*') obj->px += obj->vx;
    if((obj->py + obj->vy >= 0) && (obj->py + obj->vy <= h-1 ) && (mvinch(obj->py + obj->vy, obj->px) & A_CHARTEXT) == ' ' || (mvinch(obj->py + obj->vy, obj->px) & A_CHARTEXT) =='*') obj->py += obj->vy;
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
   // '*'表示
   /*  
   制限: UIの外枠を作成したのちボタンやテキストを描画するので幅が大きすぎると溢れちゃう
   1.横方向で行数が足りるかを算出
   whileで治るように書く．改行時に*と同じ列にtextを入れたくないから計算式を変える
   使用行数tmpを更新
    (* + space+ text) < widthLineなら1行，以上なら(* + space+ text) - widthLine L widthLine
    2.縦方向の使用行数がheightLine以内ならok．以上ならerrorとして出力
   */
   move( obj->y + 3, obj->x +3 );
   addch('*');
}

void MainScreen()
{
    Cobj c;
    UIobj menu;
    int w,h;
    char input;
    FILE *fp;

    //初期設定
    getmaxyx(stdscr, h, w);
    InitCobj(&c,(double)w/2.0, (double)h/2.0, 0.0, 0.0);
    timeout(0);
    while(1){
        erase();
        refresh();
        InitUIobj(&menu,0,0,w,h);
        DrawUI(&menu);
        DrawCursor(&c);

        // キー入力
        input = ControlCursor(&c);
        //debug-start//
        fp = fopen("debug.txt","w");
        fprintf(fp,"input:%c, px:%lf,py:%lf, mvinch: %c\n ",input,c.px,c.py,(mvinch(c.py, c.px) & A_CHARTEXT));
        fclose(fp);
        //debug-end//
        if(input == 'q') break;
        else if((input == 's') && ((mvinch(c.py, c.px) & A_CHARTEXT) == '*') ) break;
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

    /* 本体　*/
    MainScreen();
    /* 終了 */
    endwin();
    return 0;
}