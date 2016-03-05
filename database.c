#include "judge.h"
#include "log.h"
#include "oj_daemon.h"
#include "database.h"


char db_table_name[][25] =
{ "exercise", "contest", "diy" };

/**
 *概要：sql 出错写入日志
 *传入参数：提示信息
 *传回参数：NULL
 *ret：NULL
 */
inline void err_sql(char *sqlcmd)
{
	char logbuf[BUFSIZE];
	sprintf(logbuf, " [%s] %s,error(%d):%s\n", judgetype, sqlcmd,mysql_errno(g_conn),mysql_error(g_conn));
	write2log(logbuf);
}

/**
 *概要：重新连接数据库
 *传入参数：NULL
 *传回参数：NULL
 *ret：连接情况
 */
int mysql_reconnect()
{
	//重连时间为1小时
	while(1){
                sleep(1);
		mysql_close(g_conn);
		if(SQL_SUCCEED==init_mysql()){
			write2log("reconnect database success");
			return SQL_SUCCEED;
		}
		//sleep(2);
	}
	return SQL_FAILED;
}

/*
 *
 *
 *
 */
inline int executesql(char *sql)
{
	return mysql_real_query(g_conn, sql, strlen(sql));
}

/**
 *概要：sql连接函数
 *传入参数：NULL
 *传回参数：NULL
 *ret：运行状态
 */
