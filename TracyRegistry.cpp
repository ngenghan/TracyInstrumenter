#include "tracyRegistry.h"

using namespace std;

namespace Tracy
{
	TracyRegistry* TracyRegistry::instance_ = NULL;
	TracyThreadCtr* TracyRegistry::pThreadCtr_ = NULL;

	TracyRegistry* TracyRegistry::instance()
	{
		if(instance_ == NULL)
		{
			ifBEGIN_LOCK(CriticalSection)
			if(instance_ == NULL)
				instance_ = new TracyRegistry();
			ifEND_LOCK
		}
		return instance_;
	}
	TracyRegistry::TracyRegistry()
	:mutex_("tracyMutex"), theThread_(doCycle)
	{
		char fileName[BUFSIZE];

		sprintf_s(fileName, BUFSIZE, "tracy_registry");
		stream_.open(fileName, std::ios::out|std::ios::trunc);

		pThreadCtr_ = new TracyThreadCtr();
		pThreadCtr_->clear();

		theThread_.resume();
	}
	TracyRegistry::~TracyRegistry()
	{		
		if(stream_.is_open())
		{
			stream_.close();
		}
	}
	void TracyRegistry::add(TracyThread* thread)
	{
		pThreadCtr_->push_back(thread);
	}
	void TracyRegistry::remove(TracyThread* thread)
	{
		TracyThreadCtrIt i,end=pThreadCtr_->end();
		for(i=pThreadCtr_->begin();i!=end();++i)
		{
			if((*i) == thread)
			{
				pThreadCtr_->erase(i);
			}
		}
	}
	void TracyRegistry::flush()
	{
		TracyThreadCtrIt i,end=pThreadCtr_->end();
		for(i=pThreadCtr_->begin();i!=end();++i)
		{
			pThreadCtr_->flush(false, true);
		}
	}
	DWORD TracyRegistry::doCycle(LPVOID idPtr)
	{
		unsigned int timeout = 2000 + (rand()%1000);

		while(true)
		{
			sleep(timeout);
			flush();
		}
		return 0;
	}
}
