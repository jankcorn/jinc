/***************************************************************************
                          taddfunction.cpp  -  description
                             -------------------
    begin                : Mon Dec 22 2008
    copyright            : (C) 2008-2009 by Joern Ossowski
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

#include <tadd/taddfunction.hpp>

#include <common/helper.hpp>
#include <common/ptrhashmap.hpp>
#include <common/mempool.hpp>
#include <common/bddalgorithms.hpp>

#include <tadd/taddbasenode.hpp>
#include <tadd/taddinnernode.hpp>
#include <tadd/taddterminalnode.hpp>
#include <tadd/taddvarorder.hpp>
#include <tadd/taddtraversalhelper.hpp>
#include <tadd/tadd.hpp>

namespace {
  inline TADDTerminalNode* findOrAddDrain(MemPool<TADDTerminalNode>& memPool, const double value){
    if UNLIKELY(ABS(value)<0.0000001) return TADD::zeroDrain;
    return TADD::drainUniqueTable.findOrAddValue(value, memPool);
  }
  inline TADDBaseNode* findOrAdd(MemPool<TADDInnerNode>& memPool, TADDUniqueTable* uTable, TADDBaseNode* succ0, TADDBaseNode* succ1, TADDBaseNode* succ2, TADDBaseNode* succ3){
    typedef TraversalHelper<TADDBaseNode> TADDTraversalHelper;
    typedef Common::SmartAccess<TADDBaseNode> SmartAccess;
    if((succ0==succ1)&&(succ1==succ2)&&(succ2==succ3)) return succ0;
    bool symmetry=(succ1==succ2) && SmartAccess(succ0)->isSymmetric() && SmartAccess(succ1)->isSymmetric() && SmartAccess(succ2)->isSymmetric() && SmartAccess(succ3)->isSymmetric();
    if((succ1<succ2) || ((succ1==succ2) && (succ0<=succ3))){
      const TADDBaseNode* nodes[] = { succ0, succ1 , succ2, succ3};
      TADDBaseNode* node=uTable->findOrAdd(nodes, memPool);
      if(symmetry) node->setSymmetry(true);
      return node;
    } else {
      if(!SmartAccess(succ0)->isSymmetric()) succ0=Common::flipBit(succ0);
      if(!SmartAccess(succ1)->isSymmetric()) succ1=Common::flipBit(succ1);
      if(!SmartAccess(succ2)->isSymmetric()) succ2=Common::flipBit(succ2);
      if(!SmartAccess(succ3)->isSymmetric()) succ3=Common::flipBit(succ3);
      const TADDBaseNode* nodes[] = { succ0, succ2 , succ1, succ3};
      TADDBaseNode* node=uTable->findOrAdd(nodes, memPool);
      if(symmetry){
        node->setSymmetry(true);
        return node;
      } else {
        return Common::setBit(node);
      }
    }
  }
}

inline void addFunction(TADDBaseNode* node){
  const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(node).getPtr())->incNodeCount();
}

inline void removeFunction(TADDBaseNode* node){
  const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(node).getPtr())->deleteNode();
}

//************************ADDFunction Iterator ************************
TADDFunction::iterator_4 TADDFunction::begin_4() const {
  return iterator_4(rootNode,TADD::variableOrdering.size());
}

TADDFunction::iterator_4 TADDFunction::end_4() const {
  return iterator_4();
}

TADDFunction::iterator TADDFunction::begin() const {
  return iterator(rootNode,2*TADD::variableOrdering.size());
}

TADDFunction::iterator TADDFunction::end() const {
  return iterator();
}

//************************TADDFunction************************
TADDFunction::TADDFunction(const TADDFunction& function) : rootNode(function.rootNode){
  addFunction(rootNode);
}

TADDFunction::TADDFunction(const Path<2>& p){
  Path<2> path=p;
  if(path.size()>TADD::variableOrdering.size()*2) throw("Path conversion error");
  
  TADDMemPools& pools=getTADDData().memPools;
  rootNode=findOrAddDrain(pools.terminalNodeMemPool,path.getValue());

  if(rootNode!=TADD::zeroDrain){
    MemPool<TADDInnerNode>& memPool=pools.innerNodeMemPool;
    for(VariableIndexType i=(path.size()%2)?path.size()+1:path.size();i>0;i-=2){
      TADDBaseNode *w0,*w1,*w2,*w3;
      switch((i-1)<path.size()?path[i-1]:-1){
        case 0: w1=w3=TADD::zeroDrain;
                w0=w2=rootNode;
                break;
        case 1: w0=w2=TADD::zeroDrain;
                w1=w3=rootNode;
                break;
        default:
                w0=w1=w2=w3=rootNode;
                break;
      }      
      switch(path[i-2]){
        case 0: w2=w3=TADD::zeroDrain;
                break;
        case 1: w0=w1=TADD::zeroDrain;
                break;
        default:
                break;
      }
      rootNode=findOrAdd(memPool,TADD::variableOrdering.getData(i/2-1),w0,w1,w2,w3);
    }
  }

  addFunction(rootNode);
}

TADDFunction::TADDFunction(const Path<4>& p){
  Path<4> path=p;
  if(path.size()>TADD::variableOrdering.size()) throw("Path conversion error");
  
  TADDMemPools& pools=getTADDData().memPools;
  rootNode=findOrAddDrain(pools.terminalNodeMemPool,path.getValue());

  if(rootNode!=TADD::zeroDrain){
    MemPool<TADDInnerNode>& memPool=pools.innerNodeMemPool;
    for(VariableIndexType i=path.size();i>0;--i){
      switch(path[i-1]){
        case 0: rootNode=findOrAdd(memPool,TADD::variableOrdering.getData(i-1),rootNode,TADD::zeroDrain,TADD::zeroDrain,TADD::zeroDrain);
                  break;
        case 1: rootNode=findOrAdd(memPool,TADD::variableOrdering.getData(i-1),TADD::zeroDrain,rootNode,TADD::zeroDrain,TADD::zeroDrain);
                  break;
        case 2: rootNode=findOrAdd(memPool,TADD::variableOrdering.getData(i-1),TADD::zeroDrain,TADD::zeroDrain,rootNode,TADD::zeroDrain);
                  break;
        case 3: rootNode=findOrAdd(memPool,TADD::variableOrdering.getData(i-1),TADD::zeroDrain,TADD::zeroDrain,TADD::zeroDrain,rootNode);
                  break;
        default:  break;
      }
    }
  }

  addFunction(rootNode);
}

TADDFunction::TADDFunction(const std::string& variable, unsigned short value){
  VariableIndexType index=TADD::variableOrdering[variable];
  if(index==VARIABLE_INDEX_MAX){
    if UNLIKELY(!TADD::variableOrdering.appendVariable(variable)) throw "Could not append variable";
    index=TADD::variableOrdering.size()-1;
  }

  rootNode=findOrAdd(getTADDData().memPools.innerNodeMemPool,TADD::variableOrdering.getData(index),(value==0)?TADD::oneDrain:TADD::zeroDrain,(value==1)?TADD::oneDrain:TADD::zeroDrain,(value==2)?TADD::oneDrain:TADD::zeroDrain,(value==3)?TADD::oneDrain:TADD::zeroDrain);
  addFunction(rootNode);
}

TADDFunction::TADDFunction(const double value){
  rootNode=findOrAddDrain(getTADDData().memPools.terminalNodeMemPool,value);
  addFunction(rootNode);
}

TADDFunction::TADDFunction(const TADDBaseNode* node) : rootNode(const_cast<TADDBaseNode*>(node)){addFunction(rootNode);}

TADDFunction::TADDFunction() : rootNode(TADD::zeroDrain) {addFunction(rootNode);}

TADDFunction::~TADDFunction(){removeFunction(rootNode);}

TADDFunction TADDFunction::ONE(){return TADDFunction(TADD::oneDrain);}

TADDFunction TADDFunction::ZERO(){return TADDFunction(TADD::zeroDrain);}

TADDFunction TADDFunction::createConstantFunction(const double value){return TADDFunction(value);}

TADDFunction TADDFunction::projection(const std::string& variable, unsigned short value){return TADDFunction(variable,value);}

TADDFunction TADDFunction::projection(const VariableIndexType index, unsigned short value){
  if(index>=TADD::variableOrdering.size()) throw "Index out of range";
  else {
    TADDBaseNode* ret=findOrAdd(getTADDData().memPools.innerNodeMemPool,TADD::variableOrdering.getData(index),(value==0)?TADD::oneDrain:TADD::zeroDrain,(value==1)?TADD::oneDrain:TADD::zeroDrain,(value==2)?TADD::oneDrain:TADD::zeroDrain,(value==3)?TADD::oneDrain:TADD::zeroDrain);
    return TADDFunction(ret);
  }
}

TADDFunction TADDFunction::projection_2(const VariableIndexType index, bool value){
  if(index>=2*TADD::variableOrdering.size()) throw "Index out of range";
  else {
    TADDBaseNode* w0=value?TADD::zeroDrain:TADD::oneDrain;
    TADDBaseNode* w3=value?TADD::oneDrain:TADD::zeroDrain;
    TADDBaseNode* w1=(index%2==0)?w0:w3;
    TADDBaseNode* w2=(index%2==0)?w3:w0;
    TADDBaseNode* ret=findOrAdd(getTADDData().memPools.innerNodeMemPool,TADD::variableOrdering.getData(index/2),w0,w1,w2,w3);
    return TADDFunction(ret);
  }
}

unsigned long TADDFunction::size() const {
  return ::size<TADDBaseNode>(rootNode);
}

TADDFunction& TADDFunction::operator=(const TADDFunction& function){
  addFunction(function.rootNode);
  removeFunction(rootNode);
  rootNode=function.rootNode;

  return *this;
}

template <typename T>
TADDBaseNode* comp(TraversalHelper<TADDBaseNode> v, PtrHashMap2<TADDBaseNode>& computedTable, TADDMemPools& pools){
  if(TADDBaseNode* comp=T::terminalCase(v.node.ptr,pools)) return comp;

  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(v.node.ptr,T::ID),v.node.ptr)) return comp;

  TADDUniqueTable* minTable=v.getPtr()->getUniqueTable();
  TADDBaseNode* w0=comp<T>(v.getSucc(0),computedTable,pools);
  TADDBaseNode* w1=comp<T>(v.getSucc(1),computedTable,pools);
  TADDBaseNode* w2=comp<T>(v.getSucc(2),computedTable,pools);
  TADDBaseNode* w3=comp<T>(v.getSucc(3),computedTable,pools);

  TADDBaseNode* node=findOrAdd(pools.innerNodeMemPool,minTable,w0,w1,w2,w3);

  computedTable.insert(Common::calcPtr(v.node.ptr,T::ID),v.node.ptr,node);

  return node;
}

template <typename T>
TADDBaseNode* comp(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>& computedTable, TADDMemPools& pools){
  if(TADDBaseNode* comp=T::terminalCase(v1.node.ptr,v2.node.ptr,computedTable,pools)) return comp;

  T::cacheOpt(v1.node.ptr,v2.node.ptr);

  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(v1.node.ptr,T::ID),v2.node.ptr)) return comp;

  TADDBaseNode* w0;
  TADDBaseNode* w1;
  TADDBaseNode* w2;
  TADDBaseNode* w3;
  TADDUniqueTable* minTable;

  if(v1.getPtr()->getRealLevel()<=v2.getPtr()->getRealLevel()){
    minTable=v1.getPtr()->getUniqueTable();
    if(v2.getPtr()->getUniqueTable()==minTable){
      w0=comp<T>(v1.getSucc(0),v2.getSucc(0),computedTable,pools);
      w1=comp<T>(v1.getSucc(1),v2.getSucc(1),computedTable,pools);
      w2=comp<T>(v1.getSucc(2),v2.getSucc(2),computedTable,pools);
      w3=comp<T>(v1.getSucc(3),v2.getSucc(3),computedTable,pools);
    } else {
      w0=comp<T>(v1.getSucc(0),v2,computedTable,pools);
      w1=comp<T>(v1.getSucc(1),v2,computedTable,pools);
      w2=comp<T>(v1.getSucc(2),v2,computedTable,pools);
      w3=comp<T>(v1.getSucc(3),v2,computedTable,pools);
    }
  } else {
    minTable=v2.getPtr()->getUniqueTable();
    w0=comp<T>(v1,v2.getSucc(0),computedTable,pools);
    w1=comp<T>(v1,v2.getSucc(1),computedTable,pools);
    w2=comp<T>(v1,v2.getSucc(2),computedTable,pools);
    w3=comp<T>(v1,v2.getSucc(3),computedTable,pools);
  }

  TADDBaseNode* node=findOrAdd(pools.innerNodeMemPool,minTable,w0,w1,w2,w3);

  computedTable.insert(Common::calcPtr(v1.node.ptr,T::ID),v2.node.ptr,node);

  return node;
}

template <unsigned short ID, typename U>
TADDBaseNode* abstract(TraversalHelper<TADDBaseNode> node, TraversalHelper<TADDBaseNode> varSet, PtrHashMap2<TADDBaseNode>& computedTable, TADDMemPools& pools){
  if(varSet.isDrain()) return const_cast<TADDBaseNode*>(node.node.ptr);

  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(node.node.ptr,ID),varSet.node.ptr)) return comp;
  
  if(varSet.getPtr()->getUniqueTable()==node.getPtr()->getUniqueTable()){
    unsigned short index=3;
    if(varSet.getSucc(1).node.ptr!=TADD::zeroDrain) index=1;
    else if(varSet.getSucc(2).node.ptr!=TADD::zeroDrain) index=2;
    TADDBaseNode* w0=abstract<ID,U>(node.getSucc(0),varSet.getSucc(index),computedTable,pools);
    TADDBaseNode* w1=abstract<ID,U>(node.getSucc(1),varSet.getSucc(index),computedTable,pools);
    TADDBaseNode* w2=abstract<ID,U>(node.getSucc(2),varSet.getSucc(index),computedTable,pools);
    TADDBaseNode* w3=abstract<ID,U>(node.getSucc(3),varSet.getSucc(index),computedTable,pools);
    
    TADDBaseNode* result;
    if(index==1){
      TADDBaseNode* t0=comp<U>(w0,w1,computedTable,pools);
      TADDBaseNode* t1=comp<U>(w2,w3,computedTable,pools);
      result=findOrAdd(pools.innerNodeMemPool,varSet.getPtr()->getUniqueTable(),t0,t0,t1,t1);
    } else if(index==2){
      TADDBaseNode* t0=findOrAdd(pools.innerNodeMemPool,varSet.getPtr()->getUniqueTable(),w0,w1,w0,w1);
      TADDBaseNode* t1=findOrAdd(pools.innerNodeMemPool,varSet.getPtr()->getUniqueTable(),w2,w3,w2,w3);
      result=comp<U>(t0,t1,computedTable,pools);
    } else {
      TADDBaseNode* t0=comp<U>(w0,w1,computedTable,pools);
      TADDBaseNode* t1=comp<U>(w2,w3,computedTable,pools);
      result=comp<U>(t0,t1,computedTable,pools);
    }
    computedTable.insert(Common::calcPtr(node.node.ptr,ID),varSet.node.ptr,result);

    return result;
  } else {
    TADDBaseNode* result;
    if(varSet.getPtr()->getRealLevel()<node.getPtr()->getRealLevel()){
      unsigned short index=3;
      if(varSet.getSucc(1).node.ptr!=TADD::zeroDrain) index=1;
      else if(varSet.getSucc(2).node.ptr!=TADD::zeroDrain) index=2;
      TADDBaseNode* w=abstract<ID,U>(node,varSet.getSucc(index),computedTable,pools);
      result=comp<U>(w,w,computedTable,pools);
      if(index==3) result=comp<U>(result,result,computedTable,pools);
    } else {
      TADDBaseNode* w0=abstract<ID,U>(node.getSucc(0),varSet,computedTable,pools);
      TADDBaseNode* w1=abstract<ID,U>(node.getSucc(1),varSet,computedTable,pools);
      TADDBaseNode* w2=abstract<ID,U>(node.getSucc(2),varSet,computedTable,pools);
      TADDBaseNode* w3=abstract<ID,U>(node.getSucc(3),varSet,computedTable,pools);
      result=findOrAdd(pools.innerNodeMemPool,node.getPtr()->getUniqueTable(),w0,w1,w2,w3);
    }
    computedTable.insert(Common::calcPtr(node.node.ptr,ID),varSet.node.ptr,result);
    
    return result;
  }
}

template <unsigned short ID, typename U>
TADDBaseNode* idempotentAbstract(TraversalHelper<TADDBaseNode> node, TraversalHelper<TADDBaseNode> varSet, PtrHashMap2<TADDBaseNode>& computedTable, TADDMemPools& pools){
  if(node.isDrain()) return const_cast<TADDBaseNode*>(node.node.ptr);

  TraversalHelper<TADDBaseNode> newVarSet=varSet;
  while(newVarSet.getPtr()->getRealLevel()<node.getPtr()->getRealLevel()){
    if(newVarSet.getSucc(1).node.ptr!=TADD::zeroDrain) newVarSet=newVarSet.getSucc(1);
    else if(newVarSet.getSucc(2).node.ptr!=TADD::zeroDrain) newVarSet=newVarSet.getSucc(2);
    else newVarSet=newVarSet.getSucc(2);
  }
  if(newVarSet.isDrain()) return const_cast<TADDBaseNode*>(node.node.ptr);

  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(node.node.ptr,ID),varSet.node.ptr)) return comp;
  
  if(varSet.getPtr()->getUniqueTable()==node.getPtr()->getUniqueTable()){
    unsigned short index=3;
    if(varSet.getSucc(1).node.ptr!=TADD::zeroDrain) index=1;
    else if(varSet.getSucc(2).node.ptr!=TADD::zeroDrain) index=2;
    TADDBaseNode* w0=idempotentAbstract<ID,U>(node.getSucc(0),varSet.getSucc(index),computedTable,pools);
    TADDBaseNode* w1=idempotentAbstract<ID,U>(node.getSucc(1),varSet.getSucc(index),computedTable,pools);
    TADDBaseNode* w2=idempotentAbstract<ID,U>(node.getSucc(2),varSet.getSucc(index),computedTable,pools);
    TADDBaseNode* w3=idempotentAbstract<ID,U>(node.getSucc(3),varSet.getSucc(index),computedTable,pools);
    
    TADDBaseNode* result;
    if(index==1){
      TADDBaseNode* t0=comp<U>(w0,w1,computedTable,pools);
      TADDBaseNode* t1=comp<U>(w2,w3,computedTable,pools);
      result=findOrAdd(pools.innerNodeMemPool,varSet.getPtr()->getUniqueTable(),t0,t0,t1,t1);
    } else if(index==2){
      TADDBaseNode* t0=findOrAdd(pools.innerNodeMemPool,varSet.getPtr()->getUniqueTable(),w0,w1,w0,w1);
      TADDBaseNode* t1=findOrAdd(pools.innerNodeMemPool,varSet.getPtr()->getUniqueTable(),w2,w3,w2,w3);
      result=comp<U>(t0,t1,computedTable,pools);
    } else {
      TADDBaseNode* t0=comp<U>(w0,w1,computedTable,pools);
      TADDBaseNode* t1=comp<U>(w2,w3,computedTable,pools);
      result=comp<U>(t0,t1,computedTable,pools);
    }
    computedTable.insert(Common::calcPtr(node.node.ptr,ID),varSet.node.ptr,result);

    return result;
  } else {
    TADDBaseNode* w0=idempotentAbstract<ID,U>(node.getSucc(0),varSet,computedTable,pools);
    TADDBaseNode* w1=idempotentAbstract<ID,U>(node.getSucc(1),varSet,computedTable,pools);
    TADDBaseNode* w2=idempotentAbstract<ID,U>(node.getSucc(2),varSet,computedTable,pools);
    TADDBaseNode* w3=idempotentAbstract<ID,U>(node.getSucc(3),varSet,computedTable,pools);
    TADDBaseNode* result=findOrAdd(pools.innerNodeMemPool,node.getPtr()->getUniqueTable(),w0,w1,w2,w3);
    computedTable.insert(Common::calcPtr(node.node.ptr,ID),varSet.node.ptr,result);
    
    return result;
  }
}

//****************************************************** ADDITION **********************************************************
struct TADD_ADDITION_1{
  static const unsigned short ID=ArithmeticConstants::ADD_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v, TADDMemPools& pools){
    if(v.isDrain()) return (v.node.ptr==TADD::zeroDrain)?TADD::zeroDrain:findOrAddDrain(pools.terminalNodeMemPool,2.0*v.getValue());
    return 0;
  }
};

struct TADD_ADDITION_2 : Common::COMMUTATIVE<TADDBaseNode>{
  static const unsigned short ID=ArithmeticConstants::ADD_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>& computedTable, TADDMemPools& pools){
    if(v1.node.ptr==TADD::zeroDrain) return const_cast<TADDBaseNode*>(v2.node.ptr);
    if(v2.node.ptr==TADD::zeroDrain) return const_cast<TADDBaseNode*>(v1.node.ptr);
    if(v1.node.ptr==v2.node.ptr) return ::comp<TADD_ADDITION_1>(v1.node.ptr,computedTable,pools);
    if(v1.isDrain() && v2.isDrain()) return findOrAddDrain(pools.terminalNodeMemPool,v1.getValue()+v2.getValue());
    return 0;
  }
};

TADDFunction addition(const TADDFunction& v1, const TADDFunction& v2){
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_ADDITION_2>(v1.rootNode,v2.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

TADDFunction& TADDFunction::operator+=(const TADDFunction& function){
  return *this=addition(*this,function);
}

TADDFunction& TADDFunction::operator++(){
  TADDData& data=getTADDData();
  return *this=TADDFunction(::comp<TADD_ADDITION_2>(rootNode,TADD::oneDrain,*data.computedTables.arithmeticHashMap,data.memPools));
}

//****************************************************** MULTIPLICATION **********************************************************
struct TADD_MULTIPLICATION_1{
  static const unsigned short ID=ArithmeticConstants::MUL_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v, TADDMemPools& pools){
    if(v.isDrain()) return (v.node.ptr==TADD::zeroDrain)?TADD::zeroDrain:findOrAddDrain(pools.terminalNodeMemPool,v.getValue()*v.getValue());
    return 0;
  }
};

struct TADD_MULTIPLICATION_2 :  Common::COMMUTATIVE<TADDBaseNode>{
  static const unsigned short ID=ArithmeticConstants::MUL_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>& computedTable, TADDMemPools& pools){
    if(v1.node.ptr==TADD::zeroDrain || v2.node.ptr==TADD::zeroDrain) return TADD::zeroDrain;
    if(v1.node.ptr==v2.node.ptr) return ::comp<TADD_MULTIPLICATION_1>(v1.node.ptr,computedTable,pools);
    if(v1.isDrain() && v2.isDrain()) return findOrAddDrain(pools.terminalNodeMemPool,v1.getValue()*v2.getValue());
    return 0;
  }
};

TADDFunction multiplication(const TADDFunction& v1, const TADDFunction& v2){
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_MULTIPLICATION_2>(v1.rootNode,v2.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

TADDFunction& TADDFunction::operator*=(const TADDFunction& function){
  return *this=multiplication(*this,function);
}

//****************************************************** SUBTRACTION/NEGATION **********************************************************
struct TADD_NEGATION{
  static const unsigned short ID=ArithmeticConstants::SUB_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v, TADDMemPools& pools){
    if(v.isDrain()) return (v.node.ptr==TADD::zeroDrain)?TADD::zeroDrain:findOrAddDrain(pools.terminalNodeMemPool,-v.getValue());
    return 0;
  }
};

struct TADD_SUBTRACTION_2 :  Common::NOT_COMMUTATIVE<TADDBaseNode>{
  static const unsigned short ID=ArithmeticConstants::SUB_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>& computedTable, TADDMemPools& pools){
    if(v1.node.ptr==TADD::zeroDrain) return ::comp<TADD_NEGATION>(v2.node.ptr,computedTable,pools);
    if(v2.node.ptr==TADD::zeroDrain) return const_cast<TADDBaseNode*>(v1.node.ptr);
    if(v1.node.ptr==v2.node.ptr) return TADD::zeroDrain;
    if(v1.isDrain() && v2.isDrain()) return findOrAddDrain(pools.terminalNodeMemPool,v1.getValue()-v2.getValue());
    return 0;
  }
};

TADDFunction operator-(const TADDFunction& function){
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_NEGATION>(function.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

TADDFunction subtraction(const TADDFunction& v1, const TADDFunction& v2){
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_SUBTRACTION_2>(v1.rootNode,v2.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

TADDFunction& TADDFunction::operator-=(const TADDFunction& function){
  return *this=subtraction(*this,function);
}

TADDFunction& TADDFunction::operator--(){
  TADDData& data=getTADDData();
  return *this=TADDFunction(::comp<TADD_SUBTRACTION_2>(rootNode,TADD::oneDrain,*data.computedTables.arithmeticHashMap,data.memPools));
}

//****************************************************** ONEOVER **********************************************************
struct TADD_ONEOVER{
  static const unsigned short ID=ArithmeticConstants::OVE_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v, TADDMemPools& pools){
    if(v.node.ptr==TADD::zeroDrain) return TADD::zeroDrain;
    if(v.isDrain()) return findOrAddDrain(pools.terminalNodeMemPool,1.0/v.getValue());
    return 0;
  }
};

TADDFunction TADDFunction::oneOver() const {
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_ONEOVER>(rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

//****************************************************** OR **********************************************************
struct TADD_OR :  Common::COMMUTATIVE<TADDBaseNode>{
  static const unsigned short ID=BooleanConstants::OR_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>&,  TADDMemPools&){
    if(v1.node.ptr==v2.node.ptr) return const_cast<TADDBaseNode*>(v1.node.ptr);
    if(v1.isDrain()) return (v1.node.ptr==TADD::zeroDrain)?const_cast<TADDBaseNode*>(v2.node.ptr):TADD::oneDrain;
    if(v2.isDrain()) return (v2.node.ptr==TADD::zeroDrain)?const_cast<TADDBaseNode*>(v1.node.ptr):TADD::oneDrain;
    return 0;
  }
};

TADDFunction& TADDFunction::operator|=(const TADDFunction& function){
  TADDData& data=getTADDData();
  return *this=TADDFunction(::comp<TADD_OR>(rootNode,function.rootNode,*data.computedTables.booleanHashMap,data.memPools));
}

//****************************************************** AND **********************************************************
struct TADD_AND :  Common::COMMUTATIVE<TADDBaseNode>{
  static const unsigned short ID=BooleanConstants::AND_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>&,  TADDMemPools&){
    if(v1.node.ptr==v2.node.ptr) return const_cast<TADDBaseNode*>(v1.node.ptr);
    if(v1.isDrain()) return (v1.node.ptr==TADD::zeroDrain)?TADD::zeroDrain:const_cast<TADDBaseNode*>(v2.node.ptr);
    if(v2.isDrain()) return (v2.node.ptr==TADD::zeroDrain)?TADD::zeroDrain:const_cast<TADDBaseNode*>(v1.node.ptr);
    return 0;
  }
};

TADDFunction& TADDFunction::operator&=(const TADDFunction& function){
  TADDData& data=getTADDData();
  return *this=TADDFunction(::comp<TADD_AND>(rootNode,function.rootNode,*data.computedTables.booleanHashMap,data.memPools));
}

//****************************************************** NOT **********************************************************
struct TADD_NOT{
  static const unsigned short ID=BooleanConstants::DIF_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v, TADDMemPools&){
    if(v.isDrain()) return (v.node.ptr==TADD::zeroDrain)?TADD::oneDrain:TADD::zeroDrain;
    return 0;
  }
};

TADDFunction TADDFunction::operator!() const {
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_NOT>(rootNode,*data.computedTables.booleanHashMap,data.memPools));
}

//****************************************************** LESS **********************************************************
struct TADD_LESS :  Common::NOT_COMMUTATIVE<TADDBaseNode>{
  static const unsigned short ID=CompareConstants::LES_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>&, TADDMemPools&){
    if(v1.node.ptr==v2.node.ptr) return TADD::zeroDrain;
    if(v1.isDrain() && v2.isDrain()) return (v1.getValue()<v2.getValue())?TADD::oneDrain:TADD::zeroDrain;
    return 0;
  }
};

TADDFunction LESS(const TADDFunction& u, const TADDFunction& v){
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_LESS>(u.rootNode,v.rootNode,*data.computedTables.compareHashMap,data.memPools));
}

//****************************************************** LESS_EQUAL **********************************************************
struct TADD_LESS_EQUAL :  Common::NOT_COMMUTATIVE<TADDBaseNode>{
  static const unsigned short ID=CompareConstants::LEE_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>&, TADDMemPools&){
    if(v1.node.ptr==v2.node.ptr) return TADD::oneDrain;
    if(v1.isDrain() && v2.isDrain()) return (v1.getValue()<=v2.getValue())?TADD::oneDrain:TADD::zeroDrain;
    return 0;
  }
};

TADDFunction LESS_EQUAL(const TADDFunction& u, const TADDFunction& v){
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_LESS_EQUAL>(u.rootNode,v.rootNode,*data.computedTables.compareHashMap,data.memPools));
}

//****************************************************** EQUAL **********************************************************
struct TADD_EQUAL :  Common::COMMUTATIVE<TADDBaseNode>{
  static const unsigned short ID=CompareConstants::EQU_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>&, TADDMemPools&){
    if(v1.node.ptr==v2.node.ptr) return TADD::oneDrain;
    if(v1.isDrain() && v2.isDrain()) return (v1.getValue()==v2.getValue())?TADD::oneDrain:TADD::zeroDrain;
    return 0;
  }
};

TADDFunction EQUAL(const TADDFunction& u, const TADDFunction& v){
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_EQUAL>(u.rootNode,v.rootNode,*data.computedTables.compareHashMap,data.memPools));
}

//****************************************************** MAX **********************************************************
struct TADD_MAX : Common::COMMUTATIVE<TADDBaseNode>{
  static const unsigned short ID=ExtremaConstants::MAX_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>&, TADDMemPools&){
    if(v1.node.ptr==v2.node.ptr) return const_cast<TADDBaseNode*>(v1.node.ptr);
    if(v1.isDrain() && v2.isDrain()) return (v1.getValue()<=v2.getValue())?const_cast<TADDBaseNode*>(v2.node.ptr):const_cast<TADDBaseNode*>(v1.node.ptr);
    return 0;
  }
};

TADDFunction MAXIMUM(const TADDFunction& v1, const TADDFunction& v2){
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_MAX>(v1.rootNode,v2.rootNode,*data.computedTables.extremaHashMap,data.memPools));
}

TADDBaseNode* max(TraversalHelper<TADDBaseNode> u, PtrHashMap2<TADDBaseNode>& hashMap){
  if(u.isDrain()) return const_cast<TADDBaseNode*>(u.node.ptr);

  if(TADDBaseNode* comp=hashMap.find(Common::calcPtr(u.node.ptr,ExtremaConstants::MAX_OP),0)) return comp;

  TADDBaseNode* w0=::max(u.getSucc(0),hashMap);
  TADDBaseNode* w1=::max(u.getSucc(1),hashMap);
  TADDBaseNode* w2=::max(u.getSucc(2),hashMap);
  TADDBaseNode* w3=::max(u.getSucc(3),hashMap);
  
  TADDBaseNode* node=(static_cast<TADDTerminalNode*>(w0)->getValue()>=static_cast<TADDTerminalNode*>(w1)->getValue())?w0:w1;
  if(static_cast<TADDTerminalNode*>(w2)->getValue()>static_cast<TADDTerminalNode*>(node)->getValue()) node=w2;
  if(static_cast<TADDTerminalNode*>(w3)->getValue()>static_cast<TADDTerminalNode*>(node)->getValue()) node=w3;
  hashMap.insert(Common::calcPtr(u.node.ptr,ExtremaConstants::MAX_OP),0,node);

  return node;
}

double TADDFunction::max() const {
  return static_cast<TADDTerminalNode*>(::max(rootNode,*getTADDData().computedTables.extremaHashMap))->getValue();
}

//****************************************************** MIN **********************************************************
struct TADD_MIN : Common::COMMUTATIVE<TADDBaseNode>{
  static const unsigned short ID=ExtremaConstants::MIN_OP;
  static inline TADDBaseNode* terminalCase(TraversalHelper<TADDBaseNode> v1, TraversalHelper<TADDBaseNode> v2, PtrHashMap2<TADDBaseNode>&, TADDMemPools&){
    if(v1.node.ptr==v2.node.ptr) return const_cast<TADDBaseNode*>(v1.node.ptr);
    if(v1.isDrain() && v2.isDrain()) return (v1.getValue()>=v2.getValue())?const_cast<TADDBaseNode*>(v2.node.ptr):const_cast<TADDBaseNode*>(v1.node.ptr);
    return 0;
  }
};

TADDFunction MINIMUM(const TADDFunction& v1, const TADDFunction& v2){
  TADDData& data=getTADDData();
  return TADDFunction(::comp<TADD_MIN>(v1.rootNode,v2.rootNode,*data.computedTables.extremaHashMap,data.memPools));
}

TADDBaseNode* min(TraversalHelper<TADDBaseNode> u, PtrHashMap2<TADDBaseNode>& hashMap){
  if(u.isDrain()) return const_cast<TADDBaseNode*>(u.node.ptr);

  if(TADDBaseNode* comp=hashMap.find(Common::calcPtr(u.node.ptr,ExtremaConstants::MIN_OP),0)) return comp;

  TADDBaseNode* w0=::min(u.getSucc(0),hashMap);
  TADDBaseNode* w1=::min(u.getSucc(1),hashMap);
  TADDBaseNode* w2=::min(u.getSucc(2),hashMap);
  TADDBaseNode* w3=::min(u.getSucc(3),hashMap);
  
  TADDBaseNode* node=(static_cast<TADDTerminalNode*>(w0)->getValue()<=static_cast<TADDTerminalNode*>(w1)->getValue())?w0:w1;
  if(static_cast<TADDTerminalNode*>(w2)->getValue()<static_cast<TADDTerminalNode*>(node)->getValue()) node=w2;
  if(static_cast<TADDTerminalNode*>(w3)->getValue()<static_cast<TADDTerminalNode*>(node)->getValue()) node=w3;
  hashMap.insert(Common::calcPtr(u.node.ptr,ExtremaConstants::MIN_OP),0,node);

  return node;
}

double TADDFunction::min() const {
  return static_cast<TADDTerminalNode*>(::min(rootNode,*getTADDData().computedTables.extremaHashMap))->getValue();
}

//****************************************************** EXISTS/FORALL **********************************************************
TADDFunction TADDFunction::exists(const TADDFunction& varSet) const {
  TADDData& data=getTADDData();
  return TADDFunction(::idempotentAbstract<BooleanConstants::EXI_OP,TADD_OR>(rootNode,varSet.rootNode,*data.computedTables.booleanHashMap,data.memPools));
}

TADDFunction TADDFunction::forall(const TADDFunction& varSet) const {
  TADDFunction temp=!(*this);
  return !temp.exists(varSet);
}

//****************************************************** SUM/PRODUCT/MAXIMUM/MINIMUM **********************************************************
TADDFunction TADDFunction::sum(const TADDFunction& varSet) const {
  TADDData& data=getTADDData();
  return TADDFunction(::abstract<ArithmeticConstants::SUM_OP,TADD_ADDITION_2>(rootNode,varSet.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

TADDFunction TADDFunction::product(const TADDFunction& varSet) const {
  TADDData& data=getTADDData();
  return TADDFunction(::abstract<ArithmeticConstants::PRO_OP,TADD_MULTIPLICATION_2>(rootNode,varSet.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

TADDFunction TADDFunction::maximum(const TADDFunction& varSet) const {
  TADDData& data=getTADDData();
  return TADDFunction(::idempotentAbstract<ExtremaConstants::MAXIMUM_OP,TADD_MAX>(rootNode,varSet.rootNode,*data.computedTables.extremaHashMap,data.memPools));
}

TADDFunction TADDFunction::minimum(const TADDFunction& varSet) const {
  TADDData& data=getTADDData();
  return TADDFunction(::idempotentAbstract<ExtremaConstants::MINIMUM_OP,TADD_MIN>(rootNode,varSet.rootNode,*data.computedTables.extremaHashMap,data.memPools));
}

//****************************************************** CHANGE **********************************************************
TADDBaseNode* change(TraversalHelper<TADDBaseNode> v, PtrHashMap2<TADDBaseNode>& computedTable, MemPool<TADDInnerNode>& memPool){
  if(v.isDrain()) return const_cast<TADDBaseNode*>(v.node.ptr);

  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(v.node.ptr,BooleanConstants::CHA_OP),0)) return comp;

  TADDBaseNode* w0=::change(v.getSucc(0),computedTable,memPool);
  TADDBaseNode* w1=::change(v.getSucc(1),computedTable,memPool);
  TADDBaseNode* w2=::change(v.getSucc(2),computedTable,memPool);
  TADDBaseNode* w3=::change(v.getSucc(3),computedTable,memPool);

  TADDBaseNode* node=findOrAdd(memPool,v.getPtr()->getUniqueTable(),w3,w2,w1,w0);

  computedTable.insert(Common::calcPtr(v.node.ptr,BooleanConstants::CHA_OP),0,node);

  return node;
}

TADDBaseNode* change(TraversalHelper<TADDBaseNode> v, TraversalHelper<TADDBaseNode> cubeSet, PtrHashMap2<TADDBaseNode>& computedTable, MemPool<TADDInnerNode>& memPool){
  if(v.isDrain()) return const_cast<TADDBaseNode*>(v.node.ptr);

  while(cubeSet.getPtr()->getRealLevel()<v.getPtr()->getRealLevel()){
    if(cubeSet.getSucc(1).node.ptr!=TADD::zeroDrain) cubeSet=cubeSet.getSucc(1);
    else if(cubeSet.getSucc(2).node.ptr!=TADD::zeroDrain) cubeSet=cubeSet.getSucc(2);
    else cubeSet=cubeSet.getSucc(2);
  }
  if(cubeSet.isDrain()) return const_cast<TADDBaseNode*>(v.node.ptr);

  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(v.node.ptr,BooleanConstants::CHA_OP),cubeSet.node.ptr)) return comp;

  TADDBaseNode* result;
  TADDUniqueTable* minTable=v.getPtr()->getUniqueTable();
  if(cubeSet.getPtr()->getUniqueTable()==minTable){
    unsigned short index=3;
    if(cubeSet.getSucc(1).node.ptr!=TADD::zeroDrain) index=1;
    else if(cubeSet.getSucc(2).node.ptr!=TADD::zeroDrain) index=2;
    TADDBaseNode* w0=::change(v.getSucc(0),cubeSet.getSucc(index),computedTable,memPool);
    TADDBaseNode* w1=::change(v.getSucc(1),cubeSet.getSucc(index),computedTable,memPool);
    TADDBaseNode* w2=::change(v.getSucc(2),cubeSet.getSucc(index),computedTable,memPool);
    TADDBaseNode* w3=::change(v.getSucc(3),cubeSet.getSucc(index),computedTable,memPool);

    if(index==1) result=findOrAdd(memPool,minTable,w1,w0,w3,w2);
    else if(index==2) result=findOrAdd(memPool,minTable,w2,w3,w0,w1);
    else result=findOrAdd(memPool,minTable,w3,w2,w1,w0);
  } else {
    TADDBaseNode* w0=::change(v.getSucc(0),cubeSet,computedTable,memPool);
    TADDBaseNode* w1=::change(v.getSucc(1),cubeSet,computedTable,memPool);
    TADDBaseNode* w2=::change(v.getSucc(2),cubeSet,computedTable,memPool);
    TADDBaseNode* w3=::change(v.getSucc(3),cubeSet,computedTable,memPool);

    result=findOrAdd(memPool,minTable,w0,w1,w2,w3);
  }

  computedTable.insert(Common::calcPtr(v.node.ptr,BooleanConstants::CHA_OP),cubeSet.node.ptr,result);

  return result;
}

TADDFunction TADDFunction::change() const {
  TADDData& data=getTADDData();
  return TADDFunction(::change(rootNode,*data.computedTables.booleanHashMap,data.memPools.innerNodeMemPool));
}

TADDFunction TADDFunction::change(const TADDFunction& cubeSet) const {
  TADDData& data=getTADDData();
  return TADDFunction(::change(rootNode,cubeSet.rootNode,*data.computedTables.booleanHashMap,data.memPools.innerNodeMemPool));
}

//****************************************************** COFACTOR/SUBSET **********************************************************
TADDBaseNode* cofactor(TraversalHelper<TADDBaseNode> node, TraversalHelper<TADDBaseNode> assignment, PtrHashMap2<TADDBaseNode>& computedTable, MemPool<TADDInnerNode>& memPool){
  if(node.isDrain()) return const_cast<TADDBaseNode*>(node.node.ptr);
  
  while(assignment.getPtr()->getRealLevel()<node.getPtr()->getRealLevel()){
    if(assignment.getSucc(0).node.ptr!=TADD::zeroDrain) assignment=assignment.getSucc(0);
    else if(assignment.getSucc(1).node.ptr!=TADD::zeroDrain) assignment=assignment.getSucc(1);
    else if(assignment.getSucc(2).node.ptr!=TADD::zeroDrain) assignment=assignment.getSucc(2);
    else assignment=assignment.getSucc(3);
  }
  
  if(assignment.isDrain()) return const_cast<TADDBaseNode*>(node.node.ptr);
  
  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(node.node.ptr,BooleanConstants::COF_OP),assignment.node.ptr)) return comp;
  
  //do it the hard way
  TADDBaseNode* w0;
  TADDBaseNode* w1;
  TADDBaseNode* w2;
  TADDBaseNode* w3;
  TADDBaseNode* tempNode;
  if(assignment.getPtr()->getUniqueTable()==node.getPtr()->getUniqueTable()){
    if(assignment.getSucc(0).node.ptr!=TADD::zeroDrain){
      if((assignment.getSucc(0).node.ptr==assignment.getSucc(1).node.ptr) && (assignment.getSucc(2).node.ptr==assignment.getSucc(3).node.ptr)){
        w0=w2=cofactor(node.getSucc(0),assignment.getSucc(0),computedTable,memPool);
        w1=w3=cofactor(node.getSucc(1),assignment.getSucc(0),computedTable,memPool);
	tempNode=findOrAdd(memPool,node.getPtr()->getUniqueTable(),w0,w1,w2,w3);
      } else if((assignment.getSucc(0).node.ptr==assignment.getSucc(2).node.ptr) && (assignment.getSucc(1).node.ptr==assignment.getSucc(3).node.ptr)){
        w0=w1=cofactor(node.getSucc(0),assignment.getSucc(0),computedTable,memPool);
        w2=w3=cofactor(node.getSucc(2),assignment.getSucc(0),computedTable,memPool);
	tempNode=findOrAdd(memPool,node.getPtr()->getUniqueTable(),w0,w1,w2,w3);
      } else tempNode=cofactor(node.getSucc(0),assignment.getSucc(0),computedTable,memPool);
    } else if(assignment.getSucc(1).node.ptr!=TADD::zeroDrain){
      if((assignment.getSucc(0).node.ptr==assignment.getSucc(2).node.ptr) && (assignment.getSucc(1).node.ptr==assignment.getSucc(3).node.ptr)){
        w0=w1=cofactor(node.getSucc(1),assignment.getSucc(1),computedTable,memPool);
        w2=w3=cofactor(node.getSucc(3),assignment.getSucc(1),computedTable,memPool);
	tempNode=findOrAdd(memPool,node.getPtr()->getUniqueTable(),w0,w1,w2,w3);
      } else tempNode=cofactor(node.getSucc(1),assignment.getSucc(1),computedTable,memPool);
    } else if(assignment.getSucc(2).node.ptr!=TADD::zeroDrain){
      if((assignment.getSucc(0).node.ptr==assignment.getSucc(1).node.ptr) && (assignment.getSucc(2).node.ptr==assignment.getSucc(3).node.ptr)){
        w0=w2=cofactor(node.getSucc(2),assignment.getSucc(0),computedTable,memPool);
        w1=w3=cofactor(node.getSucc(3),assignment.getSucc(0),computedTable,memPool);
	tempNode=findOrAdd(memPool,node.getPtr()->getUniqueTable(),w0,w1,w2,w3);
      } else tempNode=cofactor(node.getSucc(2),assignment.getSucc(2),computedTable,memPool);
    } else tempNode=cofactor(node.getSucc(3),assignment.getSucc(3),computedTable,memPool);
  } else {
    w0=cofactor(node.getSucc(0),assignment,computedTable,memPool);
    w1=cofactor(node.getSucc(1),assignment,computedTable,memPool);
    w2=cofactor(node.getSucc(2),assignment,computedTable,memPool);
    w3=cofactor(node.getSucc(3),assignment,computedTable,memPool);

    tempNode=findOrAdd(memPool,node.getPtr()->getUniqueTable(),w0,w1,w2,w3);
  }
  
  computedTable.insert(Common::calcPtr(node.node.ptr,BooleanConstants::COF_OP),assignment.node.ptr,tempNode);
  
  return tempNode;
}

TADDFunction TADDFunction::cofactor(const TADDFunction& assignment) const {
  TADDData& data=getTADDData();
  return TADDFunction(::cofactor(rootNode,assignment.rootNode,*data.computedTables.booleanHashMap,data.memPools.innerNodeMemPool));
}

TADDFunction TADDFunction::subSet(const TADDFunction& assignment) const {
  return ((*this)*assignment);
}

//****************************************************************************************************************
TADDFunction& TADDFunction::operator^=(const TADDFunction& function){
  return *this=ITE(*this,!function,function);
}

bool TADDFunction::operator==(const TADDFunction& function) const {
  return (function.rootNode==rootNode);
}

bool TADDFunction::operator<(const TADDFunction& function) const {
  return (rootNode<function.rootNode);
}

bool TADDFunction::isConstant() const {
  return TraversalHelper<TADDBaseNode>(rootNode).isDrain();
}

double TADDFunction::getValue() const {
  return TraversalHelper<TADDBaseNode>(rootNode).getValue();
}

VariableIndexType TADDFunction::getFirstVariableIndex() const {
  return TraversalHelper<TADDBaseNode>(rootNode).getLevel();
}

// TADDFunction TADDFunction::equivalenceClasses(const TADDFunction& pattern, const TADDFunction& cubeSet) const {
//   return TADDFunction::BDD->equivalenceClasses(*this,pattern,cubeSet);
// }
//
// TADDFunction TADDFunction::constrain(const TADDFunction& g) const {
//   return TADDFunction::BDD->constrain(*this,g);
// }

TADDFunction TADDFunction::compose(const VariableIndexType index, const TADDFunction& function) const {
  //TODO: check ITE semantics and compose operator
  if(index>=TADDVarOrder::size()) return (*this);
  else return ITE(function,cofactor(TADDFunction::projection(index,1)),cofactor(TADDFunction::projection(index,0)));
}

TADDFunction TADDFunction::toggle() const {
  return TADDFunction(Common::SmartAccess<TADDBaseNode>(rootNode)->isSymmetric()?rootNode:Common::flipBit(rootNode));
}

/*
template <int N>
TADDBaseNode* moveUp(const TADDBaseNode* u, PtrHashMap3<TADDBaseNode>& computedTable, MemPool<TADDInnerNode>& memPool){
  if(u->isDrain()) return const_cast<TADDBaseNode*>(u);
  
  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(u,MoveConstants::MUP_OP),0,reinterpret_cast<TADDBaseNode*>(N))) return comp;

  TADDBaseNode* w0=moveUp<N>(static_cast<const TADDInnerNode*>(u)->getSucc0(),computedTable,memPool);
  TADDBaseNode* w1=moveUp<N>(static_cast<const TADDInnerNode*>(u)->getSucc1(),computedTable,memPool);
  TADDBaseNode* w2=moveUp<N>(static_cast<const TADDInnerNode*>(u)->getSucc2(),computedTable,memPool);
  TADDBaseNode* w3=moveUp<N>(static_cast<const TADDInnerNode*>(u)->getSucc3(),computedTable,memPool);
  TADDBaseNode* node=findOrAdd(memPool,TADD::variableOrdering.getData(u->getLevel()-N),w0,w1,w2,w3);
  computedTable.insert(Common::calcPtr(u,MoveConstants::MUP_OP),0,reinterpret_cast<TADDBaseNode*>(N),node);

  return node;
}

TADDFunction TADDFunction::moveUp() const {
  if(isConstant()) return TADDFunction(rootNode);

  return TADDFunction(::moveUp<1>(rootNode,getTADDMoveHashMap(),getTADDInnerNodeMemPool()));
}

template <int N>
TADDBaseNode* moveUp(const TADDBaseNode* u, const TADDBaseNode* v, PtrHashMap3<TADDBaseNode>& computedTable, MemPool<TADDInnerNode>& memPool){
  if(u->isDrain()) return const_cast<TADDBaseNode*>(u);

  VariableIndexType uLevel=u->getLevel();
  while(v->getLevel()<uLevel) v=static_cast<const TADDInnerNode*>(v)->getSucc1();

  if(v->isDrain()) return const_cast<TADDBaseNode*>(u);
  if((uLevel<N)&&(uLevel==v->getLevel())) return const_cast<TADDBaseNode*>(u);

  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(u,MoveConstants::MUP_OP),v,reinterpret_cast<TADDBaseNode*>(N))) return comp;

  TADDBaseNode* w0;
  TADDBaseNode* w1;
  TADDBaseNode* node;
  if(uLevel==v->getLevel()){
    w0=moveUp<N>(static_cast<const TADDInnerNode*>(u)->getSucc0(),static_cast<const TADDInnerNode*>(v)->getSucc1(),computedTable,memPool);
    w1=moveUp<N>(static_cast<const TADDInnerNode*>(u)->getSucc1(),static_cast<const TADDInnerNode*>(v)->getSucc1(),computedTable,memPool);

    node=findOrAdd(memPool,TADD::variableOrdering.getData(uLevel-N),w0,w1);
  } else {
    w0=moveUp<N>(static_cast<const TADDInnerNode*>(u)->getSucc0(),v,computedTable,memPool);
    w1=moveUp<N>(static_cast<const TADDInnerNode*>(u)->getSucc1(),v,computedTable,memPool);

    if((w0->getLevel()<=uLevel)||(w1->getLevel()<=uLevel)){
      node=const_cast<TADDBaseNode*>(u);
    } else node=findOrAdd(memPool,TADD::variableOrdering.getData(uLevel),w0,w1);
  }

  computedTable.insert(Common::calcPtr(u,MoveConstants::MUP_OP),v,reinterpret_cast<TADDBaseNode*>(N),node);

  return node;
}

TADDFunction TADDFunction::moveUp(const TADDFunction& varSet) const {
  if(isConstant()) return TADDFunction(rootNode);

  return TADDFunction(::moveUp<1>(rootNode,const_cast<TADDBaseNode*>(varSet.rootNode),getTADDMoveHashMap(),getTADDInnerNodeMemPool()));
}

template <int N>
TADDBaseNode* moveDown(const TADDBaseNode* u, PtrHashMap3<TADDBaseNode>& computedTable, MemPool<TADDInnerNode>& memPool){
  if(u->isDrain()) return const_cast<TADDBaseNode*>(u);

  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(u,MoveConstants::MDO_OP),0,reinterpret_cast<TADDBaseNode*>(N))) return comp;

  TADDBaseNode* w0=moveDown<N>(static_cast<const TADDInnerNode*>(u)->getSucc0(),computedTable,memPool);
  TADDBaseNode* w1=moveDown<N>(static_cast<const TADDInnerNode*>(u)->getSucc1(),computedTable,memPool);
  TADDBaseNode* w2=moveDown<N>(static_cast<const TADDInnerNode*>(u)->getSucc2(),computedTable,memPool);
  TADDBaseNode* w3=moveDown<N>(static_cast<const TADDInnerNode*>(u)->getSucc3(),computedTable,memPool);

  TADDBaseNode* node=(u->getLevel()+N>=TADD::variableOrdering.size())?static_cast<TADDBaseNode*>(TADD::zeroDrain):findOrAdd(memPool,TADD::variableOrdering.getData(u->getLevel()+N),w0,w1,w2,w3);
  computedTable.insert(Common::calcPtr(u,MoveConstants::MDO_OP),0,reinterpret_cast<TADDBaseNode*>(N),node);

  return node;
}

TADDFunction TADDFunction::moveDown() const {
  if(isConstant()) return TADDFunction(rootNode);

  return TADDFunction(::moveDown<1>(rootNode,getTADDMoveHashMap(),getTADDInnerNodeMemPool()));
}

template <int N>
TADDBaseNode* moveDown(const TADDBaseNode* u, const TADDBaseNode* v, PtrHashMap3<TADDBaseNode>& computedTable, MemPool<TADDInnerNode>& memPool){
  if(u->isDrain()) return const_cast<TADDBaseNode*>(u);

  VariableIndexType uLevel=u->getLevel();
  while(v->getLevel()<uLevel) v=static_cast<const TADDInnerNode*>(v)->getSucc1();

  if(v->isDrain()) return const_cast<TADDBaseNode*>(u);

  if(TADDBaseNode* comp=computedTable.find(Common::calcPtr(u,MoveConstants::MDO_OP),v,reinterpret_cast<TADDBaseNode*>(N))) return comp;

  TADDBaseNode* w0;
  TADDBaseNode* w1;
  TADDBaseNode* node;
  if(uLevel==v->getLevel()){
    w0=moveDown<N>(static_cast<const TADDInnerNode*>(u)->getSucc0(),static_cast<const TADDInnerNode*>(v)->getSucc1(),computedTable,memPool);
    w1=moveDown<N>(static_cast<const TADDInnerNode*>(u)->getSucc1(),static_cast<const TADDInnerNode*>(v)->getSucc1(),computedTable,memPool);

    if((uLevel+N>=w0->getLevel())||(uLevel+N>=w1->getLevel())) node=findOrAdd(memPool,TADD::variableOrdering.getData(uLevel),w0,w1);
    else node=findOrAdd(memPool,TADD::variableOrdering.getData(uLevel+N),w0,w1);
  } else {
    w0=moveDown<N>(static_cast<const TADDInnerNode*>(u)->getSucc0(),v,computedTable,memPool);
    w1=moveDown<N>(static_cast<const TADDInnerNode*>(u)->getSucc1(),v,computedTable,memPool);

    node=findOrAdd(memPool,u->getUniqueTable(),w0,w1);
  }

  computedTable.insert(Common::calcPtr(u,MoveConstants::MDO_OP),v,reinterpret_cast<TADDBaseNode*>(N),node);

  return node;
}

TADDFunction TADDFunction::moveDown(const TADDFunction& varSet) const {
  if(isConstant()) return TADDFunction(rootNode);

  return TADDFunction(::moveDown<1>(rootNode,const_cast<TADDBaseNode*>(varSet.rootNode),getTADDMoveHashMap(),getTADDInnerNodeMemPool()));
}
*/
boost::dynamic_bitset<> TADDFunction::getEssentialVariables() const {
  return ::getEssentialVariables<TADDBaseNode,TADD>(rootNode);
}