int init_mysql()
{
	static char timeout = 30;
	g_conn = mysql_init(NULL);
	mysql_options(g_conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	if (!mysql_real_connect(g_conn, host_name, user_name, password, db_name,port_number, 0, 0))
	{
		err_sql("function:mysql_real_connect");
		return SQL_FAILED;
	}
	char tmpbuf[] = "set names utf8";
	if (executesql(tmpbuf))
	{
		err_sql(tmpbuf);
		return SQL_FAILED;
	}
	return SQL_SUCCEED;
}
/*
 * 
 * 
 * 
 * 
 */
int init_exercise_judge(char argv[])
{
	MYSQL_ROW row;
	MYSQL_RES *res=NULL;
	char buf[255],logbuf[255];
	chdir(judge_workdir);
	int retry=3;
	int sleep_sec[]={0,1,2,4,8,16,32},sleep_id=0;
//get problem info
	do{
		sleep(sleep_sec[sleep_id++]);
		sprintf(buf,"SELECT problemid,language,userid,runid,codelen,submittime FROM oj_exercise_allstatus WHERE RUNID=%s", argv);
		if (executesql(buf))
		{
			err_sql(buf);
			return SQL_FAILED;
		}
		res = mysql_store_result(g_conn);
		if(res==NULL){
			err_sql("res=null mysql_store_result() when select * from oj_exercise_allstatus");
			return SQL_FAILED;
		}
		row = mysql_fetch_row(res);
		if(res!=NULL){
			mysql_free_result(res);
			res=NULL;
		}
		if (row == NULL)
		{
			err_sql("warming:row=null mysql_fetch_row() when select * from oj_exercise_allstatus");
			continue;
		}
		problemid = atoi(row[0]);
		lang = atoi(row[1]);
		userid = atoi(row[2]);
		runid = atoi(row[3]);
		codelen = atoi(row[4]);
		submittime = atoi(row[5]);
//dir
	//puts("make dir");
		sprintf(buf, "exercise_judge_%s", argv);
		mkdir(buf, 7777);
		chmod(buf, S_IRWXU | S_IRGRP | S_IROTH);
		chdir(buf);
	//system("pwd");
		if (lang == LANG_GCC || lang == LANG_GPP)
			strcpy(sourcefile, "src.c");
		else if (lang == LANG_JAVA)
			strcpy(sourcefile, "Main.java");
//store code
		sprintf(buf, "SELECT code FROM oj_exercise_sources WHERE RUNID=%s", argv);
		if (executesql(buf))
		{
			err_sql(buf);
			return SQL_FAILED;
		}
		res = mysql_store_result(g_conn);
		if(res==NULL){
			err_sql("res=null mysql_store_result(); select code form oj_exercise_source");
			return SQL_FAILED;
		}
		row = mysql_fetch_row(res);
		if(res!=NULL){
			mysql_free_result(res);
			res=NULL;
		}
		if (row == NULL)
		{
			err_sql("warming:store res(code) to row failed");
			continue;
		}
		FILE *fp=NULL;
		fp = fopen(sourcefile, "wt");
		if (fp == NULL)
		{
			err_system("@init_judge,system create src file fail");
			return SYS_FAILED;
		}
		fprintf(fp,"%s", row[0]);
		fflush(fp);
		fclose(fp);

		sprintf(testdatafile, "%s/%d/%d.in", exercise_input_path, problemid,problemid);
		sprintf(comparedatafile, "%s/%d/%d.out", exercise_output_path, problemid,problemid);

		sprintf(buf,"SELECT timelimit,memorylimit,outputlimit,specialjudge,javatimelimit,javamemorylimit FROM oj_exercise_problems WHERE PROBLEMID=%d", problemid);
		if (executesql(buf))
		{
			err_sql(buf);
			return SQL_FAILED;
		}
		res = mysql_store_result(g_conn);
		if(res==NULL){
			err_sql("res=null mysql_store_result(g_conn);");
			return SQL_FAILED;
		}
		row = mysql_fetch_row(res);
		if(res!=NULL){
			mysql_free_result(res);
			res=NULL;
		}
		if (row == NULL)
		{
			err_sql("warming:store problem limit to row failed");
			continue;
		}
		time_limit = atoi(row[0]);
		memory_limit = atoi(row[1]);
		output_limit = atoi(row[2]);
		spjflag = atoi(row[3]);
		java_time_limit = atoi(row[4]);
		java_memory_limit = atoi(row[5]);
		if (lang == LANG_JAVA)
		{
			time_limit = java_time_limit;
			memory_limit = java_memory_limit;
		}
		if(res!=NULL){
			mysql_free_result(res);
			res=NULL;
		}
		break;
	}while(--retry);
	if(retry)
		return SQL_SUCCEED;
	else return SQL_FAILED;
}
/*
 *
 *
 *
 */
int init_contest_judge(char argv[])
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	char buf[255],logbuf[255];
	chdir(judge_workdir);
	int retry=3;
	int sleep_sec[]={0,1,2,4,8,16,32},sleep_id=0;
	do{
		sleep(sleep_sec[sleep_id++]);
		sprintf(buf,"SELECT problemid,language,userid,contestid,runid,language,codelen,submittime FROM oj_contest_allstatus WHERE RUNID=%s",argv); //追加submittime
		if (executesql(buf))
		{
			err_sql(buf);
			return SQL_FAILED;//exit(0);
		}
		res = mysql_store_result(g_conn);
		if(res==NULL){
			err_sql("res=null select * form oj_contest_allstatus");
			return SQL_FAILED;
		}
		row = mysql_fetch_row(res);
		mysql_free_result(res);
		res=NULL;
		if (row == NULL)
		{
			err_sql("warming:mysql_fetch_row return NULL select * from oj_constest_allstatus");
			continue;
		}
		problemid = atoi(row[0]);
		lang = atoi(row[1]);
		userid = atoi(row[2]);
		contestid = atoi(row[3]);
		runid = atoi(row[4]);
		lang = atoi(row[5]);
		codelen = atoi(row[6]);
		submittime = atoi(row[7]);

//make judge dir and into it
		sprintf(buf, "contest_judge_%s", argv);
		mkdir(buf, 7777);
		chmod(buf, S_IRWXU | S_IRGRP | S_IROTH);
		chdir(buf);
		if (lang == LANG_GCC || lang == LANG_GPP)
			strcpy(sourcefile, "src.c");
		else if (lang == LANG_JAVA)
			strcpy(sourcefile, "Main.java");
//get code ;
		sprintf(buf, "SELECT code FROM oj_contest_sources WHERE RUNID = %s ", argv);
		if (executesql(buf))
		{
			err_sql(buf);
			return SQL_FAILED;//exit(0);
		}
		res = mysql_store_result(g_conn);
		if(res==NULL){
			err_sql("res=null select code form oj_contest_sources");
			return SQL_FAILED;
		}
		row = mysql_fetch_row(res);
		mysql_free_result(res);
		res=NULL;
		if (row == NULL)
		{
			err_sql("store res(code) to row failed");
			continue;
		}
		FILE *fp;
		fp = fopen(sourcefile, "wt");
		if (fp == NULL)
		{
			err_system("create src file fail");
			return SYS_FAILED;
		}
		fprintf(fp, "%s", row[0]);
		fclose(fp);
	//path
		sprintf(buf, "%s/Contest%d/%d/%d.in", contest_input_path, contestid,
			problemid, problemid);
		strcpy(testdatafile, buf);
		sprintf(buf, "%s/Contest%d/%d/%d.out", contest_output_path, contestid,
			problemid, problemid);
		strcpy(comparedatafile, buf);
//get limit info
		sprintf(buf,"SELECT timelimit,memorylimit,outputlimit,specialjudge,javatimelimit,javamemorylimit FROM oj_contest_problems WHERE PROBLEMID=%d AND CONTESTID=%d",problemid, contestid);
		if (executesql(buf))
		{
			err_sql(buf);
			return SQL_FAILED;//exit(0);
		}
		res = mysql_store_result(g_conn);
		if(res==NULL)
		{
			err_sql("res=null select * form oj_contest_problems");
			return SQL_FAILED;
		}
		row = mysql_fetch_row(res);
		mysql_free_result(res);
		res=NULL;
		if (row == NULL)
		{
			err_sql("row=null when slcect * from oj_contest_problems");
			continue;
		}
		time_limit = atoi(row[0]);
		memory_limit = atoi(row[1]);
		output_limit = atoi(row[2]);
		spjflag = atoi(row[3]);
		java_time_limit = atoi(row[4]);
		java_memory_limit = atoi(row[5]);
		if (lang == LANG_JAVA)
		{
			time_limit = java_time_limit;
			memory_limit = java_memory_limit;
		}
		if(res!=NULL)
		{
			mysql_free_result(res);
			res=NULL;
		}
		break;
	}while(--retry);
	if(retry)
		return SQL_SUCCEED;
	else return SQL_FAILED;
}

