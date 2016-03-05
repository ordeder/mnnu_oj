#!/bin/bash
#chkconfig:234 80 80
#description:Contral Online Judge Process

case $1 in
	'start')
		/home/ZSOJ/core/oj_daemon.2 start
                /home/ZSOJ/core/cheat_proc 1 &
	;;
	'stop')
		su root -c "killall oj_daemon.2"
                su root -c "killall cheat_proc"
	;;
esac
