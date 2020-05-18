#include "TracyMethod.h"

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

			//Search for next '{'
			nextDat = found + (*it).ptr_->getName().length();
			
			bool isFound = false;
			do{
				found = line.find_first_of("{", nextDat);
				if(found != std::string::npos)
				{
					isFound = true;
					break;
				}
				else
				{
					currLine++;
					ofs << line <<std::endl;
					nextDat = 0;
				}
			}while(getline(ifs, line));
			
			if(isFound)
			{
				std::ostringstream oss;
				oss << "TRACY_PRINT(" << uniqueCounter << ");";

				*registry << uniqueCounter << " : File=" << pathFileName_ << " Function=" <<(*it).ptr_->getName() << " Line=" <<(*it).ptr_->getLine()<< " Added=" <<oss.str()<<std::endl;
				uniqueCounter++;

				//'{' is the last char
				if((line.size()-1) == found)
				{
					line.append(oss.str());
				}
				else
				{
					line.insert(found+1, oss.str());
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

		ifs.close();
		ofs.close();
	}
	return status;
}