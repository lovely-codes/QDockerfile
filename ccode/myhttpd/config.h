#ifndef MYHTTPD_CONFIG_H
#define MYHTTPD_CONFIG_H

#include "common.h"


typedef struct {
	int ProcessBorn;
	BOOL DEBUG;
        BOOL NOTHREAD;
	int PORT ;
	int MaxResponseLen;
	char DocumentRoot[128];
	int MaxBlocks;
	int BlockSize;
} ConfigT;

typedef struct {
	char *configName;
	void (*configFun)(char* value);
} ConfigEntry;

ConfigT Config;

void initConfig(char *);
void configLine(char*);
void configReadLine(FILE **,char*,int);

void configProcessBorn(char *);
void configPort(char *);
void configDEBUG(char *); 
void configDocumentRoot(char *); 
void configMaxResponseLen(char *); 
void configBlockSize(char *); 
void configMaxBlocks(char *); 
void configThread(char *); 


#endif
