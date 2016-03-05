/*
	注意：经过测试，在调用execlp之后如果载入的程序运行没有出现异常会调用eixt(0)
	如果出现异常则会exit(1)
	由此我们在子进程遇到系统出错主动调用exit(SYS_FAILED)来获取更多的提示信息
	ps: SYS_SUCCEED 2
	    SYS_FAILED  3
	具体见judge.h
*/
#include "judge.h"
#include "security.h"
#include "log.h"
#include "database.h"
#include "oj_daemon.h"

int DebugFlag = 0;
int spjflag = SPJ_FALSE;

/*MYSQL_RES *res;
MYSQL_ROW row;*/

//declare


inline int max(int a, int b)
{
	return (a > b ? a : b);
}

/**
 *概要：统一系统错误日志
 *传入参数：出错提示信息
 *传回参数：NULL
 *ret：NULL
 *ps：judgetype（评测类型＋runid）定义于log.c
 */
inline void err_system(char *message)
{
	//char logbuf[BUFSIZE];
	sprintf(logbuf, " [%s] %s,error: %s\n", judgetype, message,strerror(errno));
	write2log(logbuf);
}

/**
 *概要：获取用户进程在前一次暂停到本次暂停的耗时
 *传入参数：累计时间，用户程序运行的资源变量
 *传回参数：累计时间
 *ret：NULL
 */
void get_time_usage(int *p_time_used,struct rusage *pusage)
{
		*p_time_used += (pusage->ru_utime.tv_sec*1000 + pusage->ru_utime.tv_usec/1000);
		*p_time_used += (pusage->ru_stime.tv_sec*1000 + pusage->ru_stime.tv_usec/1000);
}

/**
 *概要：获取用户进程当前的虚拟内存大小
 *传入参数：用户进程的pid
 *传回参数：NULL
 *ret：用户进程当前的虚拟内存大小
 */
long get_mem_usage(int pid)
{
		char path[255];
		FILE *fp=NULL;
		long vsize=0,resident=0,shared=0,trs=0,lrs=0,drs=0,dt=0;
		sprintf(path,"/proc/%d/statm",pid);
		//puts(path);
		fp=fopen(path,"r");
		if(fp==NULL){
			sprintf(logbuf,"open /proc/%d/statm error:%s",pid,strerror(errno));
			write2log(logbuf);
			return -1;
		}
		//statm 文件，仅读取任务虚拟地址空间大小作为开辟空间大小
		fscanf(fp,"%ld%ld%ld%ld%ld%ld%ld",&vsize,&resident,&shared,&trs,&lrs,&drs,&dt);
		fclose(fp);
		return drs;
}

void doit(int signo)
{
	switch(signo){
		case 5:puts("SIGTRAP");break;
		case 6:puts("SIGABRT");break;
		case 8:puts("SIGFEP");break;//div 0
		case 9:puts("SIGKILL");break;//been killed
		case 11:puts("SIGSEGV");break;//内存越界
		case 14:puts("SIGALRM");break;
		case 15:puts("SIGTERM");break;
		case 17:puts("SIGCHLD");break;
		case 24:puts("SIGXCPU");break;
		case 25:puts("SIGXFSZ");break;
		default:printf("unknow %d ",signo);break;
	}
}


void showrst(int rst)
{
	 printf("result:");
		switch(rst)
		{
		case STATUS_AC:printf("AC\t");break;
		case STATUS_WA:printf("WA\t");break;
		case STATUS_PE:printf("PE\t");break;
		case STATUS_RE:printf("RE\t");/*getchar();*/break;
		case STATUS_TLE:printf("TLE\t");/*getchar();*/break;
		case STATUS_MLE:printf("MLE\t");/*getchar();*/break;
		case STATUS_OLE:printf("OLE\t");/*getchar();*/break;
		default:
		printf("notice! rst=%d\a",rst);
		}
}

