#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include "sqliteFunc.h"

#define MAX_CONTEXT 1024

/*
テストしたい機能
text配列に対して範囲内に表示，テキスト入力，削除に対してカーソルの位置が意図通りにうごく ok
・データを入力，データを挿入．
・データを入力，データを更新．
・最初selectでその日のdiaryデータを取得する．ある場合とない場合で処理を変える．ある場合の取得はできた．
// ない場合に取得する情報はnullなのか．今引数にdbとdiaryObjの二種類の引数を入れているが，必要なのは日付．
// 戻り値はrcで関数側でokの場合はSQLITE_OKを返すようにする．
・更新ボタンを押したときにupdateDiary or insertDiaryTable関数のどちらかに飛べるようにする．
・削除ボタン

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
        case 127 : return KEY_BACKSPACE;
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
    // mvprintw(20,20,"UpdateText input:%d",input);
    // まずtextObj範囲内か
    if((Cobj->px < textObj->x )|| (Cobj->px > textObj->x +textObj->w) || (Cobj->py < textObj->y )|| (Cobj->py > textObj->y +textObj->h) ) return;
    //　textObjのtextObj->content[textObj->cursorIndex]で
    switch (input)
    {
        case KEY_BACKSPACE :{
            /* delete char from textObj->content[x] */
            int index=0;// '\0'以降の一文字を'\0'にする必要がある．
            if((Cobj->px == textObj->x)&&(Cobj->py == textObj->y) ) break;
            while(textObj->content[textObj->cursorIndex+index]!='\0'){
                textObj->content[textObj->cursorIndex+index -1] = textObj->content[textObj->cursorIndex+index];
                index++;
            }
            textObj->content[textObj->cursorIndex+index-1]=' '; // or '\0'
            // 左端で１文字消す場合 カーソル位置移動
            if(Cobj->px == textObj->x ){ 
                Cobj->px = textObj->x+textObj->w - 1; 
                Cobj->py -=1;
            }
            else{ // 左へカーソル位置移動
                Cobj->px = Cobj->px - 1;
            }
            break;
        }
        default :{
            
            for(int i =MAX_CONTEXT; i > textObj->cursorIndex; i-- ){
                textObj->content[i] = textObj->content[i-1];
            }
            textObj->content[textObj->cursorIndex] = input;
            // 右端に到達したらpx,pyどちらも変更する．そうでない場合はpxをインクリメント
            if(Cobj->px == textObj->x + textObj->w-1){
                Cobj->px = textObj->x;
                Cobj->py += 1;
            }
            else{
                Cobj->px += 1;
            }
            break;
        }
    }
}

void DrawText(Cobj* Cobj, textObj* textObj){
        // mvprintw(21,20,"DrawText  c->px:%d, c->py:%d",Cobj->px,Cobj->py);
        // カーソルの位置に応じてこのtextObj内のカーソルの位置を決める
        int n=0;
        // カーソルの移動 移動方向に応じてcontentのどのインデックスの間に' 'を入れるか決める
        // カーソルの移動と場所で次の'|'の位置nを決め，そこに' 'をおく
        // カーソルが範囲外
        if((Cobj->px < textObj->x )|| (Cobj->px > textObj->x +textObj->w-1) || (Cobj->py < textObj->y )|| (Cobj->py > textObj->y +textObj->h) ){
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
                for(int j = 0; j < textObj->w; j++){
                    if(textObj->content[n] == '\0') return;
                    addch(textObj->content[n]);
                    n++; 
                }
            
            }
        }else{      // カーソルが範囲内
            textObj->cursorIndex = (Cobj->px - textObj->x) + (Cobj->py - textObj->y) * (textObj->w); // cのtext内での位置(indexと対応)
            mvprintw(23,20,"DrawText  cursorIndex:%03d",textObj->cursorIndex);
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
                for(int j = 0; j < textObj->w; j++){
                    if(textObj->content[n] == '\0') return;
                    if( j + i * (textObj->w) == textObj->cursorIndex){
                        addch(' ');
                    }else{
                        addch(textObj->content[n]);
                        n++;
                    }
                }
            
            }
        }
        // 文字の更新
}
int InitTextObj(textObj* textObj,int x,int y, int w, int h, AppContent* appObj){
    int rc;
    textObj->x = x;
    textObj->y = y;
    textObj->w = w;
    textObj->h = h;
    // 時間取得
    char timeBuf[128];// 時間の文字列timeBuffer
    time_t t = time(NULL);// 基準時刻取得
    struct tm *local = localtime(&t);
    strftime(timeBuf, sizeof(timeBuf),"%Y/%m/%d",local);//%H:%M:%S %A
    // 1.DiaryObjを作成．その日付にstrftimeで作成した文字列を入れる
    strcpy(appObj->diary->date, timeBuf);
    // 2.dbをopenする
    rc = sqlite3_open("testDB.db", &appObj->db);
    if(rc){
        fprintf(stderr,"Cant't open database: %s\n",sqlite3_errmsg(appObj->db));
        sqlite3_close(appObj->db);
        return(1);
    }
    rc = createTable(appObj->db,"diary","id INTEGER PRIMARY KEY, date TEXT, title TEXT, content TEXT, created_at TEXT, updated_at TEXT");
    if(rc!=SQLITE_OK){
        fprintf(stderr,"createTable failed: %d\n",rc);
        return(1);
    }
    // 3.select関数の引数に与える．
    // strcpy(dObj.date,"2026/05/15");
    rc = selectDiaryByDate(appObj->db, appObj->diary);
    if(rc == SQLITE_NOTFOUND){// 日付の内容が見つからない場合
        // ゴミが入って処理が変にならないように初期化
        strcpy(appObj->diary->title,"");
        strcpy(appObj->diary->content,"");
        strcpy(appObj->diary->created_at,"");// updateの時にcreated_atが""ならcreated_atとupdated_atも変える
        strcpy(appObj->diary->updated_at,"");
    }else if(rc!=SQLITE_OK){
        fprintf(stderr,"selectDiaryTable failed: %d\n",rc);
        return(1);
    }
    // 4.クローズ
    sqlite3_close(appObj->db); 
    strcpy(textObj->content,appObj->diary->content);
    return 0;
}

int main(){
    Cobj c;
    int input;
    int w,h;
    textObj tObj;
    DiaryObj dObj;
    AppContent appObj;
    int rc;
    appObj.diary = &dObj;
    /* curses の設定 */
	initscr();
	curs_set(0);		// カーソルを表示する
	noecho();		// 入力されたキーを表示しない
	cbreak();		// 入力バッファを使わない(Enter 不要の入力)
	keypad(stdscr, TRUE);	// カーソルキーを使用可能にする
    start_color();
    refresh();

    // 初期化
    strcpy(tObj.content,"");
    // char text[MAX_CONTEXT]={0};
//     printf("date:%s\n", dObj.date);
// printf("title:%s\n", dObj.title);
// printf("content:%s\n", dObj.content);
    // strcpy(text,dObj.content);
    rc = InitTextObj(&tObj,2,2,10,10,&appObj); // この関数内でdb開いてtext取得して，closeまでやる
    InitCobj(&c,0,0,0,0);
    // viewText(&tObj);

    
    timeout(16);
    while(1){
        // erase();
        getmaxyx(stdscr, h, w);
        input = ControlCursor(&c);
        // mvprintw(21,21,"main KEY_BACK:%d",KEY_DC);

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