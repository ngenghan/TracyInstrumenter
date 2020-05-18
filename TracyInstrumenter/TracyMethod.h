#ifndef TRACYMETHOD_H
#define TRACYMETHOD_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <set>

using namespace std;

enum SOURCENAVTYPE {SOURCENAV_FU, SOURCENAV_FIL};
enum STATUS {STAT_OK, STAT_NOTOK, STAT_SKIP};
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
 
public:
    TracyFile(string filename);
	~TracyFile();
  
	void clear();
	void addMethod(TracyMethodWrapper);
	bool instrument(std::ofstream*, unsigned int&);
	unsigned int getNumMethod();
};
 
#endif