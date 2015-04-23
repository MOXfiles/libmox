/*
 *  Thread.h
 *  MoxFiles
 *
 *  Created by Brendan Bolles on 4/20/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#ifndef MOXFILES_THREAD_H
#define MOXFILES_THREAD_H

#include <IlmThread.h>
#include <IlmThreadMutex.h>
#include <IlmThreadPool.h>
#include <ImfThreading.h>

namespace MoxFiles
{
	using IlmThread::Thread;
	using IlmThread::Mutex;
	using IlmThread::Lock;
	using IlmThread::ThreadPool;
	using IlmThread::Task;
	using IlmThread::TaskGroup;
	
	using IlmThread::supportsThreads;
	using Imf::setGlobalThreadCount;

} // namespace

#endif // MOXFILES_THREAD_H
