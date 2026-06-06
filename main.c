#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sqliteFunc.h"
// textObjの文字数制限
#define MAX_CONTEXT 1024

/* 画面の種類　*/
typedef enum {
    NONE,   // 遷移なし
    END,    //  終了
    MAIN,   //  メイン
    TO_DO,  // todoリスト
    RECORD,  //  記録
    MAKE_TASK, // task作成screen
    /*-これはMakeTaskScreen内の画面--*/
    TASK_DATE, // task -> date
    TASK_CONTENT, // task-> task
    TASK_MAKE // 
    /*----------------------------*/
} NextStateType;
/* db管理用の数値 */
typedef enum{
    DB_EVENT_NONE,         // なし
    INSERT_DIARY_BYDATE,
    INSERT_TODO_BYDATE,
    SELECT_DIARY_BYDATE,
    SELECT_TODO_BYDATE,
    UPDATE_DIARY_BYDATE,
    UPDATE_TODO_BYDATE,
    UPDATE_TODO_STATUS,
    DELETE_DIARY_BYID,
    DELETE_TODO_BYID
} DBFuncType;
/*taskのステータス*/
typedef enum{
    TASK_UNTOUCH,
    TASK_PROGRESS,
    TASK_DONE,
    NONE_TASK_STATUS,
    
} TASK_STATUS;
/* カーソルの構造体　*/
typedef struct {
    int px, py; //Position(位置)
    double vx, vy; //Velocity(速度) 
} Cobj;            // cursor object(カーソルの場所)
typedef struct {
    int year;
    int month;
    int day;
} Date;
/* イベント判定2D配列 */
// char eventPos[200][280]; // 全体でのイベントの場所管理  ボタンの検知用 '*'
/*
    if (eventPos[c.py][c.px] == '*'){
        // event[]のそれぞれのx,yと合うか
        for(int i = 0; i < bn; i++){
            if(event[i].px == c.px && event[i].py = c.py) {
                int state = event[i].nextState;
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
    NextStateType nextState; // 次のイベント内容
    DBFuncType dbfuncNum;
    int task_status; // status of task 
} eventObj; // nextStateの内容をeventPosに入れる


void sort_bucket(eventObj *eventData, int n)
{
    int i;
    int freq[4] = {0};   /* freq[i]=iの出現回数 */
    int index[4];        /* index[i]=バケットiのデータ登録位置 */
    eventObj *bucket = (eventObj*)malloc(sizeof(eventObj)*n); /* バケット用配列 */

    /* 出現回数を調べる */
    for(i = 0; i < n; i++)
    {
        freq[eventData[i].task_status]++;
    }
    /* 各バケットのデータ挿入位置を初期化 */
    index[0] = 0;
    for(i = 1; i <= 3; i++)
    {
        index[i] = index[i-1] + freq[i-1];
    }
    /* バケットに格納 */
    for(i = 0; i < n; i++)
    {
        bucket[index[eventData[i].task_status]] = eventData[i];
        index[eventData[i].task_status]++;
    }
    /* A配列に再登録 */
    for(i = 0; i < n; i++)
    {
        eventData[i] = bucket[i];
    }
    free(bucket);
}
/* UIの部品 外枠 */
typedef struct {
    int x, y; // top left position
    int w, h; // width, height 
    int bottonNum; // nubmer of botton
    // char eventText[10][100]; // Text of botton maxbotton = 10, max text each botton = 99 
    eventObj event[10]; // event object 
} UIobj;
/* テキストフィールド UI*/
typedef struct{
    int x,y,w,h;
    int cursorIndex;
    char content[MAX_CONTEXT];
} textObj;

/* 閏年計算 */
int isLeapYear(int year){
    if((year%4 == 0) && (year%100 != 0) || (year%400 == 0)){
        return 1; // 閏年
    }
    return 0; //平年
};
/* 日付け計算 */
int calculateDate(Date *date,int dayCount){
    // 各月の日数(1月-12月)
    int daysInMonth[] = {31, 28, 31, 30, 31, 31, 30, 31, 30, 31};
    // 日付を計算 
    // 日数
    // 通常 date.day + dayCount <= daysInMonth[date.month -1];
    // over date.day + dayCount > daysInMonth[date.month -1];
    // date.day = date.day + dayCount - daysInMonth[date.month -1];
    // date.month ++;
    // 月
    // default date.month <= 12;
    // over date.month > 12;
    // date.month -= 12;
    // year ++;
    return 0;
}