/*
 *
 *
 *
 *
 */
int init_diy_contest_judge(char argv[])
{
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
	char buf[1024],logbuf[1024];
	chdir(judge_workdir);
	int retry=3;
	int sleep_sec[]={0,1,2,4,8,16,32},sleep_id=0;
	do{
		sleep(sleep_sec[sleep_id++]);
//problem info
		sprintf(buf,"SELECT diyproblemid,language,userid,diycontestid,runid,language,codelen,submittime FROM oj_diy_allstatus WHERE RUNID=%s",argv);
		if (executesql(buf))
		{
			puts(buf);
			err_sql(buf);
			return SQL_FAILED;//exit(0);
		}
		res = mysql_store_result(g_conn);
		if(res==NULL){
			err_sql("select * from oj_diy_allstatus res=null@mysql_store_result()");
			return SQL_FAILED;
		}
		row = mysql_fetch_row(res);
		if(res!=NULL){
			mysql_free_result(res);
			res=NULL;
		}
		if (row == NULL)
		{
			err_sql("select * from oj_diy_allstatus row=null@mysql_fetch_row");
			continue;
		}
		problemid = atoi(row[0]);
		lang = atoi(row[1]);
		userid = atoi(row[2]);
		contestid = atoi(row[3]);
		runid = atoi(row[4]);
		lang = atoi(row[5]);
		codelen = atoi(row[6]);
		submittime = atoi(row[7]);
		
	//make problem's woek dir	and go into it
		sprintf(buf, "diy_judge_%s",argv);
		mkdir(buf, 7777);
		chmod(buf, S_IRWXU | S_IRGRP | S_IROTH);
		chdir(buf);
		if (lang == LANG_GCC || lang == LANG_GPP)
			strcpy(sourcefile, "src.c");
		else if (lang == LANG_JAVA)
			strcpy(sourcefile, "Main.java");
		
	//get code
		sprintf(buf, "SELECT code FROM oj_diy_contest_sources WHERE RUNID=%s",argv);
		if (executesql(buf))
		{
			err_sql(buf);
			return SQL_FAILED;
		}
		res = mysql_store_result(g_conn);
		if(res==NULL){
			err_sql(" get code : res=null mysql_store_result();");
			return SQL_FAILED;
		}
		row = mysql_fetch_row(res);
		if(res!=NULL){
			mysql_free_result(res);
			res=NULL;
		}
		if (row == NULL)
		{
			err_sql("warming : get code : row=null ;");
			continue;
		}
		FILE *fp=NULL;
		fp = fopen(sourcefile, "wt");
		if (fp == NULL)
		{
			err_system("create src file fail");
			return SYS_FAILED;//exit(0);
		}
		fprintf(fp, "%s", row[0]);
		fclose(fp);
	//read path
		sprintf(buf, "%s/DiyContest%d/%d/%d.in", diy_contest_input_path, contestid,
				problemid, problemid);
		strcpy(testdatafile, buf);
		sprintf(buf, "%s/DiyContest%d/%d/%d.out", diy_contest_output_path, contestid,
				problemid, problemid);
		strcpy(comparedatafile, buf);
	//get from_proid diy的题目映射
		int from_proid;
		sprintf(buf, "SELECT from_proid FROM oj_diy_contest_problems WHERE diyproblemid=%d and diycontestid=%d",problemid,contestid);
		if (executesql(buf))
		{
			err_sql(buf);
			return SQL_FAILED;
		}
		res = mysql_store_result(g_conn);
		if(res==NULL){
			err_sql("get from_proid : res=null mysql_store_result(g_conn);");
			return SQL_FAILED;
		}
		row = mysql_fetch_row(res);
		if(res!=NULL){
			mysql_free_result(res);
			res=NULL;
		}
		if(row==NULL)
		{
			err_sql("warming: get from_pid row=null");
			continue;
		}
		from_proid=atoi(row[0]);
	//get problem limit info
		sprintf(buf,"SELECT timelimit,memorylimit,outputlimit,specialjudge,javatimelimit,javamemorylimit FROM oj_exercise_problems WHERE problemid=%d",from_proid);
		if (executesql(buf))
		{
			err_sql(buf);
			return SQL_FAILED;//exit(0);
		}
		res = mysql_store_result(g_conn);
		if(res==NULL){
			err_sql("select * from oj_exercise_problems res=null");
			return SQL_FAILED;
		}
		row = mysql_fetch_row(res);
		if(res!=NULL){
			mysql_free_result(res);
			res=NULL;
		}
		if (row == NULL)
		{
			err_sql("select * from oj_exercise_problems row=null");
			continue;
		}
		time_limit = atoi(row[0]);
		memory_limit = atoi(row[1]);
		output_limit = atoi(row[2]);
		spjflag = atoi(row[3]);
		java_time_limit = atoi(row[4]);
		java_memory_limit = atoi(row[5]);
		if (lang == LANG_JAVA)
		{
			time_limit = java_time_limit;
			memory_limit = java_memory_limit;
		}
		if(res!=NULL)
		{
			mysql_free_result(res);
			res=NULL;
		}
		break;
	}while(--retry);
	if(retry)
		return SQL_SUCCEED;
	else return SQL_FAILED;
}

