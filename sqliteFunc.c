#include<stdio.h>
#include "sqliteFunc.h"

static int callback(void* NotUsed, int argc, char** argv, char** azColName){
    int i;
    for(int i = 0; i<argc; i++){
        printf("%s=%s\n",azColName[i],argv[i]?argv[i]:"NULL");
    }
    printf("\n");
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
/*
UPDATE table SET 変更内容 WHERE 条件;
idでtitle,content,updated_atを変更している
*/
int updateDiary(sqlite3* db,int id, const char*title, const char* content, const char* updated_at) {
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE diary SET title=?, content=?, updated_at=? WHERE id=?;";
    int rc;

    rc = sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    if(rc!=SQLITE_OK) return rc;

    sqlite3_bind_text(stmt, 1, title,-1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, content,-1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, updated_at,-1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, id);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    return rc;
}

int updateToDo(sqlite3* db, int id, const char*date, const char* task, int status){
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE toDo SET date=?, task=?, status=? WHERE id=?;";
    int rc;

    rc = sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    if(rc!=SQLITE_OK) return rc;

    sqlite3_bind_text(stmt, 1,date,-1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2,task,-1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3,status);
    sqlite3_bind_int(stmt, 4,id);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    return rc;
}
int updateToDoStatus(sqlite3* db, int id,int status){
    sqlite3_stmt* stmt;
    const char* sql = "UPDATE toDo SET status=? WHERE id=?;";
    int rc;
    rc = sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    if(rc!=SQLITE_OK) return rc;

    sqlite3_bind_int(stmt, 1,status);
    sqlite3_bind_int(stmt, 2,id);
    
    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    return rc;
}

// delete
// DELETE FROM テーブル名 WHERE 条件;
int deleteDiaryByID(sqlite3* db, int id){
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM diary WHERE id=?;";
    int rc;

    rc = sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    if(rc!=SQLITE_OK) return rc;

    sqlite3_bind_int(stmt, 1,id);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    return rc;
}
int deleteTodoByID(sqlite3* db,int id){
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM toDo WHERE id=?;";
    int rc;

    rc = sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    if(rc!=SQLITE_OK) return rc;

    sqlite3_bind_int(stmt, 1,id);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    return rc;
}