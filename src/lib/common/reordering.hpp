/***************************************************************************
                          reordering.hpp  -  description
                             -------------------
    begin                : Tue Apr 13 2004
    copyright            : (C) 2004-2008 by Wolfgang Lenders, Joern Ossowski
    email                : lenders@cs.uni-bonn.de, mail@jossowski.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software;you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation;either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef REORDERING_HPP
#define REORDERING_HPP

#include <common/constants.hpp>
#include <common/threadpool.hpp>
#include <common/commondata.hpp>

#include <vector>
#include <list>
#include <stack>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <tr1/tuple>

#include <boost/random.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

template <typename BDD>
struct GroupStrategy {
  static void prepare(){
    indices.clear();
    indices.resize(size());
    for(unsigned int i=0;i<size();++i){
      indices[i]=BDD::variableOrdering.getGroup(i)->getFirst()->getLevel();
    }
  }
  static VariableIndexType size(){return BDD::variableOrdering.groupCount();}
  static std::list<VariableIndexType> swap(VariableIndexType index){
    std::list<VariableIndexType> swapList;
    VariableIndexType startIndex=indices[index];
    VariableIndexType sizeOfGroup1=indices[index+1]-indices[index];
    VariableIndexType sizeOfGroup2=((index+2<size())?indices[index+2]:BDD::variableOrdering.size())-indices[index+1];

    for(VariableIndexType i=0;i<sizeOfGroup1;++i){
      for(VariableIndexType j=0;j<sizeOfGroup2;++j){
        swapList.push_back(startIndex+sizeOfGroup1-1+j-i);
      }
    }
    indices[index+1]=startIndex+sizeOfGroup2;
    return swapList;
  }
  static std::vector<VariableIndexType> indices;
};
template <typename BDD>
std::vector<VariableIndexType> GroupStrategy<BDD>::indices=std::vector<VariableIndexType>();

template <typename BDD>
class VariableStrategy {
public:
  static void setGroupIndex(VariableIndexType index){
    groupIndex=index;
    offset=BDD::variableOrdering.getGroup(index)->getFirst()->getLevel();
  }
  static void getGroupIndex(){return groupIndex;}
  static void prepare(){}
  static VariableIndexType size(){return BDD::variableOrdering.getVariable(offset)->size();}
  static std::list<VariableIndexType> swap(VariableIndexType index){
    std::list<VariableIndexType> swapList;
    swapList.push_back(offset+index);
    return swapList;
  }
private:
  static VariableIndexType groupIndex;
  static VariableIndexType offset;
};
template <typename BDD>
VariableIndexType VariableStrategy<BDD>::groupIndex=0;
template <typename BDD>
VariableIndexType VariableStrategy<BDD>::offset=0;

template <typename BDD, typename Strategy=GroupStrategy<BDD> >
class ReorderHelper {
public:
  void prepare(){Strategy::prepare();}
  inline void scheduleProcesses(){
    boost::dynamic_bitset<> bitSet(BDD::variableOrdering.size());
    bitSet.set();
    std::list<VariableIndexType>::iterator it;
    it=jobList.begin();
    while(it!=jobList.end()){
      VariableIndexType index=*it;
      bitSet[index]=false;
      bitSet[index+1]=false;
      ++it;
    }
    it=swapList.begin();
    while(it!=swapList.end()){
      VariableIndexType index=*it;
      if(bitSet[index] && bitSet[index+1]){
        jobList.push_back(index);
        boost::function<void (ReorderHelper*, VariableIndexType)> f=&ReorderHelper::performSwap;
        CommonData::threadPool.schedule(boost::bind(f,this,index));
        swapList.erase(it++);
      } else ++it;
      
      bitSet[index]=false;
      bitSet[index+1]=false;
    }
  }
  void performSwap(VariableIndexType index){
    BDD::swap(index);
    {
      boost::mutex::scoped_lock lock(mut);
      readySwapsStack.push(index);
    }
    cond.notify_all();
  }
  void commit(){
    boost::mutex::scoped_lock lock(mut);
    scheduleProcesses();
    while(jobList.size()){
      cond.wait(lock);
      while(readySwapsStack.size()){
	      jobList.remove(readySwapsStack.top());
	      readySwapsStack.pop();
      }
      scheduleProcesses();
    }
  }
  unsigned long count(){
    commit();
    return BDD::size();
  }
  VariableIndexType size(){return Strategy::size();}
  void swap(VariableIndexType index){
    std::list<VariableIndexType> tempSwapList=Strategy::swap(index);
    swapList.splice(swapList.end(),tempSwapList);
  }
  
  std::stack<VariableIndexType> readySwapsStack;
  boost::condition cond;
  boost::mutex mut;
  
  std::list<VariableIndexType> swapList;
  std::list<VariableIndexType> jobList;
};

typedef std::vector<VariableIndexType> VariableIndexArray;

struct Individual {
  Individual() : quality(0) {}
  VariableIndexArray order;
  unsigned long quality;
};

bool operator<(const Individual& a, const Individual& b){return (a.quality < b.quality);}

namespace Common{
  namespace {
    boost::mt19937 rng(std::time(0));
    boost::uniform_int<> distribution(0,10000000);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > nextRandomNumber(rng,distribution);
  }

  std::string print(const VariableIndexArray& array){
    std::ostringstream printedArray;
    printedArray << "(";
    for(VariableIndexType i=0; i<array.size(); ++i){
      printedArray << array[i];
      if(i+1<array.size()) printedArray << ", ";
    }
    printedArray << ")";
    return printedArray.str();
  }

  VariableIndexType find(const VariableIndexArray& array, const VariableIndexType begin, const VariableIndexType query){
    std::vector<VariableIndexType>::const_iterator p=std::find(array.begin()+begin,array.end(),query);
    if(p!=array.end()) return (p-array.begin());
    return VARIABLE_INDEX_MAX;
  }

  VariableIndexType find(const VariableIndexArray& array, const VariableIndexType query){return Common::find(array,0,query);}
  
  void shift(VariableIndexArray& array, VariableIndexType index, const VariableIndexType position){
    if(index==position) return;
    VariableIndexType temp=array[index];
    if(index<position){ //jump down
      while(index<position) {
        array[index]=array[index+1];
        ++index;
      }
    } else {             //jump up
      while(index>position) {
        array[index]=array[index-1];
        --index;
      }
    }
    array[position]=temp;
  }
  
  unsigned long distance(const VariableIndexArray& firstArray, const VariableIndexArray& secondArray){
    unsigned long distance=0;
    VariableIndexType indexInTempArray;
    VariableIndexArray tempArray(firstArray);
    std::vector<VariableIndexType>::iterator p=tempArray.begin();
    for(VariableIndexType i=0; i<firstArray.size(); ++i){
      if(tempArray[i]!=secondArray[i]){
        indexInTempArray=(std::find(p+i,tempArray.end(),secondArray[i])-p);
        distance+=indexInTempArray-i;
        shift(tempArray,indexInTempArray,i);
      }
    }
    return distance;
  }
  
  void randomShuffle(VariableIndexArray& array){
    if(array.size()<2) return;
    const VariableIndexType length=array.size();
    const VariableIndexType mutationCount=Common::nextRandomNumber()%(length/10+1);
    for(VariableIndexType i=0;i<mutationCount;++i) std::swap(array[Common::nextRandomNumber()%length],array[Common::nextRandomNumber()%length]);
  }
}

template <typename T>
class Reordering{
public:
  Reordering(const T* obj) : obj(const_cast<T*>(obj)){}
  void windowPermutation2();
  void windowPermutation3();
  void windowPermutation4();
  void sifting(const double);
  void swap(const VariableIndexType pos) const;
  void shift(const VariableIndexType pos1, const VariableIndexType pos2) const;
  void geneticMinimize(const bool DEBUG=false);
private:
  void siftingImpl(VariableIndexArray&, const double);
  void swapImpl(const VariableIndexType pos) const;
  void shiftImpl(const VariableIndexType pos1, const VariableIndexType pos2) const;
  VariableIndexType siftUp(const VariableIndexType startPos, std::tr1::tuple<VariableIndexType, unsigned long>& optObserver, const double maxGrowth);
  VariableIndexType siftDown(const VariableIndexType startPos, const VariableIndexType length, std::tr1::tuple<VariableIndexType, unsigned long>& optObserver, const double maxGrowth);
  std::tr1::tuple<unsigned long,VariableIndexArray> measureIndividuals(VariableIndexArray& currentOrder, const unsigned short startIndex, Individual* const population);
  void crossover(const Individual& firstParent, const Individual& secondParent, Individual& offspring, const unsigned short type) const;
  void adoptCurrentOrderToArray(VariableIndexArray&, const VariableIndexArray&) const;
  void chooseParents(const Individual* const population, VariableIndexType& firstParentIndex, VariableIndexType& secondParentIndex) const;

  T* obj;
};

template <typename T>
  void Reordering<T>::windowPermutation2(){
  obj->prepare();
  for(unsigned i=0,optSize=obj->count();i+1<obj->size();++i){
    obj->swap(i);
    if(obj->count()>=optSize) obj->swap(i);
  }
  obj->commit();
}

template <typename T>
  void Reordering<T>::windowPermutation3(){
  enum{ABC,BAC,BCA,CBA,CAB,ACB};

  unsigned long optSize;
  short optPos;
  obj->prepare();
  if(obj->size()<3) windowPermutation2();
  else for(VariableIndexType i=0,optSize=obj->count(),optPos=ABC;i+2<obj->size();++i){
    optPos=ABC;
    obj->swap(i);  //BAC
    if(obj->count()<optSize){optSize=obj->count();optPos=BAC;}
    obj->swap(i+1);//BCA
    if(obj->count()<optSize){optSize=obj->count();optPos=BCA;}
    obj->swap(i);  //CBA
    if(obj->count()<optSize){optSize=obj->count();optPos=CBA;}
    obj->swap(i+1);//CAB
    if(obj->count()<optSize){optSize=obj->count();optPos=CAB;}
    obj->swap(i);  //ACB
    if(obj->count()<optSize){optSize=obj->count();optPos=ACB;}

    switch(optPos){
      case CBA: obj->swap(i);  //ACB=>CAB=>CBA
      case ABC: obj->swap(i+1);//ACB=>ABC
                break;
      case BCA: obj->swap(i);  //ACB=>CAB=>CBA=>BCA
      case BAC: obj->swap(i+1);//ACB=>ABC=>BAC
      case CAB: obj->swap(i);  //ACB=>CAB
      default:  break;
    }
  }
  obj->commit();
}

template <typename T>
  void Reordering<T>::windowPermutation4(){
  enum{ABCD,ABDC,ADBC,DABC,DACB,ADCB,ACDB,ACBD,CABD,CADB,CDAB,DCAB,DCBA,CDBA,CBDA,CBAD,BCAD,BCDA,BDCA,DBCA,DBAC,BDAC,BADC,BACD};

  unsigned long optSize;
  short optPos;
  obj->prepare();
  if(obj->size()<4) windowPermutation3();
  else for(VariableIndexType i=0,optSize=obj->count(),optPos=ABCD;i+3<obj->size();++i){
    optPos=ABCD;
    obj->swap(i+2);//ABDC
    if(obj->count()<optSize){optSize=obj->count();optPos=ABDC;}
    obj->swap(i+1);//ADBC
    if(obj->count()<optSize){optSize=obj->count();optPos=ADBC;}
    obj->swap(i);  //DABC
    if(obj->count()<optSize){optSize=obj->count();optPos=DABC;}
    obj->swap(i+2);//DACB
    if(obj->count()<optSize){optSize=obj->count();optPos=DACB;}
    obj->swap(i);  //ADCB
    if(obj->count()<optSize){optSize=obj->count();optPos=ADCB;}
    obj->swap(i+1);//ACDB
    if(obj->count()<optSize){optSize=obj->count();optPos=ACDB;}
    obj->swap(i+2);//ACBD
    if(obj->count()<optSize){optSize=obj->count();optPos=ACBD;}
    obj->swap(i);  //CABD
    if(obj->count()<optSize){optSize=obj->count();optPos=CABD;}
    obj->swap(i+2);//CADB
    if(obj->count()<optSize){optSize=obj->count();optPos=CADB;}
    obj->swap(i+1);//CDAB
    if(obj->count()<optSize){optSize=obj->count();optPos=CDAB;}
    obj->swap(i);  //DCAB
    if(obj->count()<optSize){optSize=obj->count();optPos=DCAB;}
    obj->swap(i+2);//DCBA
    if(obj->count()<optSize){optSize=obj->count();optPos=DCBA;}
    obj->swap(i);  //CDBA
    if(obj->count()<optSize){optSize=obj->count();optPos=CDBA;}
    obj->swap(i+1);//CBDA
    if(obj->count()<optSize){optSize=obj->count();optPos=CBDA;}
    obj->swap(i+2);//CBAD
    if(obj->count()<optSize){optSize=obj->count();optPos=CBAD;}
    obj->swap(i);  //BCAD
    if(obj->count()<optSize){optSize=obj->count();optPos=BCAD;}
    obj->swap(i+2);//BCDA
    if(obj->count()<optSize){optSize=obj->count();optPos=BCDA;}
    obj->swap(i+1);//BDCA
    if(obj->count()<optSize){optSize=obj->count();optPos=BDCA;}
    obj->swap(i);  //DBCA
    if(obj->count()<optSize){optSize=obj->count();optPos=DBCA;}
    obj->swap(i+2);//DBAC
    if(obj->count()<optSize){optSize=obj->count();optPos=DBAC;}
    obj->swap(i);  //BDAC
    if(obj->count()<optSize){optSize=obj->count();optPos=BDAC;}
    obj->swap(i+1);//BADC
    if(obj->count()<optSize){optSize=obj->count();optPos=BADC;}
    obj->swap(i+2);//BACD
    if(obj->count()<optSize){optSize=obj->count();optPos=BACD;}

    switch(optPos){
      case CABD: obj->swap(i);
      case CBAD: obj->swap(i+1);
      case ABCD: obj->swap(i);
                 break;
      case ACBD: obj->swap(i);
      case BCAD: obj->swap(i+1);
                 break;
      case ACDB: obj->swap(i);
      case BCDA: obj->swap(i+1);
      case BADC: obj->swap(i+2);
                 break;
      case CDAB: obj->swap(i);
      case CDBA: obj->swap(i+1);
      case ADBC: obj->swap(i);
      case BDAC: obj->swap(i+2);
                 obj->swap(i+1);
                 break;
      case CADB: obj->swap(i);
      case CBDA: obj->swap(i+1);
      case ABDC: obj->swap(i);
                 obj->swap(i+2);
                 break;
      case ADCB: obj->swap(i);
      case BDCA: obj->swap(i+1);
                 obj->swap(i+2);
                 obj->swap(i+1);
                 break;
      case DCAB: obj->swap(i);
      case DCBA: obj->swap(i+1);
      case DABC: obj->swap(i);
      case DBAC: obj->swap(i+2);
                 obj->swap(i+1);
                 obj->swap(i);
                 break;
      case DACB: obj->swap(i);
      case DBCA: obj->swap(i+1);
                 obj->swap(i+2);
                 obj->swap(i+1);
                 obj->swap(i);
                 break;
      default:  break;
    }
  }
  obj->commit();
}

template <typename T>
void Reordering<T>::swap(const VariableIndexType pos) const {
  obj->prepare();
  swapImpl(pos);
  obj->commit();
}

template <typename T>
void Reordering<T>::swapImpl(const VariableIndexType pos) const {
  if(pos>=obj->size()) return;
  obj->swap(pos);
}

template <typename T>
void Reordering<T>::shift(const VariableIndexType pos1, const VariableIndexType pos2) const {
  obj->prepare();
  shiftImpl(pos1,pos2);
  obj->commit();
}

template <typename T>
void Reordering<T>::shiftImpl(const VariableIndexType pos1, const VariableIndexType pos2) const {
  if(pos1==pos2) return;
  if(pos1>=obj->size()) return;
  if(pos2>=obj->size()) return;

  VariableIndexType index=pos1;
  if(index<pos2){ 
    while(index<pos2){
      obj->swap(index);
      ++index;
    }
  } else {
    while(index>pos2){
      obj->swap(index-1);
      --index;
    }
  }
}

template <typename T>
VariableIndexType Reordering<T>::siftUp(const VariableIndexType startPos, std::tr1::tuple<VariableIndexType, unsigned long>& optObserver, const double maxGrowth){
  VariableIndexType pos=startPos;
  unsigned long count=obj->count();
  while((pos>0) && (count<=static_cast<unsigned long>(maxGrowth*static_cast<double>(std::tr1::get<1>(optObserver))))){
    obj->swap(pos-1);
    count=obj->count();
    if(count<std::tr1::get<1>(optObserver)){
      std::tr1::get<0>(optObserver)=pos-1;
      std::tr1::get<1>(optObserver)=count;
    }
    --pos;
  }
  return pos;
}

template <typename T>
  VariableIndexType Reordering<T>::siftDown(const VariableIndexType startPos, const VariableIndexType length, std::tr1::tuple<VariableIndexType, unsigned long>& optObserver, const double maxGrowth){
  VariableIndexType pos=startPos;
  unsigned long count=obj->count();
  while((pos+1<length) && (count<=static_cast<unsigned long>(maxGrowth*static_cast<double>(std::tr1::get<1>(optObserver))))){
    obj->swap(pos);
    count=obj->count();
    if(count<std::tr1::get<1>(optObserver)){
      std::tr1::get<0>(optObserver)=pos+1;
      std::tr1::get<1>(optObserver)=count;
    }
    ++pos;
  }
  return pos;
}

template <typename T>
void Reordering<T>::siftingImpl(VariableIndexArray& currentOrder, const double maxGrowth){
  std::tr1::tuple<VariableIndexType, unsigned long> optObserver(0,obj->count());
  const VariableIndexType length=obj->size();

  for(VariableIndexType l=0;l<length;++l){
    VariableIndexType lastPos=Common::find(currentOrder,l);
    VariableIndexType currentElement=lastPos;
    if(lastPos<(length-lastPos-1)){
      std::tr1::get<0>(optObserver)=lastPos;
      shiftImpl(siftUp(lastPos,optObserver,maxGrowth),lastPos);
      lastPos=siftDown(lastPos,length,optObserver,maxGrowth);
    } else {
      std::tr1::get<0>(optObserver)=lastPos;
      shiftImpl(siftDown(lastPos,length,optObserver,maxGrowth),lastPos);
      lastPos=siftUp(lastPos,optObserver,maxGrowth);
    }
    shiftImpl(lastPos,std::tr1::get<0>(optObserver));
    Common::shift(currentOrder,currentElement,std::tr1::get<0>(optObserver));
  }
}

template <typename T>
void Reordering<T>::sifting(const double maxGrowth){
  const VariableIndexType length=obj->size();
  VariableIndexArray currentOrder(length);
  for(VariableIndexType i=0;i<length;++i) currentOrder[i]=i;

  obj->prepare();
  siftingImpl(currentOrder,maxGrowth);
  obj->commit();
}

template <typename T>
std::tr1::tuple<unsigned long,VariableIndexArray> Reordering<T>::measureIndividuals(VariableIndexArray& currentOrder, const unsigned short startIndex, Individual* const population){
  unsigned long bestBDDSize=ULONG_MAX;
  VariableIndexType bestBDDOrderIndex=startIndex;
  VariableIndexArray lastOrder=currentOrder;
  for(unsigned short i=startIndex;i<POPULATION_SIZE;++i){
    VariableIndexType bestPos=i;
    VariableIndexType bestDist=Common::distance(lastOrder,population[i].order);
    for(unsigned short j=i+1;j<POPULATION_SIZE;++j){
      VariableIndexType dist=Common::distance(lastOrder,population[j].order);
      if(dist<bestDist){
        bestDist=dist;
        bestPos=j;
      }
    }
    population[i].order.swap(population[bestPos].order);
    adoptCurrentOrderToArray(currentOrder,population[i].order);
    population[i].order=currentOrder;
    population[i].quality=obj->count();
    if(population[i].quality<bestBDDSize){
      bestBDDSize=population[i].quality;
      bestBDDOrderIndex=i;
    }
    lastOrder=population[i].order;
  }
  return std::tr1::tuple<unsigned long,VariableIndexArray>(bestBDDSize,population[bestBDDOrderIndex].order);
}

template <typename T>
void Reordering<T>::geneticMinimize(const bool DEBUG){
  const VariableIndexType length=obj->size();
  if(length<2) return;

  /** the population stores <POPULATION_SIZE> elements, here called "individuals"
   *  each individual is an array of VariableIndexTypes, which stand for the current position of
   *  the variable in relation to the position when geneticMinimize() was called
   *  Example:
   *  Let individual population[0] be {0, 1, 4, 2, 3} means that calling
   *    swap(2);
   *    swap(3);
   *  would bring you back to the original order
   */

  obj->prepare();
  Individual population[POPULATION_SIZE];
  Individual nextGeneration[POPULATION_SIZE];
  VariableIndexArray currentOrder(length);
  for(VariableIndexType i=0;i<length;++i) currentOrder[i]=i;
  population[0].order=currentOrder;
  population[0].quality=obj->count();

  for(unsigned short i=1;i<POPULATION_SIZE;++i){
    population[i].order=currentOrder;
    std::random_shuffle(population[i].order.begin(),population[i].order.end());
  }
  for(unsigned short i=0;i<POPULATION_SIZE;++i) nextGeneration[i].order=currentOrder;
  std::tr1::tuple<unsigned long,VariableIndexArray> bestIndividual=measureIndividuals(currentOrder,0,population);
  if(DEBUG){
    std::cout << std::endl << "----- initial population: -----" << std::endl << std::endl;
    for(unsigned short i=0;i<POPULATION_SIZE;++i){
      std::cout << "population[" << i << "]: " << Common::print(population[i].order) << " \t size: " << population[i].quality << std::endl;
    }
  }
  adoptCurrentOrderToArray(currentOrder,std::tr1::get<1>(bestIndividual));

  unsigned long iterationCount,iterationsWithoutImprovement;
  iterationCount=iterationsWithoutImprovement=0;
  unsigned long oldBestSize=std::tr1::get<0>(bestIndividual);
  while ((std::tr1::get<0>(bestIndividual)<oldBestSize) || (++iterationsWithoutImprovement <= 3)){
    std::cout << "----- creating generation " << ++iterationCount << " [" << std::tr1::get<0>(bestIndividual) << "]" <<std::endl;
    std::cout << (obj->count()) << std::endl;
    if(std::tr1::get<0>(bestIndividual)<oldBestSize) iterationsWithoutImprovement=0;
    oldBestSize=std::tr1::get<0>(bestIndividual);

    if(DEBUG) std::cout << "=========" << std::endl;
    std::sort(population,population+POPULATION_SIZE);
    for(unsigned long i=0;i<ELITARISM_WINNERS;++i) nextGeneration[i]=population[i];

    for(unsigned short i=ELITARISM_WINNERS;i<POPULATION_SIZE;++i){
      VariableIndexType firstParentIndex, secondParentIndex;
      chooseParents(population, firstParentIndex, secondParentIndex);
      crossover(population[firstParentIndex], population[secondParentIndex], nextGeneration[i], i%5);
      
      // do mutation with a chance of 5% on each newly generated order in nextGeneration
      if((population[firstParentIndex].order==nextGeneration[i].order) || (population[secondParentIndex].order==nextGeneration[i].order) || ((Common::nextRandomNumber()%10)<1)){
        Common::randomShuffle(nextGeneration[i].order);
      }
    }
    std::tr1::tuple<unsigned long,VariableIndexArray> newBestIndividual=measureIndividuals(currentOrder,ELITARISM_WINNERS,nextGeneration);
    if(std::tr1::get<0>(newBestIndividual)<std::tr1::get<0>(bestIndividual)) bestIndividual=newBestIndividual;

    std::sort(nextGeneration,nextGeneration+POPULATION_SIZE);
    for(unsigned short i=0;i<POPULATION_SIZE;++i){
      population[i].order.swap(nextGeneration[i].order);
      std::swap(population[i].quality,nextGeneration[i].quality);
      if(DEBUG) std::cout << "nextGeneration[" << i << "]: " << Common::print(nextGeneration[i].order) << " \t size: " << nextGeneration[i].quality << std::endl;
    }
    adoptCurrentOrderToArray(currentOrder,std::tr1::get<1>(bestIndividual));
  }
  obj->commit();
}

