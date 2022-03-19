#ifndef _REALTIME_BACKUP_SYSTEM_
#define _REALTIME_BACKUP_SYSTEM_

#include <unordered_map>
#include <string_view>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <list>

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>

const int option_am = 1<<1;
const int option_an = 1<<2;
const int option_at = 1<<3;
const int option_ad = 1<<4;
const int option_ra = 1<<5;
const int option_rn = 1<<6;
const size_t kbuffer_length = 1024;

enum
{
	ADD = 0,
	REMOVE,
	COMPARE,
	RECOVER,
	LIST
};

struct Info
{
	int _period;
	int _option;
	int _maximum_file_numbers;
	int _store_time;

	friend std::ostream& operator<<(std::ostream& os,const Info& info);
};

//inline bool is_ascii(int ch) { return (ch >= 0 && ch <= 127); }

class Worker;
class Logger;

class Program
{
	public:
		Program() = default;

		void start();
		void make_backup_directory();
		
		int add(const std::vector<std::string>& cmds);
		bool get_add_options(int&,int&,int&,int&,std::filesystem::path&,const std::vector<std::string>&);
		bool add_directory(const std::filesystem::path&, int&, int&, int&, int&);

		int remove(const std::vector<std::string>& cmds);

		int compare(const std::vector<std::string>& cmds);

		int recover(const std::vector<std::string>& cmds);

		int list(const std::vector<std::string>& cmds) noexcept;

		void general_command(const std::vector<std::string>& cmds);

		virtual ~Program();

	private:
		std::vector<std::string> split_commands(const std::string& input);
		bool spawn_worker(int types,const std::vector<std::string>& cmds);
		void erase_worker(const std::string& file_name);
		std::unordered_map<std::string,pthread_t> _table;
		std::unordered_map<std::string,Info> _info;
};

class Worker
{
	public:
		Worker() : _period(10), _option(0), _maximum_file_numbers(100) { }

		Worker(const std::filesystem::path& p,int period,int opt,int mfn,int st,std::string_view cmd);

		void before_get_started(long long&,const std::string&);

		void operator()(); 

		long long get_time(int&,int&,int&,int&,int&,int&);

		std::string get_backup_time(int&,int&,int&,int&,int&,int&);

		virtual ~Worker() = default;

	private:
		std::filesystem::path _absolute_path;
		int _period;
		int _option; // bit masking
		int _maximum_file_numbers;
		int _time;
};

class Logger
{
	public:
		Logger() = default;

		virtual ~Logger() = default;

	private:
};

#endif
