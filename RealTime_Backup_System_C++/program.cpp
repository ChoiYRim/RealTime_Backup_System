#include "header.h"

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

	return sr1 < sr2;
}

void Program::start()
{
	system("clear");
	_logger = std::make_shared<Logger>(); // logger class 생성
	make_backup_directory();
	while(true)
	{
		std::string input = "";

		std::cout << "> ";
		std::getline(std::cin,input);	

		auto commands = split_commands(input);
		if(commands.size() <= 0)
			continue;

		std::string command = commands[0];

		if(command == "exit") break;
		else if(command == "add") add(commands);
		else if(command == "remove") remove(commands);
		else if(command == "compare") 
		{
			if(!compare(commands))
				std::cout << "two files are same..." << std::endl;
		}
		else if(command == "recover") recover(commands);
		else if(command == "list") list(commands);
		else if(command == "clear") system("clear");
		else general_command(commands);
	}
}

void Program::make_backup_directory()
{
	if(std::filesystem::exists("BACKUP")) // 프로그램이 실행되는 위치에 디렉토리 생성
		return;
	std::filesystem::create_directory("BACKUP");	
}

std::vector<std::string> Program::split_commands(const std::string& input)
{
	std::vector<std::string> ret;
	std::stringstream stream(input);
	std::string word = "";

	if(input.length() == 0)
	{
		std::cerr << "input length must be over 0.\n";
		return ret;
	}

	while(std::getline(stream,word,' ')) ret.push_back(word);	
	return ret;
}

bool Program::spawn_worker(int types, const std::vector<std::string>& cmds)
{
	// option : add(m,n,t,d), remove(a), recover(n) -> 1,2,3,4,5,6
	int period = 0;
	int option = 0;
	int maximum_file_numbers = 0;
	int store_time = 0;
	std::filesystem::path path;

	if(types == ADD)
	{
		// add [file name] [period] ...
		if(cmds.size() < 3)
			return false;

		if(!get_add_options(period, option, maximum_file_numbers, store_time, path, cmds)) 
			return false;

		// if option d
		if(option & option_ad)
			return add_directory(path,period,option,maximum_file_numbers,store_time);
		
		if(_table.find(path.string()) != _table.end())
			return true;
	
		if(path.string().length() > 255) 
			return false;

		Worker worker(path, period, option, maximum_file_numbers, store_time, "add",_logger);
		_logger->write_log(path.string(), "added");
		std::thread thread(worker);
		_table[path.string()] = thread.native_handle();
		_info[path.string()] = Info{period,option,maximum_file_numbers,store_time};
		_workers[path.string()] = worker;
		thread.detach();
	}

	return true;
}

void Program::erase_worker(const std::string& file_name)
{
	auto it = _table.find(file_name);
	if(it != _table.end())
		pthread_cancel(it->second);
}

int Program::add(const std::vector<std::string>& cmds)
{
	if(!spawn_worker(ADD,cmds))
		return 1;
	return 0;
}