template <typename T>
void Reordering<T>::crossover(const Individual& firstParent, const Individual& secondParent, Individual& offspring, const unsigned short type) const {
  const VariableIndexType length=firstParent.order.size();
  switch(type){
  case 0: //ORDERED_CROSSOVER
    {
      boost::shared_array<VariableIndexType> selected(new VariableIndexType[length]);
      for(VariableIndexType i=0;i<length;++i) selected[i]=i%2;
      std::random_shuffle(selected.get(),selected.get()+length);
      
      // construct offspring
      VariableIndexType secondParentIndex=0;
      for(VariableIndexType i=0;i<length;++i){
        if(selected[i]) offspring.order[i]=firstParent.order[i];
        else{
          while(selected[Common::find(firstParent.order,secondParent.order[secondParentIndex])]) ++secondParentIndex;
          offspring.order[i]=secondParent.order[secondParentIndex];
          ++secondParentIndex;
        }
      }
    }
    break;
  case 1: //INVERTED_CROSSOVER
    {
      VariableIndexType firstCutPoint=Common::nextRandomNumber()%length;
      VariableIndexType secondCutPoint=Common::nextRandomNumber()%length;
      if(firstCutPoint>secondCutPoint) std::swap(firstCutPoint,secondCutPoint);
      for(VariableIndexType i=0;i<length;++i){
	      if(i>=firstCutPoint && i<=secondCutPoint) offspring.order[i]=firstParent.order[secondCutPoint-(i-firstCutPoint)];
	      else offspring.order[i]=firstParent.order[i];
      }
    }
    break;
  case 2: //PARTIALLY_MATCHED_CROSSOVER
    {
      VariableIndexType firstCutPoint=Common::nextRandomNumber()%length;
      VariableIndexType secondCutPoint=Common::nextRandomNumber()%length;
      if(firstCutPoint>secondCutPoint) std::swap(firstCutPoint,secondCutPoint);
      offspring.order=firstParent.order;
      for(VariableIndexType i=firstCutPoint;i<secondCutPoint;++i) Common::shift(offspring.order,i,Common::find(firstParent.order,secondParent.order[i]));
    }
    break;
  case 3: //CYCLE_CROSSOVER
    {
      boost::shared_array<bool> selected(new bool[length]);
      VariableIndexType selectedCounter;
      const VariableIndexArray* first=&firstParent.order;
      const VariableIndexArray* second=&secondParent.order;
      for(VariableIndexType i=0;i<length;++i) selected[i]=false;
      selectedCounter=0;
      while(selectedCounter<length){
        VariableIndexType i=0;
        while(selected[first->at(i)]) ++i;
        do{
          offspring.order[i]=first->at(i);
          selected[first->at(i)]=true;
          i=Common::find(*first,second->at(i));
          ++selectedCounter;
        } while(!selected[first->at(i)]);
        std::swap(first,second);
      }
    }
    break;
  case 4: //ALTERNATING_CROSSOVER
    {
      VariableIndexType first, second;
      first=second=0;
      VariableIndexType candidate=firstParent.order[0];
      boost::shared_array<bool> selected(new bool[length]);
      for(VariableIndexType i=0;i<length;++i) selected[i]=false;
      for(VariableIndexType i=0;i<length;++i){
        while (selected[candidate]) candidate=(i%2==0?firstParent.order[first++]:secondParent.order[second++]);
        selected[candidate]=true;
        offspring.order[i]=candidate;
      }
    }
    break;
  }
}

