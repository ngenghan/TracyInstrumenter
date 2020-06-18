#include "TracyMethod.h"

//---------------------------------------------


//---------------------------------------------
TracyDecoder::TracyDecoder()
{
	tracyThreadCtr_.clear();
}

//---------------------------------------------
TracyDecoder::~TracyDecoder()
{
	emptyOut();
}
//---------------------------------------------
void TracyDecoder::emptyOut()
{
	TracyThreadCtrIt it = tracyThreadCtr_.begin();
	for(it;it != tracyThreadCtr_.end();++it)
	{
		delete it->second;
	}
}
//---------------------------------------------
void TracyDecoder::decodeAllBinFiles(string inPath, string outPath)
{
	string outPathUpd;
	unsigned long timerStart;
	vector<wstring> files;
	wstring pathWstr(inPath.begin(),inPath.end());
	emptyOut();

	//---------------------------------------------
	//First find all bin files in the path
    if (findAllBinFiles(pathWstr, L"*.bin", files)) 
	{
		std::size_t found;
		TracyFuncStats* tracyFuncStats = new TracyFuncStats();
		unsigned int counter = 0;
		std::cout<<"-> Total "<<files.size()<<" (.bin) found"<<std::endl;

		//---------------------------------------------
		//Loop through all bin files in the path
        for (vector<wstring>::iterator it = files.begin(); it != files.end(); ++it) 
		{
			string outStr((*it).begin(),(*it).end());
			string threadName;

			//Find the thread name from the file name
			found = outStr.find_last_of("\\");
			if (found != std::string::npos)
			{
				threadName = outStr.substr(found+1,outStr.length() - 1);
				outPathUpd = outPath + threadName;
				outPathUpd.replace(outPathUpd.length()-3, 3, "csv");

				found = threadName.find_last_of("_");
				if (found != std::string::npos)
				{
					threadName = threadName.substr(0,found);
				}
			}
			else
			{
				std::cout<<"--> Cannot find thread name. Skip this file."<<std::endl;
				continue;
			}

			//Decode the file 
			std::cout<<"--> Parsing #"<<++counter<<"/"<<files.size()<<" File=["<<outStr<<"]"<<std::endl;
            decode(outStr, tracyFuncStats, timerStart);

			//Output the file to CSV
			writeToFile(outPathUpd, tracyFuncStats, timerStart);
        }
		delete tracyFuncStats;
    }
	else
	{
		std::cout<<"--> Cannot find .bin files"<<std::endl;
		return;
	}
}
//---------------------------------------------
bool TracyDecoder::findAllBinFiles(wstring path, wstring mask, vector<wstring>& files)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;
    wstring spec;
    stack<wstring> directories;

    directories.push(path);
    files.clear();

    while (!directories.empty()) 
	{
        path = directories.top();
        spec = path + L"\\" + mask;
        directories.pop();

        hFind = FindFirstFile(spec.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE)  {
            return false;
        } 

        do {
            if (wcscmp(ffd.cFileName, L".") != 0 && 
                wcscmp(ffd.cFileName, L"..") != 0) {
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    directories.push(path + L"\\" + ffd.cFileName);
                }
                else {
                    files.push_back(path + L"\\" + ffd.cFileName);
                }
            }
        } while (FindNextFile(hFind, &ffd) != 0);

        if (GetLastError() != ERROR_NO_MORE_FILES) {
            FindClose(hFind);
            return false;
        }

        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

    return true;
}
//---------------------------------------------
void TracyDecoder::decode(string fileToDecode, TracyFuncStats* tracyFuncStats, unsigned long& firstTime)
{
	bool isFirst = true;
	unsigned long delayTimer = ::GetCurrentTime();
	unsigned long timerChk;
	char dat[TRACYMSG_SIZE];
	unsigned int parseMsg = 0;
	unsigned int bytesLeft = 0;
	unsigned int totalMsg = 0;

	//Open stream to read the bin file
	std::ifstream ifs;
	ifs.open(fileToDecode.c_str(), std::ifstream::in| std::ios::binary);

	if(ifs.is_open())
	{
		//Get the file size
		ifs.seekg (0, ifs.end);
		bytesLeft = ifs.tellg();
		ifs.seekg (0, ifs.beg);
		totalMsg = bytesLeft/TRACYMSG_SIZE;

		TracyMsg msg;
		
		//Loop through sets of msg in file
		while(bytesLeft >= TRACYMSG_SIZE)
		{
			memset(&msg,0,sizeof(TracyMsg));
			memset(&dat,0,TRACYMSG_SIZE);

			//Read the msg and populate to template
			ifs.read(dat, TRACYMSG_SIZE);
			
			memcpy(&msg.header_,&dat[TRACYMSG_HEADER_OFFSET],TRACYMSG_HEADER_LEN);
			memcpy(&msg.uId_,&dat[TRACYMSG_UID_OFFSET],TRACYMSG_UID_LEN);
			memcpy(&msg.time_,&dat[TRACYMSG_TIME_OFFSET],TRACYMSG_TIME_LEN);
			memcpy(&msg.indent_,&dat[TRACYMSG_INDENT_OFFSET],TRACYMSG_INDENT_LEN);

			bytesLeft -= TRACYMSG_SIZE;

			//Validate the msg
			if(checkMsg(msg))
			{
				//Track the 1st msg and used its time to validate all other msg should be incremental 
				if(isFirst)
				{
					firstTime = msg.time_;
					timerChk = msg.time_;
					isFirst = false;
				}
				else
				{
					//Check that subsequent msg are incremental in time by 1
					if(msg.time_ != (timerChk + 1))
					{
						std::cout<<"---> Error in timer sequence"<<std::endl;
					}
					timerChk = msg.time_;
				}

				//Print out to console for update each sec
				if((::GetCurrentTime() - delayTimer) > 1000)
				{
					delayTimer = ::GetCurrentTime();
					printf("\r---> Decoding msg#%lu/%lu (%3.5f%%)",parseMsg,totalMsg,(float)((float)parseMsg/(float)totalMsg*100.0f));
				}

				//Track the no. of msg that are parsed
				parseMsg++;

				//Save the msg into the container
				tracyFuncStats->push_back(std::pair<unsigned short,unsigned short>(msg.uId_,msg.indent_));
			}
			//The msg does not have a valid header
			else
			{
				std::cout<<"---> Error in finding header of msg"<<std::endl;

				char temp;
				unsigned int skipCount = 0;
				while((ifs.peek() != '#') && (bytesLeft > 0))
				{
					ifs.read(&temp, 1);
					bytesLeft--;
					skipCount++;
				}

				std::cout<<"---> Skipped "<<skipCount<<" msg"<<std::endl;
				if(bytesLeft == 0)
				{
					std::cout<<"---> Skipped till end"<<std::endl;
				}
				
			}
		}

		printf("\r---> Decoding msg#%lu/%lu (%3.5f%%)\n",parseMsg,totalMsg,(float)((float)parseMsg/(float)totalMsg*100.0f));

		if(bytesLeft != 0)
		{
			std::cout<<"---> Error in parsing complete file"<<std::endl;
		}
	}
	ifs.close();
}
//---------------------------------------------
void TracyDecoder::writeToFile(string fileToWrite, TracyFuncStats* tracyFuncStats, unsigned long firstTime)
{
	//-----------------------------------------------
	//To output decoded to CSV for offline processing
	unsigned long timeCounter = 0;
	unsigned long delayTimer = ::GetCurrentTime();

	std::cout<<"---> Writing to CSV file["<<fileToWrite<<"]..."<<std::endl;

	//Open stream to write the output CSV
	std::ofstream ofs_csv;
	ofs_csv.open(fileToWrite.c_str(), std::ofstream::out | std::ofstream::trunc);

	//Write the header
	ofs_csv << "Time,Uid,Indent"<<std::endl;

	//Loop through all the Functions saved
	TracyFuncStatsIt start2 = tracyFuncStats->begin();
	for(start2;start2!= tracyFuncStats->end();++start2)
	{
		timeCounter++;

		//[Time][UiD][Indent]
		ofs_csv << firstTime++ <<","<< start2->first <<"," << start2->second<<std::endl;

		//Print out to console for update each sec
		if((::GetCurrentTime() - delayTimer) > 1000)
		{
			delayTimer = ::GetCurrentTime();
			printf("\r---> Writing Time#%lu/%lu (%3.5f%%)",timeCounter,tracyFuncStats->size(),(float)((float)timeCounter/(float)tracyFuncStats->size()*100.0f));
		}
	}
	printf("\r---> Writing Time#%lu/%lu (%3.5f%%)\n",timeCounter,tracyFuncStats->size(),(float)((float)timeCounter/(float)tracyFuncStats->size()*100.0f));
	
	ofs_csv.close();
}
//---------------------------------------------
bool TracyDecoder::checkMsg(TracyMsg msg)
{
	bool stat = false;
	if(msg.header_ == '#')
		stat = true;

	return stat;
}

