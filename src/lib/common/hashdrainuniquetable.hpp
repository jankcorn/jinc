/***************************************************************************
                          hashdrainuniquetable.hpp  -  description
                             -------------------
    begin                : Sat Mar 29 2008
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

#ifndef HASHDRAINUNIQUETABLE_HPP
#define HASHDRAINUNIQUETABLE_HPP

#include <common/helper.hpp>
#include <common/spinlock.hpp>

#include <vector>
#include <boost/dynamic_bitset.hpp>

inline unsigned long getHash(double value){
  Common::hash<double> h;
  return static_cast<unsigned long>(h(value));
}

//******************HashDrainUniqueTable********************
template <typename T, unsigned long M, unsigned short N>
class HashDrainUniqueTable {
public:
  HashDrainUniqueTable();
  unsigned long size() const {return count;}
  inline void resetDeadNodeCount(){deadNodes=0;}
  inline void incDeadNodeCount(){Common::atomicInc(deadNodes);}
  inline void decDeadNodeCount(){Common::atomicDec(deadNodes);}
  inline unsigned long getDeadNodes() const {return deadNodes;}
  inline bool remove(const T*);

  template <typename MP>
  inline T* findOrAddValue(double value, MP& memPool){
    const unsigned long index=getHash(value)&(M-1);
    
    T* tempNode=uniqueTable[index].ptr;
    T* lastNode=0;
    while(tempNode && tempNode->getValue()<value){
      lastNode=tempNode;
      tempNode=tempNode->getNext();
    }
    if(tempNode && !(value<tempNode->getValue())) return tempNode;
    
    return findOrAddValueIntern(index,lastNode,value,memPool);
  }  
  template <typename MP>
  inline T* findOrAddValueIntern(unsigned long index, T* startNode, double value, MP& memPool){
    SpinLock lock(uniqueTable[index].lock);

    T* tempNode=startNode?startNode:uniqueTable[index].ptr;
    T* lastNode=startNode;
    while(tempNode && tempNode->getValue()<value){
      lastNode=tempNode;
      tempNode=tempNode->getNext();
    }
    if(tempNode && !(value<tempNode->getValue())) return tempNode;
    T* result=memPool.alloc();
    result->setValue(value);

    if(lastNode==0){
      result->setNext(uniqueTable[index].ptr);
      uniqueTable[index].ptr=result;
      bitSet[index]=true;
    } else {
      result->setNext(lastNode->getNext());
      lastNode->setNext(result);
    }
    Common::atomicInc(count);
    incDeadNodeCount();

    return result;
  }  

  inline std::tr1::tuple<T*,T*,unsigned long> collectAndRemoveDeadNodes(){
    std::tr1::tuple<T*, T*, unsigned long> result(0,0,0);
    
    T* lastNode=0;
    T* currentNode=getFirst();
    while(currentNode){
      if(currentNode->preNodeCount==0){
        if(std::tr1::get<0>(result)==0) std::tr1::get<0>(result)=currentNode;
        else std::tr1::get<1>(result)->setNext(currentNode);
        std::tr1::get<1>(result)=currentNode;
        ++std::tr1::get<2>(result);

        if(lastNode==0){
          const unsigned long index=getHash(currentNode->getValue())&(M-1);

          uniqueTable[index].ptr=currentNode->getNext();
          if(uniqueTable[index].ptr==0) bitSet[index]=false;
        } else lastNode->setNext(currentNode->getNext());

        Common::atomicDec(count);
      } else lastNode=currentNode;
      currentNode=currentNode->getNext();
      if(currentNode==0){
        lastNode=0;
        currentNode=next();
      }
    }

    resetDeadNodeCount();
    if(std::tr1::get<1>(result)!=0) std::tr1::get<1>(result)->setNext(0);

    return result;
  }
  T* getFirst(){
    if(count==0) return 0;

    currentPos=bitSet.find_first();
    if(currentPos==boost::dynamic_bitset<>::npos) return 0;
    return uniqueTable[currentPos].ptr;
  }
  T* next(){
    currentPos=bitSet.find_next(currentPos);
    if(currentPos==boost::dynamic_bitset<>::npos) return 0;
    return uniqueTable[currentPos].ptr;
  }
  inline unsigned long getLevel() const {return VARIABLE_INDEX_MAX;}
  inline unsigned long getRealLevel() const {return VARIABLE_INDEX_MAX;}
private:
  unsigned long count;
  unsigned long deadNodes;

  boost::dynamic_bitset<> bitSet;
  boost::dynamic_bitset<>::size_type currentPos;

  Common::PtrAndLock<T> uniqueTable[M];
};

template <typename T, unsigned long M, unsigned short N>
HashDrainUniqueTable<T,M,N>::HashDrainUniqueTable() : count(0), deadNodes(0) {
  bitSet.resize(M,false);
  memset(uniqueTable,0,M);

  currentPos=boost::dynamic_bitset<>::npos;
}

template <typename T, unsigned long M, unsigned short N>
bool HashDrainUniqueTable<T,M,N>::remove(const T* element){
  unsigned long index=getHash(element->getValue())&(M-1);

  T* tempNode=uniqueTable[index].ptr;
  T* lastNode=0;
  while(tempNode && tempNode!=element){
    lastNode=tempNode;
    tempNode=tempNode->getNext();
  }
  if(tempNode!=element) return false;

  if(lastNode==0){
    //change entry in the unique table and check if the value is 0
    if((uniqueTable[index].ptr=tempNode->getNext())==0) bitSet[index]=false;
  } else lastNode->setNext(tempNode->getNext());
  tempNode->setNext(0);
  Common::atomicDec(count);

  return true;
}

#endif
