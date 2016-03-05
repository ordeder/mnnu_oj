#ifndef OJ_DAEMON_H
#define OJ_DAEMON_H

#include<sys/select.h>
#include<sys/socket.h>

/*judge proc status*/
#define FREE        0
#define BUSY        1
#define ABORT		2

/*
mysql 连接信息
*/
extern MYSQL *g_conn;
extern char host_name[BUFSIZE];
extern char user_name[BUFSIZE];
extern char password[BUFSIZE];
extern char db_name[BUFSIZE];
extern int port_number;
extern int max_running;
extern int sleep_time;
extern int sleep_tmp;
extern int max_judge_time;
extern int max_memory_limit;
extern int jvm_mem_usage;

/*题目信息和提交信息，这些信息在初始化评测函数init_XXX_judge()函数中被初始化*/
extern int lang, time_limit, com_limit, cmp_limit, memory_limit, output_limit;
extern int problemid;
extern int userid;
extern int contestid;
extern int runid;
extern int codelen;
extern int submittime;
extern int java_time_limit, java_memory_limit;
extern int spjflag;

		//编译时的标准输出和错误输出的文件名 fetched in loadconfig()
extern char	stdout_file_compile[129], stderr_file_compile[129],
		//运行程序时标准输出和标准出错输出的文件名 fetched in loadconfig()
		stdout_file_exec[129],stderr_file_exec[129],
		//标准输出数据和标准输入数据路径 fetched in init_XX_judge
		comparedatafile[129],testdatafile[129],
		//用户程序源程序文件名 fetchtd in init_XX_judge
		sourcefile[129];
		//评测任务工作路径 fetcued in loadconfig
		char judge_workdir[129];
		//练习x创建的文件夹名称 fetched in loadconfig
		char	exercise_input_path[129], exercise_output_path[129],
		contest_input_path[129], contest_output_path[129],
		diy_contest_input_path[129], diy_contest_output_path[129],
		spj_exec_path[129];

/*num of judge proc fetched by loadconfig*/
extern int MAX_JUDGER_SIZE;
//shutdown flag
extern int quit;


/*评测进程池中进进程的信息*/
typedef struct judge_statue{
	pid_t   judge_pid;	//评测进程pid
	int     judge_stat; //评测进程状态
	int     judge_sfd;	//通信socket
	int 	job_runid;	//正在评测的作业id
	int		job_type;
}judge_proc,*pjudge_proc;
/*评测记录*/
typedef struct job{
	char	runid[255];
	int		judge_type;
	int		empty;
	int		sys_stat;
}job,pjob;


/**
 *概要：系统出错，更新数据库结果为OTHER（system error）
 *传入参数：runid，评测类型
 *传回参数：NULL
 *ret：更新是否成功
 *ps：judgetype（评测类型＋runid）定义于log.c
 */
inline int do_err_sys(char runid[], int judgeflag);

/**
 *概要：删除文件夹（在评测进程返回SYSSUCCEED情况下）
 *传入参数：runid，评测类型
 *传回参数：NULL
 *ret：NULL
 */
inline void delete_work_dir(char argv[], int judgeflag);

/**
 *概要：加载配置信息
 *传入参数：NULL
 *传回参数：NULL
 *ret：NULL
 */
void loadconf();

/**
 *概要：调度进程读取数据库，取得未评测的记录
 *传入参数：job
 *传回参数：job
 *ret：查询是否成功
 */
int get_job(job *pjob);

/**
 *概要：评测子进程入口
 *传入参数：评测进程在进程池中的id，与服进程通信的描述符
 *传回参数：NULL
 *ret：评测进程如果在评测种遇到FAILED，exit结束进程
 */
int judge_proc_hander(int judge_id,int sfd);

/**
 *概要：重建一个评测进程
 *传入参数：进程池中的id
 *传回参数：NULL
 *ret：执行情况
 */
void remake_judge_proc(int i);

/**
 *概要：关闭内核
 *传入参数：NULL
 *传回参数：NULL
 *ret：NULL
＊ps：现关闭评测进程，然后退出
 */
void shut_down();

#endif
