/*
   oj_daemon.c created by slp 23/05/2012 
   email:    slp195@qq.com
*/


#include "judge.h"
#include "security.h"
#include "log.h"
#include "oj_daemon.h"
#include "database.h"
#include <fcntl.h>
#include <sys/resource.h>
#include <syslog.h>
#include <sys/reboot.h>

/*
mysql 连接参数
*/
MYSQL *g_conn;
char host_name[BUFSIZE];
char user_name[BUFSIZE];
char password[BUFSIZE];
char db_name[BUFSIZE];
int port_number;

/*评测机子配置*/
int max_running; //??
int sleep_time;  //??
int sleep_tmp;   //??
int max_memory_limit; //评测机子最大内�?int jvm_mem_usage;    //jvm 运行耗费的内�?int MAX_JUDGER_SIZE;  //评测进程池的大小
int max_judge_time;   //for reboot

/*题目信息和提交信息，这些信息在初始化评测函数init_XXX_judge()函数中被初始�?/
int lang, time_limit, com_limit, cmp_limit, memory_limit, output_limit;
int problemid;
int userid;
int contestid;
int runid;
int codelen;
int submittime;
int java_time_limit, java_memory_limit;
int spjflag;

		//编译时的标准输出和错误输出的文件�?fetched in loadconfig()
char	stdout_file_compile[129], stderr_file_compile[129],
		//运行程序时标准输出和标准出错输出的文件名 fetched in loadconfig()
		stdout_file_exec[129],stderr_file_exec[129],
		//标准输出数据和标准输入数据路�?fetched in init_XX_judge
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


///////////////////////local argment////////////////////

/*shutdown flag*/
int quit;
		
/*全局变量，评测进程池中进进程的信�?/
judge_proc judge_proc_pool[10];
int num_free_judge=0;

//////////////////////////////////////////////////////////

/**
 *概要：系统出错，更新数据库结果为OTHER（system error�? *传入参数：runid，评测类�? *传回参数：NULL
 *ret：更新是否成�? *ps：judgetype（评测类型＋runid）定义于log.c
 */
inline int do_err_sys(char runid[], int judgeflag)
{
	int db_ret;
	db_ret=judge_update(runid,judgeflag,STATUS_OTHER,0,0);
	if(db_ret!=SQL_SUCCEED) return db_ret;
	else return SYS_SUCCEED;
}

/**
 *概要：删除文件夹（在评测进程返回SYSSUCCEED情况下）
 *传入参数：runid，评测类�? *传回参数：NULL
 *ret：NULL
*/
inline void delete_work_dir(char argv[], int judgeflag)
{
	char buf[BUFSIZE];
	chdir(judge_workdir);
	sprintf(buf, "rm -rf %s_judge_%s", db_table_name[judgeflag], argv);
	system(buf);
}

/**
 *概要：加载配置信�? *传入参数：NULL
 *传回参数：NULL
 *ret：NULL
 */