/**
judgetype is a global argcment define in log.c
*概要：依据评测类型和评测runid初始化评测评测进程的参数
*传入参数：runid，评测类型（exetcist，contest，diy_contest）
*传回参数：NULL
*ret：系统执行情况
*/
int judge_init(char runid[],int judgeflag)
{
	int db_ret;
	switch (judgeflag)
	{
		case 0://exercise
			sprintf(judgetype, "exercise : %s",runid);
			db_ret=init_exercise_judge(runid); // rejudge
			break;
			case 1://vip contest
				sprintf(judgetype, "contest : %s", runid);
				db_ret=init_contest_judge(runid);
				break;
		case 2:
			sprintf(judgetype, "diy_contest : %s",runid);
			db_ret=init_diy_contest_judge(runid);
			break;
		default:
			write2log("init judge :Unkonw judge type !");
			return SYS_FAILED;
	}
	return db_ret;
}

/**
*概要：更新评测结果
*传入参数：runid，评测类型（exetcist，contest，diy_contest）
*传回参数：NULL
*ret：此模块的运行情况
*/
int judge_update(char runid[],int judgeflag,int result,int time_usage,int mem_usage)
{
	int db_ret;
	switch (judgeflag)
	{
		case 0:
			db_ret=exercise_updatedb(result, time_usage,mem_usage, runid);
			break;
		case 1:
			db_ret=contest_updatedb(result, time_usage,mem_usage, runid);
			break;
		case 2:
			db_ret=diy_contest_updatedb(result, time_usage,	mem_usage, runid);
			break;
		default:
			sprintf(logbuf, "%s: UNKNOW judge flag : %d ", judgetype,judgeflag);
			write2log(logbuf);
			break;
	}
	return db_ret;
}


/**
 *概要：评测进程获取用户代码编译的信息
 *传入参数：编译结果(是否re)，编译程序pid
 *传回参数：编译结果(是否re)
 *ret：此模块的运行情况
 */
int get_compile_message(int *compile_err,int pid_compile)
{
	int status = 0;
	*compile_err=0;
	pid_t w = waitpid(pid_compile, &status, WUNTRACED);
	if (w == -1)
	{
		err_system("father process waitpid for compile process");
		return SYS_FAILED;
	}
		//printf("%d \n", status);
	if (WIFEXITED(status))
	{
		if (EXIT_SUCCESS == WEXITSTATUS(status))//EXIT_SUCCESS==0
		{
			*compile_err=0;//compile success
		}
		else if(SYS_FAILED == WEXITSTATUS(status))//exit for system error
		{
			write2log("compile process exit(sys_failed)");
			return SYS_FAILED;
		}
		else {//default: compile error
			//puts("CE");
			*compile_err=1;
		}
	}
	else//system failed
	{
		if (WIFSIGNALED(status))
		{
			if (SIGALRM ==WTERMSIG(status))
			{
				sprintf(logbuf, "%s:compile time out", judgetype);
				write2log(logbuf);
			}
			else
			{
				sprintf(logbuf, "%s:compile unknow signal(%d)", judgetype, WSTOPSIG(status));
				write2log(logbuf);
			}
		}
		else if (WIFSTOPPED(status))
		{
			sprintf(logbuf, "%s:compile stopped by signal(%d)", judgetype,WSTOPSIG(status));
			write2log(logbuf);
		}
		else
		{
			sprintf(logbuf, "%s: compile unknow exit reason", judgetype);
			write2log(logbuf);
		}
		return SYS_FAILED;
	}
	return SYS_SUCCEED;
}

/**
 *概要：评测进程监视用户程序的运行状况
 *传入参数：用户程序进程id,时间，内存，评测结果指针
 *传回参数：用户程序时间，内存，评测结果
 *ret：系统出错信息存于result中
 */
