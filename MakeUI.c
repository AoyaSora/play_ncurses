#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
/* カーソルの構造体　*/
typedef struct {
    int px, py; //Position(位置)
    double vx, vy; //Velocity(速度) 
} Cobj;            // cursor object(カーソルの場所)

/* イベント判定2D配列 */
char iventPos[200][280]; // 全体でのイベントの場所管理  ボタンの検知用 '*'
/*
    if (iventPos[c.py][c.px] == '*'){
        // ivent[]のそれぞれのx,yと合うか
        for(int i = 0; i < bn; i++){
            if(ivent[i].px == c.px && ivent[i].py = c.py) {
                int state = ivent[i].nextState;
                switch(state) {
                    case END:
                        break;
                    case TO_MENU:
                        break;
                }
            }
        }
    }
*/

/* イベントの構造体　*/
typedef struct {
    unsigned long x,y; // ボタンの場所
    char text[100];     // イベント用のテキスト
    unsigned long nextState; // 次のイベント内容
} iventObj; // nextStateの内容をiventPosに入れる


/* UIの部品 外枠 */
typedef struct {
    int x, y; // top left position
    int w, h; // width, height 
    int bottonNum; // nubmer of botton
    char iventText[10][100]; // Text of botton maxbotton = 10, max text each botton = 99 
    iventObj ivent[10]; // ivent object 
} UIobj;

