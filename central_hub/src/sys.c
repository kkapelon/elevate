/*
 * Copyright (c) 2006 Kapelonis Kostis <kkapelon@freemail.gr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * central_hub -- The launcher component of Project Elevate.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/vfs.h>

#include "sys.h"

/*
 * Functions for retrieving system statistics. Linux specific.  TODO Check them
 * for memory leaks since they are running all the time.
 */

#define LINE 1024



int get_5min_load(void)
{
	FILE *where;
	float load1;
	float load5;
	float load15;

	where=fopen("/proc/loadavg","r");
	if (where==NULL)
	{
		printf("Could not open /proc/loadavg\n");
		return 0;
	}
	fscanf(where,"%f %f %f %*d/%*d %*d\n",&load1,&load5,&load15);
	//printf("Got   %f %f %f\n",load1,load5,load15);

	load5=load5*100; /* Convert 0.25 to 25% */
	fclose(where);

	return load5;
}

/* 
 * Gets the free space for the /home partition or the partition
 * where the home directory of the user is mounted.
 */
void get_free_space(long *space,int *percent)
{
	int ret;
	struct statfs st;
	long available;


	char *home_location=NULL;
	home_location=getenv("HOME");
	if(home_location==NULL)
	{
		printf("Warning could not read your $HOME variable\n");
		printf("Using /home/ as default\n");
		ret=statfs("/home",&st);
	}
	else 
		ret=statfs(home_location,&st); /* Get statistics */
	if(ret!=0) 
	{
		*space=0;
		*percent=0;
		return;
	}
	//printf("%ld %ld %ld\n",st.f_bsize,st.f_blocks,st.f_bavail);

	available=st.f_bavail;

	available=available/1024;
	available=available* st.f_bsize;
	available=available/1024/1024; /* Convert to GBs */
	*space=available;
	*percent=((float)st.f_bavail/st.f_blocks)*100;
	return;
}

/*
 * This function checks to see if the user has internet access. Currently it
 * uses an ugly hack. It connects to the web server of Microsoft (port 80) and
 * sees if the connection was successfull. If is not 3 events are possible:
 * - Microsoft.com is down (unlikely)
 * - The firewall blocks traffic to port 80 (unlikely)
 * - No internet access (the most usual reason)
 *
 * If you have any suggestion on how to better check for this, please contact me.
 */

int is_online_now(void)
{
	char *host="www.microsoft.com";
	int port=80;
	int sockfd; /* The network socket */
	struct sockaddr_in their_addr; /* The internet address structure */
	struct hostent *h; /* The host structure */
	char *ip_address; /* Ip address as a string */

	/* Get the IP of microsoft.com  */
	if ((h=gethostbyname(host))==NULL)
	  {
		  perror("dns");
		  return 0;
	  }
	/* Convert bytes to network order */
	ip_address=inet_ntoa(*((struct in_addr *)h->h_addr));

	/* Specify socket to be of type internet */
	their_addr.sin_family=AF_INET;
	/* Set the target port of the connection */
	their_addr.sin_port=htons(port);
	/* Set the target ip of the connection */
	their_addr.sin_addr.s_addr=inet_addr(ip_address);
	/* Zero the rest of the structure */
	memset(&(their_addr.sin_zero),'\0',8);

	/* Create a network socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		return 0;
	}

	//printf("Connecting to %s\n",ip_address);
	/* Open a connection */
	if(connect(sockfd,(struct sockaddr *)&their_addr,sizeof(struct sockaddr))==-1){
		perror("connect");
		close(sockfd);
		return 0; /* No internet access */
	}
	/* Since we are here we are connected */
	shutdown(sockfd,SHUT_RDWR);
	close(sockfd);
	return 1; /* We have internet access! */
}
