#include "header.h"

Worker::Worker(const std::filesystem::path& p,int period,int opt,int mfn,int st,std::string_view cmd,std::shared_ptr<Logger>& logger) : _absolute_path(p), _period(period), _option(opt), _maximum_file_numbers(mfn), _time(st), _logger(logger) { }

static std::string get_relative_path(std::string abs_path)
{
	std::string result = "";
	
	while(abs_path.length() > 0 && abs_path.back() != '/')
	{
		result.push_back(abs_path.back());
		abs_path.pop_back();
	}

	std::reverse(result.begin(),result.end());
	return result;
}

static bool comp(const std::string& s1, const std::string& s2)
{
	std::string sr1 = get_relative_path(s1);
	std::string sr2 = get_relative_path(s2);

	return sr1 > sr2;
}

void Worker::before_get_started(long long& start, const std::string& relative_path)
{
	int idx = 0;
	std::filesystem::path backup(std::filesystem::current_path()/"BACKUP");
	std::filesystem::directory_iterator dit(backup);
	std::vector<std::string> v;

	while(dit != std::filesystem::end(dit))
	{
		std::filesystem::directory_entry entry = *dit;
		std::string entry_relative_path = get_relative_path(entry.path().string());

		if(entry_relative_path.substr(0,relative_path.length()) != relative_path) 
		{
			dit++;
			continue;
		}
		
		v.push_back(entry.path().string());
		dit++;
	}

	std::sort(v.begin(),v.end(),comp);
	if(_option & option_an)
	{
		while(!v.empty() && idx < _maximum_file_numbers)
		{
			std::filesystem::remove(v.back());
			_logger->write_log(v.back(),"deleted");
			v.pop_back();
			idx++;
		}
	}
	if(_option & option_at)
	{
		while(!v.empty())
		{
			std::string str_backup = get_relative_path(v.back());
			int len = static_cast<int>(str_backup.length());
			std::string str_backup_time = "";
			for(int i = len-1; i >= 0 && str_backup[i] != '_'; i--) 
				if(str_backup[i] >= '0' && str_backup[i] <= '9')
					str_backup_time = str_backup_time + str_backup[i];
			long long backup_time = stoll(str_backup_time);
			int year,month,day,hour,minute,second;

			second = static_cast<int>(backup_time%100);
			backup_time /= 100;

			minute = static_cast<int>(backup_time%100);
			minute *= 60;
			backup_time /= 100;

			hour = static_cast<int>(backup_time%100);
			hour *= 60*60;
			backup_time /= 100;

			day = static_cast<int>(backup_time%100);
			day *= 60*60*24;
			backup_time /= 100;

			month = static_cast<int>(backup_time%100);
			month *= 60*60*24*30;
			backup_time /= 100;

			year = static_cast<int>(backup_time%100);
			year *= 60*60*24*30*12;

			backup_time = year+month+day+hour+minute+second;
			if(start-backup_time > static_cast<long long>(_time))
			{
				std::filesystem::remove(v.back());
				_logger->write_log(v.back(),"deleted");
				v.pop_back();
				continue;
			}
			break;
		}
	}

	for(int i = 0; i < v.size(); i++)
	{		
		std::string str_backup = get_relative_path(v[i]);
		int len = static_cast<int>(str_backup.length());
		std::string str_backup_time = "";
		for(int i = len-1; i >= 0 && str_backup[i] != '_'; i--) 
			if(str_backup[i] >= '0' && str_backup[i] <= '9')
				str_backup_time = str_backup_time + str_backup[i];
		long long backup_time = stoll(str_backup_time);
		int year,month,day,hour,minute,second;

		second = static_cast<int>(backup_time%100);
		backup_time /= 100;

		minute = static_cast<int>(backup_time%100);
		minute *= 60;
		backup_time /= 100;

		hour = static_cast<int>(backup_time%100);
		hour *= 60*60;
		backup_time /= 100;

		day = static_cast<int>(backup_time%100);
		day *= 60*60*24;
		backup_time /= 100;

		month = static_cast<int>(backup_time%100);
		month *= 60*60*24*30;
		backup_time /= 100;

		year = static_cast<int>(backup_time%100);
		year *= 60*60*24*30*12;

		backup_time = year+month+day+hour+minute+second;
		_logger->write_log(v[i],"added");
		_q.push({v[i],backup_time});
	}
}