void get_run_message(int runcld,int *time,long *memory,int *result)
{
	enum {UN=0,AC,WA,RE,PE,TLE,MLE,OLE,CE,OTHER,RS,RK,};
	int stat,rst=0;
	//int t=0;
	int time_usage=0;
	long mem_usage=0,now_mem=0;
	struct rusage rused;
	struct user_regs_struct regs;
	int signo=-1;
	int page_size = getpagesize()/KB; //kB
	int MEM_LMT; //kB
	int TIM_LMT; //ms
	//int jvm_mem_usage =385500;  //kB change define in oj_daemon.c and fetched by loadconfig

	if(lang == LANG_JAVA)
	{
		MEM_LMT = java_memory_limit + jvm_mem_usage;
		TIM_LMT = java_time_limit;
	}
	else{
		MEM_LMT = memory_limit;
		TIM_LMT = time_limit;
	}

	init_syscall_enable(lang);
	while(1)
	{
		if(-1==wait4(runcld,&stat,WUNTRACED,&rused))
		{
			err_system("@get_run_message:wait4");
			rst=OTHER;
			break;
		}
		//增加此次系统调用使用的时间
		get_time_usage(&time_usage,&rused);
		if(WIFEXITED(stat))
		{
			switch(WEXITSTATUS(stat))
			{
				case EXIT_SUCCESS:
				//	puts("exit(0)");
					rst=RS;
					break;
					/*这是我在调用run_user_proc()中设置的返回系统失败的标志*/
				default:
				//	puts("exit without 0");
					write2log("warming : %s  proc exit without 0 ",judgetype);
					rst=RE;
					break;
			}
			break;
		}
		/*由于某种原因而孩子进程停止运行，原因如下：
		1.re 遇到这些情况时孩子进程已经被停止运行
		2. 由于跟踪选项PTRACE_SYSCALL，每次系统调用都会返回SIGTRAP信号，表示占停
		*/
		if(WIFSTOPPED(stat))
		{
			signo=WSTOPSIG(stat);
			
			//if(signo!=SIGTRAP){
			//printf("STOPPED signo %d ",signo);
			//doit(signo);
		        //}

			switch(signo)
			{
				case SIGTRAP://情况2：继续发出跟踪chld的系统调用
					if (ptrace(PTRACE_GETREGS, runcld, NULL, &regs) < 0){
						err_system("ptrace getregs");
						rst=OTHER;
						ptrace(PTRACE_KILL,runcld,NULL,NULL);
						break;
					}
					int syscall_id=regs.orig_eax;
					if (syscall_id > 0 && is_enable_syscall(syscall_id) == 0){
						rst=RE;
						sprintf(buf,"warming: [%s] user syscall_id = %d( SYS_%s ) which is refused\n",judgetype,syscall_id,syscall_table[syscall_id].name);
						write2log(buf);
						ptrace(PTRACE_KILL, runcld, NULL, NULL);
						break;
					}
					else//调用安全
						ptrace(PTRACE_SYSCALL,runcld,NULL,NULL);
					break;
				case SIGFPE://运算异常
				case SIGSEGV://无效内存引用：越界,栈溢出
					rst=RE;
					ptrace(PTRACE_KILL,runcld,NULL,NULL);
					break;
				case SIGXFSZ://OLE
					rst=OLE;
					ptrace(PTRACE_KILL,runcld,NULL,NULL);
					break;
				case SIGCHLD://？？？这个有点玄乎
					rst=RS;
					ptrace(PTRACE_KILL,runcld,NULL,NULL);
					break;
				default:
					write2log("warming : user proc %s stopped by signo %d",judgetype,signo);
					rst=RE;
					ptrace(PTRACE_KILL,runcld,NULL,NULL);
					break;
			}
		}
		/*1.接受了：SIGKILL
		2.tle,mle
		*/
		if(WIFSIGNALED(stat))
		{
			signo=WTERMSIG(stat);
			/*
			如果rst值为UN，那么说明此kill信号非ptrace发出的
			那么将rst置为RK，表示被cpu给KILL的，原因有二：
			TLE,MLE
			*/

			//puts("signaled");
			//doit(signo);
			switch(signo)
			{
				case SIGKILL:
					if(rst==UN) rst=RK;
					break;
				default:
					write2log("warming : user proc %s killed by signo %d",judgetype,signo);
					if(rst==UN) rst=RK;
					break;
			}
			break;
		}
		//放这比较好
		now_mem=get_mem_usage(runcld)*page_size;
		if(mem_usage<now_mem) mem_usage=now_mem;
		if(mem_usage > MEM_LMT){
			//puts("memory over !");
			rst=MLE;
			ptrace(PTRACE_KILL,runcld,NULL,NULL);
		}
		else if(time_usage > TIM_LMT){
			rst=TLE;
			ptrace(PTRACE_KILL,runcld,NULL,NULL);
		}
	}
	/*由于系统不能即时对内存或者时间超出作出kill操作，在跟踪结束后重新判断tle和mle*/
	//重新检查是否超内存

	//printf("real memory = %d, time = %d\n",mem_usage,time_usage);

	if(mem_usage > MEM_LMT){
		rst=MLE;
		mem_usage=MEM_LMT;
	}
	//重新检查是否超时,同时重置time_usage
	if(time_usage > TIM_LMT){
		rst=TLE;
		time_usage = TIM_LMT+1;
	}

	if(rst==RS){
		rst = oj_compare_output(comparedatafile,stdout_file_exec);
	}
	else if(rst==RK)
	{
		sprintf(logbuf,"warming : rst=rk,time=%dms,memory=%dkB",time_usage,mem_usage);
		if(time_usage > TIM_LMT){
			time_usage = TIM_LMT + 1;
			rst=TLE;
		}
		else if(rst==RK){
			mem_usage=MEM_LMT;
			rst=MLE;
		}
		else rst=OTHER;
	}
	*time=time_usage;
        //adject to java vm memory usage...
	if(lang == LANG_JAVA){
		if(rst==MLE) mem_usage = java_memory_limit;
		else if(mem_usage > jvm_mem_usage ) mem_usage -= jvm_mem_usage;
		else mem_usage %= java_memory_limit;
        }
	*memory=mem_usage;
	*result=rst;
}

