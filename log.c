#include "log.h"
#include <time.h>

int log_open_flag=0;
char logfile[129]="/var/log/OnlineJudgeLog.log";
FILE *logfp=NULL;
char logbuf[1024],judgetype[128];


/*
打开日志
打开成功放回1，否则返回0
*/
int open_logfile(){
	//判断日志文件是否打开，如果打开则返回1
	if(log_open_flag==1)
	{
		//printf("log_open_flag opened\n");
		return 1;
	}
	//日志未打开则打开日志
	logfp=fopen(logfile,"a");
	time_t   timep;
   time   (&timep); 
	fprintf(logfp,"\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	fprintf(logfp,"|\tLog For Kernal When Started At %s",ctime(&timep));
	fprintf(logfp,"++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	//如果打开日志失败，输出错误提示，返回0
	if(logfp==NULL)
	{
		perror("open logfile fail\n");
		return 0;
	}
	//打开日志成功，将log_open_flag置为1，表示日志文件已经打开，返回1
	log_open_flag=1;
	return 1;
}

/*
关闭日志文件
关闭成功返回1，否则返回0
*/
int close_logfile(){
	//
	if(0!=fclose(logfp)){
		//如果关闭日志文件失败，输出错误提示，返回0
		perror("close logfile fail\n");
		return 0;
	}
	//关闭成功，将log_open_flag标志置为0，返回1
	log_open_flag=0;
	return 1;
}


/*
将信息写入日志
写入成功返回1，否则返回0
*/
int write2log(char *message,...){
	//判断日志文件是否打开，未打开则打开
	if(open_logfile()==0){
		//如果打开日志文件失败，则提示错误信息，返回0
		perror("open logfile fail before write info to logfile\n");
		return 0;
	}
	
	/* get time */
	char time_buff[64] = {0};
	time_t now = time(NULL);
	ctime_r( &now, time_buff);
	time_buff[strlen(time_buff)-1] = 0;

	/* print time stamp and log type */
	fprintf(logfp, "\n %s  ", time_buff );
	//将信息写入日志
	va_list arg_ptr;
	va_start(arg_ptr,message);
	vfprintf(logfp,message,arg_ptr);
	va_end(arg_ptr);
	//if(!fprintf(logfp,"%s\n",info)){
	//	//如果写入日志失败，则输出错误提示并返回0
	//	perror("write info to logfile fail\n");
	//	return 0;
	//}
	//写入日志成功，返回1
	fflush(logfp);
	return 1;
}
