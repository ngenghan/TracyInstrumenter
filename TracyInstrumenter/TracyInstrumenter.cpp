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
const char* TRACYREGISTRYFILE = ".\\Output\\tracyRegistry.txt";
const char* TRACYLOGFILE = ".\\Output\\tracyLog.txt";
const char* TRACY_INPUTFILE_INSTRUMENT = ".\\Output\\tracy_dbdump_fil.txt";
const char* TRACY_INPUTFILE_DECODE = ".\\Input\\";
const char* TRACY_OUTPUTFILE_DECODE = ".\\Output\\";

int main(int argc, char* argv[])
{
	string checker;
	char* sourceNavDumpFile = const_cast<char*>(SOURCENAV_DBDUMPFILE);

	/*if(argc != 2)
	{
		std::cout<<"Usage: "<<argv[0]<<" Path_to_SourceNav_ProjOutput_.fil"<<std::endl;
		std::cout<<"-> using default path["<< SOURCENAV_DBDUMPFILE <<"] for current run"<<std::endl;
	}
	else
	{
		checker = argv[1];
		if(checker.find(".fil") != std::string::npos)
		{
			std::fstream fs;
			fs.open(checker.c_str());
			if (fs.fail()) 
			{
				std::cout<<"Error: No such file found ["<< checker <<"]"<<std::endl;
				return -1;
			}
			else
			{
				sourceNavDumpFile = const_cast<char*>(checker.c_str());
			}
		}
		else
		{
			std::cout<<"Error: Wrong file type (.fil) ["<< checker <<"]"<<std::endl;
			return -1;
		}
	}*/

	//------------------------------------------------------
	std::cout<<"****************************************************"<<std::endl;
	std::cout<<"*                TRACY INSTRUMENTER/DECODER        *"<<std::endl;
	std::cout<<"****************************************************"<<std::endl;

	//------------------------------------------------------
	//Perform decoding of file
	sourceNavDumpFile = const_cast<char*>(TRACY_INPUTFILE_DECODE);
	std::cout<<"-> 1. Performing decoding of all bin files in ["<<sourceNavDumpFile<<"]..."<<std::endl;
	TracyDecoder tracyDecoder;
	tracyDecoder.decodeAllBinFiles(string(sourceNavDumpFile), string(TRACY_OUTPUTFILE_DECODE));



	//------------------------------------------------------
	//Perform dbdump of the SourceNavigator db4
	std::cout<<"-> 1. Performing dbdump of Source Navigator file["<<sourceNavDumpFile<<"]..."<<std::endl;
	char command[250];  
	sprintf_s(command, 250, "%s -s # \"%s\" > \"%s\"", SOURCENAV_DBDUMPPATH, sourceNavDumpFile, TRACY_INPUTFILE_INSTRUMENT);   
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
	std::ofstream ofs_log;
	ofs_log.open(TRACYLOGFILE, std::ofstream::out | std::ofstream::trunc);

	//Init
	tracyRegistry.clear();
	
	//------------------------------------------------------
	//Read in SourceNavi output
	std::cout<<"-> 2. Accessing output of dbdump["<<TRACY_INPUTFILE_INSTRUMENT<<"]..."<<std::endl;
	std::ifstream ifs;
	ifs.open(TRACY_INPUTFILE_INSTRUMENT, std::ifstream::in);

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
				ofs_log<<" -> "<<stringPrint<<": ["<<line<<"]"<<std::endl;
				delete tracyMethod;
			}
			else
			{
				//Search if there are existing TracyFile already created
				it = tracyRegistry.find(fileName);
				if (it == tracyRegistry.end())
				{
					std::cout<<" -> Creating new TracyFile["<<fileName<<"] for instrumenting recording..."<<std::endl;
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
	std::cout<<"-> 3. Starting to instrument files..."<<std::endl;
	if(totalParse > 0)
	{
		for (it=tracyRegistry.begin(); it!=tracyRegistry.end(); ++it)
		{
			tracyFile = it->second;

			std::cout <<" -> Instrumenting ["<< it->first << "] Total= " <<tracyFile->getNumMethod() <<std::endl;
			tracyFile->instrument(&ofs_registry, uniqueCounter_);
		}
	}

	ofs_registry.close();
	ofs_log.close();
	std::cout<<"-> 4. Done..."<<std::endl;

	return 0;
}

