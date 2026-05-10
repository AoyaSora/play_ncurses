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

別案
文字列を配列で保持し，カーソルの移動に応じて配列を操作．
利点- 文字を書き換えたり増やしたりする処理を同じようにできる．
方法- cの場所に応じて描画をcの前の文字配列，'|'(c)，cの後の文字列の順にする．配列自体は変更していないのでとてもいい．



innstr(char* str,int n); でカーソル位置からn文字,strに格納する
最終的なstrへの上書きは，innstrを用いて一行ずつ配列に入れていく(カーソル位置の'|'は除く)
*/


/*
A|BCにAD|BCなど文字列を操作したい場合はDrawText内でwhileを用いて行う．
表示と入力で描画と配列を制御し，関数を抜ける時に文字列配列の変更を伝えDB情報を変更する
中で変更用の配列を作成し，最後に関数を抜ける時にポインタの配列へ内容を書き換える．
戻り値は変更したかどうかでDB関数を呼ぶ．

*/

char content[MAX_CONTEXT] = "Hello,world. Nice to see you";
typedef struct{
    int x,y,w,h;
    int cursorIndex;
    char content[MAX_CONTEXT];
} textObj;

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
        case 'q': case 'Q': case'\e': return ('q'); break;
        default : break;

    }
    return (key);
}
// screenで最初に実行．範囲決めてることが，文字数と描画範囲へ影響を及ぼしている
// void viewText(textObj* textObj ){
//     // 描画
//     // wとhを用いてfor文で回す
//     unsigned long n=0;
//     char c;
//     for(int i =0; i < textObj->h; i++) {
//         move(textObj->y + i, textObj->x) ;
//         for(int j = 0 ; j < textObj->w - 1; j++ ) {
//             c = textObj->content[n];
//             if(c == '\0') return;
//             addch(c);
//             n++;
//         }
//     }
// }
void UpdateText(Cobj* Cobj, textObj* textObj, int input){
    if((input == KEY_DOWN )|| (input == KEY_UP) || (input == KEY_LEFT )|| (input == KEY_RIGHT)|| (input == -1)) return;
    mvprintw(20,20,"UpdateText input:%d",input);
    // まずtextObj範囲内か
    if((Cobj->px < textObj->x )|| (Cobj->px > textObj->x +textObj->w) || (Cobj->py < textObj->y )|| (Cobj->py > textObj->y +textObj->h) ) return;
    //　textObjのtextObj->content[textObj->cursorIndex]で
    switch (input)
    {
        case KEY_BACKSPACE :{
            /* delete char from textObj->content[x] */
            int index=0;
            while(textObj->content[textObj->cursorIndex+index]!='\0'){
                textObj->content[textObj->cursorIndex+index] = textObj->content[textObj->cursorIndex+index+1];
                index++;
            }
            break;
        }
        default :{
            for(int i =MAX_CONTEXT; i > textObj->cursorIndex; i-- ){
                textObj->content[i] = textObj->content[i-1];
            }
            textObj->content[textObj->cursorIndex] = input;
            break;
        }
    }
}

void DrawText(Cobj* Cobj, textObj* textObj){
        // カーソルの位置に応じてこのtextObj内のカーソルの位置を決める
        int n=0;
        // カーソルの移動 移動方向に応じてcontentのどのインデックスの間に' 'を入れるか決める
        // カーソルの移動と場所で次の'|'の位置nを決め，そこに' 'をおく
        // 
        textObj->cursorIndex = (Cobj->px - textObj->x) + (Cobj->py - textObj->y) * (textObj->w-1); // cのtext内での位置(indexと対応)
        // clear
        for(int i = 0; i < textObj->h; i++){
            move(textObj->y+i,textObj->x);
            for(int j = 0; j < textObj->w ; j++){
                addch(' ');
            }
        }
        // 描画
        for(int i = 0; i < textObj->h; i++){
            // 改行
            move(textObj->y+i,textObj->x);
            for(int j = 0; j < textObj->w - 1; j++){
                if(textObj->content[n] == '\0') return;
                if(j+i*(textObj->w-1) == textObj->cursorIndex){
                    addch(' ');
                }else{
                    addch(textObj->content[n]);
                    n++;
                }
            }
        
        }
        // 文字の更新
}
void InitTextObj(textObj* textObj,int x,int y, int w, int h, char content[MAX_CONTEXT]){
    textObj->x = x;
    textObj->y = y;
    textObj->w = w;
    textObj->h = h;
    strcpy(textObj->content,content);
}

int main(){
    Cobj c;
    int input;
    int w,h;
    textObj tObj;

    /* curses の設定 */
	initscr();
	curs_set(0);		// カーソルを表示する
	noecho();		// 入力されたキーを表示しない
	cbreak();		// 入力バッファを使わない(Enter 不要の入力)
	keypad(stdscr, TRUE);	// カーソルキーを使用可能にする
    start_color();
    refresh();
    char text[] = "Hello,world. Nice to see you";
    InitTextObj(&tObj,2,2,10,10,text);
    InitCobj(&c,0,0,0,0);
    // viewText(&tObj);

    timeout(16);
    while(1){
        // erase();
        getmaxyx(stdscr, h, w);
        input = ControlCursor(&c);
        mvprintw(21,21,"main input:%d",input);

        if(input=='q') break;
        // データ変更
        MoveCursor(&c);
        UpdateText(&c,&tObj,input);

        //描画
        DrawText(&c, &tObj);
        DrawCursor(&c);
        refresh();
        usleep(20000);
    }
    
    /* 終了 */
    endwin();
    return 0;
}