bool Program::get_add_options(int& period,int& option,int& maximum_file_numbers,int& store_time,std::filesystem::path& path,const std::vector<std::string>& cmds)
{
	if(cmds[1] == "-d")
	{
		if(cmds.size() < 4)
		{
			std::cerr << "invalid commands..." << std::endl;
			return false;
		}

		path = std::filesystem::path(cmds[2]);
		if(std::filesystem::exists(path) != true)
		{
			std::cerr << "file does not exist..." << std::endl;
			return false;
		}
		if(std::filesystem::is_directory(path) != true)
		{
			std::cerr << "file is not a directory..." << std::endl;
			return false;
		}

		path = std::filesystem::absolute(path);

		for(auto i = 0; i < cmds[3].length(); i++)
		{
			if(cmds[3][i] < '0' || cmds[3][i] > '9')
			{
				std::cerr << "period must be an integer over 0..." << std::endl;
				return false;
			}
		}
		period = stoi(cmds[3]);
		if(period <= 0) 
		{
			std::cerr << "period must be over 0..." << std::endl;
			return false;
		}

		// options
		option |= option_ad;
		for(auto i = 4; i < cmds.size(); i++)
		{
			std::string cmd = cmds[i];
			if(cmd[0] == '-') // option flag
			{
				if(cmd[1] != 'm' && cmd[1] != 'n' && cmd[1] != 't') // wrong flag
				{
					std::cerr << "invalid flag..." << std::endl;
					return false;
				}
				if(cmd[1] == 'm')
					option |= option_am;
				else if(cmd[1] == 'n')
				{
					option |= option_an;
					if(i+1 >= cmds.size())
					{
						std::cerr << "invalid commands..." << std::endl;
						return false;
					}
					for(auto j = 0; j < cmds[i+1].length(); j++)
					{
						if(cmds[i+1][j] < '0' || cmds[i+1][j] > '9') 
						{
							std::cerr << "invalid commands..." << std::endl;
							return false;
						}
					}
					maximum_file_numbers = stoi(cmds[i+1]);
					if(maximum_file_numbers <= 0)
					{
						std::cerr << "invalid commands..." << std::endl;
						return false;
					}
					i++;
				}
				else if(cmd[1] == 't')
				{
					option |= option_at;
					if(i+1 >= cmds.size()) 
					{
						std::cerr << "invalid commands..." << std::endl;
						return false;
					}
					for(auto j = 0; j < cmds[i+1].length(); j++)
					{
						if(cmds[i+1][j] < '0' || cmds[i+1][j] > '9')
						{
							std::cerr << "invalid commands..." << std::endl;
							return false;
						}
					}
					store_time = stoi(cmds[i+1]);
					i++;
				}
			}
			else
			{
				std::cerr << "invalid commands..." << std::endl;
				return false;
			}
		}
		return true;
	}

	path = std::filesystem::path(cmds[1]);
	if(std::filesystem::exists(path) != true)
	{
		std::cerr << "file does not exist..." << std::endl;
		return false;
	}

	path = std::filesystem::absolute(path);
	for(auto i = 0; i < cmds[2].length(); i++)
	{
		if(cmds[2][i] < '0' || cmds[2][i] > '9')
		{
			std::cerr << "period must be an integer over 0..." << std::endl;
			return false;
		}
	}
	period = stoi(cmds[2]);
	if(period <= 0)
	{
		std::cerr << "period must be over 0..." << std::endl;
		return false;
	}

	// options
	for(auto i = 3; i < cmds.size(); i++)
	{
		std::string cmd = cmds[i];
		if(cmd[0] == '-') // option flag
		{
			if(cmd[1] != 'm' && cmd[1] != 'n' && cmd[1] != 't' && cmd[1] != 'd') // wrong
			{
				std::cerr << "invalid flag..." << std::endl;
				return false;
			}
			if(cmd[1] == 'm')
				option |= option_am;
			else if(cmd[1] == 'n')
			{
				option |= option_an;
				if(i+1 >= cmds.size()) 
				{
					std::cerr << "invalid commands..." << std::endl;
					return false;
				}
				for(auto j = 0; j < cmds[i+1].length(); j++)
				{
					if(cmds[i+1][j] < '0' || cmds[i+1][j] > '9') 
					{
						std::cerr << "invalid commands..." << std::endl;
						return false;
					}
				}
				maximum_file_numbers = stoi(cmds[i+1]);
				if(maximum_file_numbers <= 0)
				{
					std::cerr << "invalid commands..." << std::endl;
					return false;
				}
				i++;
			}
			else if(cmd[1] == 't')
			{
				option |= option_at;
				if(i+1 >= cmds.size()) 
				{
					std::cerr << "invalid commands..." << std::endl;
					return false;
				}
				for(auto j = 0; j < cmds[i+1].length(); j++)
				{
					if(cmds[i+1][j] < '0' || cmds[i+1][j] > '9') 
					{
						std::cerr << "invalid commands..." << std::endl;
						return false;
					}
				}
				store_time = stoi(cmds[i+1]);
				i++;
			}
			else
			{
				option |= option_ad;
				if(std::filesystem::is_directory(path) != true) 
				{
					std::cerr << "invalid commands..." << std::endl;
					return false;
				}
			}
		}
		else
		{
			std::cerr << "invalid commands..." << std::endl;
			return false;
		}
	}
	
	if(!(option & option_ad))
	{
		if(std::filesystem::is_regular_file(path) != true) 
		{
			std::cerr << "file is not a regular file..." << std::endl;
			return false;
		}
	}

	return true;
}

bool Program::add_directory(const std::filesystem::path& path,int& period,int& option,int& maximum_file_numbers,int& store_time)
{
	std::filesystem::directory_iterator dit(path);
	while(dit != std::filesystem::end(dit))
	{
		const std::filesystem::directory_entry& entry = *dit;

		if(std::filesystem::is_regular_file(entry))
		{
			if(_table.find(entry.path().string()) != _table.end())
			{
				dit++;
				continue;
			}
			if(entry.path().string().length() > 255)
			{
				dit++;
				continue;
			}

			Worker worker(entry.path(), period, option, maximum_file_numbers, store_time, "add",_logger);
			_logger->write_log(entry.path().string(), "added");
			std::thread thread(worker);
			_table[entry.path().string()] = thread.native_handle();
			_info[entry.path().string()] = Info{period,option,maximum_file_numbers,store_time};
			_workers[entry.path().string()] = worker;
			thread.detach();
		}
		else if(std::filesystem::is_directory(entry))
		{
			if(!add_directory(entry, period, option, maximum_file_numbers, store_time)) 
				return false;
		}
		dit++;
	}

	return true;
}

