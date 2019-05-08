/***************************************************************************
                          future.hpp  -  description
                             -------------------
    begin                : Thu Sep 4 2008
    copyright            : (C) 2008 by Joern Ossowski
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

#ifndef FUTURE_HPP
#define FUTURE_HPP

#include <common/helper.hpp>
#include <common/commondata.hpp>

#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>

//******************Future********************
template <typename T>
class Future{
public:
  explicit Future(const T& v) : valid(true), value(v){}
  Future(boost::function<T ()> funct) : valid(false), value(){
    boost::function<void (Future*, boost::function<T ()>&)> f=&Future::waitForFunction;
    CommonData::threadPool.schedule(boost::bind(f,this,funct));
  }
  Future(const Future& v) : valid(false), value(){
    boost::mutex::scoped_lock lock(mutex);
    if(v.isValid()){
      value=v.getValueWithoutLocking();
      valid=true;
    } else {
      boost::function<void (Future*, const Future&)> f=&Future::waitForFuture;
      CommonData::threadPool.schedule(boost::bind(f,this,boost::cref(v)));
    }
  }
  ~Future(){
    wait();
  }
  bool isValid() const {
    boost::mutex::scoped_try_lock lock(mutex);
    return lock.owns_lock()?valid:false;
  }
  void wait() const {
    boost::mutex::scoped_lock lock(mutex);
    while(!valid){
      cond.wait(lock);
    }
  }
  T getValueWithoutLocking() const {
    return value;
  }  
  T getValue() const {
    wait();
    return value;
  }
  void waitForFuture(const Future& v){
    T temp=v.getValue();
    {
      boost::mutex::scoped_lock lock(mutex);
      value=temp;
      valid=true;
      cond.notify_all();
    }
  }
  void waitForFunction(boost::function<T ()>& funct){
    T temp=funct();
    {
      boost::mutex::scoped_lock lock(mutex);
      value=temp;
      valid=true;
      cond.notify_all();
    }
  }
private:
  bool valid;
  T value;
  mutable boost::mutex mutex;
  mutable boost::condition cond;
};

namespace Helper {
  template <typename T>
  struct FutureContainer {
    FutureContainer(unsigned long size, boost::function<boost::function<T ()> ()> factory){
      for(unsigned long i=0;i<size;++i){
        elements.push_back(new Future<T>(factory()));
      }
    }
    ~FutureContainer(){
      typename std::vector<Future<T>*>::iterator it=elements.begin();
      while(it!=elements.end()){
        delete (*it);
        ++it;
      }
    }
    Future<T>& operator[](unsigned long index){return *(elements[index]);}
    std::vector<Future<T>*> elements;
  };
  
  template <typename T>
  T callFunction(T (*function)(const T&, const T&), const Future<T>& v0, const Future<T>& v1){
    return (*function)(v0.getValue(),v1.getValue());
  }

  template <typename T>
  unsigned long getSizeT(const T& v){
    return v.size();
  }

  template <typename T>
  unsigned long getSize(const Future<T>& v){
    return v.getValue().size();
  }

  template <typename T>
  boost::function<T ()> createFunction(T (*function)(const T&, const T&), const Future<T>& v0, const Future<T>& v1){
    return boost::bind(callFunction<T>,function,boost::cref(v0),boost::cref(v1));
  }

  template <typename T>
  boost::function<unsigned long ()> createSizeFunction(const T& v){
    return boost::bind(getSizeT<T>,boost::cref(v));
  }

  template <typename T>
  boost::function<unsigned long ()> createSizeFunction(const Future<T>& v){
    return boost::bind(getSize<T>,boost::cref(v));
  }
}

#endif
