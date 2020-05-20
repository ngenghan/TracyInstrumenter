#include "tracy.h"
#include "tracyRegistry.h"

using namespace std;
#if defined(__cplusplus) && defined(TRACY)
namespace Tracy
{
	void TracyThread::printX(TRACY_INDENT_BRANCHTYPE type, unsigned short uId, const char* ctlStr, ...)
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
		filer_.print(type, indentLevel_, uId, workingBuf_, increment);
	}
	void TracyThread::print(TRACY_INDENT_BRANCHTYPE type, unsigned short uId, const char* ctlStr)
	{
		filer_.print(type, indentLevel_, uId, const_cast<char*>(ctlStr), (int)::strle(ctlStr));
	}

	void Filer::print(TRACY_INDENT_BRANCHTYPE type, unsigned short indentLevel, unsigned short uId, char* dat, int datLen)
	{
		ifBEGIN_OBJECT_LOCK(CriticalSection)

		unsigned long timeNow = TRACY_PROLOG;
		if((BUFSET - currBufIndex_[currBuf_]) < (dataLen + SIZEHEADER))
		{
			if(bufSwapCount_ > 0)
			{
				flush(timeNow, true, true);
			}
			
			currBuf_ = (currBuf_ == 0)?1:0;
			currBufIndex_[currBuf_] = 0;
			bufSwapCount++;
		}
	
		buf_[currBuf_][currBufIndex_[currBuf]++] = '#';
		memcpy(&(buf_[currBuf_][currBufIndex_[currBuf_]]), &Id, sizeof(unsigned short));	currBufIndex_[currBuf]+=sizeof(unsigned short);
		memcpy(&(buf_[currBuf_][currBufIndex_[currBuf_]]), &timeNow, sizeof(unsigned long));	currBufIndex_[currBuf]+=sizeof(unsigned long);
		memcpy(&(buf_[currBuf_][currBufIndex_[currBuf_]]), &indentLevel, sizeof(unsigned short));	currBufIndex_[currBuf]+=sizeof(unsigned short);
		buf_[currBuf_][currBufIndex_[currBuf]++] = (char)type;
		buf_[currBuf_][currBufIndex_[currBuf]++] = '[';
		if(datLen > 0)
		{
			memcpy(&(buf_[currBuf_][currBufIndex_[currBuf_]]), dat, datLen);	currBufIndex_[currBuf]+=datLen;
		}
		buf_[currBuf_][currBufIndex_[currBuf]++] = ']';

		ifEND_LOCK
	}
	void Filer::flush(unsigned long timeNow, bool isForced, bool flushOtherBuf)
	{
		ifBEGIN_OBJECT_LOCK(CriticalSection)

		unsigned int bufToFlush = currBuf_;
		if(flushOtherBuf)
		{
			if(bufSwapCount_ > 0)
			{
				bufToFlush = (bufToFlush == 0)?1:0;
			}
			else
			{
				return;
			}
		}

		if((currBufIndex_[bufToFlush] > 0) &&
			(((lastWrote_ - timeNow) >= (WRITETIMEOUT + randomTimeOut_))
			|| isForced))
		{
			stream_.write(&(buf_[bufToFlush][0]), currBufIndex_[bufToFlush]);
			stream_.flush();
			currBufIndex_[bufToFlush] = 0;
			lastWrote_ = timeNow;
			bufSwapCount_--;
		}
	
		ifEND_LOCK
	}
}
#endif
			
