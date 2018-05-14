#include"logger.h"
#include<unistd.h>

int main(){
	logger log("testlog");
	
	log.print("Start Write log");

	errno=6;
	log.error("The error is:");
	return 0;
}
