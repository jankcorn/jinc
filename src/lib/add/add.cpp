/***************************************************************************
                        add.cpp  -  description
 -------------------
 begin                : Wed Apr 14 2004
 copyright            : (C) 2004-2008 by Joern Ossowski
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

#include <add/add.hpp>

#include <common/constants.hpp>
#include <common/helper.hpp>
#include <common/ptrhashmap.hpp>
#include <common/elementpool.hpp>
#include <common/commondata.hpp>

#include <add/addinnernode.hpp>
#include <add/addterminalnode.hpp>

#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread/tss.hpp>

namespace {
  ADDTerminalNode* createZeroDrain(){
    ADDTerminalNode* node=getADDData().memPools.terminalNodeMemPool.alloc();
    node->setValue(0.0);
    ++node->preNodeCount;
    return node;
  }
  ADDTerminalNode* initAndReturnOneDrain(){
    Group<ADDUniqueTable>::varOrder=&ADD::variableOrdering;
    ADDTerminalNode* node=ADD::drainUniqueTable.findOrAddValue(1.0,getADDData().memPools.terminalNodeMemPool);
    ++node->preNodeCount;
    ADD::drainUniqueTable.decDeadNodeCount();
    return node;
  }
  void deleteNodeAndSubTree(ADDBaseNode* node){
    if LIKELY(!node->isDrain()){
      ADDInnerNode* innerNode=static_cast<ADDInnerNode*>(node);
      
      if(Common::atomicDecAndTest(innerNode->getSucc0()->preNodeCount)) deleteNodeAndSubTree(innerNode->getSucc0());
      if(Common::atomicDecAndTest(innerNode->getSucc1()->preNodeCount)) deleteNodeAndSubTree(innerNode->getSucc1());
      
      innerNode->uniqueTable->incDeadNodeCount();
    } else {
      ADD::drainUniqueTable.incDeadNodeCount();
    }
  }
  void deleteOnlySubTree(ADDInnerNode* node){
    if(Common::atomicDecAndTest(node->getSucc0()->preNodeCount)) deleteNodeAndSubTree(node->getSucc0());
    if(Common::atomicDecAndTest(node->getSucc1()->preNodeCount)) deleteNodeAndSubTree(node->getSucc1());      
  }
  void identifyDeadNodes(std::tr1::tuple<ADDInnerNode*, ADDInnerNode*, unsigned long>& tuple){
    ADDInnerNode* currentNode=std::tr1::get<0>(tuple);
    while(currentNode){
      deleteOnlySubTree(currentNode);
      currentNode=currentNode->getNext();
    }
  }
  void collectDeadNodes(boost::shared_array<std::tr1::tuple<ADDInnerNode*, ADDInnerNode*, unsigned long> >& deadNodeList, unsigned long index,ADDUniqueTable* uniqueTable){
    deadNodeList[index]=uniqueTable->collectAndRemoveDeadNodes();
  }
  void releaseNodes(boost::shared_array<std::tr1::tuple<ADDInnerNode*, ADDInnerNode*, unsigned long> >& deadNodeList){
    for(VariableIndexType i=0;i<ADD::variableOrdering.size();++i){
      if(std::tr1::get<2>(deadNodeList[i])>0){
        for(unsigned int j=0;j<ADD::innerMemPools.size();++j){
          if(ADD::innerMemPools[j]->size()>0){
            ADD::innerMemPools[j]->freeGroup(deadNodeList[i]);
            deadNodeList[i]=std::tr1::tuple<ADDInnerNode*, ADDInnerNode*, unsigned long>(0,0,0);
            break;
          }
        }
      }
    }  
  }
  void clearMap2(ADDHashMap2* ptr){ptr->clear();}
  void clearMap3(ADDHashMap3* ptr){ptr->clear();}
  
  boost::mutex mutex;
}

std::vector<ADDHashMap2*> ADD::hashMaps2Params;
std::vector<ADDHashMap3*> ADD::hashMaps3Params;
std::vector<MemPool<ADDInnerNode>*> ADD::innerMemPools;

ADDTerminalNodeUniqueTable ADD::drainUniqueTable;
ADDVariableOrdering ADD::variableOrdering;
ADDTerminalNode* ADD::zeroDrain=createZeroDrain();
ADDTerminalNode* ADD::oneDrain=initAndReturnOneDrain();

ADDData& getADDData(){
  static boost::thread_specific_ptr<ADDData> ptr;
  if UNLIKELY(ptr.get()==0){
    ADDData* data=new ADDData();

    data->computedTables.arithmeticHashMap=new ADDHashMap2(65536,16777216,0.8);
    data->computedTables.booleanHashMap=new ADDHashMap2(65536,16777216,0.8);
    data->computedTables.extremaHashMap=new ADDHashMap2(65536,16777216,0.8);
    data->computedTables.compareHashMap=new ADDHashMap2(65536,16777216,0.8);

    data->computedTables.moveHashMap=new ADDHashMap3(65536,16777216,0.8);
    data->computedTables.matrixHashMap=new ADDHashMap3(65536,16777216,0.8);

    {
      boost::mutex::scoped_lock lock(mutex);
      ADD::hashMaps2Params.push_back(data->computedTables.arithmeticHashMap);
      ADD::hashMaps2Params.push_back(data->computedTables.booleanHashMap);
      ADD::hashMaps2Params.push_back(data->computedTables.extremaHashMap);
      ADD::hashMaps2Params.push_back(data->computedTables.compareHashMap);
      ADD::hashMaps3Params.push_back(data->computedTables.moveHashMap);
      ADD::hashMaps3Params.push_back(data->computedTables.matrixHashMap);
      ADD::innerMemPools.push_back(&data->memPools.innerNodeMemPool);
    }
    ptr.reset(data);
  }
  return *ptr;
}

void ADD::garbageCollect(){
  for(unsigned int i=0;i<hashMaps2Params.size();++i){
    CommonData::threadPool.schedule(boost::bind(clearMap2,hashMaps2Params[i]));
  }
  for(unsigned int i=0;i<hashMaps3Params.size();++i){
    CommonData::threadPool.schedule(boost::bind(clearMap3,hashMaps3Params[i]));
  }
    
  CommonData::threadPool.wait();
  boost::shared_array<std::tr1::tuple<ADDInnerNode*, ADDInnerNode*, unsigned long> > deadNodeList(new std::tr1::tuple<ADDInnerNode*, ADDInnerNode*, unsigned long> [variableOrdering.size()]);
  for(VariableIndexType i=variableOrdering.size();i>0;--i){
    ADDUniqueTable* uniqueT=variableOrdering.getData(i-1);
    if(uniqueT->getDeadNodes()>0) CommonData::threadPool.schedule(boost::bind(collectDeadNodes,deadNodeList,i-1,uniqueT));
    else deadNodeList[i-1]=std::tr1::tuple<ADDInnerNode*, ADDInnerNode*, unsigned long>(0,0,0);
  }
  CommonData::threadPool.wait();
  getADDData().memPools.terminalNodeMemPool.freeGroup(ADD::drainUniqueTable.collectAndRemoveDeadNodes());
  for(VariableIndexType i=0;i<variableOrdering.size();++i){
    if(std::tr1::get<2>(deadNodeList[i])>0) CommonData::threadPool.schedule(boost::bind(identifyDeadNodes,deadNodeList[i]));
  }
  CommonData::threadPool.wait();
  releaseNodes(deadNodeList);
  
  for(VariableIndexType i=0;i<variableOrdering.size();++i){
    ADDUniqueTable* uniqueT=variableOrdering.getData(i);
    if(uniqueT->getDeadNodes()>0) CommonData::threadPool.schedule(boost::bind(collectDeadNodes,deadNodeList,i-1,uniqueT));
  }
  getADDData().memPools.terminalNodeMemPool.freeGroup(ADD::drainUniqueTable.collectAndRemoveDeadNodes());
  
  CommonData::threadPool.wait();
  releaseNodes(deadNodeList);
}

unsigned long ADD::size(){
  unsigned long ret=0;
  
  for(VariableIndexType i=0;i<variableOrdering.size();++i) ret+=variableOrdering.getData(i)->size();
  ret+=drainUniqueTable.size();
  
  //do not count zero or one drain if they have no predecessors
  if(oneDrain->preNodeCount==1) --ret;
  if(zeroDrain->preNodeCount!=1) ++ret;
  
  return ret;
}

unsigned long ADD::deadNodes(){
  unsigned long ret=0;
  
  for(VariableIndexType i=0;i<variableOrdering.size();++i) ret+=variableOrdering.getData(i)->getDeadNodes();
  ret+=drainUniqueTable.getDeadNodes();
  
  return ret;  
}

bool ADD::clear(){
  garbageCollect();
  /*TODO improve handling */
  //count number of inner nodes
  unsigned long innerNodes=0;
  for(VariableIndexType i=0;i<variableOrdering.size();++i) innerNodes+=variableOrdering.getData(i)->size();
  
  if(innerNodes==0){
    variableOrdering.clear();
    
    return true;
  } else {
    bool ret=false;
    for(VariableIndexType i=variableOrdering.size();i>0;--i) ret|=variableOrdering.removeVariable(i-1);
    
    return ret;
  }
}