int Program::remove(const std::vector<std::string>& cmds)
{
	if(cmds.size() < 2)
	{
		std::cerr << "Usage : remove [FILE NAME or OPTION]" << std::endl;
		return 1;
	}

	if(cmds[1] == "-a")
	{
		if(cmds.size() > 2)
		{
			std::cerr << "Usage : remove -a" << std::endl;
			return 1;
		}
		
		std::vector<std::string> delete_vector;

		for(auto& ele : _table)
			delete_vector.push_back(ele.first);
		for(const auto& ele : delete_vector)
		{
			erase_worker(ele);
			_logger->write_log(ele,"removed");
			_table.erase(ele);
			_info.erase(ele);
			_workers.erase(ele);
		}

		return 0;
	}

	if(cmds.size() > 2)
	{
		std::cerr << "Usage : remove [FILE NAME]" << std::endl;
		return 1;
	}

	std::string file_name = cmds[1];
	if(std::filesystem::exists(file_name) != true)
	{
		std::cerr << "file doesn't exist..." << std::endl;
		return 1;
	}

	std::filesystem::path target = std::filesystem::absolute(file_name);
	if(_table.find(target.string()) == _table.end() && _info.find(target.string()) == _info.end())
	{
		std::cerr << target.string() << " does not exist in backup list..." << std::endl;
		return 1;
	}
	erase_worker(target.string());
	_logger->write_log(target.string(),"removed");
	_table.erase(target.string());
	_info.erase(target.string());
	_workers.erase(target.string());
	return 0;
}

int Program::compare(const std::vector<std::string>& cmds)
{
	if(cmds.size() != 3)
	{
		std::cerr << "Usage : compare [FILE NAME1] [FILE NAME2]" << std::endl;
		return 1;
	}

	std::string file_name1 = cmds[1];
	std::string file_name2 = cmds[2];
	if(std::filesystem::exists(file_name1) != true)
	{
		std::cerr << "the first file does not exist..." << std::endl;
		return 1;
	}
	if(std::filesystem::exists(file_name2) != true)
	{
		std::cerr << "the seconds file does not exist..." << std::endl;
		return 1;
	}

	std::string tmp = "";
	std::string c1 = "",c2 = "";
	std::filesystem::path target1 = std::filesystem::absolute(file_name1);
	std::filesystem::path target2 = std::filesystem::absolute(file_name2);
	std::ifstream file1_contents(target1,std::ifstream::in);
	std::ifstream file2_contents(target2,std::ifstream::in);

	while(getline(file1_contents,tmp) && c1.length() <= 1024*1024) // 1MB까지 내용이 같으면 같다고 판단
		c1 += tmp;
	while(getline(file2_contents,tmp) && c2.length() <= 1024*1024)
		c2 += tmp;

	file1_contents.close();
	file2_contents.close();

	auto file1_size = std::filesystem::file_size(target1);
	auto file2_size = std::filesystem::file_size(target2);
	struct stat file1_stat_buf,file2_stat_buf;

	if(lstat(target1.string().c_str(),&file1_stat_buf) < 0)
		return 1;
	if(lstat(target2.string().c_str(),&file2_stat_buf) < 0)
		return 1;

	if(file1_size == file2_size && c1 == c2)
		return 0;

	/*
	std::cout << "file1 : [size : " << file1_size << " bytes]" 
		<< ", mtime : " << ctime(&file1_stat_buf.st_mtime);
	std::cout << "file2 : [size : " << file2_size << " bytes]" 
		<< ", mtime : " << ctime(&file2_stat_buf.st_mtime);
	*/
	return 1;
}

