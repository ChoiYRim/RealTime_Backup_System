#include "header.h"

Logger::Logger() { }

void Logger::write_log(const std::string& file_name, std::string_view message)
{
	std::lock_guard<std::mutex> lock(mutex);

	std::ofstream log_file("log.txt",std::ofstream::out|std::ofstream::app);

	time_t cur_time;
	struct tm cur_tm;
	std::string contents = "";
	int year,month,day,hour,minute,second;

	cur_time = time(nullptr);
	localtime_r(&cur_time,&cur_tm);

	year = (cur_tm.tm_year+1900)%100;
	month = cur_tm.tm_mon+1;
	day = cur_tm.tm_mday;
	hour = cur_tm.tm_hour;
	minute = cur_tm.tm_min;
	second = cur_tm.tm_sec;

	std::string str_year = std::to_string(year);
	std::string str_month = std::to_string(month);
	std::string str_day = std::to_string(day);
	std::string str_hour = std::to_string(hour);
	std::string str_minute = std::to_string(minute);
	std::string str_second = std::to_string(second);

	if(str_year.length() < 2)
		str_year = "0"+str_year;
	if(str_month.length() < 2)
		str_month = "0"+str_month;
	if(str_day.length() < 2)
		str_day = "0"+str_day;
	if(str_hour.length() < 2)
		str_hour = "0"+str_hour;
	if(str_minute.length() < 2)
		str_minute = "0"+str_minute;
	if(str_second.length() < 2)
		str_second = "0"+str_second;

	contents = "["+str_year+str_month+str_day+" ";
	contents += str_hour+str_minute+str_second+"] ";
	contents += file_name+" ";
	contents += message.data();

	log_file << contents << "\n";
	log_file.flush();
	log_file.close();
}

Logger::~Logger()
{
}