/* カーソルの初期化 */
void InitCobj(Cobj *obj, double px,double py,double vx,double vy)
{
    obj->px = px; obj->py = py;
    obj->vx = vx; obj->vy = vy;
}
/* UIの外枠の初期化 */
void InitUIobj(UIobj * obj, int x, int y, int w, int h,int bn, eventObj *event)
{
    obj->x = x; obj->y = y;
    obj->w = w; obj->h = h;
    obj->bottonNum = bn;
    for(int i=0; i < obj->bottonNum; i++){
        event[i].text[99] = '\0';
        obj->event[i] = event[i];
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
        case 127 : return KEY_BACKSPACE;
        // case '\e' : return 
        // case ' ' : return ('s'); break;
        // case 'q': case 'Q':  return ('q'); break;
        default : break;

    }
    return (key);
}
/* カーソルの移動制御　*/
/* ここでUI情報の最大値最小値受け取ったら移動範囲を制限できる　*/
void MoveCursor(Cobj *cobj, UIobj *uiobj)
{
	// int	w, h;
	// getmaxyx(stdscr, h, w);
    if((cobj->px + cobj->vx > uiobj->x) && (cobj->px + cobj->vx < uiobj->x + uiobj->w-1 )) cobj->px += cobj->vx;
    if((cobj->py + cobj->vy > uiobj->y) && (cobj->py + cobj->vy < uiobj->y + uiobj->h-1 )) cobj->py += cobj->vy;
}
/* カーソルの表示　*/
void DrawCursor(Cobj *obj)
{
    move((int)(obj->py),(int)(obj->px));
    addch('>');
}
void DrawTextCursor(Cobj *obj)
{
    move((int)(obj->py),(int)(obj->px));
    addch('|');
}

