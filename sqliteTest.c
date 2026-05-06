#include<stdio.h>
#include<sqlite3.h>

int createTable(sqlite3 *db, const char* tableName, const char* columns);

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

int insertTable(sqlite3* db,  char* tableName, const char* columns){
    sqlite3_stmt* stmt;
    char sql[512];
    char placeholders[256];
    int rc;

    int colCount = count_columns(columns);
    make_placeholders(placeholders,colCount);
    //sqlに準備文の埋め込み
    snprintf(sql,sizeof(sql),"INSERT INTO %s(%s) VALUES (%s);",tableName,columns,placeholders);
    // INSERT文の準備
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(rc!= SQLITE_OK) return rc;

    
    // sqlite3_prepare_v2(sqlite3 *db, const void* zSal, int nByte, sqlite3_stmt** ppStmt, const char** pzTail)
    /*
    第一引数 db: データベース接続
    第二引数 zSql: コンパイルするステートメント
    第三引数 nByte: -1の場合，最初のゼロ終端文字まで読み込む．nByteが正の場合，zSqlから読み込まれる最大バイト数，nByteが0の場合，準備済みステートメントは生成されない．呼び出し元が指定された文字列がnull終端されている事を知っている場合，null終端文字を含む入力文字列のバイト数であるnByteパラメータを渡すとパフォーマンスがわずかに向上する．
    第四引数 *ppStmt: sqlite3_step()を利用して実行できるコンパイル済みの準備積みステートメントを指すように設定されている．エラー発生時，*ppStmtはnULLになる．ppStmtはNULLにできない．
    第五引数 pzTail: NULLでない場合，*pzTailはzSql内の最初のSQL文の末尾の次の最初のバイトを指す．複数のsql文をloopで実行する場合に使用する
    
    */
    // sqlite3_prepare_v2() -> sqlite3_bind_text() -> sqlite3_step(stmt) -> sqlite3_finalize(stmt)
}
//columnの数を数える '\0'で終了
int count_columns(const char *columns) {
    int count = 1;
    for (int i = 0; columns[i]; i++) {
        if (columns[i] == ',') count++;
    }
    return count;
}
// columnの数だけ準備文のための?,の文字列を作成する 
void make_placeholders(char *buf, int n) {
    buf[0] = '\0';

    for (int i = 0; i < n; i++) {
        strcat(buf, "?");   // '\0'の前に追加
        if (i != n - 1) strcat(buf, ", "); // 最後の?じゃなかったら ','を追加
    }
}