/**
 *概要：编译进程编译用户程序
 *传入参数：NULL
 *传回参数：NULL
 *ret：如果系统出错，exit(SYS_FAILED)发出提示信息给评测进程
 */
void compile_user_source()
{
	stdout = freopen(stdout_file_compile, "w", stdout);
	stderr = freopen(stderr_file_compile, "w", stderr);
	if (stdout == NULL || stderr == NULL)
	{
		err_system("@compile process ,can't creat file for stdout and stderr");
		exit(SYS_FAILED);
	}
	switch (lang)
	{
		case LANG_GCC:
			execlp("gcc", "gcc", "--static","-std=c99", "-ansi","-Wall", "-O1", sourcefile, (char*) NULL);
			break;
		case LANG_GPP:
			execl("/usr/bin/g++", "g++", "--static","-Wall", "-O1", sourcefile, (char*) NULL);
			break;
		case LANG_JAVA:
			execl("/usr/java/jdk1.7.0_03/bin/javac", "javac", "-J-Xmx256m", sourcefile, (char*) NULL);
	}
	err_system("compile dev the function execlp");
	exit(SYS_FAILED);
}
/**
 *概要：运行用户程序
 *传入参数：NULL
 *传回参数：NULL
 *ret：如果系统出错，exit(SYS_FAILED)发出提示信息给评测进程，见get_run_message()
 */