// 範囲内にeventObjのみ描画
void DrawBtnUI(eventObj* events,int buttonNum, int x, int y, int w, int h)
{
    // event配列と場所，幅を決めてボタンと内容のみ描画
    // hはLine数今回はeventsの要素数に合わせる
    // x,y,wはUIObjから取得
    for(int j =y; j < y+h; j++)
    {
        for(int i = x; i < x+w-2; i++)
        {
            mvaddch(j-1,i,' ');
        }
    }
        // ボタン描画
        // text描画
        for(int i=0; i < buttonNum; i++){
            //ボタン描画
            mvaddch( y+i, x, '*');
            // ButtonPos[obj->y+bottonHeight[i]][obj->x+1] = '*'; //共通の配列にボタンを追加
            // iven管理
            events[i].x = x;
            events[i].y = y + i;
            // c = 0;
            // //text描画
            int len = strlen(events[i].text);
            int row = events[i].y;
            int col = events[i].x;
            for(int j=0;j < len; j++){
                mvaddch(row,col + 1 + j,events[i].text[j]);
            }
        }

}
/* 引数で左上のxyと幅，高さ,共有用のボタン配列を受け取る　*/
void DrawBtnOutLineUI(UIobj *obj, eventObj* event)
{
    int widthLine = obj->w - 2;
    int heightLine = obj->h -2;
    int textCol,textRow; //colが列 rowが行
    // if(btnLines != 0) heightLine = btnLines;
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

   // event表示 
   // '*' と eventを表示->text
   int ln = 0; // ボタンの行指定用
   int lnAdd = 0; //eventひとつ分の行数
   int averageLn = heightLine/obj->bottonNum; //均等にボタンを配置する用
   int textStartWidth = obj->x + 3;
   int c = 0; // textの描画x位置

   int lnNum=0;
   int overAveLen = 1; // 1 = true 
   int bottonHeight[obj->bottonNum];
   //計算
   // テキストの合計行数計算　結果をoverAveLenに格納
   bottonHeight[0] = 0;
   for(int i=0; i < obj->bottonNum; i++){ //ボタンの数繰り返し
        //文字の数取得し，合計行数がheightLine数を超えないか
        int textlen = strlen(event[i].text);
        int rowNum = (textlen / (widthLine-2));
        if(textlen % (widthLine-2) != 0) {
            rowNum+=1;
            // mvprintw(9+i,5,"textlen: %d,widthLine-2: %d rowNum:%d",textlen,(widthLine-2),rowNum);
        }
        if(i < obj->bottonNum -1) { 
            bottonHeight[i+1] = bottonHeight[i] + rowNum;
            // mvprintw(10+i,5,"bottonheihg: %d",bottonHeight[i+1]);
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
            if(event[i].task_status != NONE_TASK_STATUS)
            {
                switch(event[i].task_status)
                {
                    case TASK_UNTOUCH:
                    addstr("untouch");
                    break;
                    case TASK_PROGRESS:
                    addstr("running..");
                    break;
                    case TASK_DONE:
                    addstr("complete");
                    break;
                    default:
                    // addch(' ');
                    break;
                }
            }
            // ButtonPos[obj->y+bottonHeight[i]][obj->x+1] = '*'; //共通の配列にボタンを追加
            // iven管理
            event[i].x = obj->x+1;
            event[i].y = obj->y+ bottonHeight[i];
            // c = 0;
            // //text描画
            int len = strlen(event[i].text);
            int row = obj->y + bottonHeight[i];
            int space = 1;
            if(event[i].task_status != NONE_TASK_STATUS) space = 9;
            int col = textStartWidth + space; // statusの文字分
            for(int j=0;j < len; j++){
                mvaddch(row,col,event[i].text[j]);
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
            // ButtonPos[obj->y+bottonHeight[i]+1][obj->x+1] = '*'; //共通の配列にボタンを追加
            // iven管理
            event[i].x = obj->x+1;
            event[i].y = obj->y+ bottonHeight[i];
            //text描画
            int len = strlen(obj->event[i].text);
            int row = obj->y + bottonHeight[i]; // 前は+1だった
            int col = textStartWidth +9; // ステータスの文字分
            for(int j=0;j < len; j++){
                mvaddch(row,col,obj->event[i].text[j]);
                if( col > (obj->w)) {  // 2 = '*' + ' '
                    // textがUIの外枠'|'にかなったら改行
                    col = textStartWidth;
                    row +=1;
                }else col++;
            }
        }
    }else{  // そのままやるとオーバーする場合 
        mvaddch(0,0,'c');
        // show "too small"
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
            for(int i = 0; i < textObj->h-3; i++){
                move(textObj->y+i,textObj->x);
                for(int j = 0; j < textObj->w ; j++){
                    addch(' ');
                    
                }
            }
            // 描画
            for(int i = 0; i < textObj->h-3; i++){
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
            for(int i = 0; i < textObj->h-3; i++){
                move(textObj->y+i,textObj->x);
                for(int j = 0; j < textObj->w ; j++){
                    addch(' ');
                }
            }
            // 描画
            for(int i = 0; i < textObj->h-3; i++){
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
            // 文字追加の処理ここでabortが起きる
            for(int i =MAX_CONTEXT-1; i > textObj->cursorIndex; i-- ){
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
        return(-1);
    }
    rc = createTable(appObj->db,"diary","id INTEGER PRIMARY KEY, date TEXT, title TEXT, content TEXT, created_at TEXT, updated_at TEXT");
    if(rc!=SQLITE_OK){
        fprintf(stderr,"createTable failed: %d\n",rc);
        return(-1);
    }
    // 3.select関数の引数に与える．
    // strcpy(dObj.date,"2026/05/15");
    rc = selectDiaryByDate(appObj->db, appObj->diary);
    if(rc == SQLITE_NOTFOUND){// 日付の内容が見つからない場合
        // ゴミが入って処理が変にならないように初期化
        strcpy(appObj->diary->date,"");
        strcpy(appObj->diary->title,"");
        strcpy(appObj->diary->content,"sampleText");
        strcpy(appObj->diary->created_at,"");// updateの時にcreated_atが""ならcreated_atとupdated_atも変える
        strcpy(appObj->diary->updated_at,"");
    }else if(rc!=SQLITE_OK){
        fprintf(stderr,"selectDiaryTable failed: %d\n",rc);
        return(-1);
    }
    // 4.クローズ
    sqlite3_close(appObj->db); 
    strcpy(textObj->content,appObj->diary->content);
    return 0;
}



// 問題点 引数に指定する際使わないものが出てくる．使ってない
void dbEvent(DBFuncType num, AppContent appCon){
    int rc;
    switch(num) {
        case INSERT_DIARY_BYDATE:
            insertDiaryTable(appCon.db, appCon.diary);
            break;
        case INSERT_TODO_BYDATE:
            insertToDoTable(appCon.db, appCon.todos);
            break;
        case SELECT_DIARY_BYDATE:
            selectDiaryByDate(appCon.db, appCon.diary);
            break;
        case SELECT_TODO_BYDATE:
            selectToDoByDate(appCon.db,appCon.todos->date, appCon.todos, TODO_MAX, &appCon.todoCount);
            break;
        case UPDATE_DIARY_BYDATE:
            updateDiary(appCon.db, appCon.diary);
            break;
        case UPDATE_TODO_BYDATE:
            updateToDo(appCon.db, appCon.todos);
            break;
        case UPDATE_TODO_STATUS:
            updateToDoStatus(appCon.db, appCon.todos,appCon.todoCount);
            break;
        case DELETE_DIARY_BYID:
            deleteDiaryByID(appCon.db,appCon.diary);
            break;
        case DELETE_TODO_BYID:
            deleteTodoByID(appCon.db, appCon.todos);
            break;
        case DB_EVENT_NONE:
            break;
    }
}

int MainScreen()
{
    Cobj c;
    UIobj menu;
    int w,h;
    char input;

    //初期設定
    InitCobj(&c,1,1, 0.0, 0.0);

    //構造体の初期化
    eventObj eventData[4] = {
        {0, 0, "task",TO_DO, SELECT_TODO_BYDATE, NONE_TASK_STATUS},
        {0, 0, "record", RECORD, DB_EVENT_NONE, NONE_TASK_STATUS},
        {0, 0, "makeTask", MAKE_TASK, DB_EVENT_NONE, NONE_TASK_STATUS},
        {0, 0, "end", END, DB_EVENT_NONE, NONE_TASK_STATUS}
    };

    timeout(16);    // fps:60くらい
    while(1){
        erase();
        refresh();
        getmaxyx(stdscr, h, w);
        InitUIobj(&menu,0,0,w,h, sizeof(eventData)/sizeof(eventData[0]),eventData);
        DrawBtnOutLineUI(&menu,eventData);
        DrawCursor(&c);
        // キー入力
        input = ControlCursor(&c);
        if (input == 'q') return END;
        else if(input == '\n'){
            //    if(eventPos[c.py][c.px] == '*') {
                for(int i = 0; i < sizeof(eventData)/sizeof(eventData[0]); i++ ) {
                    if(eventData[i].x == c.px && eventData[i].y == c.py){
                        // ここでボタンを押された時 DB関連のボタンなら sqliteFuncで定義された関数を使用したい
                        // eventData[i].dbfuncNum();
                        /*
                        if(eventData[i].dbfuncNum != DB_EVENT_NONE){
                            rc = dbEvent(eventData[i].dbfuncNum);
                        }
                        if(eventData[i].nextState!=NONE) return eventData[i].nextState;
                        */
                        if(eventData[i].nextState!=NONE) return eventData[i].nextState;
                        // return eventData[i].nextState;
                    }
                }
            // }
        }
        MoveCursor(&c,&menu);
        // 動作速度調節
        usleep(20000);
    }
    return 0;
}
int TO_DOScreen(){
    Cobj c; 
    UIobj todoUI; 
    int w,h; 
    char input; 
    AppContent appObj;
    int rc;
    // int count;
    // date
    char timeBuf[64];
    time_t t = time(NULL);
    struct tm *local = localtime(&t);
    strftime(timeBuf, sizeof(timeBuf),"%Y/%m/%d",local);// %Y/%m/%d%A

    // 構造体の初期化
    ToDoObj todos[TODO_MAX];
    eventObj eventData[100];
    //初期設定
    getmaxyx(stdscr, h, w);
    InitCobj(&c,4,4, 0.0, 0.0);

    // open DB
    rc = sqlite3_open("testDB.db", &appObj.db);
     if(rc){
        fprintf(stderr,"Cant't open database: %s\n",sqlite3_errmsg(appObj.db));
        sqlite3_close(appObj.db);
        return(-1);
    }
    // select diary table where date=""
    rc = selectToDoByDate(appObj.db, timeBuf, todos, TODO_MAX, &appObj.todoCount);
    if(rc == SQLITE_NOTFOUND){
       appObj.todoCount = 0;
    }
    else if(rc!=SQLITE_OK){
        fprintf(stderr,"selectDiaryTable failed: %d\n",rc);
        sqlite3_close(appObj.db);
        return(-1);
    }
   
    // store tasks to struct
    for(int i = 0; i < appObj.todoCount; i++)
    {
        // strcpy(eventData[i].text,todos[i].task);
        // eventData.text <- task text
        strncpy(eventData[i].text, todos[i].task, sizeof(eventData[i].text) - 1);
        eventData[i].text[sizeof(eventData[i].text) - 1] = '\0';
        // eventData.
        eventData[i].task_status = todos[i].status;
        
        eventData[i].nextState = NONE;
    }
    sqlite3_close(appObj.db); 


    // store end main and end button.
    strncpy(eventData[appObj.todoCount].text, "back",sizeof(eventData[appObj.todoCount].text) -1);
    eventData[appObj.todoCount].text[sizeof(eventData[appObj.todoCount].text) -1] = '\0';
    eventData[appObj.todoCount].task_status = NONE_TASK_STATUS;
    eventData[appObj.todoCount].nextState = MAIN;


     // bucket sort use todos[] todoCount
    sort_bucket(eventData, appObj.todoCount);
    timeout(17);
    while(1){
        erase();
        refresh();
            // mvprintw(10,10,"eventData[appObj.todoCount].text:%d",appObj.todoCount);

        InitUIobj(&todoUI,0,0,w,h,appObj.todoCount+1,eventData);
        DrawBtnOutLineUI(&todoUI,eventData);
        DrawCursor(&c);

        // キー入力
        input = ControlCursor(&c);
        if (input == 'q') return END;
        else if(input == '\n'){
            //    if(eventPos[c.py][c.px] == '*') {
                for(int i = 0; i < sizeof(eventData)/sizeof(eventData[0]); i++ ) {
                    if(eventData[i].x == c.px && eventData[i].y == c.py){
                        // change status button
                        switch(eventData[i].task_status) 
                        {
                            case NONE_TASK_STATUS:
                                // eventData[i].task_status = TASK_UNTOUCH;
                                // todos[i].status = TASK_UNTOUCH;
                                
                                break;
                            case TASK_UNTOUCH:
                                eventData[i].task_status = TASK_PROGRESS;
                                todos[i].status = TASK_PROGRESS;
                                break;
                            case TASK_PROGRESS:
                                eventData[i].task_status = TASK_DONE;
                                todos[i].status = TASK_DONE;
                                break;
                            case TASK_DONE:
                                eventData[i].task_status = TASK_UNTOUCH;
                                todos[i].status = TASK_UNTOUCH;
                                break;
                            default:
                            break;
                        }
                        // backbutton
                        if(eventData[i].nextState == MAIN) 
                        {
                            if(appObj.todoCount!= 0)
                            {
                                // save status
                                // update todo
                                rc =sqlite3_open("testDB.db", &appObj.db);
                                if(rc)
                                {
                                    fprintf(stderr,"Cant't open database: %s\n",sqlite3_errmsg(appObj.db));
                                    sqlite3_close(appObj.db);
                                    return(-1);
                                }
                                rc = updateToDoStatus(appObj.db, todos,appObj.todoCount);
                                if(rc != SQLITE_DONE)
                                {
                                    sqlite3_close(appObj.db);
                                    return (-1);
                                }
                                sqlite3_close(appObj.db);
                            }
                            return eventData[i].nextState;
                        }
                    }
                }
            // }
        }else if(input == 'r')
        {
            sort_bucket(eventData, appObj.todoCount+1);
        }
        MoveCursor(&c,&todoUI);
        // 動作速度調節
        usleep(20000);
    }
    return 0;

}
// 範囲内，makeTaskのdate情報を更新，描画.カーソルを受け取る
int smallTaskDate(UIobj* ui, Date *dateObj, Cobj *c)
{
    int dayCount = 0;
    // 今日のyear, month, dateを数値で取得
    time_t timer;
    struct tm *local_time;
    int year, month, day;
    char date[64];
    int input;
    // 1. 現在時刻（協定世界時からの経過秒数）を取得
    time(&timer);

    // 2. ローカル時刻（現地時間）の構造体に変換
    local_time = localtime(&timer);
    while(1)
    {
         // key操作
        input = ControlCursor(c);
        if(input == KEY_UP)
        {
            // 日付カウントインクリメント
            dayCount ++;
            
        }else if (input == KEY_DOWN)
        {
            /* 日付カウントデクリメント */
            if(dayCount >= 0)
            {
                dayCount--;
            }
        }else if(input == '\e')
        {
            // escでreturn
            dateObj->year = year;
            dateObj->month = month;
            dateObj->day = day;
            return 0;
        }

        // change "date"
        if (dayCount == -1)
        {
            /* everyday task をdbに保存 */
            strncpy(date," everyday ",sizeof(date)-1);
            date[sizeof(date)-1] = '\0';
        }else
        {
            /* change date by today's date */
            local_time->tm_mday =  dateObj->day + dayCount;
            local_time->tm_mon = dateObj->month-1;
            local_time->tm_year = dateObj->year;
            // mvprintw(10,10,"dayCount:%d",dayCount);

            // 3. 各要素を数値として抽出
            mktime(local_time);
            // mvprintw(11,10,"local_time:%d",local_time);

            year = local_time->tm_year; // tm_yearは1900年からの経過年数
            month = local_time->tm_mon + 1;    // tm_monは0～11で取得される
            day = local_time->tm_mday;         // tm_mdayはそのまま日を表す
            sprintf(date, "%04d/%02d/%02d",year,month,day);
        }
        // 描画
        mvaddstr( ui->y+2,ui->x+4 ,date);
        usleep(50000);
    }
       
}

int makeTaskScreen(void)
{
    Cobj c;
    UIobj makeTaskUI;
    int w,h;
    char input;
    int uiStatus = MAKE_TASK;
        char timeBuf[64];


    // 全体の外枠は描画，ボタンの場所は割合で作成．drawBtnUIいらない
    // time
    time_t timer;
    struct tm *local_time;
    // int year, month, day;

    Date date;
    // 1. 現在時刻（協定世界時からの経過秒数）を取得
    time(&timer);

    // 2. ローカル時刻（現地時間）の構造体に変換
    local_time = localtime(&timer);
    // 3. 各要素を数値として抽出
    date.year = local_time->tm_year + 1900; // tm_yearは1900年からの経過年数
    date.month = local_time->tm_mon + 1;    // tm_monは0～11で取得される
    date.day = local_time->tm_mday;         // tm_mdayはそのまま日を表す


    InitCobj(&c,1,1,0.0,0.0);
    eventObj eventData[4] = {
        {1, 0, "date", TASK_DATE, DB_EVENT_NONE,NONE_TASK_STATUS},
        {1, h*1/4, "task", TASK_CONTENT, DB_EVENT_NONE,NONE_TASK_STATUS},
        {1, h/2, "due", TASK_MAKE, DB_EVENT_NONE,NONE_TASK_STATUS },
        {0, h*3/4, "back",MAIN, DB_EVENT_NONE,NONE_TASK_STATUS},
    };
    eventObj dummy[0];
    /*
    date task 
    左右のキーでdate, taskへ遷移
    上下のキーで日付変更
    enterキーて保存
    */
   timeout(16);
   while(1)
   {
    erase();
    refresh();
    getmaxyx(stdscr,h,w);
    InitUIobj(&makeTaskUI, 0,0,w,h, 4,eventData);
    sprintf(timeBuf,"%04d/%02d/%02d",date.year,date.month,date.day);
    mvaddstr(makeTaskUI.y+2,makeTaskUI.x+4,timeBuf);
    DrawBtnOutLineUI(&makeTaskUI,eventData); // outlineを描画
    // DrawBtnUI(eventData, sizeof(eventData)/sizeof(eventData[0]), 1, h-2,w,2);
    DrawCursor(&c);
    input = ControlCursor(&c); //pv pyがcobjに加えられる + 入力keyを返す
    if(input == '\n') 
    {
        for(int i = 0; i < sizeof(eventData)/sizeof(eventData[0]); i++ ) {
            if(eventData[i].x == c.px && eventData[i].y == c.py){
                switch (eventData[i].nextState)
                {
                    case MAIN:
                        return MAIN;
                        break;
                    case TASK_DATE:// ここでtaskDateSmallなどのscreenへ行く
                        /* code */
                        smallTaskDate(&makeTaskUI,&date,&c);
                        break;
        
                    default:
                        break;
                }
            //    return eventData[i].nextState;
            }
        }
    }
    MoveCursor(&c, &makeTaskUI);
    usleep(20000);
   }
    return 0;
}
// 指定された位置への移動 
int restrictedMoveCursor(Cobj* c, UIobj* ui, eventObj* event[], int btnNum) // 引数: cobj, 全体UIの大きさ，ボタンPos
{
    int i = c->px;
    if(c->px != 0){
        while( ui->x < i && i < ui->x+ui->w )// uiの中なら
        {
            i += c->vx;
            for(int j = 0; j < btnNum; j++)
            {
                if(i == event[j]->x)
                {
                    c->vx = i; // 移動分追加
                    MoveCursor(c, ui);
                }
            }
            
        }
    }
    return 0;
}

int diaryScreen(void)
{
    Cobj c;
    int input;
    int w,h;
    UIobj diaryUI;
    textObj tObj;
    DiaryObj dObj;
    ToDoObj toObj;
    AppContent appObj;
    int rc;
    appObj.diary = &dObj;
    appObj.todos[0] = toObj;
    int contentSize;

    // time relation
    char timeBuf[64];
    time_t t;
    struct tm *local;

    // event
    //構造体の初期化
    eventObj eventData[0] = {
        // {0, 0, "to do",TO_DO, DB_EVENT_NONE},
        // {0, 0, "record", RECORD,DB_EVENT_NONE},
        // {0, 0, "none", NONE, DB_EVENT_NONE}
    };
    eventObj btnEvents[2] = { 
        {0,0,"update",NONE, UPDATE_DIARY_BYDATE, NONE_TASK_STATUS},
        {0,0,"back", MAIN, DB_EVENT_NONE,NONE_TASK_STATUS}
    };
    refresh();

    // 初期化
    strcpy(tObj.content,"");
    // char text[MAX_CONTEXT]={0};
        // printf("date:%s\n", dObj.date);
    // printf("title:%s\n", dObj.title);
    // printf("content:%s\n", dObj.content);
        // strcpy(text,dObj.content);
    // textObj Initialization
    
    InitUIobj(&diaryUI,0,0,20,20,0,eventData);
    rc = InitTextObj(&tObj,diaryUI.x+1,diaryUI.y+1,diaryUI.w-2,diaryUI.h-2,&appObj); // この関数内でdb開いてtext取得して，closeまでやる
    // contentSize = sizeof(dObj.content)/sizeof(dObj.content[0]);
    // mvprintw(19,21,"diary contentSize:%d",contentSize);

    InitCobj(&c,tObj.x,tObj.y,0,0);
    // viewText(&tObj);
    // erase();
    // refresh();
    timeout(16);
    while(1){
        getmaxyx(stdscr, h, w);
        // input = 0;
        input = ControlCursor(&c);
        if(input != -1) mvprintw(21,21,"diaryScreen KEY:%02d",input);
        // 時間取得
                t = time(NULL);// 基準時刻取得
                local = localtime(&t);
                strftime(timeBuf, sizeof(timeBuf),"%Y/%m/%d",local);// %Y/%m/%d%A
                //dobj更新
                strcpy(dObj.date,timeBuf);
        if(input=='\n')
        {
            for(int i = 0; i < sizeof(btnEvents)/sizeof(btnEvents[0]); i++ ) {
                if(btnEvents[i].x == c.px && btnEvents[i].y == c.py){
                    if(btnEvents[i].nextState == NONE)
                    {
                        if(btnEvents[i].dbfuncNum == UPDATE_DIARY_BYDATE)
                        {
                            // 時間取得
                            t = time(NULL);// 基準時刻取得
                            local = localtime(&t);
                            strftime(timeBuf, sizeof(timeBuf),"%Y/%m/%d",local);//%H:%M:%S %A
                            // 1.DiaryObjを作成．その日付にstrftimeで作成した文字列を入れる
                            strcpy(dObj.date, timeBuf);
                            // 2.dbをopenする
                            rc = sqlite3_open("testDB.db", &appObj.db);
                            if(rc){
                                fprintf(stderr,"Cant't open database: %s\n",sqlite3_errmsg(appObj.db));
                                sqlite3_close(appObj.db);
                                return(-1);
                            }
                            rc = selectDiaryByDate(appObj.db, appObj.diary);
                            if(rc == SQLITE_NOTFOUND){// 日付の内容が見つからない場合
                                // insert
                                // insert文テスト
                                // 時間取得
                                char timeBuf[64];// 時間の文字列timeBuffer
                                time_t t = time(NULL);// 基準時刻取得
                                struct tm *local = localtime(&t);
                                strftime(timeBuf, sizeof(timeBuf),"%Y/%m/%d",local);// %Y/%m/%d%A
                                //dobj更新
                                strcpy(dObj.date,timeBuf);
                                strcpy(dObj.title,"this is titile");
                                strcpy(dObj.content, tObj.content);
                                strftime(timeBuf, sizeof(timeBuf),"%H:%M:%S",local);// %Y/%m/%d%A
                                strcpy(dObj.created_at, timeBuf);
                                strcpy(dObj.updated_at, timeBuf);
                                //db更新
                                // rc = sqlite3_open("testDB.db", &appObj.db);
                                // if(rc){
                                //     fprintf(stderr,"Cant't open database: %s\n",sqlite3_errmsg(appObj.db));
                                //     sqlite3_close(appObj.db);
                                //     return(-1);
                                // }

                                rc = insertDiaryTable(appObj.db, &dObj);
                                if(rc != SQLITE_DONE){
                                    fprintf(stderr, "can't insertDiaryTable: %s\n", sqlite3_errmsg(appObj.db));
                                    sqlite3_close(appObj.db);
                                    return(-1);
                                }
                                // rc = insertToDoTable(appObj.db, &tObj);

                                // 4.クローズ
                                sqlite3_close(appObj.db); 
                            }else if(rc!=SQLITE_OK){
                            fprintf(stderr,"selectDiaryTable failed: %d\n",rc);
                            return(-1);
                            }else{
                                // update
                                // updatediary
                                // if (created_at =='') updateでcreated_atとupdated_atどっちも変える
                                // else updateでcontentとtitle，updatedを変える
                                    // update
                                // 時間取得
                                t = time(NULL);// 基準時刻取得
                                local = localtime(&t);
                                strftime(timeBuf, sizeof(timeBuf),"%H:%M:%S",local);// %Y/%m/%d%A
                                
                                //dobj更新
                                // strcpy(dObj.created_at, timeBuf);
                                strcpy(dObj.updated_at, timeBuf);
                                strcpy(dObj.content, tObj.content);

                                //db更新
                                rc = sqlite3_open("testDB.db", &appObj.db);
                                if(rc){
                                    fprintf(stderr,"Cant't open database: %s\n",sqlite3_errmsg(appObj.db));
                                    sqlite3_close(appObj.db);
                                    return(-1);
                                }

                                rc = updateDiary(appObj.db, &dObj);
                                if(rc != SQLITE_DONE){
                                    fprintf(stderr, "can't updateDiary: %s\n", sqlite3_errmsg(appObj.db));
                                    sqlite3_close(appObj.db);
                                    return(-1);
                                }
                                // 4.クローズ
                                sqlite3_close(appObj.db); 
                                // break;
                            }
                        }
                    }
                    else
                    {
                        return btnEvents[i].nextState;
                    }
                }
            }
        }
        // データ変更
        MoveCursor(&c,&diaryUI);
        if(input != '\n') UpdateText(&c,&tObj,input);

        //描画
        DrawBtnOutLineUI( &diaryUI, eventData);
        DrawText(&c, &tObj);
        DrawBtnUI(btnEvents, 2, diaryUI.x+1,diaryUI.y+diaryUI.h-3, diaryUI.w, 3);
        DrawTextCursor(&c);
        

        refresh();
        usleep(20000);
    }
    return 0;
}
int main(void)
{
    /* curses の設定 */
	initscr();
	curs_set(0);		// カーソルを表示しない
	noecho();		// 入力されたキーを表示しない
	cbreak();		// 入力バッファを使わない(Enter 不要の入力)
	keypad(stdscr, TRUE);	// カーソルキーを使用可能にする
    /* DB関連 */
    sqlite3* db;
    int rc;

    /* 本体 ( screen関数の戻り値が次の画面への状態　)*/
    int i = 1;
    int nextScreen = MainScreen();
    while(i){
        switch(nextScreen){

            case MAIN:
                nextScreen = MainScreen();
                break;
            case TO_DO:
                nextScreen = TO_DOScreen();
                break;
            case RECORD:
                erase();
                refresh();
                nextScreen = diaryScreen();
                break;
            case MAKE_TASK:
                nextScreen = makeTaskScreen();
                break;
            case -1:
                fprintf(stderr,"error \n");
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