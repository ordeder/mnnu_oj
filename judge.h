#ifndef JUDGE_H
#define JUDGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <mysql/mysql.h>

#define STD_MB 1048576
#define STD_T_LIM 2
#define STD_F_LIM (STD_MB<<5)
#define STD_M_LIM (STD_MB<<7)

#define LANG_GCC     1
#define LANG_GPP     2
#define LANG_VC      3
#define LANG_VCPP    4
#define LANG_JAVA    5
#define LANG_PASCAL  6

#define FETCH_READY     0
#define FETCH_RUNNING   1
#define FETCH_JUDGED    2
#define FETCH_R_READY   3
#define FETCH_R_RUNNING 4
#define FETCH_R_JUDGED  5
//judge result
#define STATUS_NULL     0
#define STATUS_AC       1
#define STATUS_WA       2
#define STATUS_RE       3
#define STATUS_PE       4
#define STATUS_TLE      5
#define STATUS_MLE      6
#define STATUS_OLE      7
#define STATUS_CE       8
#define STATUS_OTHER    9
#define STATUS_RS       10
#define STATUS_RK       11

#define SPJ_FALSE       0
#define SPJ_TRUE        1

#define KB              1024         // 1K
#define MB              KB * KB  // 1M
#define GB              KB * MB  // 1G
#define BUFSIZE 1024

#define ABS(a) ((a)>0?(a):-(a))
#define CHARLEN sizeof(char)
#define IS_SPACE_OR_RETURN(a) ((' '==a)||('\t'==a)||('\n'==a))

//system return
#define SQL_SUCCEED 0
#define SQL_FAILED 1
#define SYS_SUCCEED 2
#define SYS_FAILED 3

//judge type
#define MAX_JUDGE_TYPE    3
#define JUDGE_EXERCISE    0
#define JUDGE_CONTEST     1
#define JUDGE_EXAM        2

//compile info
#define compile_time_limit 5000
#define GCC_COMPILE_ERROR  1


extern int DebugFlag;
//judge message


//judge usg limit
#define  stack_size_limit  (1024*5)

//other
char buf[1024];


/*
debug:
*/
inline void err_system(char *message);

/**
judgetype is a global argcment define in log.c
 *概要：依据评测类型和评测runid初始化评测评测进程的参数
 *传入参数：runid，评测类型（exetcist，contest，diy_contest）
 *传回参数：NULL
 *ret：系统执行情况
 */
int judge_init(char runid[],int judgeflag);
/**
 *概要：更新评测结果
 *传入参数：runid，评测类型（exetcist，contest，diy_contest）
 *传回参数：NULL
 *ret：此模块的运行情况
 */
int judge_update(char runid[],int judgeflag,int result,int time_usage,int mem_usage);


/*评测主程序*/
int judge_main(char argv[], int judgeflag);
//设定定时器
int set_alarm(int milliseconds);

//超时处理函数
void timeout(int signo);

int set_limit();

int oj_compare_output(char *exefile, char *cmpfile);
int oj_compare_spj_output(char *spj_dir, char *exec_output, int problemid,
char *std_output, char *cmp_pre);

#endif