void loadconf()
{
	//打开配置文件
	FILE *fp;
	fp = fopen("/etc/oj_daemon.conf", "r");
	perror("load judge config");
	char confentry[128], confvalue[128];
	write2log("+--The paraments you has load for judge sever'configuartion as flows:");
	//读取配置文件，并将配置文件里面的值赋值给变量
	while (fscanf(fp, "%s", confentry) != EOF)
	{
		if(confentry[0]=='#'){
			char *p=fgets(confentry,sizeof(confentry),fp);
			if(p==NULL)
				puts("there is no new line after the lase empty commentory!");
			continue;
		}
		fscanf(fp, "%s", confvalue);
		if (strcmp(confentry, "HostName") == 0)
			strcpy(host_name, confvalue);
		else if (strcmp(confentry, "UserName") == 0)
			strcpy(user_name, confvalue);
		else if (strcmp(confentry, "Port") == 0)
			port_number = atoi(confvalue);
		else if (strcmp(confentry, "PassWord") == 0)
			strcpy(password, confvalue);
		else if (strcmp(confentry, "DatabaseName") == 0)
			strcpy(db_name, confvalue);
		else if (strcmp(confentry, "CompileTimeLimit") == 0)
			cmp_limit = atoi(confvalue);
		else if (strcmp(confentry, "ExerciseInputPath") == 0)
			strcpy(exercise_input_path, confvalue);
		else if (strcmp(confentry, "ExerciseOutputPath") == 0)
			strcpy(exercise_output_path, confvalue);
		else if (strcmp(confentry, "ContestInputPath") == 0)
			strcpy(contest_input_path, confvalue);
		else if (strcmp(confentry, "DiyContestOutputPath") == 0)
			strcpy(diy_contest_output_path, confvalue);
		else if (strcmp(confentry, "DiyContestInputPath") == 0)
			strcpy(diy_contest_input_path, confvalue);
		else if (strcmp(confentry, "ContestOutputPath") == 0)
			strcpy(contest_output_path, confvalue);
		else if (strcmp(confentry, "SpecialJudgeExePath") == 0)
			strcpy(spj_exec_path, confvalue);
		else if (strcmp(confentry, "DebugFlag") == 0)
			DebugFlag = atoi(confvalue);
		else if (strcmp(confentry, "WorkDir") == 0)
			strcpy(judge_workdir, confvalue);
		else if (strcmp(confentry, "MAX_JUDGER_NUM") == 0)
			MAX_JUDGER_SIZE = atoi(confvalue);
		else if (strcmp(confentry, "max_judge_time")==0)
			max_judge_time = atoi(confvalue);
		else if (strcmp(confentry, "max_memory_limit")==0)
			max_memory_limit = atoi(confvalue); 	
		else if (strcmp(confentry, "jvm_mem_usage")==0)
			jvm_mem_usage = atoi(confvalue);
		sprintf(logbuf,"|\t%s = %s ", confentry, confvalue);
		write2log(logbuf); 	
	}
	fclose(fp);
        write2log("+----------------------------------------------");
	strcpy(stdout_file_exec, "exec_out");
	strcpy(stderr_file_exec, "exec_err");
	strcpy(stdout_file_compile, "compile_out");
	strcpy(stderr_file_compile, "compile_err");
}

/**
 *概要：调度进程读取数据库，取得未评测的记�? *传入参数：job
 *传回参数：job
 *ret：查询是否成�?*/
int get_job(job *pjob)
{
	int i;
	MYSQL_ROW row;
	MYSQL_RES *res=NULL;
	static int tb=0;
	char sql_frmt[]="SELECT runid FROM oj_%s_allstatus WHERE fetched=0 ORDER BY runid ASC limit 1";
	char sql_cmd[255];
	pjob->empty=1;
	for(i=0;i<MAX_JUDGE_TYPE;i++)
	{
		sprintf(sql_cmd,sql_frmt,db_table_name[tb]);
		if(executesql(sql_cmd)){
			err_sql(sql_cmd);
			return SQL_FAILED;
		}
		res = mysql_store_result(g_conn);
		if(res==NULL){
			write2log("get_job():mysql_store_result error");
			return SQL_FAILED;
		}
		/*get a new job*/
		if (mysql_num_rows(res) == 1){
			row = mysql_fetch_row(res);
			if(res!=NULL){
				mysql_free_result(res);
				res=NULL;
			}
			if(row==NULL)
			{
				write2log("get_job():row==NULL");
				return SQL_FAILED;
			}
			pjob->empty=0;
			pjob->judge_type=tb;
			tb=(tb+1)%MAX_JUDGE_TYPE;
			break;
		}
		/*hasn't got a new job,check other table*/
		tb=(tb+1)%MAX_JUDGE_TYPE;
		if(res!=NULL){
			mysql_free_result(res);
			res=NULL;
		}
	}
	if(!(pjob->empty)){
		strcpy(pjob->runid,row[0]);
		sprintf(sql_cmd,"UPDATE oj_%s_allstatus SET FETCHED=1 WHERE RUNID=%s",db_table_name[pjob->judge_type],row[0]);
		if(executesql(sql_cmd)){
			printf("%s/n",mysql_error(g_conn));
			return SQL_FAILED;
		}
		//printf("get_job : type[%d] runid[%s] \n",pjob->judge_type,pjob->runid);
	}
	return SQL_SUCCEED;
}