/* カーソルの初期化 */
void InitCobj(Cobj *obj, double px,double py,double vx,double vy)
{
    obj->px = px; obj->py = py;
    obj->vx = vx; obj->vy = vy;
}
/* UIの外枠の初期化 */
void InitUIobj(UIobj * obj, int x, int y, int w, int h, int bn, iventObj *ivent)
{
    obj->x = x; obj->y = y;
    obj->w = w; obj->h = h;
    obj->bottonNum = bn;
    for(int i=0; i < obj->bottonNum; i++){
        ivent[i].text[99] = '\0';
        obj->ivent[i] = ivent[i];
    }
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

/* 引数で左上のxyと幅，高さ,共有用のボタン配列を受け取る　*/
void DrawUI(UIobj *obj, char ButtonPos[][280], iventObj* ivent)
{
    int widthLine = obj->w - 2;
    int heightLine = obj->h -2;
    int textCol,textRow; //colが列 rowが行
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

   // ivent表示 引数に当たり判定用の2dマトリクスを入れれば他のとこでも使える
   // '*' と iventTextを表示
   int ln = 0; // ボタンの行指定用
   int lnAdd = 0; //iventひとつ分の行数
   int averageLn = heightLine/obj->bottonNum; //均等にボタンを配置する用
   int textStartWidth = obj->x + 3;
   int c = 0; // textの描画x位置
//    int bottonPos[heightLine][widthLine]; // 中身の配列

   int lnNum=0;
   int overAveLen = 1; // 1 = true 
   int bottonHeight[obj->bottonNum];
   //計算
   bottonHeight[0] = 0;
   for(int i=0; i < obj->bottonNum; i++){ //ボタンの数繰り返し
        //文字の数取得し，合計行数がheightLine数を超えないか
        int textlen = strlen(obj->iventText[i]);
        int rowNum = (textlen / (widthLine-2));
        if(textlen % (widthLine-2) != 0) {
            rowNum+=1;
            // mvprintw(9+i,5,"textlen: %d,widthLine-2: %d rowNum:%d",textlen,(widthLine-2),rowNum);
        }
        if(i < obj->bottonNum -1) { 
            bottonHeight[i+1] = bottonHeight[i] + rowNum;
            mvprintw(10+i,5,"bottonheihg: %d",bottonHeight[i+1]);
        }
        lnNum += rowNum;
        if(averageLn < rowNum) overAveLen = 0;
    }
    // mvprintw(10, 5, "orverAveLen: %d,lnNum: %d, heightLine: %d", overAveLen,lnNum,heightLine); // 10行5列目に表示
    // mvprintw(11,5,"bottonHeight[0]: %d,bottonHeigh[1]: %d",bottonHeight[0],bottonHeight[1]);
    if(overAveLen == 1){ //均等における場合(それぞれがaveを超えていないのでそのまま描画できる)
        bottonHeight[0]=1;
        for(int n=1;n < obj->bottonNum;n++) bottonHeight[n] = bottonHeight[n-1] + averageLn;
        //描画
        mvaddch(0,0,'a');
        // mvprintw(10, 5, "orverAveLen: %d,lnNum: %d, heightLine: %d", overAveLen,lnNum,heightLine); // 10行5列目に表示
        for(int i=0; i < obj->bottonNum; i++){
            //ボタン描画
            mvaddch( obj->y+bottonHeight[i], obj->x+1, '*');
            ButtonPos[obj->y+bottonHeight[i]][obj->x+1] = '*'; //共通の配列にボタンを追加
            // iven管理
            ivent[i].x = obj->x+1;
            ivent[i].y = obj->y+ bottonHeight[i];
            // c = 0;
            // //text描画
            int len = strlen(ivent[i].text);
            int row = obj->y + bottonHeight[i];
            int col = textStartWidth;
            for(int j=0;j < len; j++){
                mvaddch(row,col,ivent[i].text[j]);
                if( col > (obj->w) ) {  // 2 = '*' + ' '
                    // textがUIの外枠'|'にかなったら改行
                    col = textStartWidth;
                    row +=1;
                }else col++;
            }
        }
    }else if(overAveLen != 1 && lnNum <= heightLine){ //均等におけないがui中に描画できる
        //描画
        // bottonHeight[0] = 1;
        mvaddch(1,0,'b');
        for(int i=0; i < obj->bottonNum; i++){
            //ボタン描画
            mvaddch( obj->y+bottonHeight[i]+1, obj->x+1, '*');
            ButtonPos[obj->y+bottonHeight[i]+1][obj->x+1] = '*'; //共通の配列にボタンを追加
            // iven管理
            ivent[i].x = obj->x+1;
            ivent[i].y = obj->y+ bottonHeight[i];
            //text描画
            int len = strlen(obj->iventText[i]);
            int row = obj->y + bottonHeight[i] + 1;
            int col = textStartWidth;
            for(int j=0;j < len; j++){
                mvaddch(row,col,obj->iventText[i][j]);
                if( col > (obj->w)) {  // 2 = '*' + ' '
                    // textがUIの外枠'|'にかなったら改行
                    col = textStartWidth;
                    row +=1;
                }else col++;
            }
        }
    }else{  // そのままやるとオーバーする場合 
        mvaddch(0,0,'c');
    }
}

int MainScreen()
{
    Cobj c;
    UIobj menu;
    int w,h;
    char input;


    //初期設定
    getmaxyx(stdscr, h, w);
    InitCobj(&c,4,4, 0.0, 0.0);

    //構造体の初期化
    iventObj iventData[2] = {
        {0, 0, "start the lainbow another way",TO_DO},
        {0, 0, "next situation xxx", END}
    };

    timeout(0);
    while(1){
        erase();
        refresh();
        
        InitUIobj(&menu,3,3,20,10,2,iventData);
        DrawUI(&menu,iventPos,iventData);
        DrawCursor(&c);
        unsigned long test=100;
        // キー入力
        input = ControlCursor(&c);
        if (input == 'q') return END;
        else if(input == 's'){
               if(iventPos[c.py][c.px] == '*') {
                for(int i = 0; i < sizeof(iventData)/sizeof(iventData[0]); i++ ) {
                    if(iventData[i].x == c.px && iventData[i].y == c.py){
                        // unsigned long state = iventData[i].nextState;
                        // switch(state) {
                        //     case TO_DO:
                        //         return TO_DO;
                        //     case END:
                        //         return END;
                        //     default: return END;
                        // }
                        // 上のはしたのと同じ
                        return iventData[i].nextState;
                    }
                }
            }
        }
        MoveCursor(&c);

        // 動作速度調節
        usleep(20000);
    }
    return 0;
}

/* 画面の種類　*/
enum {
    END, // 0: 終了
    MAIN, // 1: メイン
    TO_DO,// 2: todoリスト
    RECORD// 3: 記録
};
int main(void)
{
    /* curses の設定 */
	initscr();
	curs_set(0);		// カーソルを表示しない
	noecho();		// 入力されたキーを表示しない
	cbreak();		// 入力バッファを使わない(Enter 不要の入力)
	keypad(stdscr, TRUE);	// カーソルキーを使用可能にする
    
    /* 本体 ( screen関数の戻り値が次の画面への状態　)*/
    int i = 1;
    int nextScreen = MainScreen();
    while(i){
        switch(nextScreen){
            case MAIN:
                nextScreen = MainScreen();
                break;
            case TO_DO:
                nextScreen = MainScreen();
                break;
            case RECORD:
                break;
            case END:
                /* 終了 */
                endwin();
                return 0;
            default: 
                break;
        }
    }

    /* 終了 */
    endwin();
    return 0;
}