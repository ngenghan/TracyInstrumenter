// TracyInstrumenter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include "TracyMethod.h"

using namespace std;

const char* SOURCENAV_DBDUMPPATH = ".\\SourceNavigator\\dbdump.exe";
const char* SOURCENAV_DBDUMPFILE = "D:\\NEH\\01 VS\\2005Panzer\\sn\\SNDB4\\panzer.fil";
const char* TRACYREGISTRYFILE = "tracyRegistry.txt";
const char* TRACY_INPUTFILE = ".\\SourceNavigator\\tracy_dbdump_fil.txt";

int _tmain(int argc, _TCHAR* argv[])
{
	//------------------------------------------------------
	//Perform dbdump of the SourceNavigator db4
	char command[250];  
	sprintf (command, "%s -s # \"%s\" > \"%s\"", SOURCENAV_DBDUMPPATH, SOURCENAV_DBDUMPFILE, TRACY_INPUTFILE);   
	system(command);

	//------------------------------------------------------
	//Perform preprocessing of dbdump
	STATUS stat;
	std::string fileName;
	TracyFile* tracyFile;
	TracyMethod *tracyMethod;
	unsigned int totalParse = 0;

	typedef std::map<string,TracyFile*> TracyRegistry;
	TracyRegistry tracyRegistry;
	TracyRegistry::iterator it;

	unsigned int uniqueCounter_= 1;
	std::ofstream ofs_registry;
	ofs_registry.open(TRACYREGISTRYFILE, std::ofstream::out | std::ofstream::trunc);

	//Init
	tracyRegistry.clear();
	
	//------------------------------------------------------
	//Read in SourceNavi output
	std::ifstream ifs;
	ifs.open(TRACY_INPUTFILE, std::ifstream::in);

	if (ifs.is_open()) 
	{
		string line;
		while(getline(ifs, line))
		{
			tracyMethod = new TracyMethod();
			TracyMethodWrapper wrapper(tracyMethod);

			stat = tracyMethod->populateData(SOURCENAV_FIL, line, fileName);
			if(stat != STAT_OK)
			{
				string stringPrint = (stat == STAT_SKIP)?"Skipped":"Error";
				std::cout<<stringPrint<<": ["<<line<<"]"<<std::endl;
				delete tracyMethod;
			}
			else
			{
				//Search if there are existing TracyFile already created
				it = tracyRegistry.find(fileName);
				if (it == tracyRegistry.end())
				{
					tracyFile = new TracyFile(fileName);
					tracyRegistry.insert(std::pair<string,TracyFile*>(fileName,tracyFile));
				}
				else
				{
					tracyFile = tracyRegistry[fileName];
				}

				tracyFile->addMethod(tracyMethod);

				totalParse++;
			}
		}
		ifs.close();
	}
	else
	{
		std::cout<<"Failed to open file"<<std::endl;
	}

	//------------------------------------------------------
	//Time to instrument the codes
	if(totalParse > 0)
	{
		for (it=tracyRegistry.begin(); it!=tracyRegistry.end(); ++it)
		{
			tracyFile = it->second;

			std::cout <<"Instrumenting ["<< it->first << "] Total= " <<tracyFile->getNumMethod() <<std::endl;
			tracyFile->instrument(&ofs_registry, uniqueCounter_);
		}
	}

	ofs_registry.close();


	return 0;
}

