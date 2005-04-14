/*
 * ha_logger.c utility to log a message to the logging daemon
 *
 * Copyright (C) 2004 Guochun Shi <gshi@ncsa.uiuc.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <glib.h>
#include <clplumbing/cl_log.h>
#include <clplumbing/ipc.h>
#include <clplumbing/GSource.h>
#include <clplumbing/cl_malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ha_config.h>
#include <clplumbing/loggingdaemon.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>

#define EXIT_OK		0
#define EXIT_FAIL	1
#define MAXMSGSIZE		4096

int LogToLoggingDaemon(int priority, const char * buf, int bstrlen, gboolean use_pri_str);
extern IPC_Channel * get_log_chan(void);


static gboolean
send_log_msg(gpointer data)
{
	static int count = 0;
	char	msgstring[MAXMSGSIZE];
	int	priority; 
	static int	dropmsg = 0;
	long	maxcount = (long) data;
	IPC_Channel* chan =  get_log_chan();
	
	
	if (chan == NULL){
		cl_log(LOG_ERR, "logging channel is NULL");
		return FALSE;

	}
	if (count >= maxcount){
		cl_log(LOG_INFO, "total message dropped: %d", dropmsg);
		return FALSE;
	}
	
	if (chan->send_queue->current_qlen 
	    == chan->send_queue->max_qlen){
		return TRUE;		
	}
	

	priority = LOG_INFO;
	msgstring[0]=0;
	
	sprintf(msgstring, "Message %d", count++);
	
	if (LogToLoggingDaemon(priority, msgstring,MAXMSGSIZE, FALSE) != HA_OK){			
		printf("sending out messge %d failed\n", count);
		dropmsg++;
	}
	

	
	return TRUE;
}



int
main(int argc, char** argv)
{

	long maxcount;
	GMainLoop* loop;

	if (argc < 2){
		fprintf(stderr, "Wrong parameter\n");
		return 1;
	}
	
	maxcount = atoi(argv[1]);
	
	cl_log_set_facility(LOG_LOCAL7);
	cl_log_set_uselogd(TRUE);
	
	if(!cl_log_test_logd()){
		return EXIT_FAIL;
	}
	
	cl_log_set_logd_channel_source(NULL, NULL);
	
	g_idle_add(send_log_msg, (gpointer)maxcount);
	

	loop = g_main_loop_new(NULL, FALSE);
	g_main_run(loop);
	return(1);
	
}
