target:oj_daemon
oj_daemon: oj_daemon.o judge_dev.o database.o log.o security.o judge.h log.h security.h database.h oj_daemon.h
	gcc -I/usr/include/mysql oj_daemon.o judge_dev.o database.o log.o security.o -Wall -O1 -o oj_daemon  -L/usr/lib/mysql -lmysqlclient
oj_daemon.o : oj_daemon.c judge.h log.h security.h database.h oj_daemon.h
	gcc -I/usr/include/mysql -Wall -O1 -c oj_daemon.c  -L/usr/lib/mysql -lmysqlclient
judge.o : judge_dev.c judge.h log.h security.h database.h oj_daemon.h
	gcc -I/usr/include/mysql -Wall -O1 -c judge_dev.c   -L/usr/lib/mysql -lmysqlclient
datebase.o: database.c judge.h log.h security.h database.h oj_daemon.h
	gcc -I/usr/include/mysql -Wall -O1 -c database.c  -L/usr/lib/mysql -lmysqlclient
log.o:log.c log.h
	gcc -Wall -O1 -c log.c
security.o : security.c security.h judge.h
	gcc -Wall -O1 -c security.c
clean:
	rm -f *.o