void run_user_proc()
{
	stdin = freopen(testdatafile, "r", stdin);
	if(stdin==NULL)
	{
		err_system("can't open input data(%s)");
		exit(SYS_FAILED);
	}
	stdout = freopen(stdout_file_exec, "w", stdout);
	stderr = freopen(stderr_file_exec, "w", stderr);
	if (stdin == NULL || stdout == NULL || stderr == NULL)
	{
		err_system("process open stderr or stdout file failed when run a.out");
		exit(SYS_FAILED);
	}
	//资源限制
	struct rlimit rlmt;
	rlmt.rlim_cur=rlmt.rlim_max=0;
	/*阻止创建core文件*/
	if(setrlimit(RLIMIT_CORE,&rlmt)<0){
		err_system("setrlimit rlimit_core");
	}
	/*阻止创建子进程*/
	if(setrlimit(RLIMIT_NPROC,&rlmt)<0){
		err_system("setrlimit rlimit_nproc");
	}
	
	rlmt.rlim_cur=rlmt.rlim_max=time_limit/1000+1;
	if(setrlimit(RLIMIT_CPU,&rlmt)<0){
		err_system("setrlimit rlimit_cpu");
	}
	//set to max available memory
	rlmt.rlim_cur =rlmt.rlim_max = max_memory_limit*MB;
	if (setrlimit(RLIMIT_AS, &rlmt) < 0){
		err_system("setrlimit rlimit_as");
	}

	/*递归栈空间的限制 5MB,system default is 10MB*/
/*
	rlmt.rlim_cur =rlmt.rlim_max = stack_size_limit * KB;
	if (setrlimit(RLIMIT_STACK, &rlmt) < 0){
		err_system("setrlimit rlimit_stack");
	}
*/
	rlmt.rlim_cur =rlmt.rlim_max = output_limit * KB;
	if (setrlimit(RLIMIT_FSIZE, &rlmt) < 0){
		err_system("setrlimit rlimit_fsize");
	}
	//设置进程可以跟踪
	if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0)
	{
		err_system("son process failed when call ptrace(PTRACE_TRACEME...)");
		exit(SYS_FAILED);
	}
	//载入程序
	switch (lang)
	{
		case LANG_GCC:
		case LANG_GPP:
			execl("./a.out", "a.out", (char*) NULL);
			break;
		case LANG_JAVA:
			execl("/usr/java/jdk1.7.0_03/bin/java", "java", "-Xmx128m", "-Xmx256m", "Main", (char *) NULL);
			break;
	}
	err_system("execl a.out failed ");
	exit(SYS_FAILED);
}

/**
 *概要：评测进程主函数，这是主要的调度流程
 *传入参数：runid(argv),评测类型
 *传回参数：NULL
 *ret：如果系统出错，exit(SYS_FAILED)发出提示信息给评测进程，见get_run_message()
 */
int judge_main(char argv[], int judgeflag)
{
	int db_ret;
	db_ret=judge_init(argv,judgeflag);
	if(db_ret!=SQL_SUCCEED) return db_ret;

	pid_t pid_compile;
	if ((pid_compile = fork()) < 0)
	{
		err_system("create compile process fail");
		return SYS_FAILED;
	}
	if (pid_compile == 0) //compile process
	{
		compile_user_source();
	}
	////facher process	
	else
	{ 
		int compile_err=0;
		if(SYS_FAILED==get_compile_message(&compile_err,pid_compile))
			return SYS_FAILED;
		if(compile_err){
			db_ret=judge_update(argv,judgeflag,STATUS_CE,0,0);
			if(db_ret!=SQL_SUCCEED) return db_ret;
			else return SYS_SUCCEED;
		}
		
		pid_t pid_exec;
		if ((pid_exec = fork()) < 0)
		{
			err_system("make a new process for run a.out failed");
			return SYS_FAILED;
		}
		//son to run process
		if (pid_exec == 0)
		{
			run_user_proc();
		}
		//father to get run message
		else
		{
			int time_usage=0;
			long mem_usage=0;
			int result;
			/*跟踪监控获取运行信息*/
			get_run_message(pid_exec,&time_usage,&mem_usage,&result);
			//打印评测结果
			//printf("problemid: %d ",problemid);
			//showrst(result);
			//printf("runtime: %dms\t mem_used: %ldB\t\n",time_usage,mem_usage);
			db_ret=judge_update(argv,judgeflag,result,time_usage,mem_usage);
			if (db_ret!=SQL_SUCCEED)
				return db_ret;
		}
	}
	return SYS_SUCCEED;
}