TADDFunction ITE(const TADDFunction& p_if, const TADDFunction& p_then, const TADDFunction& p_else){
  return TADDFunction((p_if&p_then)|(!p_if&p_else));
}


TADDBaseNode* MM(TraversalHelper<TADDBaseNode> A, TraversalHelper<TADDBaseNode> B, TraversalHelper<TADDBaseNode> varSet, const VariableIndexType m, TADDComputedTables& computedTables, TADDMemPools& pools){
  if((A.node.ptr==TADD::zeroDrain) || (B.node.ptr==TADD::zeroDrain)) return TADD::zeroDrain;
  if((A.isDrain() && B.isDrain()) || (m==0)) return findOrAddDrain(pools.terminalNodeMemPool,A.getValue()*B.getValue()*static_cast<double>(1<<m));

  if(TADDBaseNode* comp=computedTables.matrixHashMap->find(Common::calcPtr(A.node.ptr,MatrixConstants::MMA_OP),B.node.ptr,varSet.node.ptr)) return comp;

  TADDUniqueTable* uniqueTable=varSet.getPtr()->getUniqueTable();

  TraversalHelper<TADDBaseNode> A0, A1, A2, A3;
  if(uniqueTable==A.getPtr()->getUniqueTable()){
    A0=A.getSucc(0);
    A1=A.getSucc(1);
    A2=A.getSucc(2);
    A3=A.getSucc(3);
  } else A0=A1=A2=A3=A;

  TraversalHelper<TADDBaseNode> B0, B1, B2, B3;
  if(uniqueTable==B.getPtr()->getUniqueTable()){
    B0=B.getSucc(0);
    B1=B.getSucc(1);
    B2=B.getSucc(2);
    B3=B.getSucc(3);
  } else B0=B1=B2=B3=B;

  TraversalHelper<TADDBaseNode> newVarSet=varSet.getSucc(2);

  TADDBaseNode* w00=::MM(A0,B0,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w12=::MM(A1,B2,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w01=::MM(A0,B1,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w13=::MM(A1,B3,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w20=::MM(A2,B0,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w32=::MM(A3,B2,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w21=::MM(A2,B1,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w33=::MM(A3,B3,newVarSet,m-1,computedTables,pools);

  TADDBaseNode* c0=::comp<TADD_ADDITION_2>(w00,w12,*computedTables.arithmeticHashMap,pools);
  TADDBaseNode* c1=::comp<TADD_ADDITION_2>(w01,w13,*computedTables.arithmeticHashMap,pools);
  TADDBaseNode* c2=::comp<TADD_ADDITION_2>(w20,w32,*computedTables.arithmeticHashMap,pools);
  TADDBaseNode* c3=::comp<TADD_ADDITION_2>(w21,w33,*computedTables.arithmeticHashMap,pools);

  TADDBaseNode* result=findOrAdd(pools.innerNodeMemPool,uniqueTable,c0,c1,c2,c3);

  computedTables.matrixHashMap->insert(Common::calcPtr(A.node.ptr,MatrixConstants::MMA_OP),B.node.ptr,varSet.node.ptr,result);

  return result;
}

TADDFunction MM(const TADDFunction& A, const TADDFunction& B, const TADDFunction& varSet){
  TraversalHelper<TADDBaseNode> node=varSet.rootNode;

  VariableIndexType m=0;
  while(!node.isDrain()){
    node=node.getSucc(2);
    ++m;
  }

  TADDData& data=getTADDData();
  return TADDFunction(::MM(A.rootNode,B.rootNode,varSet.rootNode,m,data.computedTables,data.memPools));
}

TADDBaseNode* MV(TraversalHelper<TADDBaseNode> A, TraversalHelper<TADDBaseNode> b, TraversalHelper<TADDBaseNode> varSet, const VariableIndexType m, TADDComputedTables& computedTables, TADDMemPools& pools){
  if((A.node.ptr==TADD::zeroDrain) || (b.node.ptr==TADD::zeroDrain)) return TADD::zeroDrain;
  if((A.isDrain() && b.isDrain()) || (m==0)) return findOrAddDrain(pools.terminalNodeMemPool,A.getValue()*b.getValue()*static_cast<double>(1<<m));

  if(TADDBaseNode* comp=computedTables.matrixHashMap->find(Common::calcPtr(A.node.ptr,MatrixConstants::MAV_OP),b.node.ptr,varSet.node.ptr)) return comp;

  TADDUniqueTable* uniqueTable=varSet.getPtr()->getUniqueTable();

  TraversalHelper<TADDBaseNode> A0, A1, A2, A3;
  if(uniqueTable==A.getPtr()->getUniqueTable()){
    A0=A.getSucc(0);
    A1=A.getSucc(1);
    A2=A.getSucc(2);
    A3=A.getSucc(3);
  } else A0=A1=A2=A3=A;

  TraversalHelper<TADDBaseNode> b0, b1;
  if(uniqueTable==b.getPtr()->getUniqueTable()){
    b0=b.getSucc(0);
    b1=b.getSucc(2);
  } else b0=b1=b;

  TraversalHelper<TADDBaseNode> newVarSet=varSet.getSucc(2);

  TADDBaseNode* w00=::MV(A0,b0,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w11=::MV(A1,b1,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w20=::MV(A2,b0,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w31=::MV(A3,b1,newVarSet,m-1,computedTables,pools);

  TADDBaseNode* c0=::comp<TADD_ADDITION_2>(w00,w11,*computedTables.arithmeticHashMap,pools);
  TADDBaseNode* c1=::comp<TADD_ADDITION_2>(w20,w31,*computedTables.arithmeticHashMap,pools);

  TADDBaseNode* result=findOrAdd(pools.innerNodeMemPool,uniqueTable,c0,c0,c1,c1);

  computedTables.matrixHashMap->insert(Common::calcPtr(A.node.ptr,MatrixConstants::MAV_OP),b.node.ptr,varSet.node.ptr,result);

  return result;
}

TADDFunction MV(const TADDFunction& A, const TADDFunction& b, const TADDFunction& varSet){
  TraversalHelper<TADDBaseNode> node=varSet.rootNode;

  VariableIndexType m=0;
  while(!node.isDrain()){
    node=node.getSucc(2);
    ++m;
  }

  TADDData& data=getTADDData();
  return TADDFunction(::MV(A.rootNode,b.rootNode,varSet.rootNode,m,data.computedTables,data.memPools));
}

TADDBaseNode* VM(TraversalHelper<TADDBaseNode> b, TraversalHelper<TADDBaseNode> A, TraversalHelper<TADDBaseNode> varSet, const VariableIndexType m, TADDComputedTables& computedTables, TADDMemPools& pools){
  if((A.node.ptr==TADD::zeroDrain) || (b.node.ptr==TADD::zeroDrain)) return TADD::zeroDrain;
  if((A.isDrain() && b.isDrain()) || (m==0)) return findOrAddDrain(pools.terminalNodeMemPool,A.getValue()*b.getValue()*static_cast<double>(1<<m));

  if(TADDBaseNode* comp=computedTables.matrixHashMap->find(Common::calcPtr(b.node.ptr,MatrixConstants::VMA_OP),A.node.ptr,varSet.node.ptr)) return comp;

  TADDUniqueTable* uniqueTable=varSet.getPtr()->getUniqueTable();

  TraversalHelper<TADDBaseNode> A0, A1, A2, A3;
  if(uniqueTable==A.getPtr()->getUniqueTable()){
    A0=A.getSucc(0);
    A1=A.getSucc(1);
    A2=A.getSucc(2);
    A3=A.getSucc(3);
  } else A0=A1=A2=A3=A;

  TraversalHelper<TADDBaseNode> b0, b1;
  if(uniqueTable==b.getPtr()->getUniqueTable()){
    b0=b.getSucc(0);
    b1=b.getSucc(2);
  } else b0=b1=b;

  TraversalHelper<TADDBaseNode> newVarSet=varSet.getSucc(2);

  TADDBaseNode* w00=::VM(b0,A0,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w12=::VM(b1,A2,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w01=::VM(b0,A1,newVarSet,m-1,computedTables,pools);
  TADDBaseNode* w13=::VM(b1,A3,newVarSet,m-1,computedTables,pools);

  TADDBaseNode* c0=::comp<TADD_ADDITION_2>(w00,w12,*computedTables.arithmeticHashMap,pools);
  TADDBaseNode* c1=::comp<TADD_ADDITION_2>(w01,w13,*computedTables.arithmeticHashMap,pools);

  TADDBaseNode* result=findOrAdd(pools.innerNodeMemPool,uniqueTable,c0,c0,c1,c1);

  computedTables.matrixHashMap->insert(Common::calcPtr(b.node.ptr,MatrixConstants::VMA_OP),A.node.ptr,varSet.node.ptr,result);

  return result;
}

TADDFunction VM(const TADDFunction& b, const TADDFunction& A, const TADDFunction& varSet){
  TraversalHelper<TADDBaseNode> node=varSet.rootNode;

  VariableIndexType m=0;
  while(!node.isDrain()){
    node=node.getSucc(2);
    ++m;
  }

  TADDData& data=getTADDData();
  return TADDFunction(::VM(b.rootNode,A.rootNode,varSet.rootNode,m,data.computedTables,data.memPools));
}
