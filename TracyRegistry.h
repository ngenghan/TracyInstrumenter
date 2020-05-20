#ifndef TRACYREGISTRY_H_
#define TRACYREGISTRY_H_

#include <iostream>
#include <fstream>
#include <map>

using namespace std;
namespace TRACY
{
	typedef list<TracyThread*> TracyThreadCtr;
	typedef TracyThreadCtr::iterator TracyThreadCtrIt;

	const unsigned int BUFSIZE = 1000;

	class TracyRegistry
	{
	public:
		static TracyRegistry* instance();
	private:
		static TracyRegistry* instance_;
		mutable infra::os::Mutex mutex_;
		std::ofstream stream_;
		static TracyThreadCtr* pThreadCtr_;
		Thread theThread_;

		TracyRegistry();
		~TracyRegistry();
	public:
		static void flush();
		void add(TracyThread* thread);
		void remove(TracyThread* thread);
		static DWORD doCycle(LPVOID idPtr);
	};
}
#endif