/*
int set_alarm(int milliseconds)
{
	struct itimerval t;
	t.it_value.tv_sec = milliseconds / 1000;
	t.it_value.tv_usec = milliseconds % 1000 * 1000; //Î¢Ãë
	t.it_interval.tv_sec = 0;
	t.it_interval.tv_usec = 0;
	return setitimer(ITIMER_REAL, &t, NULL);
}
*/
//超时处理函数
/*
void timeout(int signo)
{
	if (signo == SIGALRM)
	{
		exit(0);
	}
}
*/
/*
*judge the output of the process
*/
int oj_compare_output(char *exefile, char *cmpfile)
{
	struct stat data_stat, output_stat;
	char output_name[256] =
	{ 0 };
	char data_name[256] =
	{ 0 };
	FILE *data_fin = NULL, *output_fin = NULL;
	char *data_buff = NULL, *output_buff = NULL;
	int data_sz = 0, output_sz = 0;

	snprintf(output_name, sizeof(output_name), "%s", cmpfile);
	snprintf(data_name, sizeof(data_name), "%s", exefile);

	data_fin = fopen(data_name, "r");
	output_fin = fopen(output_name, "r");

	if (data_fin == NULL)
	{
		//write_log( LOG_ERROR, "data file open failed\n" );
		if (data_fin)
			fclose(data_fin);
		if (output_fin)
			fclose(output_fin);
		return STATUS_OTHER;
	}

	if (output_fin == NULL)
	{
		if (data_fin)
			fclose(data_fin);
		if (output_fin)
			fclose(output_fin);
		return STATUS_WA;
	}

	if (stat(data_name, &data_stat) == -1)
	{
		if (data_fin)
			fclose(data_fin);
		if (output_fin)
			fclose(output_fin);
		return STATUS_OTHER;
	}

	if (stat(output_name, &output_stat) == -1)
	{
		if (data_fin)
			fclose(data_fin);
		if (output_fin)
			fclose(output_fin);
		return STATUS_OTHER;
	}

	data_sz = data_stat.st_size;
	output_sz = output_stat.st_size;

	//write_log( LOG_DEBUG, "data_sz = %d, output_sz = %d\n", data_sz, output_sz );
	if (output_sz > data_sz * 3)
	{
		fclose(data_fin);
		fclose(output_fin);
		return STATUS_OLE;
	}

	data_buff = (char*) calloc(data_sz + 256, 1);
	output_buff = (char*) calloc(output_sz + 256, 1);

	int retval = fread(data_buff, 1, data_sz, data_fin);
	if (data_sz != retval)
	{
		//write_log( LOG_WARNING, "file size mismatch: %s, fread returned %d but it should be %d\n", data_name, retval, data_sz );
	}

	retval = fread(output_buff, 1, output_sz, output_fin);
	if (output_sz != retval)
	{
		//write_log( LOG_WARNING, "output size mismatch: run.out, fread returned %d but it should be %d\n", retval, output_sz );
	}

	if (data_fin)
		fclose(data_fin);
	if (output_fin)
		fclose(output_fin);

	int p = 0, q = 0;

	/* judge AC */
	int ac = 1;
	while (p < data_sz && q < output_sz)
	{
		while (p < data_sz && data_buff[p] == '\r')
			p++;
		while (q < output_sz && output_buff[q] == '\r')
			q++;
		if (data_buff[p] != output_buff[q])
		{
			ac = 0;
			break;
		}
		p++, q++;
	}

	while (p < data_sz && data_buff[p] == '\r')
		p++;
	while (q < output_sz && output_buff[q] == '\r')
		q++;

	if (ac == 1 && p >= data_sz && q >= output_sz)
	{
		if (data_buff != NULL)
		{
			free(data_buff);
			data_buff = NULL;
		}

		if (output_buff != NULL)
		{
			free(output_buff);
			output_buff = NULL;
		}
		return STATUS_AC;
	}

	/* judge PE */
	p = q = 0;
	int pe = 1;
	while (p < data_sz && q < output_sz)
	{
		while (p < data_sz && isspace(data_buff[p]))
			p++;
		while (q < output_sz && isspace(output_buff[q]))
			q++;
		if (data_buff[p] != output_buff[q])
		{
			pe = 0;
			//write_log( LOG_DEBUG, "data = %c output = %c\n", data_buff[p], output_buff[q] );
			break;
		}
		p++, q++;
	}

	while (p < data_sz && isspace(data_buff[p]))
		p++;
	while (q < output_sz && isspace(output_buff[q]))
		q++;

	if (data_buff != NULL)
	{
		free(data_buff);
		data_buff = NULL;
	}

	if (output_buff != NULL)
	{
		free(output_buff);
		output_buff = NULL;
	}

	if (pe == 1 && p >= data_sz && q >= output_sz)
		return STATUS_PE;
	else
		return STATUS_WA;
}

