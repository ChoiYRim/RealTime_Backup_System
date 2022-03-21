#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

string str = "test1||test2||test3||test4";

vector<string> split(const string& s)
{
    vector<string> ret;
    char* tok;
    char* c_string = new char[s.length()];
    
    strcpy(c_string,s.c_str());
    tok = strtok(c_string,"||");
    //printf("%s\n", tok);
    ret.push_back(string(tok));
    while((tok = strtok(NULL,"||")) != NULL)
    {
        //printf("%s\n", tok);
        ret.push_back(string(tok));
    }
    
    delete[] c_string;
    return ret;
}

void print(const vector<string>& v)
{
    int idx = 0;
    for(const auto& ele : v)
        printf("[%d] : %s\n", idx++, ele.c_str());
}

int main(int argc,char* argv[])
{
    auto v = split(str.c_str());
    //print(v);
    return 0;
}
