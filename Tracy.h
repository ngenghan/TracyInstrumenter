#ifndef __TRACY_H
#define __TRACY_H

#if defined(__cplusplus) && defined(TRACY)

#include <windows.h>
#include <fstream>
#include "ifosLock.h"

struct TRACY_VOID	{bool val;};
enum TRACY_LEVEL	{L0=0,L1,L2,L3,L4,L5};
enum TRACY_INDENT_BRANCHTYPE	{INDENT_NA=32, INDENT_STARTBRANCH='{', INDENT_MIDBRANCH='|', INDENT_PARAMBRANCH='@',INDENT_ENDBRANCH='}'};

const unsigned int BUFSIZE = 1024;
const unsigned int TRACY_FUNCLEVEL_DEF = L5;
const unsigned int TRACY_PARAMLEVEL_DEF = L5;
const unsigned int BUFNUM = 1024 * BUFSIZE;
const unsigned int MAXFILENAME = 250;
const unsigned int SIZEHEADER = 3 + sizeof(unsigned long) + (2 * sizeof(unsigned short)) + sizeof(char);
const unsigned int FILER_BUFCOUNT = 2;

#define TRACY_PROLOG ::GetTickCount()
#define TRACY_GETTHREAD Tracy::TrachThread* = infra::app::getCurrentThread()->getTracyThread();

#define TRACY_SET_INSTRUMENT_LEVEL(F,P) \
	TRACY_GETTHREAD \
	tracyThread->setFuncLevel(F); \
	tracyThread->setParamLevel(P);

#define TRACY_VFUNC(level, uId) \
	TRACY_GETTHREAD \
	TRACY_VFUNC_(tracyThread, level, uId, TRACY_VOID)

namespace TRACY
{
class Filer
{
private:
	ifDECLARE_LOCK(CriticalSection)
	unsigned int randomTimeOut_;
	unsigned int bufSwapCount_;
	unsigned int currBuf_;
	unsigned int currBufIndex_[FILER_BUFCOUNT];
	std::ofstream stream;
	char buf_[FILER_BUFCOUNT][BUFSET];
	unsigned long lastWrote_;

public:
	Filer()
	:currBuf_(0),lastWrote_(0),bufSwapCount_(0)
	{
		randomTimeOut_ = rand()%1000;
		
		memset(&(currBufIndex_[0]),0,FILER_BUFCOUNT*sizeof(unsigned int));
		memset(&(buf_[0][0]),0,BUFSET*FILER_BUFCOUNT);
	}
	~Filer()
	{
		if(stream_.is_open())
		{
			stream_.close();
		}
	}
	void init(const char* qName)
	{
		char fileName[BUFSIZE];
		sprintf_s(fileName, BUFSIZE, "tracy_%s",qName);
		fileName[MAXFILENAME-1] = '\0';

		stream_.open(fileName, std::ios::out|std::ios::trunc|std::ios::binary);
	}
	void print(TRACY_INDENT_BRANCHTYPE type, unsigned short indentLevel, unsigned int uId, char* dat, int datLen);
	voiod flush(unsigned long timeNow, bool isForced, bool flushOtherBuf);
	
};

class TracyThread
{
private:
	Filer filer_;
	char workingBuf_[BUFSIZE];
	unsigned short funcLevel_;
	unsigned short paramLevel_;
	unsigned short indentLevel_;
public:
	TracyThread(const char* qName)
	:funcLevel_(TRACY_FUNCLEVEL_DEF),
	paramLevel_(TRACY_PARAMLEVEL_DEF),
	indentLevel_0)
	{
		filer_.init(qName);

	}
	~TracyThread()
	{
	}
	unsigned short getFuncLevel()	{return funcLevel_;}
	unsigned short getParamLevel()	{return paramLevel_;}
	void setFuncLevel(unsigned short val)	{funcLevel_ = val;}
	void setParamLevel(unsigned short val)	{paramLevel_ = val;}
	void decrIndent()	{indentLevel_--;}
	void incrIndent()	{indentLevel_++;}
	void flush(bool isForced, bool flushOtherBuf)
	{
		unsigned long timeNow = TRACY_PROLOG;
		filer_.flush(timeNow, isForced, flushOtherBuf);
	}
	void printX(TRACY_INDENT_BRANCHTYPE type, unsigned short uId, const char* ctlStr, ...);
	void print(TRACY_INDENT_BRANCHTYPE type, unsigned short uId, const char* ctlStr);
};

template <typename T>
class TracyTracer
{
private:
	char workingBuf_[BUFSIZE];
	unsigned short uId_;
	unsigned short m_level;
	T* m_pRetVal;
	Tracy::TracyThread* tracyThread_;
public:
	TracyTracer(Tracy::TracyThread* tracyThread, unsigned short level, unsigned short uId):
	m_pRetVal(NULL),
	m_level(level),
	uId_(uId),
	tracyThread_(tracyThread)
	{
		if(tracyThread_->getFuncLevel() >= m_level)
		{
			tracyThread_->print(INDENT_STARTBRANCH, uId_, "");
			tracyThread_->incrIndent();
		}
	}
	TracyTracer(Tracy::TracyThread* tracyThread, unsigned short level, unsigned short uId, const char* ctlStr, ...):
	m_pRetVal(NULL),
	m_level(level),
	uId_(uId),
	tracyThread_(tracyThread)
	{
		if(tracyThread_->getFuncLevel() >= m_level)
		{
			memset(&(workingBuf_[0]),0,BUFSIZE);
			va_list argList;
			va_start(argList, ctlStr);
			int increment = vsprintf_s(workingBuf_, BUFSIZE, ctlStr, argList);
			va_end(argList);

			if(increment < 0)
			{
				memset(&(workingBuf_[0]),0,BUFSIZE);
				sprintf_s(&(workingBuf_[0]),BUFSIZE,"TracError: Err in arg(A)");
			}
			else if((increment + SIZEHEADER) >= BUFSIZE)
			{
				workingBuf_[BUFSIZE-1]='\0';
			}
		
			tracyThread_->print(INDENT_STARTBRANCH, uId_, workingBuf_);
			tracyThread_->incrIndent();
		}
	}	
	~TracyTracer(0
	{
		if(tracyThread_->getParamLevel() >= m_level || tracyThread_->getFuncLevel() >= m_level)
		{ 	
			tracyThread_->decrIndent();

			if(tracyThread-->getParamLevel() >= m_level)
			{
				if(m_pRetVal)
				{
					tracyThread_->printX(INDENT_ENDBRANCH, uId_, "[0x%x]", Return());
				}
			}
		}
	}
	void SetRetVal(T *retVal)
	{
		if(retVal)
		{
			m_pRetVal = retVal;
		}
	}
	T Return()
	{
		if(m_pRetVal)
		{
			return *m_pRetVal;
		}
		else
		{
			return T();
		}
	}
};
}
#else
#endif
#endif