bool ADD::swap(const VariableIndexType index){
  if UNLIKELY(index+1>=variableOrdering.size()) return false;

  ADDMemPools& pools=getADDData().memPools;
  MemPool<ADDInnerNode>& innerMemPool=pools.innerNodeMemPool;
  MemPool<ADDTerminalNode>& terminalMemPool=pools.terminalNodeMemPool;
  
  ADDBaseNode* w0;
  ADDBaseNode* w1;
  ADDBaseNode* w00;
  ADDBaseNode* w01;
  ADDBaseNode* w10;
  ADDBaseNode* w11;
  
  ADDUniqueTable* tempTable1=variableOrdering.getData(index);
  ADDUniqueTable* tempTable2=variableOrdering.getData(index+1);
  ADDInnerNode* currentTable=tempTable1->getFirst();
  ADDInnerNode* currentNode;
  ADDInnerNode* tempNode;
  while(currentTable){
    currentNode=currentTable;
    while(currentNode){
      tempNode=currentNode->getNext();
      
      w0=currentNode->getSucc0();
      w1=currentNode->getSucc1();
      if(w0->getLevel()==index+1){
        w00=static_cast<ADDInnerNode*>(w0)->getSucc0();
        w01=static_cast<ADDInnerNode*>(w0)->getSucc1();
        if(w1->getLevel()==index+1){
          w10=static_cast<ADDInnerNode*>(w1)->getSucc0();
          w11=static_cast<ADDInnerNode*>(w1)->getSucc1();
        } else w10=w11=w1;
        if(w00==w10) w0=w00;
        else {
          const ADDBaseNode* nodes[]={ w00 , w10 };
          w0=tempTable1->findOrAdd(nodes,innerMemPool);
        }
        
        if(w01==w11) w1=w11;
        else {
          const ADDBaseNode* nodes[]={ w01 , w11 };
          w1=tempTable1->findOrAdd(nodes,innerMemPool);
        }
        
        tempTable1->remove(currentNode);
        currentNode->setNode(tempTable2,w0,w1,innerMemPool,terminalMemPool);
      } else {
        if(w1->getLevel()==index+1){
          w00=w01=w0;
          w10=static_cast<ADDInnerNode*>(w1)->getSucc0();
          w11=static_cast<ADDInnerNode*>(w1)->getSucc1();
          if(w00==w10) w0=w00;
          else {
            const ADDBaseNode* nodes[]={ w00 , w10 };
            w0=tempTable1->findOrAdd(nodes,innerMemPool);
          }
          
          if(w01==w11) w1=w11;
          else {
            const ADDBaseNode* nodes[]={ w01 , w11 };
            w1=tempTable1->findOrAdd(nodes,innerMemPool);
          }
          
          tempTable1->remove(currentNode);
          currentNode->setNode(tempTable2,w0,w1,innerMemPool,terminalMemPool);
        }
      }
      currentNode=tempNode;
    }
    currentTable=tempTable1->next();
  }
  
  variableOrdering.swapVariables(index);
  
  return true;
}
