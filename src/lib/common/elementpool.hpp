/***************************************************************************
                          elementpool.hpp  -  description
                             -------------------
    begin                : Wed Wep 16 2008
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

#ifndef ELEMENTPOOL_HPP
#define ELEMENTPOOL_HPP

#include <boost/thread/thread.hpp>

//**********************ElementPool*******************
template <typename T, unsigned short N>
class ElementPool {
public:
  ElementPool(T* (*create)(void)) : index(0) {
    for(unsigned short i=0;i<N;++i) elements[i]=(*create)();
  }
  ElementPool() : index(0) {
    for(unsigned short i=0;i<N;++i) elements[i]=new T();
  }
  ~ElementPool(){
    for(unsigned short i=0;i<N;++i){
      delete elements[i];
    }
  }
  inline T& alloc(){
    boost::mutex::scoped_lock lock(mutex);
    return *elements[index++];
  }
  inline void free(T& element){
    boost::mutex::scoped_lock lock(mutex);
    elements[--index]=&element;
  }
  inline void perform(void (*funct)(T*)){
    boost::mutex::scoped_lock lock(mutex);
    for(unsigned short i=index;i<N;++i) (*funct)(elements[i]);
  }
private:
  unsigned short index;
  T* elements[N];
  boost::mutex mutex;
};

template <typename T>
class ElementPool<T,1> {
public:
  ElementPool(T* (*create)(void)){element=(*create)();}
  ElementPool(){element=new T();}
  ~ElementPool(){delete element;}
  inline T& alloc(){return *element;}
  inline void free(T&){}
  inline void perform(void (*funct)(T*)){(*funct)(element);}
private:
  T* element;
};

template <typename T, typename P>
class VolatileElement {
public:
  VolatileElement(P& ep) : elementPool(ep), element(ep.alloc()) {}
  ~VolatileElement(){elementPool.free(element);}
  T& operator()(){return element;}
private:
  P& elementPool;
  T& element;
};

#endif
