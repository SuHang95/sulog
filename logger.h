#ifndef _LOGGER_H__
#define _LOGGER_H__

#include<atomic>
#include<string>
#include<fstream>
#include<mutex>
#include<ctime>
#include<cstdarg>
#include<cstring>
#include<atomic>
#ifdef _WIN32
#include<windows.h>
#elif __gnu_linux__
#include<cstring>
#include<sys/time.h>
#endif

#define Info ((int)0)
#define Debug ((int)1)
#define Warning ((int)2)
#define Error ((int)3)
#define buffsize ((int)0x1000)

class logger {
	friend inline void swap(logger&, logger&);
public:
	logger(){}
	logger(const std::string& name,int _degree=0):
		degree(_degree){
		counter = new std::atomic<int>(1);
		mutex_for_data = new std::mutex();

		filename = name;
		logfile = new std::fstream();
		std::string newname = name + gethour();
		logfile->open(newname, std::ios_base::in | std::ios_base::out | std::ios_base::ate | std::ios_base::app);

		time_t nowtime;
		nowtime = time(NULL);
		struct tm* local;
		local = localtime(&nowtime);

		hour = new int(local->tm_hour);
		buffer = new char[buffsize];
	}
	logger(const logger& other){
		int _counter=other.counter->fetch_add(1)+1;

		degree=other.degree.load();
		logfile = other.logfile;
		mutex_for_data = other.mutex_for_data;
		counter = other.counter;
		filename = other.filename;
		hour = other.hour;
		buffer=other.buffer;
	}
	logger& operator=(const logger& other) {
		int other_counters = other.counter->fetch_add(1)+1;

		if (counter->fetch_add(-1) == 1) {
			delete logfile;
			delete mutex_for_data;
			delete counter;
			delete []buffer;
		}

		degree=other.degree.load();
		logfile = other.logfile;
		mutex_for_data = other.mutex_for_data;
		counter = other.counter;
		filename = other.filename;
		hour = other.hour;
		buffer = other.buffer;
	}

	logger(logger&& other) noexcept {
		int _counter = other.counter->fetch_add(1)+1;
	
		degree=other.degree.load();	
		logfile = other.logfile;
		mutex_for_data = other.mutex_for_data;
		counter = other.counter;
		filename = other.filename;
		hour = other.hour;
		buffer = other.buffer;
	}

	logger& operator=(logger&& other) {
		swap(*this, other);
		return *this;
	}



	void error(const char* format,...) const{
		if(degree>Error)
			return;
		va_list args;
		va_start(args, format);

		char* buff=nullptr;
		char* errstr=strerror(errno);
		size_t error_len=strlen(errstr);	

		size_t len=vsnprintf(buffer,len,format,args)
		if(buffsize<len){
			buff=new char[len+error_len];
		}
		
		va_end(args);

		strcat(buffer,errstr);
		std::string towrite(buffer,strlen(buffer));
		Print(towrite,Error);
		if(buffer!=NULL)
			delete buffer;		
	}

	

	void debug(const char* format,...) const{
		va_list args;
		va_start(args, format);
		
		if(degree>Debug)
			return;

		while(len<=vsnprintf(buffer,len,format,args)){
			delete buffer;
			len*=4;
			buffer=new char[len];
		}
		va_end(args);
		std::string towrite(buffer,strlen(buffer));
		Print(towrite,Debug);
		if(buffer!=NULL)
			delete buffer;
	}

	void debug(const std::string& text) const{
		if(degree>Debug)
			return;
		Print(text,Debug);
	}

	void warning(const char* format,...) const{
		va_list args;
		va_start(args, format);
		
		if(degree>Warning)
			return;

		size_t _len=strlen(format)*10;
		size_t len=0x100>_len?0x100:_len;
		char* buffer=new char[len];
		while(len<=vsnprintf(buffer,len,format,args)){
			delete buffer;
			len*=4;
			buffer=new char[len];
		}
		va_end(args);
		std::string towrite(buffer,strlen(buffer));
		Print(towrite,Warning);
		if(buffer!=NULL)
			delete buffer;
	}

	void warning(const std::string& text) const{
		if(degree>Warning)
			return;
		Print(text,Warning);
	}


	void print(const char* format,...) const{
		va_list args;
		
		va_start(args, format);
		size_t _len=strlen(format)*10;
		size_t len=0x100>_len?0x100:_len;
		char* buffer=new char[len];
		while(len<=vsnprintf(buffer,len,format,args)){
			delete []buffer;
			len*=4;
			buffer=new char[len];
		}
		va_end(args);
		std::string towrite(buffer,strlen(buffer));
		Print(towrite);
		if(buffer!=NULL)
			delete []buffer;
	}

