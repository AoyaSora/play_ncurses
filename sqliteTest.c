#include<stdio.h>
#include<sqlite3.h>
typedef struct {
    const char *date;
    const char *title;
    const char *content;
    const char *created_at;
    const char *updated_at;
} DiaryObj;
typedef struct {
    const char *date;
    const char *task;
    int status;
} ToDoObj;

int createTable(sqlite3 *db, const char* tableName, const char* columns);
int insertDiaryTable(sqlite3* db, const DiaryObj *d);
int insertToDoTable(sqlite3* db, ToDoObj *t);

static int callback(void* NotUsed, int argc, char** argv, char** azColName){
    int i;
    for(int i = 0; i<argc; i++){
        printf("%s=%s\n",azColName[i],argv[i]?argv[i]:"NULL");
    }
    printf("\n");
    return 0;
}

int main(void){
    sqlite3* db;
    char* zErrMsg = 0;
    int rc;
    // DiaryObjectの作成
    DiaryObj d = {
        "2026/05/5",
        "タイトル:ガンバ",
        "今日の中身",
        "20:20",
        "20:40"
    };
    ToDoObj t = {
        "2026/05/5",
        "英語勉強 25min"
    };

    // if(argc!=3){
    //     fprintf(stderr,"Usage%s DATABASE SQL-STATEMENT\n", argv[0]);
    //     return (1);
    // }
    // sqlite3_open( データベースファイル名,db接続オブジェクト)
    rc = sqlite3_open("testDB.db",&db);
    if(rc){
        fprintf(stderr,"Cant't open database: %s\n",sqlite3_errmsg(db));
        sqlite3_close(db);
        return(1);
    }

    // create table
    rc = createTable(db,"toDo","id INTEGER PRIMARY KEY, date TEXT, task TEXT, status INTEGER");
    if(rc!=SQLITE_OK){
        fprintf(stderr,"createTable failed: %d\n",rc);
        return(1);
    }
    rc = createTable(db,"diary","id INTEGER PRIMARY KEY, date TEXT, title TEXT, content TEXT, created_at TEXT, updated_at TEXT");
    if(rc!=SQLITE_OK){
        fprintf(stderr,"createTable failed: %d\n",rc);
        return(1);
    }

    // insert
    rc = insertDiaryTable(db,&d);
    if(rc!=SQLITE_DONE){
        fprintf(stderr,"insertDiaryTable failed: %d\n",rc);
        return(1);
    }
    rc = insertToDoTable(db,&t);
    if(rc!=SQLITE_DONE){
        fprintf(stderr,"insertDiaryTable failed: %d\n",rc);
        return(1);
    }

    sqlite3_close(db);
    return 0;
}

int createTable(sqlite3 *db, const char* tableName, const char* columns){
    char* zErrMsg = 0;
    char sql[512];
    int rc;

    //sqlにテーブル定義を埋め込み
    snprintf(sql,sizeof(sql), "CREATE TABLE IF NOT EXISTS %s (%s);",tableName, columns);
    // exec reate table SQL
    rc = sqlite3_exec(db,sql,NULL,0,&zErrMsg);
    if(rc!=SQLITE_OK){
        fprintf(stderr,"SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return rc;
    }
    return SQLITE_OK;
}

int insertDiaryTable(sqlite3* db, const DiaryObj *d){
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO diary(date,title,content,created_at,updated_at) VALUES (?,?,?,?,?);";
    int rc;

    // INSERT文の準備
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(rc!= SQLITE_OK) return rc;
    
    // bindで'?'に挿入   
    sqlite3_bind_text(stmt, 1, d->date, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, d->title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, d->content, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, d->created_at, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, d->updated_at, -1, SQLITE_STATIC);

    
    // step実行
    rc = sqlite3_step(stmt);// ここでのエラーは関数を使用するときに書く．

    // 後処理
    sqlite3_finalize(stmt);
    return rc;

    /*  sqlite3_prepare_v2(sqlite3 *db, const void* zSal, int nByte, sqlite3_stmt** ppStmt, const char** pzTail)
    第一引数 db: データベース接続
    第二引数 zSql: コンパイルするステートメント
    第三引数 nByte: -1の場合，最初のゼロ終端文字まで読み込む．nByteが正の場合，zSqlから読み込まれる最大バイト数，nByteが0の場合，準備済みステートメントは生成されない．呼び出し元が指定された文字列がnull終端されている事を知っている場合，null終端文字を含む入力文字列のバイト数であるnByteパラメータを渡すとパフォーマンスがわずかに向上する．
    第四引数 *ppStmt: sqlite3_step()を利用して実行できるコンパイル済みの準備積みステートメントを指すように設定されている．エラー発生時，*ppStmtはnULLになる．ppStmtはNULLにできない．
    第五引数 pzTail: NULLでない場合，*pzTailはzSql内の最初のSQL文の末尾の次の最初のバイトを指す．複数のsql文をloopで実行する場合に使用する
    */
   /*   sqlite3_bind_*(sqlite3_stmt*, )
   第一引数 sqlite3_stmt: 常にsqlite3_prepare_v2()またはその派生関数から返されるsqlite3_stmtオブジェクトへのポインタ
   第二引数 : 設定するSQlパラメータのインデックス．(?,?,)の場所一番左が1同じ名前のSQLパラメータが複数回使用される場合，二度目以降の出現場所h最初の出現箇所と同じインデックスになる．
   第三引数 : パラメータにバインドする値．
   第四引数 : パラメータのバイト数．負の場合は最初のゼロ終端までのバイト数．未定義もある．
   使用するとしたらsqlite3_bind_int,sqlite3_bind_textぐらい?
   第五引数 : 3番目のパラメータによって参照されるオブジェクトの有効期間を制御または示す．
   */
    // sqlite3_prepare_v2() -> sqlite3_bind_text() -> sqlite3_step(stmt) -> sqlite3_finalize(stmt)
}

int insertToDoTable(sqlite3* db, ToDoObj *t){
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO toDo(date,task,status) VALUES (?,?,?)";
    int rc;

    rc = sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    if(rc!= SQLITE_OK) return rc;

    sqlite3_bind_text(stmt, 1,  t->date, -1,SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, t->task, -1,SQLITE_STATIC);
    sqlite3_bind_int(stmt,3, t->status);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    return rc;
}

// update
int update

// delete