template <typename T>
void Reordering<T>::adoptCurrentOrderToArray(VariableIndexArray& currentOrder, const VariableIndexArray& targetOrder) const {
  for(VariableIndexType i=0;i<currentOrder.size();++i){
    if(currentOrder[i]!=targetOrder[i]){
      VariableIndexType indexInCurrentOrder=Common::find(currentOrder,i,targetOrder[i]);
      Common::shift(currentOrder,indexInCurrentOrder,i);
      shiftImpl(indexInCurrentOrder,i);
    }
  }
}

template <typename T>
void Reordering<T>::chooseParents(const Individual* const population, VariableIndexType& firstParentIndex, VariableIndexType& secondParentIndex) const {
  unsigned long probabilitySum=0;
  unsigned long worstQuality=0;
  for(unsigned short i=0;i<POPULATION_SIZE;++i){
    if(population[i].quality>worstQuality) worstQuality=population[i].quality;
    probabilitySum+=population[i].quality;
  }
  probabilitySum=POPULATION_SIZE*worstQuality+POPULATION_SIZE-probabilitySum;
  {
    unsigned long sum=0;
    const unsigned long firstChoice=Common::nextRandomNumber()%probabilitySum;
    firstParentIndex=0;
    while(sum+(worstQuality-population[firstParentIndex].quality+1)<firstChoice){
      sum+=worstQuality-population[firstParentIndex].quality+1;
      ++firstParentIndex;
    }
  }
  {
    unsigned long sum=0;
    const unsigned long secondChoice=Common::nextRandomNumber()%probabilitySum;
    secondParentIndex=0;
    while(sum+(worstQuality-population[secondParentIndex].quality+1)<secondChoice){
      sum+=worstQuality-population[secondParentIndex].quality+1;
      ++secondParentIndex;
    }
  }
}

#endif
