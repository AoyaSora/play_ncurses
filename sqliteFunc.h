/* sqliteFunc.h */
#ifndef SQLITEFUNC_H
#define SQLITEFUNC_H

#include<sqlite3.h>

typedef struct {
    char date[64];
    char title[256];
    char content[1024];
    char created_at[64];
    char updated_at[64];
} DiaryObj;
typedef struct {
    char date[64];
    char task[64];
    int status;
} ToDoObj;


int createTable(sqlite3 *db, const char* tableName, const char* columns);
int insertDiaryTable(sqlite3* db, const DiaryObj *d);
int insertToDoTable(sqlite3* db, ToDoObj *t);
int updateDiary(sqlite3* db,int id, const char*title, const char* content, const char* updated_at);
int updateToDo(sqlite3* db, int id, const char*date, const char* task, int status);
int updateToDoStatus(sqlite3* db, int id,int status);
int deleteDiaryByID(sqlite3* db, int id);
int deleteTodoByID(sqlite3* db,int id);
int selectDiaryByDate(sqlite3* db, DiaryObj* d);
int selectToDoByDate(sqlite3* db,const char* date,ToDoObj t[],int maxCount);
#endif