/*
*[sig_int]
*/
static void sig_int(int signo)
{
	write2log("son get SIGINT,SET quit =1 ,it will quit after finish the runing job");
	quit=1;
}

static void sig_term(int signo)
{
	write2log("son get SIGTERM,SET quit =1 ,it will quit after finish the runing job");
	quit=1;
}

/**
 *概要：评测子进程入口
 *传入参数：评测进程在进程池中的id，与服进程通信的描述符
 *传回参数：NULL
 *ret：评测进程如果在评测种遇到FAILED，exit结束进程
 */
int judge_proc_hander(int judge_id,int sfd)
{
	job newjob;
	int judge_ret,size=sizeof(job);
	quit=0;
	
	if(signal(SIGINT,sig_int)==SIG_ERR)
	{
		write2log("judge_proc_hander signal(SIGINT,sig_int)==SIG_ERR");
		exit(0);
	}
	
	if(signal(SIGTERM,sig_term)==SIG_ERR)
	{
		write2log("judge_proc_hander signal(SIGTERM,sig_term)==SIG_ERR");
		exit(0);
	}
	
	if(SQL_FAILED==init_mysql())
	{
		sprintf(logbuf,"MYSQL ERROR: %s\n",mysql_error(g_conn));
		write2log(logbuf);
		exit(0);
	}
	/*评测进程向服进程发送信息，表示进程初始化顺利，可以接受评测任务�?to：make_judge_proc*/
	newjob.sys_stat=SYS_SUCCEED;
	if( size != write(sfd,&newjob,size)){
		err_system("son send free message");
		exit(0);
	}
	
	while(!quit)
	{
		if( size != read(sfd,&newjob,size)){
			sprintf(logbuf,"son[%d],get newjob error(call read()):%s",judge_id,strerror(errno));
			write2log(logbuf);
			exit(0);
		}
		//retry when sql_failed 
		int sql_retry=2;
		do{
			judge_ret=judge_main(newjob.runid,newjob.judge_type);
			if(judge_ret == SYS_SUCCEED){
				newjob.sys_stat=SYS_SUCCEED;
				delete_work_dir(newjob.runid,newjob.judge_type);
				break;
			}
			else if (judge_ret == SYS_FAILED)
			{
				sprintf(logbuf,"son[%d] judge_type[%d] runid[%s] system error:%s"\
					,judge_id,newjob.judge_type,newjob.runid,strerror(errno));
				write2log(logbuf);
				do_err_sys(newjob.runid,newjob.judge_type);
				exit(1);
			}
			else if (judge_ret == SQL_FAILED)
			{
				usleep(100);
				sprintf(logbuf,"son[%d] judge_type[%d] runid[%s] mysql  error:%s"\
						,judge_id,newjob.judge_type,newjob.runid,mysql_error(g_conn));
				write2log(logbuf);
				//阻塞直到连上为止
				write2log("son reconnect to database...");
				mysql_reconnect();
				write2log("son reconnect to database success,continue to run...");
			}
		}while(--sql_retry);
		
		if( size != write(sfd,&newjob,size)){
			err_system("send free message(call write())");
			exit(0);
		}
	}
	return 1;
}


/**
 *概要：创建评测进�? *传入参数：进程池中的id
 *传回参数：NULL
 *ret：执行情�? */
int make_judge_proc(int judge_id)
{
		int sfd[2];
		pid_t pid;
		if(socketpair(AF_UNIX,SOCK_STREAM,0,sfd)==-1) {
			sprintf(buf,"father creat son[%d] error when call socketpair err:%s",judge_id,strerror(errno));
			write2log(logbuf);
			return SYS_FAILED;
		}
		pid=fork();
		switch(pid)
		{
		case -1:
			sprintf(buf,"fahter creat son[%d] error when call fork err:%s",judge_id,strerror(errno));
			write2log(logbuf);
			return SYS_FAILED;
		case 0:
			close(sfd[0]);
			judge_proc_hander(judge_id,sfd[1]);
			exit(0);
	 	default://father note son proc message
		 	close(sfd[1]);
			judge_proc_pool[judge_id].judge_pid=pid;
			judge_proc_pool[judge_id].judge_stat=FREE;
			judge_proc_pool[judge_id].judge_sfd=sfd[0];
		}
		++num_free_judge;
		job ret_job;
		/*确保评测进程的数据库已经连接，from judge_proc_hander*/
		read(judge_proc_pool[judge_id].judge_sfd,&ret_job,sizeof(job));
		return SYS_SUCCEED;
}

