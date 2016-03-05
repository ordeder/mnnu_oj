#ifndef DATABASE_H
#define DATABASE_H

extern char db_table_name[][25];

inline void err_sql(char *sqlcmd);

int init_mysql();

/**
 *概要：重新连接数据库
 *传入参数：NULL
 *传回参数：NULL
 *ret：连接情况
 */
int mysql_reconnect();

inline int executesql(char *sql);

 /*初始化评测*/
int init_exercise_judge(char argv[]);
int init_diy_contest_judge(char argv[]);
int init_contest_judge(char argv[]);
/*更新数据库结果*/
int exercise_updatedb(int result, int timeusage, int memoryusage, char argv[]);
int contest_updatedb(int result, int timeusage, int memoryusage, char argv[]);
int diy_contest_updatedb(int result, int timeusage, int memoryusage,char argv[]);

#endif
