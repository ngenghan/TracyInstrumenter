#ifndef TRACYMETHOD_H
#define TRACYMETHOD_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <map>
#include <windows.h>
#include <vector>
#include <stack>

using namespace std;

enum SOURCENAVTYPE {SOURCENAV_FU, SOURCENAV_FIL};
enum STATUS {STAT_OK, STAT_NOTOK, STAT_SKIP};
const unsigned int TRACYMSG_SIZE = 8;
const unsigned int TRACYMSG_HEADER_OFFSET = 0;
const unsigned int TRACYMSG_HEADER_LEN = 1;
const unsigned int TRACYMSG_UID_OFFSET = 1;
const unsigned int TRACYMSG_UID_LEN = 2;
const unsigned int TRACYMSG_TIME_OFFSET = 3;
const unsigned int TRACYMSG_TIME_LEN = 4;
const unsigned int TRACYMSG_INDENT_OFFSET = 7;
const unsigned int TRACYMSG_INDENT_LEN = 1;
//---------------------------------------------
struct TracyMsg
{
	char header_;
	unsigned short uId_;
	unsigned long time_;
	char indent_;
};
//---------------------------------------------
class TracyDecoder
{
private:
	typedef std::vector<std::pair<unsigned short, unsigned short>> TracyFuncStats;
	typedef TracyFuncStats::iterator TracyFuncStatsIt;
	//typedef std::map<unsigned int, TracyFuncStats*> TracyFuncCtr;
	//typedef TracyFuncCtr::iterator TracyFuncCtrIt;
	typedef std::map<string, TracyFuncStats*> TracyThreadCtr;
	typedef TracyThreadCtr::iterator TracyThreadCtrIt;


	TracyThreadCtr tracyThreadCtr_;
	void emptyOut();
	bool checkMsg(TracyMsg msg);
	void decode(string fileToDecode, TracyFuncStats* tracyFuncStats, unsigned long& firstTime);
	bool findAllBinFiles(wstring path, wstring mask, vector<wstring>& files);
	void writeToFile(string fileToWrite, TracyFuncStats* tracyFuncStats, unsigned long firstTime);
	 
public:
    TracyDecoder();
	~TracyDecoder();
	
	void decodeAllBinFiles(string inPath, string outPath);
};
//---------------------------------------------
class TracyMethod
{
private:
	string name_;
	unsigned int line_;
	unsigned int colStart_;
 
public:
    TracyMethod();
	~TracyMethod();
  
	void clear();
	STATUS populateData(SOURCENAVTYPE type, string line, string &fileName);
	STATUS populateData_FU(string line, string &fileName);
	STATUS populateData_FIL(string line, string &fileName);
	string getName();
	unsigned int getLine();
	//bool operator<(const TracyMethod &tracyMethod) const;
};
class TracyMethodWrapper
{
public:
	TracyMethodWrapper(TracyMethod* ptr)
	{
		ptr_ = ptr;
	};
	~TracyMethodWrapper()
	{};

	TracyMethod* ptr_;
	bool operator<(const TracyMethodWrapper &rhs) const;
};
//---------------------------------------------
class TracyFile
{
private:
	string pathFileName_;
	std::set<TracyMethodWrapper> tracyMethodCtr_;
	std::map<string,int> mapParam_;
 
public:
    TracyFile(string filename);
	~TracyFile();
  
	void clear();
	void addMethod(TracyMethodWrapper);
	bool instrument(std::ofstream*, unsigned int&);
	unsigned int getNumMethod();
	std::size_t browseToFind(string toFind, std::ifstream* ifsP, std::ofstream* ofsP, string &line, unsigned int &currLine, std::size_t &nextDat);
};
 
#endif