/* sqliteFunc.h */
#ifndef SQLITEFUNC_H
#define SQLITEFUNC_H

#include<sqlite3.h>
#define TODO_MAX 100
typedef struct {
    char date[128];
    char title[256];
    char content[1024];
    char created_at[64];
    char updated_at[64];
    int id;
} DiaryObj;
typedef struct {
    char date[128];
    char task[64];
    int status;
    int id;
} ToDoObj;
/* アプリ内で使用するデータのまとまり */
typedef struct{
    sqlite3* db;
    DiaryObj* diary;
    ToDoObj todos[TODO_MAX];
    int todoCount;
} AppContent;


int createTable(sqlite3 *db, const char* tableName, const char* columns);
int insertDiaryTable(sqlite3* db, DiaryObj *d);
int insertToDoTable(sqlite3* db, ToDoObj *t);
int updateDiary(sqlite3* db, DiaryObj* d);
int updateToDo(sqlite3* db, ToDoObj* t);
int updateToDoStatus(sqlite3* db, ToDoObj* t);
int deleteDiaryByID(sqlite3* db, DiaryObj* d);
int deleteTodoByID(sqlite3* db, ToDoObj* t);
int selectDiaryByDate(sqlite3* db, DiaryObj* d);
int selectToDoByDate(sqlite3* db,const char* date,ToDoObj t[],int maxCount,int* count);
#endif