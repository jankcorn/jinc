/***************************************************************************
                          mempool.hpp  -  description
                             -------------------
    begin                : Tue Dec 09 2003
    copyright            : (C) 2003-2008 by Joern Ossowski
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

#ifndef MEMPOOL_HPP
#define MEMPOOL_HPP

#include <tr1/tuple>

const unsigned long NUMBER_OF_NODES=100000;
#include <iostream>
//**********************MemPool*******************
template <typename T>
class MemPool{
public:
  MemPool();
  ~MemPool();

  inline T* alloc();
  inline void free(T*);
  inline void freeGroup(std::tr1::tuple<T*,T*,unsigned long>);
  inline unsigned long peak() const {return numberOfObjects<=peakNumberOfObjects?peakNumberOfObjects:numberOfObjects;}
  inline unsigned long size() const {return numberOfObjects;}
private:
  struct Chunk{
    enum{size=NUMBER_OF_NODES};
    T mem[size];
    Chunk* next;
  };
  Chunk* chunks;
  T* head;
  unsigned long peakNumberOfObjects;
  unsigned long numberOfObjects;

  inline void grow();
};

template <typename T>
MemPool<T>::MemPool() : chunks(0), head(0), peakNumberOfObjects(0), numberOfObjects(0) {}

template <typename T>
MemPool<T>::~MemPool(){
  Chunk* p;
  Chunk* n=chunks;
  while(n){
    p=n;
    n=n->next;
    delete p;
  }
}

template <typename T>
T* MemPool<T>::alloc(){
  if(!head) grow();
  T* p=head;
  head=p->getNext();
  p->setNext(0);
  ++numberOfObjects;

  return p;
}

template <typename T>
void MemPool<T>::free(T* n){
  T* p=n;
  p->setNext(head);
  head=p;
  if(numberOfObjects>peakNumberOfObjects) peakNumberOfObjects=numberOfObjects;
  --numberOfObjects;
}

template <typename T>
void MemPool<T>::freeGroup(std::tr1::tuple<T*,T*,unsigned long> group){
  if(std::tr1::get<0>(group)==0) return;

  std::tr1::get<1>(group)->setNext(head);
  head=std::tr1::get<0>(group);
  if(numberOfObjects>peakNumberOfObjects) peakNumberOfObjects=numberOfObjects;
  numberOfObjects-=std::tr1::get<2>(group);
}


template <typename T>
void MemPool<T>::grow(){
  Chunk* n=new Chunk;
  n->next=chunks;
  chunks=n;

  T* p=n->mem;
  for(unsigned long i=0;i<Chunk::size-1;++i,++p) p->setNext(&p[1]);
  p->setNext(0);

  head=n->mem;
}
#endif