/**
 *概要：重建一个评测进�? *传入参数：进程池中的id
 *传回参数：NULL
 *ret：执行情�? */
void remake_judge_proc(int i)
{
	judge_proc_pool[i].judge_stat=BUSY;
	close(judge_proc_pool[i].judge_sfd);
	if(SYS_FAILED==make_judge_proc(i)){
		sprintf(logbuf,"remake son[%d] failed err:%s",i,strerror(errno));
		write2log(logbuf);
		shut_down();
	}
	else{
		sprintf(logbuf,"remake son[%d] success",i);
		write2log(logbuf);
	}
}

/**
 *概要：关闭内�? *传入参数：NULL
 *传回参数：NULL
 *ret：NULL
＊ps：现关闭评测进程，然后退�? */
void shut_down()
{
	int i;
	quit=1;
	for(i=0;i<MAX_JUDGER_SIZE;i++)
		if(judge_proc_pool[i].judge_pid!=-1)
			kill(judge_proc_pool[i].judge_pid,SIGINT);
	write2log("father has called shut_down() shut down Kernal");
	exit(0);
}

/**
 *概要：服进程收到SIGCHLD的处理方�? *传入参数：NULL
 *传回参数：NULL
 *ret：NULL
＊ps：评测进程如果挂掉了，服进程会接受此信号，进入此函数处理
 */
void chld_hander()
{
	pid_t pid;
	int stat,i,live=1,signo=-1;
	pid=wait(&stat);
	if(WIFEXITED(stat))
	{
		live=-1;
	}
	if(WIFSTOPPED(stat))
	{
		signo=WSTOPSIG(stat);
		switch(signo)
		{
			case SIGCHLD:
				live=1;
				break;
			case SIGTRAP:
			case SIGFPE:
			case SIGSEGV:
			case SIGXFSZ:
			default:
				live=-2;
				break;
			}
		}
	if(WIFSIGNALED(stat))
	{
		signo=WTERMSIG(stat);
		live=-3;
	}
	/**孩子进程退出并且父进程未发出退出信�?*/
	if(live!=1 && !quit)
	{
		for(i=0;i<MAX_JUDGER_SIZE;i++)
			if(judge_proc_pool[i].judge_pid==pid) break;
		if(live == -1)
			sprintf(logbuf,"son[%d] runid[%d] type[%d] call eixt() success",i,judge_proc_pool[i].job_runid,judge_proc_pool[i].job_type);
		else if(live ==-2)
			sprintf(logbuf,"warming : son[%d] runid[%d] type[%d] stoped by signo(%d)",i,judge_proc_pool[i].job_runid,judge_proc_pool[i].job_type,signo);
		else sprintf(logbuf,"son[%d] runid[%d] type[%d] termed by signo(%d)",i,judge_proc_pool[i].job_runid,judge_proc_pool[i].job_type,signo);
		write2log(logbuf);
		if(i<MAX_JUDGER_SIZE){
			remake_judge_proc(i);
		}
		else{
			write2log("warming : pid not found in remake judger process");
		}
	}
}


int init_daemon(void){
	pid_t pid;
	if((pid=fork())<0)
	{
		perror("fork error\n");
		exit(1);
	}
	else if (pid !=0)
		exit(0);
	setsid();

	if((pid=fork())<0)
	{
		perror("fork error\n");
		exit(1);
	}
	else if (pid != 0)
		exit(1);
	int i;
	for(i=0;i<1024;i++)
		close(i);
	chdir("/");
	umask(0);
	return 0;
}

