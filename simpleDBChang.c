#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define MAX_CONTEXT 1024
// #include "sqliteFunc.h"
/*
テストしたい機能
・データを入力，データを挿入．
・データを入力，データを更新．
画面遷移はここでは行わない

main screen
    * todo screen
        view task, status
        * changeStatus
        * makeTask
        * back main
    * diary screen
        view created_at, updated_at
        feature: input and chenge text
        * updated_button
        * back main
*/
// テキストの上書き(更新)はDBからtextを配列で取得して表示.カーソルの位置に合わせて配列の情報を書き換える．
// この時削除するとそれ以降の文字の場所が全て-1されるようにする．
// 文字が追加されるとそれ以降の文字の場所が全て+1されるようにする．
// hello -> hel|lo( helと'|'を表示したのちlo)
// textを表示できる領域-1('|'分)テキストを表示することで，'|'の描画の行が変わった時他の行のtextに干渉しないようにする
// DBの上書きは文字列を配列に取得して，配列を変更，変更した配列をDBへinsertする

/*
文字列について
char *pStr = "String";
char arrStr[] = "String";
ポインタで文字列を使用すると，文字列リテラルとして初期化され，ROM領域に確保される．
そのため，
pStr[0] = 's';
のようにROMに書き込むことはできない．
char str[] = {'S', 't', 'r', 'i', 'n', 'g', '\0'}の略なので変数としてRAMに確保される．
ただしポインタに対して新しい文字列リテラルを割り当てることはできる

今回は，DBから文字列を持ってきて表示，上書きしたいので
char str[]を使用する
"|hello,world"と表示させる．
・右に'|'を動かすと"h|ello,world"と表示させたい．
カーソル位置から右の文字列をinnstrで取得して，カーソル位置に'|'を描画，innstrで取得した文字列を表示．
・左に'|'を動かすと"Hel|lo,world"を"He|llo,world"と表示させたい．
'|'の左の文字から"l|lo,world"をinnstrで取得，'heのあとに'|'を描画，innstrで取得した文字列から'|'を除いた文字列を描画．
・上下に'|'を移動させる場合，行だけ変えて左に'|'を動かす処理を行う．'|'があった行はinnstrで'|'を除いた文字列を描画

innstr(char* str,int n); でカーソル位置からn文字,strに格納する
最終的なstrへの上書きは，innstrを用いて一行ずつ配列に入れていく(カーソル位置の'|'は除く)
*/


/*
描画範囲の列数は10，行数は10．これにすると列数が'|'の分-1され 10x9．

*/

char content[MAX_CONTEXT] = "Hello,world. Nice to see you";

/* カーソルの構造体　*/
typedef struct {
    int px, py; //Position(位置)
    double vx, vy; //Velocity(速度) 
} Cobj;   
// moveUpdateCursor関数を作成(textを更新したい時)
void MoveCursor(Cobj *obj)
{
	int	w, h;
	getmaxyx(stdscr, h, w);
    if((obj->px + obj->vx >= 0) && (obj->px + obj->vx <= w-1 )) {
        obj->px += obj->vx;
    }
    if((obj->py + obj->vy >= 0) && (obj->py + obj->vy <= h-1 )) {
        obj->py += obj->vy;
    }
}
/* カーソルの表示　*/
void DrawCursor(Cobj *obj)
{
    move((int)(obj->py),(int)(obj->px));
    addch('|');
}
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
        case ' ' : return ('s'); break;
        case 'q': case 'Q': case'\e': return ('q'); break;
        default : break;

    }
    return (key);
}
// screenで最初に実行．範囲決めてることが，文字数と描画範囲へ影響を及ぼしている
void viewText(char content[MAX_CONTEXT],int x,int y, int w, int h){
    // 描画
    // wとhを用いてfor文で回す
    unsigned long n=0;
    char c;
    for(int i =0; i < h; i++) {
        move(y+i,x);
        for(int j=0; j < w-1;j++ ) {
            c = content[n];
            if(c == '\0') return;
            addch(c);
            n++;
        }
    }
}

int main(){
    Cobj c;
    char input;
    int w,h;
    WINDOW *win1;


    /* curses の設定 */
	initscr();
	curs_set(0);		// カーソルを表示する
	noecho();		// 入力されたキーを表示しない
	cbreak();		// 入力バッファを使わない(Enter 不要の入力)
	keypad(stdscr, TRUE);	// カーソルキーを使用可能にする
    start_color();
    refresh();

    win1 = newwin(10, 10, 5, 5);

    InitCobj(&c,0,0,0,0);
    viewText(content,2,2,10,10);
            mvwprintw(win1, 0, 0, "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRST");

    timeout(16);
    while(1){
        wrefresh(win1);
        // erase();
        getmaxyx(stdscr, h, w);
        input = ControlCursor(&c);
        if(input=='q') break;
        MoveCursor(&c);
        DrawCursor(&c);
        refresh();
        usleep(20000);
    }
    
    /* 終了 */
    delwin(win1);
    endwin();
    return 0;
}