//special judge结果比对函数
int oj_compare_spj_output(char *spj_dir, char *exec_output, int problemid,
		char *std_output, char *cmp_pre)
{
	char spj_exe[33];
	sprintf(spj_exe, "%s_spj_%d", cmp_pre, problemid);
	sprintf(buf, "%s/%s", spj_dir, spj_exe);
	pid_t spj_pid;
	if ((spj_pid = fork()) < 0)
	{
		sprintf(logbuf, "%s:create spj process error", judgetype);
		write2log(logbuf);
	}
	else if (spj_pid == 0)
	{
		stdout = freopen("spj_output", "w", stdout);
		if (stdout == NULL)
		{
			sprintf(logbuf, "%s:spj iorediect error", judgetype);
			if (write2log(logbuf) != 1)
				perror("write to log fail\n");
			exit(0);
		}
		execl(buf, spj_exe, std_output, exec_output, (char *) NULL);
		printf("%d", STATUS_OTHER);
		exit(0);
	}
	else
	{
		int spj_status;
		if (wait4(spj_pid, &spj_status, 0, NULL) < 0)
		{
			sprintf(logbuf, "%s:wait4 failed, %d:%s", judgetype, errno,
					strerror(errno));
			if (write2log(logbuf) != 1)
				perror("write to log fail\n");
		}
		if (WIFEXITED(spj_status))
		{
			if (WEXITSTATUS(spj_status) == EXIT_SUCCESS)
			{
				FILE *spj_fp = fopen("spj_output", "r");
				if (spj_fp == NULL)
				{
					sprintf(logbuf, "%s:open spj_output file fail\n", judgetype);
					if (write2log(logbuf) != 1)
						perror("write to log fail\n");
				}
				int res = STATUS_OTHER;
				if (fscanf(spj_fp, "%d", &res) == EOF)
				{
					sprintf(
							logbuf,
							"%s:function fscanf return null in funtion oj_compare_spj_output",
							judgetype);
					if (write2log(logbuf) != 1)
						perror("write to log fail\n");
					fclose(spj_fp);
					return res;
				}
				fclose(spj_fp);
				return res;
			}
			else
			{
				sprintf(logbuf, "%s:%s exit :%d", judgetype, spj_exe,
						WEXITSTATUS(spj_status));
				if (write2log(logbuf) != 1)
					perror("write to log fail\n");
				return STATUS_OTHER;
			}
		}
		else if (WIFSIGNALED(spj_status) && WTERMSIG(spj_status) == SIGALRM)
		{
			sprintf(logbuf, "%s:%s run time out", judgetype, spj_exe);
			if (write2log(logbuf) != 1)
				perror("write to log fail\n");
			return STATUS_OTHER;
		}
		else
		{
			sprintf(logbuf, "%s:%s runtime error", judgetype, spj_exe);
			if (write2log(logbuf) != 1)
				perror("write to log fail\n");
			return STATUS_OTHER;
		}
	}
	return STATUS_OTHER;
}