int Program::recover(const std::vector<std::string>& cmds)
{
	if(cmds.size() != 2 && cmds.size() != 4)
	{
		std::cerr << "invalid commands..." << std::endl;
		return 1;
	}

	std::string file_name = cmds[1];
	std::string new_file = "";
	
	if(cmds.size() == 4)
	{
		if(cmds[2] != "-n")
		{
			std::cerr << "invalid commands..." << std::endl;
			return 1;
		}
		new_file = cmds[3];
	}

	if(std::filesystem::exists(file_name) != true)
	{
		std::cerr << "file does not exist..." << std::endl;
		return 1;
	}
	if(cmds.size() > 2 && std::filesystem::exists(new_file) == true)
	{
		std::cerr << new_file << " already exist..." << std::endl;
		return 1;
	}
	if(std::filesystem::is_regular_file(file_name) != true)
	{
		std::cerr << "file is not a regular file..." << std::endl;
		return 1;
	}

	int idx = 1;
	int choose;
	auto backup_files = get_backup_files(file_name);

	if(backup_files.size() <= 0)
	{
		std::cerr << "there is no backup file..." << std::endl;
		return 1;
	}

	std::cout << "0. exit" << std::endl;
	for(const auto& backup_file : backup_files)
	{
		std::string tmp = get_relative_path(backup_file);
		std::cout << idx++ << ". " << tmp << " ";
		std::cout << "[" << std::filesystem::file_size(backup_file) << " bytes]" << std::endl;
	}
	std::cout << "Choose file to recover : ";
	std::cin >> choose;
	getchar();

	if(choose == 0 || choose-1 >= backup_files.size())
		return 0;

	std::string target = backup_files[choose-1];
	std::ifstream from(target);
	std::string contents = "";

	if(from.is_open() != true)
	{
		std::cerr << "failed to open " << target << std::endl;
		return 1;
	}

	if(new_file.length() > 0)
	{
		std::ofstream to(new_file);
		if(to.is_open())
		{
			while(getline(from,contents))
				to << contents << '\n';
			to.close();
		}
		else
		{
			std::cerr << "failed to open " << new_file << std::endl;
			from.close();
			return 1;
		}

		from.close();
		_logger->write_log(std::filesystem::absolute(file_name).string(),"recovered");
		return 0;
	}

	std::ofstream to(file_name);
	if(to.is_open())
	{
		while(getline(from,contents))
			to << contents << '\n';
		to.close();
	}
	else
	{
		std::cerr << "failed to open " << file_name << std::endl;
		from.close();
		return 1;
	}

	from.close();
	_logger->write_log(std::filesystem::absolute(file_name).string(),"recovered");
	return 0;
}

void Program::scan_backup_directory(const std::string& file_name, std::vector<std::string>& backup_files)
{
	std::filesystem::path backup(std::filesystem::current_path()/"BACKUP");
	std::filesystem::directory_iterator dit(backup);
	std::vector<std::string> v = {"compare",file_name};

	while(dit != std::filesystem::end(dit))
	{
		std::filesystem::directory_entry entry = *dit;
		std::string entry_relative_path = get_relative_path(entry.path().string());
		v.push_back(entry.path().string());

		if(compare(v) != 0)
		{
			dit++;
			v.pop_back();
			continue;
		}
		
		v.pop_back();
		backup_files.push_back(entry.path());
		dit++;
	}
}

std::vector<std::string> Program::get_backup_files(const std::string& file_name) noexcept
{
	std::vector<std::string> backup_files;
	scan_backup_directory(file_name, backup_files);
	std::sort(backup_files.begin(),backup_files.end(),comp);
	return backup_files;
}

int Program::list(const std::vector<std::string>& cmds) noexcept
{
	int idx = 0;

	for(const auto& ele : _table)
	{
		std::cout << "[" << std::setw(5) << std::right << idx++ << "] : ";
		std::cout << std::setw(70) << std::left << ele.first << " ";
		std::cout << _info[ele.first] << std::endl;
	}

	return 0;
}

void Program::general_command(const std::vector<std::string>& cmds)
{
	FILE* p = NULL;
	char args[kbuffer_length];
	int cmd_len = static_cast<int>(cmds.size());

	memset((char*)args,0,sizeof(args));
	sprintf(args, "%s ", cmds[0].c_str());
	for(int i = 1; i < cmd_len && cmds[i].length()+strlen(args) < kbuffer_length; i++)
		sprintf(args, "%s %s ", args, cmds[i].c_str());

	if(cmds[0] == "vi" || cmds[0] == "vim")
	{
		system(args);
		return;
	}

	if((p = popen(args,"r")) == NULL)
	{
		std::cerr << "popen() error\n" << std::endl;
		return;
	}

	memset((char *)args,0,sizeof(args));
	while(fgets(args,kbuffer_length,p) != NULL)
	{
		std::cout << args;
		memset((char *)args,0,sizeof(args));
	}
	std::cout << std::endl;
}

Program::~Program()
{
	// 현재 작동 중인 스레드 종료
	for(auto& ele : _table) erase_worker(ele.first);
}

std::ostream& operator<<(std::ostream& os,const Info& info)
{
	os << "[period : " << std::setw(3) << std::left << info._period << "] ";
	if(info._option & option_am)
		os << "[m option : true ] ";
	else
		os << "[m option : false] ";
	if(info._option & option_an)
		os << "[n option : " << std::setw(3) << std::left << info._maximum_file_numbers << "] ";
	else
		os << "[n option : " << std::setw(3) << std::left << "] ";
	if(info._option & option_at)
		os << "[t option : " << std::setw(4) << std::left << info._store_time << "] ";
	else
		os << "[t option : " << std::setw(4) << std::left << "] ";
	if(info._option & option_ad)
		os << "[d option : true ] ";
	else
		os << "[d option : false] ";
	return os;
}