void Worker::operator()() // backup thread 
{
	struct tm cur_tm;
	time_t cur_time;
	long long start;
	std::string relative_path = get_relative_path(_absolute_path.string());
	int cur_year,cur_month,cur_day,cur_hour,cur_minute,cur_second;
	auto init_mtime = std::filesystem::last_write_time(_absolute_path);
	std::filesystem::path backup(std::filesystem::current_path()/"BACKUP");

	cur_time = time(nullptr);
	localtime_r(&cur_time,&cur_tm);

	cur_year = cur_tm.tm_year+1900;
	cur_month = cur_tm.tm_mon+1;
	cur_day = cur_tm.tm_mday;
	cur_hour = cur_tm.tm_hour;
	cur_minute = cur_tm.tm_min;
	cur_second = cur_tm.tm_sec;
	start = get_time(cur_year,cur_month,cur_day,cur_hour,cur_minute,cur_second);

	before_get_started(start,relative_path); // 이미 백업된 파일들이 있다면 옵션에 맞춰서 수정
	
	while(true)
	{
		long long cur;

		cur_time = time(nullptr);
		localtime_r(&cur_time,&cur_tm);

		cur_year = cur_tm.tm_year+1900;
		cur_month = cur_tm.tm_mon+1;
		cur_day = cur_tm.tm_mday;
		cur_hour = cur_tm.tm_hour;
		cur_minute = cur_tm.tm_min;
		cur_second = cur_tm.tm_sec;
		cur = get_time(cur_year,cur_month,cur_day,cur_hour,cur_minute,cur_second);

		// period마다 백업 수행
		if(start+static_cast<long long>(_period) <= cur)
		{
			int year = cur_year%100;
			int q_size = static_cast<int>(_q.size());
			std::string backup_time = "";
			std::string file_name = "";

			backup_time = get_backup_time(year,cur_month,cur_day,cur_hour,cur_minute,cur_second);
			file_name = backup.string()+"/"+relative_path+"_"+backup_time;	

			if(_option & option_am) // mtime이 바뀌었을 때마다 수행
			{
				auto new_mtime = std::filesystem::last_write_time(_absolute_path);
				if(init_mtime == new_mtime) 
				{
					start = start+_period;
					continue;
				}
				init_mtime = new_mtime;
			}
			
			if(_option & option_an) // 백업한 파일들의 최대 개수 (최근 maximum_file_numbers 개를 제외하곤 모두 삭제)
			{
				while(!_q.empty() && q_size > _maximum_file_numbers)
				{
					auto file_path = _q.front().first;
					std::filesystem::remove(file_path);
					_logger->write_log(file_path.string(),"deleted");
					_q.pop();
				}
			}
			if(_option & option_at) // 보관한 시간이 _time을 넘어가면 삭제
			{
				while(!_q.empty())
				{
					auto file_path = _q.front().first;
					auto store_time = _q.front().second;
					long long diff = cur-store_time;

					if(diff > static_cast<long long>(_time))
					{
						std::filesystem::remove(file_path);
						_logger->write_log(file_path.string(),"deleted");
						_q.pop();
						continue;
					}
					break;
				}
			}

			try {
				std::filesystem::copy_file(_absolute_path,file_name);
			} catch(std::filesystem::filesystem_error & e)
			{
				start = start+_period;
				continue;
			}

			_logger->write_log(file_name,"generated");
			_q.push({file_name,cur});
			start = start+_period;
		}
	}
}


long long Worker::get_time(int& year,int& month,int& day,int& hour,int& minute,int& second)
{
	long long result = 0;
	
	year -= 2000;
	result += (year*60*60*24*30*12);
	result += (month*60*60*24*30); // 달은 그냥 30일로 고정
	result += (day*60*60*24);
	result += (hour*60*60);
	result += (minute*60);
	result += second;

	return result;
}

std::vector<std::string> Worker::get_backup_files()
{
	auto temp_q = _q;
	std::vector<std::string> v;
	
	while(!temp_q.empty())
	{
		v.push_back(temp_q.front().first.string());
		temp_q.pop();
	}

	return v;
}

std::string Worker::get_backup_time(int& year,int& month,int& day,int& hour,int& minute,int& second)
{
	std::string str_year = std::to_string(year);
	std::string str_month = std::to_string(month);
	std::string str_day = std::to_string(day);
	std::string str_hour = std::to_string(hour);
	std::string str_minute = std::to_string(minute);
	std::string str_second = std::to_string(second);

	if(str_year.length() < 2) str_year = "0"+str_year;
	if(str_month.length() < 2) str_month = "0"+str_month;
	if(str_day.length() < 2) str_day = "0"+str_day;
	if(str_hour.length() < 2) str_hour = "0"+str_hour;
	if(str_minute.length() < 2) str_minute = "0"+str_minute;
	if(str_second.length() < 2) str_second = "0"+str_second;
	
	return str_year+str_month+str_day+str_hour+str_minute+str_second;
}
