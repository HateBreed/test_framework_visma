#!/bin/bash

if [ ! -f testfw.conf ] ; then 
	echo "No configuration \"testfw.conf\" found. Quitting"
	exit 1
else
	source testfw.conf
fi

if [ -z $SMTP_SRV ] ; then
	echo "SMTP_SRV not configured in \"testfw.conf\". Quitting."
	exit 1
fi

if [ -z $SENDER_ADDRESS ] ; then
	echo "SENDER_ADDRESS not configured in \"testfw.conf\". Quitting."
	exit 1
fi

if [ $# -eq 2 ] ; then

	DATE=$(date +"%Y.%m.%d-%H_%M_%S")
	TESTSUBJECT="$SUBJECT $2 $DATE"
	LOGFILE="run_log_$1_$DATE"

	if $(./testfw -u $1 -t $2 1>run_log_$1_$DATE) ; then
		if [ $(which mailx) ] ; then
			mailx -S smtp=$SMTP_SRV -r $SENDER_ADDRESS -s $TESTSUBJECT -v $1 < $LOGFILE
		else
			echo "No mailx binary found. Tests are not sent, see $LOGFILE"
		fi
	else
		ERROR="$TESTSUBJECT [FAILED - NO SUCH TEST OR USER]"
		if [ $(which mailx) ] ; then
			mailx -S smtp=$SMTP_SRV -r $SENDER_ADDRESS -s $ERROR -v $1 < $LOGFILE
		else
			echo "No mailx binary found. Test running was not successful, see $LOGFILE"
		fi
	fi
	exit 0
else
	exit 1
fi


	