//---------------------------------------------
TracyMethod::TracyMethod()
{
	clear();
}

//---------------------------------------------
TracyMethod::~TracyMethod()
{
}

//---------------------------------------------
void TracyMethod::clear()
{
	name_ = "";
	line_ = 0;
	colStart_ = 0;
}
//---------------------------------------------
string TracyMethod::getName()
{
	return name_;
}
//---------------------------------------------
unsigned int TracyMethod::getLine()
{
	return line_;
}
//---------------------------------------------
STATUS TracyMethod::populateData(SOURCENAVTYPE type, string line, string &fileName)
{
	STATUS status = STAT_NOTOK;

	switch(type)
	{
	//-------------------------------------
	case SOURCENAV_FU:
		status = populateData_FU(line, fileName);
		break;
	//-------------------------------------
	case SOURCENAV_FIL:
		status = populateData_FIL(line, fileName);
		break;
	//-------------------------------------
	default:
		break;
	}

	return status;
}
//---------------------------------------------
STATUS TracyMethod::populateData_FU(string line, string &fileName)
{
	STATUS status = STAT_NOTOK;
	std::size_t nextDat = 0;

	//-------------------------------
	//1) Find the method name
	std::size_t found = line.find_first_of(" ", nextDat);
	if(found == std::string::npos)
	{
		return status;
	}
	name_ = line.substr(nextDat, found);
	nextDat = (found+1);
	//-------------------------------
	//2) Find the line
	found = line.find_first_of(".", nextDat);
	if(found == std::string::npos)
	{
		return status;
	}
	line_ = atoi(line.substr(nextDat, found-nextDat).c_str());
	nextDat = (found+1);
	//-------------------------------
	//3) Find the column
	found = line.find_first_of(" ", nextDat);
	if(found == std::string::npos)
	{
		return status;
	}
	colStart_ = atoi(line.substr(nextDat, found-nextDat).c_str());
	nextDat = (found+1);
	//-------------------------------
	//3) Find the pathFileName_
	found = line.find_first_of("#", nextDat);
	if(found == std::string::npos)
	{
		return status;
	}
	fileName = line.substr(nextDat, found-nextDat);
	//-------------------------------
	status = STAT_OK;
	return status;
}
//---------------------------------------------
STATUS TracyMethod::populateData_FIL(string line, string &fileName)
{
	STATUS status = STAT_NOTOK;
	std::size_t found = 0;
	std::size_t nextDat = 0;
	bool isCpp = true;

	//-------------------------------
	//1) Find the pathFileName_
	found = line.find(".cpp ", nextDat);
	if(found == std::string::npos)
	{
		found = line.find(".h ", nextDat);
		if(found == std::string::npos)
		{
			return status;
		}
		isCpp = false;
	}
	fileName = line.substr(nextDat, found-nextDat+(isCpp?4:2));
	nextDat = (found + 1 + (isCpp?4:2));
	//-------------------------------
	//2) Find the line
	found = line.find_first_of(".", nextDat);
	if(found == std::string::npos)
	{
		return status;
	}
	line_ = atoi(line.substr(nextDat, found-nextDat).c_str());
	nextDat = (found+1);
	//-------------------------------
	//3) Find the column
	found = line.find_first_of(" ", nextDat);
	if(found == std::string::npos)
	{
		return status;
	}
	colStart_ = atoi(line.substr(nextDat, found-nextDat).c_str());
	nextDat = (found+1);
	//-------------------------------
	//4) Check if it is '#' or Class 
	if(line.at(nextDat) == '#')
	{
		//-------------------------------
		//4.1) '#'
		nextDat = (found+3);
		found = line.find_first_of("#", nextDat);

		string temp = line.substr(nextDat, found-nextDat);
		std::size_t tSize = temp.find_last_of(" ");
		if(tSize == std::string::npos)
		{
			return status;
		}

		name_ = temp.substr(0, tSize);

		string type = temp.substr(tSize+1, temp.length()-tSize-1);
		if(type.compare("fu") == 0)
		{
			status = STAT_OK;
		}
		else
		{
			status = STAT_SKIP;
		}
	}
	else
	{
		//-------------------------------
		//4.2) Class Name (NotUsed)
		found = line.find_first_of(" ", nextDat);
		if(found == std::string::npos)
		{
			return status;
		}
		nextDat = (found+1);
		//-------------------------------
		//4.3) Name
		found = line.find_first_of(" ", nextDat);
		if(found == std::string::npos)
		{
			return status;
		}
		name_ = line.substr(nextDat, found-nextDat);
		nextDat = (found+1);
		//-------------------------------
		//4.3) type (NotUsed)
		found = line.find_first_of("#", nextDat);
		if(found == std::string::npos)
		{
			return status;
		}
		string typeName = line.substr(nextDat, found-nextDat);
		if(typeName.compare("mi") == 0)
		{
			status = STAT_OK;
		}
		else
		{
			status = STAT_SKIP;
		}
	}
	//-------------------------------
	return status;
}
//---------------------------------------------
bool TracyMethodWrapper::operator<(const TracyMethodWrapper &rhs) const 
{ 
	return (ptr_->getLine() < rhs.ptr_->getLine()); 
}
//---------------------------------------------
TracyFile::TracyFile(string fileName)
{
	clear();
	pathFileName_ = fileName;

	mapParam_.insert( std::pair<string,int>("bool", 1));
	mapParam_.insert( std::pair<string,int>("char", 1));
	mapParam_.insert( std::pair<string,int>("unsigned char", 1));
	mapParam_.insert( std::pair<string,int>("byte", 1));
	mapParam_.insert( std::pair<string,int>("short", 2));
	mapParam_.insert( std::pair<string,int>("unsigned short", 2));
	mapParam_.insert( std::pair<string,int>("int", 4));
	mapParam_.insert( std::pair<string,int>("unsigned int", 4));
	mapParam_.insert( std::pair<string,int>("DWORD", 4));
	mapParam_.insert( std::pair<string,int>("UDWORD", 4));
	mapParam_.insert( std::pair<string,int>("float", 4));
	mapParam_.insert( std::pair<string,int>("double", 8));
	mapParam_.insert( std::pair<string,int>("long", 8));
	mapParam_.insert( std::pair<string,int>("unsigned long", 8));
}

