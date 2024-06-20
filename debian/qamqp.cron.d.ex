#
# Regular cron jobs for the qamqp package
#
0 4	* * *	root	[ -x /usr/bin/qamqp_maintenance ] && /usr/bin/qamqp_maintenance
