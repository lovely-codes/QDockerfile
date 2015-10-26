#include "myhttpd.h"
#include "common.h"
#include "net.h"
#include "config.h"
#include "handler.h"

void StopServer(){
	closeServer(myServer);
}

int main(int argc, char** argv){

	char *configFile = "./etc/myhttpd.conf";

	if(argc == 2) {
		configFile = argv[1];	
	}

	printf("\n");
	initConfig(configFile);
	myServer = initNServer(Config.PORT,Config.ProcessBorn,Config.DocumentRoot);
	signal(SIGINT, StopServer); 
	LOG("INIT server AT %d \n\n",Config.PORT);
	openServer(myServer);
	loopServer(myServer,loopMainHandler);
	StopServer();
	return 0;
}
