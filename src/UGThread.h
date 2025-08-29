/****************************************************************************************************************
 * filename     UGThread.h
 * describe     thread can be simple create by inherit this class
 * author       Created by dawson on 2019/04/19
 * Copyright    ?2007 - 2029 Sunvally. All Rights Reserved.
 ***************************************************************************************************************/

#ifndef _UGREEN_THREAD_H_
#define _UGREEN_THREAD_H_
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#ifndef _MSC_VER
#include <pthread.h>
#else
#include <Windows.h>
const uint32_t MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
  DWORD dwType;      // Must be 0x1000.
  LPCSTR szName;     // Pointer to name (in user addr space).
  DWORD dwThreadID;  // Thread ID (-1=caller thread).
  DWORD dwFlags;     // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
#endif
/* #ifdef ENABLE_LAZY_EVENT
#include "UGreenEvent.hpp"
#endif
#define THREAD_RETURN void*
typedef THREAD_RETURN (*routine_pt)(void *);
//#define ENABLE_PTHREAD

#ifdef ENABLE_PTHREAD
#include <pthread.h>
#ifdef _MSC_VER
#pragma comment(lib, "pthreadVC2.lib")
#endif
#else
#include <future>
#include <chrono>
#endif

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif*/

#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif

class UGThread
{
public:
    UGThread(const char *pname = NULL)
    {
        isRuning_      = 0;
        isExit_		= 0;

        setThreadName(pname);
    }

    virtual ~UGThread()
    {
        stop();
        threadHandle_.reset();
    }

    virtual int threadProc() = 0;

    virtual int onThreadStart()
    {
        return 0;
    }

    virtual void onThreadStop() {}

    virtual int run(int priority = 0, int bsystem = 0)
    {
        int ret = 0;
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if(!isRuning_)
        {
            ret = create(priority, bsystem);
            isRuning_ = 1;
        }

        return ret;
    }

    int stop()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
       if(isRuning_)
       {
            isRuning_ = 0;
            if(threadHandle_ && threadHandle_->joinable())
            {
                threadHandle_->join();
            }
       }
        
        return 0;
    }

    bool isAlive()
    {
        return isRuning_;
    }

    void threadSleep(int ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    void setThreadName(const char *pname)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if(pname)
        {
            threadName_ = pname;
        }
        else if(!threadName_.empty())
        {
            threadName_.clear();
        }
    }

private:
    int create(int priority = 0, int bsystem = 0)
    {
        if (!isRuning_)
        {
            threadHandle_ = std::make_shared<std::thread>(UGThread::initThreadProc, this);
            threadHandle_->get_id();
            return 0;
        }

        return -1;
    }

    static void initThreadProc(void *arg)
    {
        UGThread *pthis = static_cast<UGThread *>(arg);
        assert(pthis);
        pthis->isRuning_ = 1;
#ifdef _MSC_VER
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = pthis->threadName_.c_str();
        info.dwThreadID = -1;
        info.dwFlags = 0;
        __try {
          RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
#else
        pthread_setname_np(pthread_self(), pthis->threadName_.c_str());
#endif

        int ret = pthis->onThreadStart();
        if(ret == 0)
        {
            pthis->threadProc();
        }
        pthis->onThreadStop();
        pthis->isRuning_ = 0;
        pthis->isExit_ = 1;
    }

protected:
    std::atomic<bool>   isRuning_;

    std::atomic<bool>	isExit_;
    std::recursive_mutex    mutex_;
    std::string     threadName_;
    std::shared_ptr<std::thread>     threadHandle_;
};
#endif