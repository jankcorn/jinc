/***************************************************************************
                          threadpool.hpp  -  description
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

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>

#include <list>

struct SingleThread;

struct ThreadPool {
  ThreadPool(const unsigned long numberOfThreads);
  ~ThreadPool();
  void schedule(boost::function<void ()> function);
  void wait();
  
  bool stop;
  unsigned long activeThreads;
  unsigned long numberOfThreads;
  boost::thread_group threads;
  mutable boost::mutex mutex;
  boost::condition threadAvailable, needThread;
  std::list<boost::function<void ()> > functionQueue;
};

struct SingleThread{
  SingleThread(ThreadPool& tp) : threadPool(tp){}
  void operator()(){
    beginThread();
  }
  void beginThread();
  ThreadPool& threadPool;
};

#endif