/*
 *
 *
 *
 *
 */
int exercise_updatedb(int result, int timeusage, int memoryusage, char argv[])
{
		MYSQL_RES *res=NULL;
		MYSQL_ROW row;
		char buf[2000];
		int S[10] ={ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		S[result]=1;
		sprintf(
				buf,
				"UPDATE oj_exercise_pro_status SET ac=ac+%d,wa=wa+%d,re=re+%d,pe=pe+%d,tle=tle+%d,mle=mle+%d,ole=ole+%d,ce=ce+%d,other=other+%d WHERE problemid=%d",
				S[STATUS_AC], S[STATUS_WA], S[STATUS_RE], S[STATUS_PE],
				S[STATUS_TLE], S[STATUS_MLE], S[STATUS_OLE], S[STATUS_CE],
				S[STATUS_OTHER], problemid);
		if (executesql(buf))
		{
				err_sql(buf);
				return SQL_FAILED;
		}
		sprintf(buf,
				"UPDATE oj_exercise_allstatus SET runtime=%d,runmemory=%d,runresult=%d,judgetime=UNIX_TIMESTAMP(NOW()),fetched=2 WHERE runid=%s",
				timeusage, memoryusage, result, argv);
		if (executesql(buf))
		{
			err_sql(buf);
			return SQL_FAILED;
		}
		switch(result)
		{
		case STATUS_AC:
			sprintf(buf,"UPDATE oj_exercise_problems SET accepts=accepts+1 WHERE problemid=%d",problemid);
			if (executesql(buf))
			{
				err_sql(buf);
				return SQL_FAILED;
			}
			sprintf(buf,
					"UPDATE oj_users SET accepts=accepts+1 WHERE USERID=%d and (select runresult from oj_exercise_recentstatus where userid=%d and problemid=%d) <> 1",
					userid, userid, problemid);
			if (executesql(buf))
			{
				err_sql(buf);
				return SQL_FAILED;
			}
////////如果第一次提交 有记录吗？？？？？
			int rent_runtime, rent_runmemory, rent_ac_count, rent_err_count;
			sprintf(buf,
					"SELECT runtime,runmemory,ac_count,err_count FROM oj_exercise_recentstatus WHERE PROBLEMID=%d AND USERID=%d",
					problemid, userid);
			if (executesql(buf))
			{
				err_sql(buf);
				return SQL_FAILED;
			}
			res = mysql_store_result(g_conn);
			if(res==NULL){
				err_sql("res=null mysql_store_result(g_conn);");
				return SQL_FAILED;
			}
			row = mysql_fetch_row(res);
			if(res!=NULL){
				mysql_free_result(res);
				res=NULL;
			}
			if (row == NULL)
			{
				sprintf(buf,"%s:SELECT judgetime FROM `oj_exercise_recentstatus` WHERE PROBLEMID=%d error",
						judgetype, problemid);
				err_sql(buf);
				return SQL_FAILED;
			}
			rent_runtime = atoi(row[0]);
			rent_runmemory = atoi(row[1]);
			rent_ac_count = atoi(row[2]);
			rent_err_count = atoi(row[3]);
			//printf("rent_runtime=%d rent_runmemory=%d rent_ac_count = %d ren_err_count=%d\n",
			// atoi(row[0]), atoi(row[1]), atoi(row[2]), atoi(row[3]));
			if (rent_ac_count == 0)
			{
				sprintf(buf,
						"UPDATE oj_exercise_recentstatus SET runtime=%d,runmemory=%d,runresult=%d,judgetime=(select judgetime from oj_exercise_allstatus where runid =%d),runid=%d,language=%d,codelen=%d,submittime=%d WHERE PROBLEMID=%d AND USERID=%d\n",
						timeusage, memoryusage, result, runid, runid, lang,codelen, submittime, problemid, userid);
				if (executesql(buf))
				{
					err_sql(buf);
					return SQL_FAILED;
				}
			}
			else if (timeusage < rent_runtime)
			{
				sprintf(buf,
						"UPDATE oj_exercise_recentstatus SET runtime=%d,runmemory=%d,runresult=%d,judgetime=(select judgetime from oj_exercise_allstatus where runid =%d),runid=%d,language=%d,codelen=%d,submittime=%d WHERE PROBLEMID=%d AND USERID=%d\n",
						timeusage, memoryusage, result, runid, runid, lang,codelen, submittime, problemid, userid);
				//printf("Update timeusage < rent_runtime:%s\n", buf);
				if (executesql(buf))
				{
					err_sql(buf);
					return SQL_FAILED;
				}
			}
			else if (memoryusage < rent_runmemory)
			{
				sprintf(buf,
						"UPDATE oj_exercise_recentstatus SET runtime=%d,runmemory=%d,runresult=%d,judgetime=(select judgetime from oj_exercise_allstatus where runid =%d),runid=%d,language=%d,codelen=%d,submittime=%d "
							"WHERE PROBLEMID=%d AND USERID=%d\n", timeusage,memoryusage, result, runid, runid, lang, codelen,
						submittime, problemid, userid);
				//printf("Update memoryusage < rent_runmemory:%s\n", buf);
				if (executesql(buf))
				{
					err_sql(buf);
					return SQL_FAILED;
				}
			}
			sprintf(buf,"UPDATE oj_exercise_recentstatus SET ac_count=ac_count+1 WHERE PROBLEMID=%d AND USERID=%d",
					problemid, userid);
			if (executesql(buf))
			{
				err_sql(buf);
				return SQL_FAILED;
			}
			break;
		//没有通过的结果更新
		case STATUS_CE:
			sprintf(buf,"select * from oj_exercise_error where runid=%s",argv);
			if (executesql(buf))
			{
				err_sql(buf);
				return SQL_FAILED;
			}
			res = mysql_store_result(g_conn);
			if (mysql_num_rows(res) == 1){
				puts("ce has the note");
				mysql_free_result(res);
				res=NULL;
			}
			else
			{
				mysql_free_result(res);
				res=NULL;
				FILE *fp=NULL;
				fp = fopen(stderr_file_compile, "r");
				if (fp == NULL)
				{
					write2log("exercise update ,ce section open stderr_file failed");
					return SYS_FAILED;
				}
				int i = 0;
				char t;
				char ce[1000];
				while (t = fgetc(fp))
				{
					if (feof(fp) || i > 900)
						break;
					ce[i++] = t;
				}
				fclose(fp);
				ce[i] = '\0';
				char *end = buf;
				sprintf(buf, "INSERT INTO oj_exercise_error VALUES(%d,", runid);
				end += strlen(buf);
				*end++ = '\'';
				end += mysql_real_escape_string(g_conn, end, ce, strlen(ce));
				*end++ = '\'';
				*end++ = ')';
				*end = 0;
				if (executesql(buf))
				{
					err_sql(buf);
					return SQL_FAILED;
				}
			}
		case STATUS_WA:
		case STATUS_RE:
		case STATUS_PE:
		case STATUS_TLE:
		case STATUS_MLE:
		case STATUS_OLE:
		default:
			sprintf(buf,"UPDATE oj_exercise_recentstatus SET err_count=err_count+1  WHERE problemid=%d and userid=%d",problemid,userid);
			if (executesql(buf))
			{
				err_sql(buf);
				return SQL_FAILED;
			}
	}
	if(res!=NULL){
		mysql_free_result(res);
		res=NULL;
	}
	return SQL_SUCCEED;
}

/*
 *
 *
 *
 *
 */
int contest_updatedb(int result, int timeusage, int memoryusage, char argv[])
{
	MYSQL_RES *res=NULL;
	int sql_ret;
	int S[10] ={ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	char buf[6553];
	//8更新allstatus
	sprintf(
			buf,
			"UPDATE oj_contest_allstatus SET runtime=%d,runmemory=%d,runresult=%d,judgetime=UNIX_TIMESTAMP(NOW()),fetched=2 WHERE runid=%s",
			timeusage, memoryusage, result, argv);
	if (sql_ret = executesql(buf))
		goto funcEnd;
	//9更新prostatus
	S[result] = 1;
	sprintf(
			buf,
			"UPDATE oj_contest_pro_status SET ac=ac+%d,wa=wa+%d,re=re+%d,pe=pe+%d,tle=tle+%d,mle=mle+%d,ole=ole+%d,ce=ce+%d,other=other+%d WHERE problemid=%d AND contestid=%d",
			S[STATUS_AC], S[STATUS_WA], S[STATUS_RE], S[STATUS_PE],
			S[STATUS_TLE], S[STATUS_MLE], S[STATUS_OLE], S[STATUS_CE],
			S[STATUS_OTHER], problemid, contestid);
	if (sql_ret = executesql(buf))
		goto funcEnd;
	//10更新problems
	if (result == STATUS_AC)
	{
		sprintf(
				buf,
				"UPDATE oj_contest_problems SET accepts=accepts+1 WHERE problemid=%d AND contestid=%d",
				problemid, contestid);
		if (sql_ret = executesql(buf))
			goto funcEnd;
	}
	//11更新recent_status
	if (result != STATUS_AC)
	{
		sprintf(
				buf,
				"UPDATE oj_contest_recentstatus SET err_count=err_count+1,runtime=%d,runmemory=%d,runresult=%d,judgetime=(select judgetime from oj_contest_allstatus where runid =%d),runid=%d,language=%d,codelen=%d,submittime=%d WHERE PROBLEMID=%d AND USERID=%d AND CONTESTID=%d and runresult<>1",
				timeusage, memoryusage, result, runid, runid, lang, codelen,
				submittime, problemid, userid, contestid);
		if (sql_ret = executesql(buf))
			goto funcEnd;
	}
	else
	{
		//假如运行结果为AC,更新原来没有AC的记录,如果原来记录为AC则保持不变
		sprintf(
				buf,
				"UPDATE oj_contest_recentstatus SET runtime=%d,runmemory=%d,runresult=%d,judgetime=(select judgetime from oj_contest_allstatus where runid =%d),runid=%d,language=%d,codelen=%d,submittime=%d WHERE PROBLEMID=%d AND USERID=%d AND CONTESTID=%d and runresult<>1",
				timeusage, memoryusage, result, runid, runid, lang, codelen,
				submittime, problemid, userid, contestid);
		if (sql_ret = executesql(buf))
			goto funcEnd;
	}
	//CE处理
	if (result == STATUS_CE)
	{
		sprintf(buf,"select * from oj_contest_error where runid=%s",argv);
		if (executesql(buf))
		{
				err_sql(buf);
				return SQL_FAILED;
		}
		res = mysql_store_result(g_conn);
		if (mysql_num_rows(res) == 1){
			puts("ce has the note");
			mysql_free_result(res);
			return SQL_SUCCEED;
		}
		if(res!=NULL){
			mysql_free_result(res);
			res=NULL;
		}
		FILE *fp=NULL;
		fp = fopen(stderr_file_compile, "r");
		int i = 0;
		char ch;
		char ce[1000];
		char *end=NULL;
		while (ch = fgetc(fp))
		{
			if (feof(fp))
				break;
			ce[i++] = ch;
			//防止溢出
			if (i > 900)
				break;
		}
		fclose(fp);
		ce[i] = '\0';
		end = buf;
		sprintf(buf, "INSERT INTO oj_contest_error VALUES(%d,", runid);
		end += strlen(buf);
		*end++ = '\'';
		end += mysql_real_escape_string(g_conn, end, ce, strlen(ce));
		*end++ = '\'';
		*end++ = ')';
		*end = 0;
		if (sql_ret = executesql(buf))
			goto funcEnd;
	}

	funcEnd: if (sql_ret)
	{
		err_sql(buf);
		return SQL_FAILED;
	}
	if(res!=NULL)
	{
		mysql_free_result(res);
		res=NULL;
	}
	return SQL_SUCCEED;
}

/*
 *
 *
 *
 *
 */
int diy_contest_updatedb(int result, int timeusage, int memoryusage,char argv[])
{
	MYSQL_RES *res=NULL;
	int sql_errcode;
	char buf[6553];
	int S[10] =
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	//8更新allstatus
	sprintf(buf,"UPDATE oj_diy_allstatus SET runtime=%d,runmemory=%d,runresult=%d,judgetime=UNIX_TIMESTAMP(NOW()),fetched=2 WHERE runid=%s",timeusage, memoryusage, result, argv);
	if (sql_errcode = executesql(buf))
		goto funcEnd;
	//9更新prostatus
	S[result] = 1;
	sprintf(
			buf,
			"UPDATE oj_diy_contest_pro_status SET ac=ac+%d,wa=wa+%d,re=re+%d,pe=pe+%d,tle=tle+%d,mle=mle+%d,ole=ole+%d,ce=ce+%d,other=other+%d WHERE diyproblemid=%d AND diycontestid=%d",
			S[STATUS_AC], S[STATUS_WA], S[STATUS_RE], S[STATUS_PE],
			S[STATUS_TLE], S[STATUS_MLE], S[STATUS_OLE], S[STATUS_CE],
			S[STATUS_OTHER], problemid, contestid);
	if (sql_errcode = executesql(buf))
		goto funcEnd;
	//10更新problems
	if (result == STATUS_AC)
	{
		sprintf(buf,"UPDATE oj_diy_contest_problems SET accepts=accepts+1 WHERE diyproblemid=%d AND diycontestid=%d",problemid, contestid);
		if (sql_errcode = executesql(buf))
			goto funcEnd;
	}
	//11更新recent_status
	if (result != STATUS_AC)
	{
		sprintf(buf,"UPDATE oj_diy_contest_recentstatus SET err_count=err_count+1, runtime=%d,runmemory=%d,runresult=%d,judgetime=(select judgetime from oj_diy_allstatus where runid =%d),runid=%d,language=%d,codelen=%d,submittime=%d WHERE DIYPROBLEMID=%d AND USERID=%d AND DIYCONTESTID=%d and runresult<>1",timeusage, memoryusage, result, runid, runid, lang, codelen,submittime, problemid, userid, contestid);
		if (sql_errcode = executesql(buf))
			goto funcEnd;
	}
	else
	{
		//假如运行结果为AC,更新原来没有AC的记录,如果原来记录为AC则保持不变
		sprintf(buf,"UPDATE oj_diy_contest_recentstatus SET runtime=%d,runmemory=%d,runresult=%d,judgetime=(select judgetime from oj_diy_allstatus where runid =%d),runid=%d,language=%d,codelen=%d,submittime=%d WHERE DIYPROBLEMID=%d AND USERID=%d AND DIYCONTESTID=%d and runresult<>1",timeusage, memoryusage, result, runid, runid, lang, codelen,submittime, problemid, userid, contestid);
		if (sql_errcode = executesql(buf))
			goto funcEnd;
	}
	//CE处理
	if (result == STATUS_CE)
	{
		sprintf(buf,"select * from oj_diy_contest_error where runid=%s",argv);
		if (executesql(buf))
		{
				err_sql(buf);
				return SQL_FAILED;
		}
		res = mysql_store_result(g_conn);
		if (mysql_num_rows(res) == 1){
				puts("ce has the note");
				mysql_free_result(res);
				res=NULL;
				return SQL_SUCCEED;
		}
		mysql_free_result(res);
		res=NULL;
		FILE *fp=NULL;
		fp = fopen(stderr_file_compile, "r");
		int i = 0;
		char ch;
		char ce[1000];
		char *end;
		while (ch = fgetc(fp))
		{
			if (feof(fp))
				break;
			ce[i++] = ch;
			//防止溢出
			if (i > 900)
				break;
		}
		fclose(fp);
		ce[i] = '\0';
		end = buf;
		sprintf(buf, "INSERT INTO oj_diy_contest_error VALUES(%d,", runid);
		end += strlen(buf);
		*end++ = '\'';
		end += mysql_real_escape_string(g_conn, end, ce, strlen(ce));
		*end++ = '\'';
		*end++ = ')';
		*end = 0;
		if (sql_errcode = executesql(buf))
			goto funcEnd;
	}

funcEnd:
	if (sql_errcode)
	{
		err_sql(buf);
		return SQL_FAILED;//exit(0);
	}
	if(res!=NULL)
	{
		
	}
	return SQL_SUCCEED;
}

