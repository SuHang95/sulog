#include<iostream>
#include<thread>
#include<stdio.h>
#include"logger.h"

#ifdef __gnu_linux__
#include<unistd.h>
#endif


#define max_thread_num 100
volatile int threadnum = 0;
volatile int threadsize = 100;

void test(logger &&log);


void test1(const logger &log) {
	if (++threadnum >= 100)
		return;
	char sub[100];
	log.warning("this is the %d thread!", threadnum);
	logger that(log);
	if (threadnum % 2 == 0) {
		std::thread testthread(test, that);
		testthread.detach();
	}
	else {
		std::thread testthread(test1, that);
		testthread.detach();
	}
	threadsize--;
}

void test3() {
	logger Log("ForTest");
	std::thread test(test1, Log);
	test.detach();
}


int main() {
	test3();
	while (threadsize>1) {
#ifdef _WIN32
		Sleep(100);
#elif __gnu_linux__
		usleep(100);
#endif
	}
#ifdef _WIN32 
	system("pause");
#endif 
	return 0;
}





void test(logger &&log) {
	if (++threadnum >= 100)
		return;
	char sub[100];
	log.debug("this is the %d thread!", threadnum);
	
	logger that(log);
	if (threadnum % 2 == 0) {
		std::thread testthread(test, that);
		testthread.detach();
	}
	else {
		std::thread testthread(test1, that);
		testthread.detach();
	}
	threadsize--;
	
}