//---------------------------------------------
TracyFile::~TracyFile()
{
	clear();
}

//---------------------------------------------
void TracyFile::clear()
{
	pathFileName_ = "";

	std::set<TracyMethodWrapper>::iterator it;
	for (it=tracyMethodCtr_.begin(); it!=tracyMethodCtr_.end(); ++it)
	{
		delete (*it).ptr_;
	}
	tracyMethodCtr_.clear();
}
//---------------------------------------------
void TracyFile::addMethod(TracyMethodWrapper tracyMethodWrapper)
{
	tracyMethodCtr_.insert(tracyMethodWrapper);
}
//---------------------------------------------
unsigned int TracyFile::getNumMethod()
{
	return (unsigned int)tracyMethodCtr_.size();
}
//---------------------------------------------
bool TracyFile::instrument(std::ofstream* registry, unsigned int &uniqueCounter)
{
	bool status = false;
	string line;
	std::set<TracyMethodWrapper>::iterator it;
	std::vector<string> paramList;
	paramList.clear();

	//-------------------------------
	std::size_t found,nextDat; 
	unsigned int currLine = 1;
	unsigned int lineToSkip;

	std::ifstream ifs;
	ifs.open(pathFileName_.c_str(), std::ifstream::in);
	//-------------------------------
	std::string newFile = pathFileName_;
	newFile.insert(newFile.find_last_of("."),"_TRACY");
	std::ofstream ofs;
	ofs.open(newFile.c_str(), std::ofstream::out | std::ofstream::trunc);
	//-------------------------------
	if (ifs.is_open() && ofs.is_open()) 
	{
		for (it=tracyMethodCtr_.begin(); it!=tracyMethodCtr_.end(); ++it)
		{
			//---------------------------
			lineToSkip = (*it).ptr_->getLine() - currLine;
			for(unsigned int i=1;i<=lineToSkip;i++)
			{
				currLine++;
				getline(ifs, line);
				ofs << line <<std::endl;
			}

			//-------------------------------
			getline(ifs, line);//Line of interest
			
			//Check that the method name is found inside the line
			found = line.find((*it).ptr_->getName());
			if(found == std::string::npos)
			{
				return status;
			}

			string param;
			bool searchingStill = true;
			std::size_t paramComma, paramClose;
			std::size_t isFoundChar = std::string::npos;

			//----------------------------------------------
			//Search for next '('
			paramList.clear();
			nextDat = found + (*it).ptr_->getName().length();
			isFoundChar = browseToFind("(", &ifs, &ofs, line, currLine, nextDat);

			while(searchingStill)
			{
				paramClose = line.find_first_of(")",isFoundChar);
				paramComma = line.find_first_of(",",isFoundChar);
				if((paramComma == std::string::npos) || (paramComma > paramClose))
				{
					//No "," found, check if no parameters
					//paramClose = line.find_first_of(")",isFoundChar);

					if(paramClose == std::string::npos)
					{
						//No "," or ")" found, check next line
						isFoundChar = 0;
						getline(ifs, line);
					}
					else
					{
						//Found ")" and no input param
						if((paramClose - isFoundChar) == 1)
						{
							//There is no input parameter
						}
						//Exist last input params
						else
						{
							param = line.substr(isFoundChar+1, (paramClose - isFoundChar - 1));
							paramList.push_back(param);
						}
						nextDat = paramClose;
						searchingStill = false;
					}
				}
				//Found a ","
				else
				{
					param = line.substr(isFoundChar+1, (paramComma - isFoundChar - 1));
					
					//Ensure that next char is valid
					if((paramComma + 1) <= line.length())
					{
						isFoundChar = paramComma + 1;
					}
					else
					{
						isFoundChar = 0;
						getline(ifs, line);
					}
					paramList.push_back(param);
				}
			}
					

			//----------------------------------------------
			//Search for next '{'
			isFoundChar = browseToFind("{", &ifs, &ofs, line, currLine, nextDat);

			if(isFoundChar != std::string::npos)
			{
				std::ostringstream oss;
				//oss << "TRACY_VFUNC(L5," << uniqueCounter << ");";
				oss << "TRACY_VFUNC(L5," << uniqueCounter;

				std::size_t spaceIdx;
				string currParamType;
				string currParamName;
				string currParam;
				std::map<string, int>::iterator mapIt;
				for(unsigned int i=0;i<paramList.size();++i)
				{
					currParam = paramList[i];
					spaceIdx = currParam.find_last_of(" ");
					if(spaceIdx != std::string::npos)
					{
						currParamType = currParam.substr(0,spaceIdx);
						currParamName = currParam.substr(spaceIdx+1,currParam.size() - spaceIdx - 1);

						mapIt = mapParam_.find(currParamType);
						if(mapIt != mapParam_.end())
						{
							oss << "," << currParamName;
						}
					}
				}
				oss << ");";

				*registry << uniqueCounter << "," << pathFileName_ << "," <<(*it).ptr_->getName() << "," <<(*it).ptr_->getLine()<< "," <<oss.str()<<std::endl;
				uniqueCounter++;

				//Already instrumented
				if(line.find(oss.str()) != std::string::npos)
				{
					std::cout <<" -> Already instrumented Function["<< (*it).ptr_->getName() << "] Line["<<(*it).ptr_->getLine()<<"] with [" <<oss.str() <<"]"<<std::endl;
				}
				else
				{
					//'{' is the last char
					if((line.size()-1) == isFoundChar)
					{
						line.append(oss.str());
					}
					else
					{
						line.insert(isFoundChar+1, oss.str());
					}
				}

				
				//---------------------------
				//do{
					currLine++;
					ofs << line <<std::endl;
				//}while(getline(ifs, line));

				status = true;
			}
		}

		while(getline(ifs, line))
		{
			ofs << line <<std::endl;
		}
	}

	ifs.close();
	ofs.close();

	//-------------------------------
	//Perform the renaming
	string oriFile = pathFileName_;
	oriFile.insert(oriFile.find_last_of("."),"_ORI");
	if(rename(pathFileName_.c_str(), oriFile.c_str()) == 0)
	{
		rename(newFile.c_str(), pathFileName_.c_str());
	}

	return status;
}
//---------------------------------------------
std::size_t TracyFile::browseToFind(string toFind, std::ifstream* ifsP, std::ofstream* ofsP, string &line, unsigned int &currLine, std::size_t &nextDat)
{
	std::size_t found = std::string::npos; 
	do{
		found = line.find_first_of(toFind, nextDat);
		if(found != std::string::npos)
		{
			break;
		}
		else
		{
			currLine++;
			(*ofsP) << line <<std::endl;
			nextDat = 0;
		}
	}while(getline(*ifsP, line));

	return found;
}