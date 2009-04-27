/*
 *  CDVBLog.cpp
 *  Video4Mac
 *
 *  Created by Chris Roberts on 12/04/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */


#include <glob.h>
#include <pthread.h>
#include "CDVBLog.h"
#include <string>
#include <iostream>
#include <sstream>

using namespace Video4Mac;

static pthread_once_t g_LogOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_Mutex;
static FILE *g_FD;

char* CreatePathByExpandingTildePath(char* path)
{
	glob_t globbuf;
	char **v;
	char *expandedPath = NULL, *result = NULL;
	
	assert(path != NULL);
	if (glob(path, GLOB_TILDE, NULL, &globbuf) == 0) //success
	{
		v = globbuf.gl_pathv; //list of matched pathnames
		expandedPath = v[0]; //number of matched pathnames, gl_pathc == 1
		
		result = (char*)malloc(strlen(expandedPath) + 1); //the extra char is for the null-termination
		if(result)
			strncpy(result, expandedPath, strlen(expandedPath) + 1); //copy the null-termination as well

		globfree(&globbuf);
	}
	
	return result;
}

void CDVBLog::Initialize(void)
{
	pthread_mutex_init(&g_Mutex, NULL);
}
	
void CDVBLog::Log(kDVBLogTargets LogLevel, const char *format, ... )
{
	
	pthread_once(&g_LogOnce, CDVBLog::Initialize);
	
	char *path = "~/Library/Logs/";
	pthread_mutex_lock(&g_Mutex);
	
	if (!g_FD)
	{
		char *expandedPath;
		
		expandedPath = CreatePathByExpandingTildePath(path);
		expandedPath = (char *)realloc(expandedPath, strlen(expandedPath)+8);
		strcat(expandedPath, "DVB.log");
		
		g_FD = fopen(expandedPath, "a+");
		if (!g_FD)
		{
			fprintf(stderr, "could not open log file.\n");
			exit(0);
		}
	}

	fprintf(g_FD, "--> %2x ", LogLevel);
	va_list args;
	va_start (args, format);
	vfprintf (g_FD, format, args);
	va_end (args);
	fflush(g_FD);
	pthread_mutex_unlock(&g_Mutex);

}