int main()
{
		int i,tot_job=0;
		struct timeval tv;
		job newjob,job_ret;
		fd_set rfdset;
		quit=0;
		/*load config file*/
		//loadconf();
		
		/*tobe daemon*/
		//daemonize();
		init_daemon();
		write2log("daemonize success.");
		loadconf();
		/*init mysql and connect*/
		if(SQL_FAILED==init_mysql())
		{
			err_sql("father init connect ERROR");
			mysql_reconnect();			
		}
		write2log("mysql connect success.");

		/*init proc pool*/
		for(i=0;i<MAX_JUDGER_SIZE;i++){
			judge_proc_pool[i].judge_pid=-1;
			judge_proc_pool[i].judge_stat=BUSY;
		}
		for(i=0;i<MAX_JUDGER_SIZE;i++){
			if(SYS_FAILED==make_judge_proc(i))
			shut_down();
		}
		write2log("init judge process pool success.");
		
		/*catch the signal of sigchld*/
		if(SIG_ERR == signal(SIGCHLD,chld_hander))
		{
			err_system("job manager : call SIG_ERR == signal(SIGCHLD,chld_hander");
			shut_down();
		}
		write2log("yahoo! kernel start success.");
		tv.tv_sec=1;
		tv.tv_usec=5000;
		while(1)
		{
				//printf("\nno.%d \n",++t);
				FD_ZERO(&rfdset);
				for(i=0;i<MAX_JUDGER_SIZE;i++)
						FD_SET(judge_proc_pool[i].judge_sfd,&rfdset);
				int ns = select(FD_SETSIZE,&rfdset,NULL,NULL,&tv);
				if(ns > 0)
				{
						for (i = 0; i < MAX_JUDGER_SIZE; i++)
						{
								if (FD_ISSET(judge_proc_pool[i].judge_sfd, &rfdset))
								{
									//write2log("get free son msg");
									if(sizeof(job) != read(judge_proc_pool[i].judge_sfd,&job_ret,sizeof(job)))
									{
										sprintf(logbuf,"read(judge_proc_pool[i].judge_sfd,&job_ret,sizeof(job))):%s"\
											,strerror(errno));
										write2log(logbuf);
										shut_down();
									}
									judge_proc_pool[i].judge_stat=FREE;
									num_free_judge++;
								}
						}
				}
				while (num_free_judge>0)
				{
					if(SQL_FAILED==get_job(&newjob))
					{
						//printf(logbuf,"newjob: %d empty:%d \n",newjob.judge_type,newjob.empty);
						write2log("sqlfailed when get a new job! try to reconnect mysql...");
						mysql_reconnect();
						write2log("reconnect mysql success,continue to run...");
						break;
					}

					if(!(newjob.empty))
					{
						//printf("newjob: type[%d] runid[%s] \n",newjob.judge_type,newjob.runid);
						tot_job++;
						for(i=0;i<MAX_JUDGER_SIZE;i++)
						{
							if(judge_proc_pool[i].judge_stat==FREE)
							{
								judge_proc_pool[i].judge_stat=BUSY;
								--num_free_judge;
								if(sizeof(job) != write(judge_proc_pool[i].judge_sfd,&newjob,sizeof(job)))
								{
									sprintf(logbuf,"father send job to son[%d] error:%s",i,strerror(errno));
									write2log(logbuf);
												/*
												?? see other son?
												*/
									shut_down();
								}
								judge_proc_pool[i].job_runid=atoi(newjob.runid);
								judge_proc_pool[i].job_type=newjob.judge_type;
								newjob.empty=1;
								break;
							}
						}
						if(i == MAX_JUDGER_SIZE){
							sprintf(logbuf,"can't not find son[%d],not exist!\n",num_free_judge);
							write2log(logbuf);
							shut_down();
						}
					}
					else{
						usleep(100000);
					}
					usleep(1000);
				}
				if(tot_job > max_judge_time){
					write2log("judged tot_job > max_judge_time,call reboot,all normal!");
					//shut_down();
					quit=1;
					for(i=0;i<MAX_JUDGER_SIZE;i++)
					kill(judge_proc_pool[i].judge_pid,SIGINT);
					sleep(6);
					//reboot
					sync();
					perror("sync");
					puts("reboot");
					reboot(RB_AUTOBOOT);
					perror("reboot");
					puts("system");
					system("reboot");
					perror("system");
				}
				sleep(1);
		}
		return 0;
}

