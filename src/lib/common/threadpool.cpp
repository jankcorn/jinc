/***************************************************************************
                          threadpool.cpp  -  description
                             -------------------
    begin                : Tue Jan 06 2009
    copyright            : (C) 2009 by Joern Ossowski
    email                : mail@jossowski.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <common/threadpool.hpp>
#include <common/helper.hpp>

ThreadPool::ThreadPool(const unsigned long noThreads) : stop(false), activeThreads(0), numberOfThreads(noThreads){
  if(numberOfThreads>0){
    for(unsigned long i=0;i<numberOfThreads;++i){
      threads.create_thread(SingleThread(*this));
    }
  }
}

ThreadPool::~ThreadPool(){
  {
    boost::mutex::scoped_lock lock(mutex);
    stop=true;
  }
  
  needThread.notify_all();
  threads.join_all();

  assert(activeThreads==0);
}

void ThreadPool::schedule(boost::function<void ()> function){
  boost::mutex::scoped_lock lock(mutex);
  
  if(numberOfThreads>0){
    functionQueue.push_front(function);
    needThread.notify_one();
  } else {
    function();
  }
}

void ThreadPool::wait(){
  boost::mutex::scoped_lock lock(mutex);
  while((!functionQueue.empty()) || (activeThreads>0)){
    threadAvailable.wait(lock);
  }
}

void SingleThread::beginThread(){
  boost::mutex::scoped_lock lock(threadPool.mutex);
  for(;;){
    if(threadPool.stop) break;
    if(threadPool.functionQueue.size()==0){
      threadPool.needThread.wait(lock);
    } else {
      boost::function<void ()> f=threadPool.functionQueue.back();
      Common::atomicInc(threadPool.activeThreads);
      threadPool.functionQueue.pop_back();
      lock.unlock();
      f();
      lock.lock();
      Common::atomicDec(threadPool.activeThreads);
      lock.unlock();
      threadPool.threadAvailable.notify_all();
      lock.lock();
    }
  }
}
