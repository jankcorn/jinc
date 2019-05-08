/***************************************************************************
                          ptrhashmap.hpp  -  description
                             -------------------
    begin                : Wed Mar 16 2005
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

#ifndef PTRHASHMAP_HPP
#define PTRHASHMAP_HPP

#include <string>
#include <iostream>
#include <vector>

#include <common/helper.hpp>

template <typename T, unsigned short N>
struct PointerTuple{
  PointerTuple() : content(0){
    for(unsigned int i=0;i<N;++i){
      key[i]=0;
    }
  }
  T* key[N];
  T* content;
};

template <typename T, unsigned short N, typename Base>
class PtrHashMap{
public:
  typedef typename std::vector<PointerTuple<T,N> > hashType;
  typedef typename hashType::iterator iterator;
  typedef typename hashType::const_iterator constIterator;
  typedef typename hashType::reverse_iterator reverseIterator;

  PtrHashMap(const unsigned long hashValues, const unsigned long maxHashValues, const double resizeR) : resizes(0), maxRatio(0), counter(0) {
    resizeRatio=static_cast<unsigned short>(128.0*resizeR);
    numberOfHashValues=2;
    //shift for smallest hashmap
    shift=sizeof(unsigned long)*8-1;
    while((numberOfHashValues<<1)<=hashValues){
      numberOfHashValues<<=1;
      --shift;
    }
    maxNumberOfHashValues=maxHashValues;

    hashMap.resize(numberOfHashValues,PointerTuple<T,N>());
  }
  void clear(){
    if(counter==0) return;
    unsigned short tempRatio=(counter*128)/numberOfHashValues;
    if(tempRatio>maxRatio) maxRatio=tempRatio;

    hashMap.assign(numberOfHashValues,PointerTuple<T,N>());

    counter=0;
  }
  unsigned long count() const {return counter;}
  T* find(const constIterator p, const T** keys) const {
    for(int i=0;i<N;++i){
      if(p->key[i]!=keys[i]) return 0;
    }
    return p->content;
  }
  void insert(const iterator p, const T** keys, const T* content){
    //if there is no item increase counter
    if(p->content==0){
      ++counter;
      //insert keys
      for(int i=0;i<N;++i){
	p->key[i]=const_cast<T*>(keys[i]);
      }
      //insert item
      p->content=const_cast<T*>(content);
      if UNLIKELY((numberOfHashValues<maxNumberOfHashValues)&&(counter*128>numberOfHashValues*resizeRatio)) resize();
    } else {
      //insert keys
      for(int i=0;i<N;++i){
	p->key[i]=const_cast<T*>(keys[i]);
      }
      //insert item
      p->content=const_cast<T*>(content);
    }
  }
  void printStat(const std::string& name) const {
    unsigned short tempRatio=(counter*128)/numberOfHashValues;
    if(tempRatio>maxRatio) maxRatio=tempRatio;

    //do not print debug information for not used hash map
    if(maxRatio==0) return;

    std::cout << "####################################" << std::endl;
    std::cout << "Name               : " << name << std::endl;
    std::cout << "Number of Resizes  : " << resizes << std::endl;
    std::cout << "Current Fill Ratio : " << (static_cast<double>(tempRatio)/128.0*100.0) << std::endl;
    std::cout << "Max Fill Ratio     : " << (static_cast<double>(maxRatio)/128.0*100.0) << std::endl;
    std::cout << "Size               : " << numberOfHashValues << std::endl;
    std::cout << "Max Size           : " << maxNumberOfHashValues << std::endl;
    std::cout << "####################################" << std::endl;
  }
  unsigned short resizes;
  mutable unsigned short maxRatio;
  unsigned short resizeRatio;
  unsigned long counter;
  unsigned long numberOfHashValues;
  unsigned long maxNumberOfHashValues;
  unsigned short shift;
  std::vector<PointerTuple<T,N> > hashMap;

  void resize(){
    maxRatio=(counter*128)/(numberOfHashValues*2);

    const unsigned long oldSize=numberOfHashValues;
    numberOfHashValues<<=1;
    --shift;

    hashMap.resize(numberOfHashValues,PointerTuple<T,N>());
    ++resizes;

    counter=0;
    reverseIterator it=hashMap.rbegin()+oldSize;
    for(unsigned long i=0;i<oldSize;++i,++it) {
      if(it->content!=0){
	unsigned long index=Base::hashFunction(*it);
	hashMap[index>>shift]=(*it);
	//clean old position (otherwise counter would not have a correct value)
	hashMap[index>>(shift+1)]=PointerTuple<T,N>();
	++counter;
      }
    }
  }
};

template <typename T>
class PtrHashMap2 : public PtrHashMap<T,2,PtrHashMap2<T> >{
public:
  typedef PtrHashMap<T,2,PtrHashMap2<T> > base;

  PtrHashMap2(const unsigned long hashValues, const unsigned long maxHashValues, const double resizeR) : PtrHashMap<T,2,PtrHashMap2<T> >(hashValues,maxHashValues,resizeR){}

  static unsigned long hashFunction(const PointerTuple<T,2>& tuple){return Common::hashFunction<T*>(tuple.key[0],tuple.key[1]);}

  T* find( const T* key1, const T* key2 ) const {
    typename std::vector<PointerTuple<T,2> >::const_iterator it = this->hashMap.begin() + ( Common::hashFunction<const T*>( key1, key2 ) >> this->shift );
    const T* keys[] = { key1, key2 };
    return base::find( it, keys );
  }

  void insert(const T* key1, const T* key2, const T* content){
    typename std::vector<PointerTuple<T,2> >::iterator it=this->hashMap.begin()+(Common::hashFunction<const T*>(key1,key2)>>this->shift);
    const T* keys[]={key1,key2};
    base::insert(it,keys,content);
  }
};

template <typename T>
class PtrHashMap3 : public PtrHashMap<T,3,PtrHashMap3<T> >{
public:
  typedef PtrHashMap<T,3,PtrHashMap3<T> > base;

  PtrHashMap3(const unsigned long hashValues, const unsigned long maxHashValues, const double resizeR) : PtrHashMap<T,3,PtrHashMap3<T> >(hashValues,maxHashValues,resizeR){}

  static unsigned long hashFunction(const PointerTuple<T,3>& tuple){return Common::hashFunction<T*>(tuple.key[0],tuple.key[1],tuple.key[2]);}
  T* find(const T* key1, const T* key2, const T* key3) const {
    typename std::vector<PointerTuple<T,3> >::const_iterator it=this->hashMap.begin()+(Common::hashFunction<const T*>(key1,key2,key3)>>this->shift);
    const T* keys[]={key1,key2,key3};
    return base::find(it,keys);
  }
  void insert(const T* key1, const T* key2, const T* key3, const T* content){
    typename std::vector<PointerTuple<T,3> >::iterator it=this->hashMap.begin()+(Common::hashFunction<const T*>(key1,key2,key3)>>this->shift);
    const T* keys[]={key1,key2,key3};
    base::insert(it,keys,content);
  }
};

template <typename T>
class PtrHashMap4 : public PtrHashMap<T,4,PtrHashMap4<T> >{
public:
  typedef PtrHashMap<T,4,PtrHashMap4<T> > base;

  PtrHashMap4(const unsigned long hashValues, const unsigned long maxHashValues, const double resizeR) : PtrHashMap<T,4,PtrHashMap4<T> >(hashValues,maxHashValues,resizeR){}

  static unsigned long hashFunction(const PointerTuple<T,4>& tuple){return Common::hashFunction<const T*>(tuple.key[0],tuple.key[1],tuple.key[2],tuple.key[3]);}
  T* find(const T* key1, const T* key2, const T* key3, const T* key4) const {
    typename std::vector<PointerTuple<T,4> >::const_iterator it=this->hashMap.begin()+(Common::hashFunction<const T*>(key1,key2,key3,key4)>>this->shift);
    const T* keys[]={key1,key2,key3,key4};
    return base::find(it,keys);
  }
  void insert(const T* key1, const T* key2, const T* key3, const T* key4, const T* content){
    typename std::vector<PointerTuple<T,4> >::iterator it=this->hashMap.begin()+(Common::hashFunction<const T*>(key1,key2,key3,key4)>>this->shift);
    const T* keys[]={key1,key2,key3,key4};
    base::insert(it,keys,content);
  }
};

template <typename T>
class PtrHashMap5 : public PtrHashMap<T,5,PtrHashMap5<T> >{
public:
  typedef PtrHashMap<T,5,PtrHashMap5<T> > base;

  PtrHashMap5(const unsigned long hashValues, const unsigned long maxHashValues, const double resizeR) : PtrHashMap<T,5,PtrHashMap5<T> >(hashValues,maxHashValues,resizeR){}

  static unsigned long hashFunction(const PointerTuple<T,5>& tuple){return Common::hashFunction<const T*>(tuple.key[0],tuple.key[1],tuple.key[2],tuple.key[3],tuple.key[4]);}
  T* find(const T* key1, const T* key2, const T* key3, const T* key4, const T* key5) const {
    typename std::vector<PointerTuple<T,5> >::const_iterator it=this->hashMap.begin()+(Common::hashFunction<const T*>(key1,key2,key3,key4,key5)>>this->shift);
    const T* keys[]={key1,key2,key3,key4,key5};
    return base::find(it,keys);
  }
  void insert(const T* key1, const T* key2, const T* key3, const T* key4, const T* key5, const T* content){
    typename std::vector<PointerTuple<T,5> >::iterator it=this->hashMap.begin()+(Common::hashFunction<const T*>(key1,key2,key3,key4,key5)>>this->shift);
    const T* keys[]={key1,key2,key3,key4,key5};
    base::insert(it,keys,content);
  }
};

#endif
