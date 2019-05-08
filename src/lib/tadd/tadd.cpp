/***************************************************************************
                          tadd.cpp  -  description
                             -------------------
    begin                : Mon Dec 22 2008
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

#include <tadd/tadd.hpp>

#include <common/constants.hpp>
#include <common/helper.hpp>
#include <common/ptrhashmap.hpp>
#include <common/elementpool.hpp>
#include <common/commondata.hpp>

#include <tadd/taddinnernode.hpp>
#include <tadd/taddterminalnode.hpp>
#include <tadd/taddtraversalhelper.hpp>

#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread/tss.hpp>

namespace {
  TADDTerminalNode* createZeroDrain(){
    TADDTerminalNode* node=getTADDData().memPools.terminalNodeMemPool.alloc();
    node->setValue(0.0);
    ++node->preNodeCount;
    return node;
  }
  TADDTerminalNode* initAndReturnOneDrain(){
    Group<TADDUniqueTable>::varOrder=&TADD::variableOrdering;
    TADDTerminalNode* node=TADD::drainUniqueTable.findOrAddValue(1.0,getTADDData().memPools.terminalNodeMemPool);
    ++node->preNodeCount;
    TADD::drainUniqueTable.decDeadNodeCount();
    return node;
  }
  void deleteNodeAndSubTree(TADDBaseNode* node){
    TraversalHelper<TADDBaseNode> t(node);
    if LIKELY(!t.isDrain()){
      if(Common::atomicDecAndTest(const_cast<TADDBaseNode*>(t.getSuccPtr(0))->preNodeCount)) deleteNodeAndSubTree(const_cast<TADDBaseNode*>(t.getSuccPtr(0)));
      if(Common::atomicDecAndTest(const_cast<TADDBaseNode*>(t.getSuccPtr(1))->preNodeCount)) deleteNodeAndSubTree(const_cast<TADDBaseNode*>(t.getSuccPtr(1)));
      if(Common::atomicDecAndTest(const_cast<TADDBaseNode*>(t.getSuccPtr(2))->preNodeCount)) deleteNodeAndSubTree(const_cast<TADDBaseNode*>(t.getSuccPtr(2)));
      if(Common::atomicDecAndTest(const_cast<TADDBaseNode*>(t.getSuccPtr(3))->preNodeCount)) deleteNodeAndSubTree(const_cast<TADDBaseNode*>(t.getSuccPtr(3)));
      
      t.getPtr()->uniqueTable->incDeadNodeCount();
    } else {
      TADD::drainUniqueTable.incDeadNodeCount();
    }
  }
  void deleteOnlySubTree(TADDInnerNode* node){
    TraversalHelper<TADDBaseNode> t(node);
    if(Common::atomicDecAndTest(const_cast<TADDBaseNode*>(t.getSuccPtr(0))->preNodeCount)) deleteNodeAndSubTree(const_cast<TADDBaseNode*>(t.getSuccPtr(0)));
    if(Common::atomicDecAndTest(const_cast<TADDBaseNode*>(t.getSuccPtr(1))->preNodeCount)) deleteNodeAndSubTree(const_cast<TADDBaseNode*>(t.getSuccPtr(1)));
    if(Common::atomicDecAndTest(const_cast<TADDBaseNode*>(t.getSuccPtr(2))->preNodeCount)) deleteNodeAndSubTree(const_cast<TADDBaseNode*>(t.getSuccPtr(2)));
    if(Common::atomicDecAndTest(const_cast<TADDBaseNode*>(t.getSuccPtr(3))->preNodeCount)) deleteNodeAndSubTree(const_cast<TADDBaseNode*>(t.getSuccPtr(3)));
  }
  void identifyDeadNodes(std::tr1::tuple<TADDInnerNode*, TADDInnerNode*, unsigned long>& tuple){
    TADDInnerNode* currentNode=std::tr1::get<0>(tuple);
    while(currentNode){
      deleteOnlySubTree(currentNode);
      currentNode=currentNode->getNext();
    }
    getTADDData().memPools.innerNodeMemPool.freeGroup(tuple);
  }
  void collectDeadNodes( boost::shared_array<std::tr1::tuple<TADDInnerNode*, TADDInnerNode*, unsigned long> >& deadNodeList, unsigned long index,TADDUniqueTable* uniqueTable){
    deadNodeList[index]=uniqueTable->collectAndRemoveDeadNodes();
  }
  void releaseNodes(boost::shared_array<std::tr1::tuple<TADDInnerNode*, TADDInnerNode*, unsigned long> >& deadNodeList){
    for(VariableIndexType i=0;i<TADD::variableOrdering.size();++i){
      if(std::tr1::get<2>(deadNodeList[i])>0){
        for(unsigned int j=0;j<TADD::innerMemPools.size();++j){
          if(TADD::innerMemPools[j]->size()>0){
            TADD::innerMemPools[j]->freeGroup(deadNodeList[i]);
            deadNodeList[i]=std::tr1::tuple<TADDInnerNode*, TADDInnerNode*, unsigned long>(0,0,0);
            break;
          }
        }
      }
    }  
  }
  void clearMap2(TADDHashMap2* ptr){ptr->clear();}
  void clearMap3(TADDHashMap3* ptr){ptr->clear();}
  
  boost::mutex mutex;
}

std::vector<TADDHashMap2*> TADD::hashMaps2Params;
std::vector<TADDHashMap3*> TADD::hashMaps3Params;
std::vector<MemPool<TADDInnerNode>*> TADD::innerMemPools;

TADDTerminalNodeUniqueTable TADD::drainUniqueTable;
TADDVariableOrdering TADD::variableOrdering;
TADDTerminalNode* TADD::zeroDrain=createZeroDrain();
TADDTerminalNode* TADD::oneDrain=initAndReturnOneDrain();

TADDData& getTADDData(){
  static boost::thread_specific_ptr<TADDData> ptr;
  if UNLIKELY(ptr.get()==0){
    TADDData* data=new TADDData();

    data->computedTables.arithmeticHashMap=new TADDHashMap2(65536,16777216,0.8);
    data->computedTables.booleanHashMap=new TADDHashMap2(65536,16777216,0.8);
    data->computedTables.extremaHashMap=new TADDHashMap2(65536,16777216,0.8);
    data->computedTables.compareHashMap=new TADDHashMap2(65536,16777216,0.8);

    data->computedTables.moveHashMap=new TADDHashMap3(65536,16777216,0.8);
    data->computedTables.matrixHashMap=new TADDHashMap3(65536,16777216,0.8);

    {
      boost::mutex::scoped_lock lock(mutex);
      TADD::hashMaps2Params.push_back(data->computedTables.arithmeticHashMap);
      TADD::hashMaps2Params.push_back(data->computedTables.booleanHashMap);
      TADD::hashMaps2Params.push_back(data->computedTables.extremaHashMap);
      TADD::hashMaps2Params.push_back(data->computedTables.compareHashMap);
      TADD::hashMaps3Params.push_back(data->computedTables.moveHashMap);
      TADD::hashMaps3Params.push_back(data->computedTables.matrixHashMap);
      TADD::innerMemPools.push_back(&data->memPools.innerNodeMemPool);
    }
    ptr.reset(data);
  }
  return *ptr;
}

void TADD::garbageCollect(){
  for(unsigned int i=0;i<hashMaps2Params.size();++i){
    CommonData::threadPool.schedule(boost::bind(clearMap2,hashMaps2Params[i]));
  }
  for(unsigned int i=0;i<hashMaps3Params.size();++i){
    CommonData::threadPool.schedule(boost::bind(clearMap3,hashMaps3Params[i]));
  }
  //MatrixComputedTable.clear();
    
  CommonData::threadPool.wait();
  boost::shared_array<std::tr1::tuple<TADDInnerNode*, TADDInnerNode*, unsigned long> > deadNodeList(new std::tr1::tuple<TADDInnerNode*, TADDInnerNode*, unsigned long> [variableOrdering.size()]);
  for(unsigned long i=variableOrdering.size();i>0;--i){
    TADDUniqueTable* uniqueT=variableOrdering.getData(i-1);
    if(uniqueT->getDeadNodes()>0) CommonData::threadPool.schedule(boost::bind(collectDeadNodes,deadNodeList,i-1,uniqueT));
    else deadNodeList[i-1]=std::tr1::tuple<TADDInnerNode*, TADDInnerNode*, unsigned long>(0,0,0);
  }
  CommonData::threadPool.wait();
  getTADDData().memPools.terminalNodeMemPool.freeGroup(TADD::drainUniqueTable.collectAndRemoveDeadNodes());
  for(unsigned long i=0;i<variableOrdering.size();++i){
    if(std::tr1::get<2>(deadNodeList[i])>0) CommonData::threadPool.schedule(boost::bind(identifyDeadNodes,deadNodeList[i]));
  }
  CommonData::threadPool.wait();
  releaseNodes(deadNodeList);

  for(unsigned long i=0;i<variableOrdering.size();++i){
    TADDUniqueTable* uniqueT=variableOrdering.getData(i);
    if(uniqueT->getDeadNodes()>0) CommonData::threadPool.schedule(boost::bind(collectDeadNodes,deadNodeList,i-1,uniqueT));
  }
  getTADDData().memPools.terminalNodeMemPool.freeGroup(TADD::drainUniqueTable.collectAndRemoveDeadNodes());
  
  CommonData::threadPool.wait();
  releaseNodes(deadNodeList);
}

unsigned long TADD::size(){
  unsigned long ret=0;

  for(VariableIndexType i=0;i<variableOrdering.size();++i) ret+=variableOrdering.getData(i)->size();
  ret+=drainUniqueTable.size();

  //do not count zero or one drain if they have no predecessors
  if(oneDrain->preNodeCount==1) --ret;
  if(zeroDrain->preNodeCount!=1) ++ret;

  return ret;
}

unsigned long TADD::deadNodes(){
  unsigned long ret=0;
  
  for(VariableIndexType i=0;i<variableOrdering.size();++i) ret+=variableOrdering.getData(i)->getDeadNodes();
  ret+=drainUniqueTable.getDeadNodes();
  
  return ret;  
}

bool TADD::clear(){
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

bool TADD::swap(const VariableIndexType index){
  if UNLIKELY(index+1>=variableOrdering.size()) return false;

  TADDMemPools& pools=getTADDData().memPools;
  MemPool<TADDInnerNode>& innerMemPool=pools.innerNodeMemPool;
  MemPool<TADDTerminalNode>& terminalMemPool=pools.terminalNodeMemPool;

  TADDBaseNode* r[4];
  TADDBaseNode* w[4][4];

  TADDUniqueTable* tempTable1=variableOrdering.getData(index);
  TADDUniqueTable* tempTable2=variableOrdering.getData(index+1);
  TADDInnerNode* currentTable=tempTable1->getFirst();
  TADDInnerNode* currentNode;
  TADDInnerNode* tempNode;
  while(currentTable){
    currentNode=currentTable;
    while(currentNode){
      tempNode=currentNode->getNext();

      if((currentNode->getSucc0()->getLevel()==index+1)||(currentNode->getSucc1()->getLevel()==index+1)||(currentNode->getSucc2()->getLevel()==index+1)||(currentNode->getSucc3()->getLevel()==index+1)){
        for(short i=0;i<4;++i){
          r[i]=currentNode->getSucc(i);
          if(r[i]->getLevel()==index+1){
            w[i][0]=static_cast<TADDInnerNode*>(r[i])->getSucc0();
            w[i][1]=static_cast<TADDInnerNode*>(r[i])->getSucc1();
            w[i][2]=static_cast<TADDInnerNode*>(r[i])->getSucc2();
            w[i][3]=static_cast<TADDInnerNode*>(r[i])->getSucc3();
          } else w[i][0]=w[i][1]=w[i][2]=w[i][3]=r[i];
        }

        for(short i=0;i<4;++i){
          if((w[0][i]==w[1][i]) && (w[1][i]==w[2][i]) && (w[2][i]==w[3][i])) r[i]=w[0][i];
          else {
            const TADDBaseNode* nodes[]={ w[0][i] , w[1][i], w[2][i], w[3][i] };
            r[i]=tempTable1->findOrAdd(nodes,innerMemPool);
          } 
        }

        tempTable1->remove(currentNode);
        currentNode->setNode(tempTable2,r[0],r[1],r[2],r[3],innerMemPool,terminalMemPool);
      }
      currentNode=tempNode;
    }
    currentTable=tempTable1->next();
  }

  variableOrdering.swapVariables(index);

  return true;
}
