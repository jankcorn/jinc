/***************************************************************************
                          hashuniquetable.hpp  -  description
                             -------------------
    begin                : Mon Nov 28 2005
    copyright            : (C) 2005-2008 by Joern Ossowski
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

#ifndef HASHUNIQUETABLE_HPP
#define HASHUNIQUETABLE_HPP

#include <common/helper.hpp>
#include <common/spinlock.hpp>

#include <vector>
#include <tr1/tuple>

#include <boost/dynamic_bitset.hpp>

//******************HashUniqueTable********************
template <typename T, typename B, unsigned short N>
class HashUniqueTable {
public:
  HashUniqueTable(const VariableIndexType);
  unsigned long size() const {return count;}
  inline void resetDeadNodeCount(){deadNodes=0;}
  inline void incDeadNodeCount(){Common::atomicInc(deadNodes);}
  inline void decDeadNodeCount(){Common::atomicDec(deadNodes);}
  inline unsigned long getDeadNodes() const {return deadNodes;}
  inline void add(T*);
  inline bool remove(const T*);
  template <typename M>
  inline T* findOrAdd(const B** nodes, M& memPool){
    const unsigned long index=Common::ArrayHelper<const B*,N>::hashFromArray(nodes)>>shift;
    
    T* lastNode=0;
    T* tempNode=uniqueTable[index].ptr;    
    while(tempNode){
      int i=0;
      while(i<N && (tempNode->getSucc(i)==nodes[i])) ++i;
      if(i==N) return tempNode;
      if(tempNode->getSucc(i)>nodes[i]) break;
      lastNode=tempNode;
      tempNode=tempNode->getNext();
    }

    return findOrAddIntern(index,lastNode,nodes,memPool);
  }

  template <typename M>
  inline T* findOrAddIntern(unsigned long index, T* startNode, const B** nodes, M& memPool){
    SpinLock lock(uniqueTable[index].lock);

    T* lastNode=startNode;
    T* tempNode=startNode?startNode:uniqueTable[index].ptr;
    
    while(tempNode){
      int i=0;
      while(i<N && (tempNode->getSucc(i)==nodes[i])) ++i;
      if(i==N) return tempNode;
      if(tempNode->getSucc(i)>nodes[i]) break;
      lastNode=tempNode;
      tempNode=tempNode->getNext();
    }
    
    T* result=memPool.alloc();
    result->initNode(this,const_cast<B**>(nodes));
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
          B* nodes[N];
          for(unsigned short i=0;i<N;++i) nodes[i]=currentNode->getSucc(i);
          const unsigned long index=Common::ArrayHelper<B*,N>::hashFromArray(nodes)>>shift;

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
    return uniqueTable[currentPos].ptr;
  }
  T* next(){
    currentPos=bitSet.find_next(currentPos);
    if(currentPos==boost::dynamic_bitset<>::npos) return 0;
    return uniqueTable[currentPos].ptr;
  }
  void setLevel(const VariableIndexType newLevel){level=newLevel;}
  inline VariableIndexType getLevel() const {return level-skip;}
  inline VariableIndexType getRealLevel() const {return level;}
  inline bool deleteable() const {return (count==0);}
  inline static void setSkip(unsigned long newSkip){skip=newSkip;}
  inline static void setInitHashSize(unsigned int newInit){initHashSize=newInit;}
  inline static void setMaxHashSize(unsigned int newMax){maximumHashSize=newMax;}
  inline static unsigned long getSkip(){return skip;}
  void resize();
private:
  static unsigned int initHashSize;
  static unsigned int maximumHashSize;
  static VariableIndexType skip;
  VariableIndexType level;
  unsigned long count;
  unsigned int hashSize;
  unsigned int maxHashSize;
  unsigned short shift;
  unsigned short collisionListSizeLimit;
  unsigned long deadNodes;

  boost::dynamic_bitset<> bitSet;
  boost::dynamic_bitset<>::size_type currentPos;

  std::vector<Common::PtrAndLock<T> > uniqueTable;
};
template <typename T, typename B, unsigned short N> VariableIndexType HashUniqueTable<T,B,N>::skip=0;
//template <typename T, typename B, unsigned short N> unsigned int HashUniqueTable<T,B,N>::initHashSize=256;
template <typename T, typename B, unsigned short N> unsigned int HashUniqueTable<T,B,N>::initHashSize=131072;
template <typename T, typename B, unsigned short N> unsigned int HashUniqueTable<T,B,N>::maximumHashSize=4194304;

template <typename T, typename B, unsigned short N>
HashUniqueTable<T,B,N>::HashUniqueTable(const VariableIndexType newLevel) : level(newLevel), count(0), deadNodes(0) {
  collisionListSizeLimit=12;

  hashSize=2;
  //shift for smallest hashmap
  shift=sizeof(unsigned long)*8-1;
  while((hashSize<<1)<=initHashSize){
    hashSize<<=1;
    --shift;
  }
  maxHashSize=maximumHashSize;

  bitSet.resize(hashSize,false);
  uniqueTable.resize(hashSize);

  currentPos=boost::dynamic_bitset<>::npos;
}

template <typename T, typename B, unsigned short N>
void HashUniqueTable<T,B,N>::add(T* result){
  B* nodes[N];
  for(unsigned short i=0;i<N;++i) nodes[i]=result->getSucc(i);
  const unsigned long index=Common::ArrayHelper<B*,N>::hashFromArray(nodes)>>shift;

  T* tempNode=uniqueTable[index].ptr;
  T* lastNode=0;
  while(tempNode){
    int i=0;
    while(i<N && (tempNode->getSucc(i)==nodes[i])) ++i;
    if(i==N || tempNode->getSucc(i)>nodes[i]) break;
    lastNode=tempNode;
    tempNode=tempNode->getNext();
  }

  if(lastNode==0){
    result->setNext(uniqueTable[index].ptr);
    uniqueTable[index].ptr=result;
    bitSet[index]=true;
  } else {
    result->setNext(lastNode->getNext());
    lastNode->setNext(result);
  }
  Common::atomicInc(count);
}

template <typename T, typename B, unsigned short N>
bool HashUniqueTable<T,B,N>::remove(const T* value){
  B* nodes[N];
  for(unsigned short i=0;i<N;++i) nodes[i]=value->getSucc(i);
  const unsigned long index=Common::ArrayHelper<B*,N>::hashFromArray(nodes)>>shift;

  T* tempNode=uniqueTable[index].ptr;
  T* lastNode=0;
  while(tempNode && tempNode!=value){
    lastNode=tempNode;
    tempNode=tempNode->getNext();
  }
  if(tempNode!=value) return false;

  if(lastNode==0){
    //change entry in the unique table and check if the value is 0
    if((uniqueTable[index].ptr=tempNode->getNext())==0) bitSet[index]=false;
  } else lastNode->setNext(tempNode->getNext());

  tempNode->setNext(0);
  Common::atomicDec(count);
  return true;
}

template <typename T, typename B, unsigned short N>
void HashUniqueTable<T,B,N>::resize(){
  std::vector<T*> tempUniqueTable(2*hashSize);
  boost::dynamic_bitset<> tempBitSet(2*hashSize,false);

  --shift;

  //transfer all nodes from old unique table to new one
  T* currentTable=getFirst();
  T* currentNode;
  T* tempNode;
  T* evenTable;
  T* oddTable;
  unsigned long index;
  while(currentTable){
    currentNode=currentTable;
    tempNode=currentNode->getNext();
    //shift is already updated so that the shift has to be reverted
    
    B* nodes[N];
    for(unsigned short i=0;i<N;++i) nodes[i]=currentNode->getSucc(i);
    const unsigned long index=Common::ArrayHelper<B*,N>::hashFromArray(nodes)>>(shift+1);
    
    evenTable=0;
    oddTable=0;

    while(currentNode){
      tempNode=currentNode->getNext();
      //shift is already updated
      if((Common::ArrayHelper<B*,N>::hashFromArray(nodes)>>shift)&1){
        if(oddTable!=0) oddTable->setNext(currentNode);
        else tempUniqueTable[2*index+1]=currentNode;
        oddTable=currentNode;
      } else {
        if(evenTable!=0) evenTable->setNext(currentNode);
        else tempUniqueTable[2*index]=currentNode;
        evenTable=currentNode;
      }

      currentNode=tempNode;
    }

    if(oddTable!=0){
      oddTable->setNext(0);
      tempBitSet[2*index+1]=true;
    } else tempUniqueTable[2*index+1]=0;
    if(evenTable!=0){
      evenTable->setNext(0);
      tempBitSet[2*index]=true;
    } else tempUniqueTable[2*index]=0;

    currentTable=next();
  }

  uniqueTable.swap(tempUniqueTable);
  bitSet.swap(tempBitSet);

  hashSize*=2;
  currentPos=boost::dynamic_bitset<>::npos;
}

#endif