	void print(const std::string& text) const{
		Print(text,Info);
	}

	~logger() {
		int _counter=counter->fetch_add(-1)-1;
		if (_counter == 0) {
		#ifdef _DEBUG
			print("Destroy in a destructor with counter=%d!\n",0);
		#endif
			delete logfile;
			delete mutex_for_data;
			delete counter;
			delete []buffer;
		}
	}
	inline void set_degree(int a){
		degree=a;
	}
	void close() {
		mutex_for_data->lock();
		logfile->close();
		mutex_for_data->lock();
	}
private:
	std::fstream* logfile;
	inline std::string gettime() const;
	inline std::string gethour() const;
	inline void reopen() const;
	std::string filename;
	int* hour;
	std::mutex* mutex_for_data;
	std::atomic<int> degree;
	//the buffer output to stream
	char* buffer;

	void Print(const std::string& text,int type=Info) const {
		static int check_hour;
		if(degree>type)
			return;

		if(check_hour++%100==0){
			time_t nowtime;
			nowtime = time(NULL);
			struct tm* local;
			local = localtime(&nowtime);
			int nowhour = local->tm_hour;

			if (nowhour != *hour) {
				reopen();

				mutex_for_data->lock();
				*hour = nowhour;
				mutex_for_data->unlock();
			}
		}
		mutex_for_data->lock();
		if (logfile->is_open()){
			switch(type){
				case Debug:			
					(*logfile) << gettime() << " DEBUG " << text << std::endl;
					break;
				case Info:
					(*logfile) << gettime() << " INFO " << text << std::endl;
					break;
				case Warning:
					(*logfile) << gettime() << " WARN " << text << std::endl;
					break;
				case Error:
					(*logfile) << gettime() << " ERROR " << text << std::endl;
					break;
					
			}
			logfile->flush();
		}
		mutex_for_data->unlock();
	}
private:
	volatile std::atomic<int>* counter;
};

//return the time with second
std::string logger::gettime() const {
	char buf[100] = { 0 };
	#ifdef _WIN32
		SYSTEMTIME sys;
		GetLocalTime(&sys);
		sprintf_s(buf, 100, "%4d-%02d-%02d-%02d:%02d:%02d-%03d", \
			sys.wYear, sys.wMonth, sys.wDay, sys.wHour, \
			sys.wMinute, sys.wSecond, sys.wMilliseconds);
	#else
		time_t nowtime;
		nowtime = time(NULL);
		struct tm* local;
		local = localtime(&nowtime);
		strftime(buf, 100, "%Y-%m-%d-%H:%M:%S", local);
		#ifdef __gnu_linux__
			struct timeval tv;
			gettimeofday(&tv, NULL);
			int millisecond = (tv.tv_usec / 1000) % 1000;
			char _buff[10];
			sprintf(_buff, "-%03d", millisecond);
			strcat(buf, _buff);
		#endif
	#endif
	return std::string(buf, strlen(buf));
}

inline void swap(logger& left, logger& right) {
	using std::swap;

	int tmp=left.degree.load();
	left.degree=right.degree.load();
	right.degree=tmp;

	swap(left.counter, right.counter);
	swap(left.logfile, right.logfile);
	swap(left.hour, right.hour);
	swap(left.mutex_for_data, right.mutex_for_data);
	swap(left.filename, right.filename);

	return;
}



inline std::string logger::gethour() const
{
	char buf[100] = { 0 };
#ifdef _WIN32
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	sprintf_s(buf, 100, "%4d-%02d-%02d-%02d", \
		sys.wYear, sys.wMonth, sys.wDay, sys.wHour);
#else
	time_t nowtime;
	nowtime = time(NULL);
	struct tm* local;
	local = localtime(&nowtime);
	strftime(buf, 100, "%Y-%m-%d-%H", local);
#endif

	return std::string(buf, strlen(buf));
}


inline void logger::reopen() const {
	mutex_for_data->lock();
	if (logfile->is_open())
		logfile->close();

	std::string nowhour = gethour();
	std::string newname = filename + nowhour;
	logfile->open(newname, std::ios_base::in | std::ios_base::out | std::ios_base::ate | std::ios_base::app);
	mutex_for_data->unlock();
}


#endif
