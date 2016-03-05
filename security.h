#ifndef SECURITY_H
#define SECURITY_H

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
#include<pwd.h>

/* syscall table entry */
typedef struct table{
    /* syscall name */
    char *name;

    /* 
	* permission, 0 for forbidden, 1 for pass,
	* 2 for pass and check memory usage.Need
	* more precise and standard definition.
	* */
	int permit;

    /*
	* callback process function, if NOT null,
	* judge program will call it to check the
	* syscall and parameters.
	* return value: 0 for unchange, nonzero values
	* indicates change the return value by the judge
	* */
	int (*process)(int);
}table,*TABLE;

extern table syscall_table[];
//此表存放系统调用允许调用次数
extern short int syscall_enable[1024];

//C和C++程序允许调用的系统调用表，execve调用另外处理
//下表的调用不限次数
extern int syscall_enable_c_cpp[];

//JVM虚拟机允许调用的系统调用表
//此调用列表来源于武汉大学WOJ开源项目
extern int syscall_enable_java[];

//初始化系统允许调用的系统调用表
void init_syscall_enable(int lang);
int is_enable_syscall(int syscall_id);

#endif
