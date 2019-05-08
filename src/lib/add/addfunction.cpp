/***************************************************************************
                          addfunction.cpp  -  description
                             -------------------
    begin                : Thu Apr 15 2004
    copyright            : (C) 2004-2009 by Joern Ossowski
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

#include <add/addfunction.hpp>

#include <common/helper.hpp>
#include <common/ptrhashmap.hpp>
#include <common/mempool.hpp>
#include <common/bddalgorithms.hpp>

#include <add/addbasenode.hpp>
#include <add/addinnernode.hpp>
#include <add/addterminalnode.hpp>
#include <add/addvarorder.hpp>
#include <add/addtraversalhelper.hpp>
#include <add/add.hpp>

namespace {
  inline ADDTerminalNode* findOrAddDrain(MemPool<ADDTerminalNode>& memPool, const double value){
    if UNLIKELY(ABS(value)<0.0000001) return ADD::zeroDrain;
    return ADD::drainUniqueTable.findOrAddValue(value, memPool);
  }
  inline ADDInnerNode* findOrAdd(MemPool<ADDInnerNode>& memPool, ADDUniqueTable* uTable, ADDBaseNode* succ0, ADDBaseNode* succ1){
    const ADDBaseNode* nodes[] = { succ0, succ1 };
    return uTable->findOrAdd(nodes, memPool);
  }
}

inline void addFunction(ADDBaseNode* node){
  node->incNodeCount();
}

inline void removeFunction(ADDBaseNode* node){
  node->deleteNode();
}

//************************ADDFunction Iterator ************************
ADDFunction::iterator ADDFunction::begin() const {
  return iterator(rootNode,ADD::variableOrdering.size());
}

ADDFunction::iterator ADDFunction::end() const {
  return iterator();
}

//************************ADDFunction************************
ADDFunction::ADDFunction(const ADDFunction& function) : rootNode(function.rootNode){
  addFunction(rootNode);
}

ADDFunction::ADDFunction(const Path<2>& p){
  Path<2> path=p;
  if(path.size()>ADD::variableOrdering.size()) throw("Path conversion error");
  
  ADDMemPools& pools=getADDData().memPools;
  rootNode=findOrAddDrain(pools.terminalNodeMemPool,path.getValue());

  if(rootNode!=ADD::zeroDrain){
    MemPool<ADDInnerNode>& memPool=pools.innerNodeMemPool;
    for(VariableIndexType i=path.size();i>0;--i){
      switch(path[i-1]){
        case 0: rootNode=findOrAdd(memPool,ADD::variableOrdering.getData(i-1),rootNode,ADD::zeroDrain);
                  break;
        case 1: rootNode=findOrAdd(memPool,ADD::variableOrdering.getData(i-1),ADD::zeroDrain,rootNode);
                  break;
        default:  break;
      }
    }
  }

  addFunction(rootNode);
}

ADDFunction::ADDFunction(const std::string& variable, bool value){
  VariableIndexType index=ADD::variableOrdering[variable];
  if(index==VARIABLE_INDEX_MAX){
    if UNLIKELY(!ADD::variableOrdering.appendVariable(variable)) throw "Could not append variable";
    index=ADD::variableOrdering.size()-1;
  }
  
  rootNode=findOrAdd(getADDData().memPools.innerNodeMemPool,ADD::variableOrdering.getData(index),value?ADD::zeroDrain:ADD::oneDrain,value?ADD::oneDrain:ADD::zeroDrain);
  addFunction(rootNode);
}

ADDFunction::ADDFunction(const double value){
  rootNode=findOrAddDrain(getADDData().memPools.terminalNodeMemPool,value);
  addFunction(rootNode);
}

ADDFunction::ADDFunction(const ADDBaseNode* node) : rootNode(const_cast<ADDBaseNode*>(node)){addFunction(rootNode);}

ADDFunction::ADDFunction() : rootNode(ADD::zeroDrain) {addFunction(rootNode);}

ADDFunction::~ADDFunction(){removeFunction(rootNode);}

ADDFunction ADDFunction::ONE(){return ADDFunction(ADD::oneDrain);}

ADDFunction ADDFunction::ZERO(){return ADDFunction(ADD::zeroDrain);}

ADDFunction ADDFunction::createConstantFunction(const double value){return ADDFunction(value);}

ADDFunction ADDFunction::projection(const std::string& variable, bool value){return ADDFunction(variable,value);}

ADDFunction ADDFunction::projection(const VariableIndexType index, bool value){
  if(index>=ADD::variableOrdering.size()) throw "Index out of range";
  else {
    ADDBaseNode* ret=findOrAdd(getADDData().memPools.innerNodeMemPool,ADD::variableOrdering.getData(index),value?ADD::zeroDrain:ADD::oneDrain,value?ADD::oneDrain:ADD::zeroDrain);
    return ADDFunction(ret);
  }
}

unsigned long ADDFunction::size() const {
  return ::size<ADDBaseNode>(rootNode);
}

ADDFunction& ADDFunction::operator=(const ADDFunction& function){
  addFunction(function.rootNode);
  removeFunction(rootNode);
  rootNode=function.rootNode;

  return *this;
}

template <typename T>
ADDBaseNode* comp(const ADDBaseNode* v, PtrHashMap2<ADDBaseNode>& computedTable, ADDMemPools& pools){
  if(ADDBaseNode* comp=T::terminalCase(v,pools)) return comp;

  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(v,T::ID),v)) return comp;

  ADDUniqueTable* minTable=v->getUniqueTable();
  ADDBaseNode* w0=comp<T>(static_cast<const ADDInnerNode*>(v)->getSucc0(),computedTable,pools);
  ADDBaseNode* w1=comp<T>(static_cast<const ADDInnerNode*>(v)->getSucc1(),computedTable,pools);

  ADDBaseNode* node=(w0==w1)?w0:findOrAdd(pools.innerNodeMemPool,minTable,w0,w1);

  computedTable.insert(Common::calcPtr(v,T::ID),v,node);

  return node;
}

template <typename T>
ADDBaseNode* comp(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>& computedTable, ADDMemPools& pools){
  if(ADDBaseNode* comp=T::terminalCase(v1,v2,computedTable,pools)) return comp;

  T::cacheOpt(v1,v2);

  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(v1,T::ID),v2)) return comp;

  ADDBaseNode* w0;
  ADDBaseNode* w1;
  ADDUniqueTable* minTable;

  if(v1->getRealLevel()<=v2->getRealLevel()){
    minTable=v1->getUniqueTable();
    if(v2->getUniqueTable()==minTable){
      w0=comp<T>(static_cast<const ADDInnerNode*>(v1)->getSucc0(),static_cast<const ADDInnerNode*>(v2)->getSucc0(),computedTable,pools);
      w1=comp<T>(static_cast<const ADDInnerNode*>(v1)->getSucc1(),static_cast<const ADDInnerNode*>(v2)->getSucc1(),computedTable,pools);
    } else {
      w0=comp<T>(static_cast<const ADDInnerNode*>(v1)->getSucc0(),v2,computedTable,pools);
      w1=comp<T>(static_cast<const ADDInnerNode*>(v1)->getSucc1(),v2,computedTable,pools);
    }
  } else {
    minTable=v2->getUniqueTable();
    w0=comp<T>(v1,static_cast<const ADDInnerNode*>(v2)->getSucc0(),computedTable,pools);
    w1=comp<T>(v1,static_cast<const ADDInnerNode*>(v2)->getSucc1(),computedTable,pools);
  }

  ADDBaseNode* node=(w0==w1)?w0:findOrAdd(pools.innerNodeMemPool,minTable,w0,w1);

  computedTable.insert(Common::calcPtr(v1,T::ID),v2,node);

  return node;
}

template <unsigned short ID, typename U>
ADDBaseNode* abstract(const ADDBaseNode* node, const ADDBaseNode* varSet, PtrHashMap2<ADDBaseNode>& computedTable, ADDMemPools& pools){
  if(varSet->isDrain()) return const_cast<ADDBaseNode*>(node);
    
  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(node,ID),varSet)) return comp;
  
  if(varSet->getUniqueTable()==node->getUniqueTable()){
    ADDBaseNode* w0=abstract<ID,U>(static_cast<const ADDInnerNode*>(node)->getSucc0(),static_cast<const ADDInnerNode*>(varSet)->getSucc1(),computedTable,pools);
    ADDBaseNode* w1=abstract<ID,U>(static_cast<const ADDInnerNode*>(node)->getSucc1(),static_cast<const ADDInnerNode*>(varSet)->getSucc1(),computedTable,pools);
    ADDBaseNode* result=comp<U>(w0,w1,computedTable,pools);
    computedTable.insert(Common::calcPtr(node,ID),varSet,result);
    
    return result;
  } else {
    ADDBaseNode* result;
    if(varSet->getRealLevel()<node->getRealLevel()){
      ADDBaseNode* w=abstract<ID,U>(node,static_cast<const ADDInnerNode*>(varSet)->getSucc1(),computedTable,pools);
      result=comp<U>(w,w,computedTable,pools);
    } else {
      ADDBaseNode* w0=abstract<ID,U>(static_cast<const ADDInnerNode*>(node)->getSucc0(),varSet,computedTable,pools);
      ADDBaseNode* w1=abstract<ID,U>(static_cast<const ADDInnerNode*>(node)->getSucc1(),varSet,computedTable,pools);
      result=(w0==w1)?w0:findOrAdd(pools.innerNodeMemPool,node->getUniqueTable(),w0,w1);
    }
    computedTable.insert(Common::calcPtr(node,ID),varSet,result);
    
    return result;
  }
}

template <unsigned short ID, typename U>
ADDBaseNode* idempotentAbstract(const ADDBaseNode* node, const ADDBaseNode* varSet, PtrHashMap2<ADDBaseNode>& computedTable, ADDMemPools& pools){
  if(node->isDrain()) return const_cast<ADDBaseNode*>(node);
  
  ADDBaseNode* newVarSet=const_cast<ADDBaseNode*>(varSet);
  while(newVarSet->getRealLevel()<node->getRealLevel()) newVarSet=static_cast<ADDInnerNode*>(newVarSet)->getSucc1();
  if(newVarSet->isDrain()) return const_cast<ADDBaseNode*>(node);
  
  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(node,ID),newVarSet)) return comp;
  
  if(newVarSet->getUniqueTable()==node->getUniqueTable()){
    ADDBaseNode* w0=idempotentAbstract<ID,U>(static_cast<const ADDInnerNode*>(node)->getSucc0(),static_cast<ADDInnerNode*>(newVarSet)->getSucc1(),computedTable,pools);
    ADDBaseNode* w1=idempotentAbstract<ID,U>(static_cast<const ADDInnerNode*>(node)->getSucc1(),static_cast<ADDInnerNode*>(newVarSet)->getSucc1(),computedTable,pools);
    ADDBaseNode* result=comp<U>(w0,w1,computedTable,pools);
    computedTable.insert(Common::calcPtr(node,ID),newVarSet,result);
    
    return result;
  } else {
    ADDBaseNode* w0=idempotentAbstract<ID,U>(static_cast<const ADDInnerNode*>(node)->getSucc0(),newVarSet,computedTable,pools);
    ADDBaseNode* w1=idempotentAbstract<ID,U>(static_cast<const ADDInnerNode*>(node)->getSucc1(),newVarSet,computedTable,pools);
    ADDBaseNode* result=(w0==w1)?w0:findOrAdd(pools.innerNodeMemPool,node->getUniqueTable(),w0,w1);
    computedTable.insert(Common::calcPtr(node,ID),newVarSet,result);
    
    return result;
  }
}

//****************************************************** ADDITION **********************************************************
struct ADD_ADDITION_1{
  static const unsigned short ID=ArithmeticConstants::ADD_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v, ADDMemPools& pools){
    if(v->isDrain()) return (v==ADD::zeroDrain)?ADD::zeroDrain:findOrAddDrain(pools.terminalNodeMemPool,2.0*static_cast<const ADDTerminalNode*>(v)->getValue());
    return 0;
  }
};

struct ADD_ADDITION_2 : Common::COMMUTATIVE<ADDBaseNode>{
  static const unsigned short ID=ArithmeticConstants::ADD_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>& computedTable, ADDMemPools& pools){
    if(v1==ADD::zeroDrain) return const_cast<ADDBaseNode*>(v2);
    if(v2==ADD::zeroDrain) return const_cast<ADDBaseNode*>(v1);
    if(v1==v2) return ::comp<ADD_ADDITION_1>(v1,computedTable,pools);
    if(v1->isDrain() && v2->isDrain()) return findOrAddDrain(pools.terminalNodeMemPool,static_cast<const ADDTerminalNode*>(v1)->getValue()+static_cast<const ADDTerminalNode*>(v2)->getValue());
    return 0;
  }
};

ADDFunction addition(const ADDFunction& v1, const ADDFunction& v2){
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_ADDITION_2>(v1.rootNode,v2.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

ADDFunction& ADDFunction::operator+=(const ADDFunction& function){
  return *this=addition(*this,function);
}

ADDFunction& ADDFunction::operator++(){
  ADDData& data=getADDData();
  return *this=ADDFunction(::comp<ADD_ADDITION_2>(rootNode,ADD::oneDrain,*data.computedTables.arithmeticHashMap,data.memPools));
}

//****************************************************** MULTIPLICATION **********************************************************
struct ADD_MULTIPLICATION_1{
  static const unsigned short ID=ArithmeticConstants::MUL_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v, ADDMemPools& pools){
    if(v->isDrain()) return (v==ADD::zeroDrain)?ADD::zeroDrain:findOrAddDrain(pools.terminalNodeMemPool,static_cast<const ADDTerminalNode*>(v)->getValue()*static_cast<const ADDTerminalNode*>(v)->getValue());
    return 0;
  }
};

struct ADD_MULTIPLICATION_2 :  Common::COMMUTATIVE<ADDBaseNode>{
  static const unsigned short ID=ArithmeticConstants::MUL_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>& computedTable, ADDMemPools& pools){
    if(v1==ADD::zeroDrain || v2==ADD::zeroDrain) return ADD::zeroDrain;
    if(v1==v2) return ::comp<ADD_MULTIPLICATION_1>(v1,computedTable,pools);
    if(v1->isDrain() && v2->isDrain()) return findOrAddDrain(pools.terminalNodeMemPool,static_cast<const ADDTerminalNode*>(v1)->getValue()*static_cast<const ADDTerminalNode*>(v2)->getValue());
    return 0;
  }
};

ADDFunction multiplication(const ADDFunction& v1, const ADDFunction& v2){
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_MULTIPLICATION_2>(v1.rootNode,v2.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

ADDFunction& ADDFunction::operator*=(const ADDFunction& function){
  return *this=multiplication(*this,function);
}

//****************************************************** SUBTRACTION/NEGATION **********************************************************
struct ADD_NEGATION{
  static const unsigned short ID=ArithmeticConstants::SUB_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v, ADDMemPools& pools){
    if(v->isDrain()) return (v==ADD::zeroDrain)?ADD::zeroDrain:findOrAddDrain(pools.terminalNodeMemPool,-static_cast<const ADDTerminalNode*>(v)->getValue());
    return 0;
  }
};

struct ADD_SUBTRACTION_2 :  Common::NOT_COMMUTATIVE<ADDBaseNode>{
  static const unsigned short ID=ArithmeticConstants::SUB_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>& computedTable, ADDMemPools& pools){
    if(v1==ADD::zeroDrain) return ::comp<ADD_NEGATION>(v2,computedTable,pools);
    if(v2==ADD::zeroDrain) return const_cast<ADDBaseNode*>(v1);
    if(v1==v2) return ADD::zeroDrain;
    if(v1->isDrain() && v2->isDrain()) return findOrAddDrain(pools.terminalNodeMemPool,static_cast<const ADDTerminalNode*>(v1)->getValue()-static_cast<const ADDTerminalNode*>(v2)->getValue());
    return 0;
  }
};

ADDFunction operator-(const ADDFunction& function){
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_NEGATION>(const_cast<ADDBaseNode*>(function.rootNode),*data.computedTables.arithmeticHashMap,data.memPools));
}

ADDFunction subtraction(const ADDFunction& v1, const ADDFunction& v2){
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_SUBTRACTION_2>(v1.rootNode,v2.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

ADDFunction& ADDFunction::operator-=(const ADDFunction& function){
  return *this=subtraction(*this,function);
}

ADDFunction& ADDFunction::operator--(){
  ADDData& data=getADDData();
  return *this=ADDFunction(::comp<ADD_SUBTRACTION_2>(rootNode,ADD::oneDrain,*data.computedTables.arithmeticHashMap,data.memPools));
}

//****************************************************** ONEOVER **********************************************************
struct ADD_ONEOVER{
  static const unsigned short ID=ArithmeticConstants::OVE_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v, ADDMemPools& pools){
    if(v==ADD::zeroDrain) return ADD::zeroDrain;
    if(v->isDrain()) return findOrAddDrain(pools.terminalNodeMemPool,1.0/static_cast<const ADDTerminalNode*>(v)->getValue());
    return 0;
  }
};

ADDFunction ADDFunction::oneOver() const {
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_ONEOVER>(rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

//****************************************************** OR **********************************************************
struct ADD_OR :  Common::COMMUTATIVE<ADDBaseNode>{
  static const unsigned short ID=BooleanConstants::OR_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>&, ADDMemPools&){
    if(v1==v2) return const_cast<ADDBaseNode*>(v1);
    if(v1->isDrain()) return (v1==ADD::zeroDrain)?const_cast<ADDBaseNode*>(v2):ADD::oneDrain;
    if(v2->isDrain()) return (v2==ADD::zeroDrain)?const_cast<ADDBaseNode*>(v1):ADD::oneDrain;
    return 0;
  }
};

ADDFunction& ADDFunction::operator|=(const ADDFunction& function){
  ADDData& data=getADDData();
  return *this=ADDFunction(::comp<ADD_OR>(rootNode,function.rootNode,*data.computedTables.booleanHashMap,data.memPools));
}

//****************************************************** AND **********************************************************
struct ADD_AND : Common::COMMUTATIVE<ADDBaseNode>{
  static const unsigned short ID=BooleanConstants::AND_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>&, ADDMemPools&){
    if(v1==v2) return const_cast<ADDBaseNode*>(v1);
    if(v1->isDrain()) return (v1==ADD::zeroDrain)?ADD::zeroDrain:const_cast<ADDBaseNode*>(v2);
    if(v2->isDrain()) return (v2==ADD::zeroDrain)?ADD::zeroDrain:const_cast<ADDBaseNode*>(v1);
    return 0;
  }
};

ADDFunction& ADDFunction::operator&=(const ADDFunction& function){
  ADDData& data=getADDData();
  return *this=ADDFunction(::comp<ADD_AND>(rootNode,function.rootNode,*data.computedTables.booleanHashMap,data.memPools));
}

//****************************************************** NOT **********************************************************
struct ADD_NOT{
  static const unsigned short ID=BooleanConstants::DIF_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v, ADDMemPools&){
    if(v->isDrain()) return (v==ADD::zeroDrain)?ADD::oneDrain:ADD::zeroDrain;
    return 0;
  }
};

ADDFunction ADDFunction::operator!() const {
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_NOT>(rootNode,*data.computedTables.booleanHashMap,data.memPools));
}

//****************************************************** LESS **********************************************************
struct ADD_LESS :  Common::NOT_COMMUTATIVE<ADDBaseNode>{
  static const unsigned short ID=CompareConstants::LES_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>&, ADDMemPools&){
    if(v1==v2) return ADD::zeroDrain;
    if(v1->isDrain() && v2->isDrain()) return (static_cast<const ADDTerminalNode*>(v1)->getValue()<static_cast<const ADDTerminalNode*>(v2)->getValue())?ADD::oneDrain:ADD::zeroDrain;
    return 0;
  }
};

ADDFunction LESS(const ADDFunction& u, const ADDFunction& v){
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_LESS>(u.rootNode,v.rootNode,*data.computedTables.compareHashMap,data.memPools));
}

//****************************************************** LESS_EQUAL **********************************************************
struct ADD_LESS_EQUAL :  Common::NOT_COMMUTATIVE<ADDBaseNode>{
  static const unsigned short ID=CompareConstants::LEE_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>&, ADDMemPools&){
    if(v1==v2) return ADD::oneDrain;
    if(v1->isDrain() && v2->isDrain()) return (static_cast<const ADDTerminalNode*>(v1)->getValue()<=static_cast<const ADDTerminalNode*>(v2)->getValue())?ADD::oneDrain:ADD::zeroDrain;
    return 0;
  }
};

ADDFunction LESS_EQUAL(const ADDFunction& u, const ADDFunction& v){
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_LESS_EQUAL>(u.rootNode,v.rootNode,*data.computedTables.compareHashMap,data.memPools));
}

//****************************************************** EQUAL **********************************************************
struct ADD_EQUAL :  Common::COMMUTATIVE<ADDBaseNode>{
  static const unsigned short ID=CompareConstants::EQU_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>&, ADDMemPools&){
    if(v1==v2) return ADD::oneDrain;
    if(v1->isDrain() && v2->isDrain()) return (static_cast<const ADDTerminalNode*>(v1)->getValue()==static_cast<const ADDTerminalNode*>(v2)->getValue())?ADD::oneDrain:ADD::zeroDrain;
    return 0;
  }
};

ADDFunction EQUAL(const ADDFunction& u, const ADDFunction& v){
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_EQUAL>(u.rootNode,v.rootNode,*data.computedTables.compareHashMap,data.memPools));
}

//****************************************************** MAX **********************************************************
struct ADD_MAX : Common::COMMUTATIVE<ADDBaseNode>{
  static const unsigned short ID=ExtremaConstants::MAX_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>&, ADDMemPools&){
    if(v1==v2) return const_cast<ADDBaseNode*>(v1);
    if(v1->isDrain() && v2->isDrain()) return (static_cast<const ADDTerminalNode*>(v1)->getValue()<=static_cast<const ADDTerminalNode*>(v2)->getValue())?const_cast<ADDBaseNode*>(v2):const_cast<ADDBaseNode*>(v1);
    return 0;
  }
};

ADDFunction MAXIMUM(const ADDFunction& v1, const ADDFunction& v2){
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_MAX>(v1.rootNode,v2.rootNode,*data.computedTables.extremaHashMap,data.memPools));
}

ADDBaseNode* max(const ADDBaseNode* u, PtrHashMap2<ADDBaseNode>& hashMap){
  if(u->isDrain()) return const_cast<ADDBaseNode*>(u);

  if(ADDBaseNode* comp=hashMap.find(Common::calcPtr(u,ExtremaConstants::MAX_OP),0)) return comp;

  ADDBaseNode* w0;
  ADDBaseNode* w1;

  w0=::max(static_cast<const ADDInnerNode*>(u)->getSucc0(),hashMap);
  w1=::max(static_cast<const ADDInnerNode*>(u)->getSucc1(),hashMap);

  ADDBaseNode* node=(static_cast<ADDTerminalNode*>(w0)->getValue()>=static_cast<ADDTerminalNode*>(w1)->getValue())?w0:w1;
  hashMap.insert(Common::calcPtr(u,ExtremaConstants::MAX_OP),0,node);

  return node;
}

double ADDFunction::max() const {
  return static_cast<ADDTerminalNode*>(::max(rootNode,*getADDData().computedTables.extremaHashMap))->getValue();
}

//****************************************************** MIN **********************************************************
struct ADD_MIN : Common::COMMUTATIVE<ADDBaseNode>{
  static const unsigned short ID=ExtremaConstants::MIN_OP;
  static inline ADDBaseNode* terminalCase(const ADDBaseNode* v1, const ADDBaseNode* v2, PtrHashMap2<ADDBaseNode>&, ADDMemPools&){
    if(v1==v2) return const_cast<ADDBaseNode*>(v1);
    if(v1->isDrain() && v2->isDrain()) return (static_cast<const ADDTerminalNode*>(v1)->getValue()<=static_cast<const ADDTerminalNode*>(v2)->getValue())?const_cast<ADDBaseNode*>(v1):const_cast<ADDBaseNode*>(v2);
    return 0;
  }
};

ADDFunction MINIMUM(const ADDFunction& v1, const ADDFunction& v2){
  ADDData& data=getADDData();
  return ADDFunction(::comp<ADD_MIN>(v1.rootNode,v2.rootNode,*data.computedTables.extremaHashMap,data.memPools));
}

ADDBaseNode* min(const ADDBaseNode* u, PtrHashMap2<ADDBaseNode>& hashMap){
  if(u->isDrain()) return const_cast<ADDBaseNode*>(u);

  if(ADDBaseNode* comp=hashMap.find(Common::calcPtr(u,ExtremaConstants::MIN_OP),0)) return comp;

  ADDBaseNode* w0;
  ADDBaseNode* w1;

  w0=::min(static_cast<const ADDInnerNode*>(u)->getSucc0(),hashMap);
  w1=::min(static_cast<const ADDInnerNode*>(u)->getSucc1(),hashMap);

  ADDBaseNode* node=(static_cast<ADDTerminalNode*>(w0)->getValue()<=static_cast<ADDTerminalNode*>(w1)->getValue())?w0:w1;
  hashMap.insert(Common::calcPtr(u,ExtremaConstants::MIN_OP),0,node);

  return node;
}

double ADDFunction::min() const {
  return static_cast<ADDTerminalNode*>(::min(rootNode,*getADDData().computedTables.extremaHashMap))->getValue();
}

//****************************************************** EXISTS/FORALL **********************************************************
ADDFunction ADDFunction::exists(const ADDFunction& varSet) const {
  ADDData& data=getADDData();
  return ADDFunction(::idempotentAbstract<BooleanConstants::EXI_OP,ADD_OR>(rootNode,varSet.rootNode,*data.computedTables.booleanHashMap,data.memPools));
}

ADDFunction ADDFunction::forall(const ADDFunction& varSet) const {
  ADDFunction temp=!(*this);
  return !temp.exists(varSet);
}

//****************************************************** SUM/PRODUCT/MAXIMUM/MINIMUM **********************************************************
ADDFunction ADDFunction::sum(const ADDFunction& varSet) const {
  ADDData& data=getADDData();
  return ADDFunction(::abstract<ArithmeticConstants::SUM_OP,ADD_ADDITION_2>(rootNode,varSet.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

ADDFunction ADDFunction::product(const ADDFunction& varSet) const {
  ADDData& data=getADDData();
  return ADDFunction(::abstract<ArithmeticConstants::PRO_OP,ADD_MULTIPLICATION_2>(rootNode,varSet.rootNode,*data.computedTables.arithmeticHashMap,data.memPools));
}

ADDFunction ADDFunction::maximum(const ADDFunction& varSet) const {
  ADDData& data=getADDData();
  return ADDFunction(::idempotentAbstract<ExtremaConstants::MAXIMUM_OP,ADD_MAX>(rootNode,varSet.rootNode,*data.computedTables.extremaHashMap,data.memPools));
}

ADDFunction ADDFunction::minimum(const ADDFunction& varSet) const {
  ADDData& data=getADDData();
  return ADDFunction(::idempotentAbstract<ExtremaConstants::MINIMUM_OP,ADD_MIN>(rootNode,varSet.rootNode,*data.computedTables.extremaHashMap,data.memPools));
}

//****************************************************** CHANGE **********************************************************
ADDBaseNode* change(const ADDBaseNode* v, PtrHashMap2<ADDBaseNode>& computedTable, MemPool<ADDInnerNode>& memPool){
  if(v->isDrain()) return const_cast<ADDBaseNode*>(v);

  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(v,BooleanConstants::CHA_OP),0)) return comp;

  ADDUniqueTable* minTable=v->getUniqueTable();
  ADDBaseNode* w0=::change(static_cast<const ADDInnerNode*>(v)->getSucc0(),computedTable,memPool);
  ADDBaseNode* w1=::change(static_cast<const ADDInnerNode*>(v)->getSucc1(),computedTable,memPool);

  ADDBaseNode* node=(w0==w1)?w0:findOrAdd(memPool,minTable,w1,w0);

  computedTable.insert(Common::calcPtr(v,BooleanConstants::CHA_OP),0,node);

  return node;
}

ADDBaseNode* change(const ADDBaseNode* v, ADDBaseNode* cubeSet, PtrHashMap2<ADDBaseNode>& computedTable, MemPool<ADDInnerNode>& memPool){
  if(v->isDrain()) return const_cast<ADDBaseNode*>(v);

  while(cubeSet->getRealLevel()<v->getRealLevel()) cubeSet=static_cast<ADDInnerNode*>(cubeSet)->getSucc1();
  if(cubeSet->isDrain()) return const_cast<ADDBaseNode*>(v);

  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(v,BooleanConstants::CHA_OP),cubeSet)) return comp;

  ADDBaseNode* result;
  ADDUniqueTable* minTable=v->getUniqueTable();
  if(cubeSet->getUniqueTable()==v->getUniqueTable()){
    ADDBaseNode* w0=::change(static_cast<const ADDInnerNode*>(v)->getSucc0(),static_cast<ADDInnerNode*>(cubeSet)->getSucc1(),computedTable,memPool);
    ADDBaseNode* w1=::change(static_cast<const ADDInnerNode*>(v)->getSucc1(),static_cast<ADDInnerNode*>(cubeSet)->getSucc1(),computedTable,memPool);

    result=(w0==w1)?w0:findOrAdd(memPool,minTable,w1,w0);
  } else {
    ADDBaseNode* w0=::change(static_cast<const ADDInnerNode*>(v)->getSucc0(),cubeSet,computedTable,memPool);
    ADDBaseNode* w1=::change(static_cast<const ADDInnerNode*>(v)->getSucc1(),cubeSet,computedTable,memPool);

    result=(w0==w1)?w0:findOrAdd(memPool,minTable,w0,w1);
  }

  computedTable.insert(Common::calcPtr(v,BooleanConstants::CHA_OP),cubeSet,result);

  return result;
}

ADDFunction ADDFunction::change() const {
  ADDData& data=getADDData();
  return ADDFunction(::change(rootNode,*data.computedTables.booleanHashMap,data.memPools.innerNodeMemPool));
}

ADDFunction ADDFunction::change(const ADDFunction& cubeSet) const {
  ADDData& data=getADDData();
  return ADDFunction(::change(rootNode,cubeSet.rootNode,*data.computedTables.booleanHashMap,data.memPools.innerNodeMemPool));
}

//****************************************************** COFACTOR/SUBSET **********************************************************
ADDBaseNode* cofactor(const ADDBaseNode* node, const ADDBaseNode* assignment, PtrHashMap2<ADDBaseNode>& computedTable, MemPool<ADDInnerNode>& memPool){
  if(node->isDrain()) return const_cast<ADDBaseNode*>(node);
  
  while(assignment->getRealLevel()<node->getRealLevel()){
    assignment=(static_cast<const ADDInnerNode*>(assignment)->getSucc0()==ADD::zeroDrain)?static_cast<const ADDInnerNode*>(assignment)->getSucc1():static_cast<const ADDInnerNode*>(assignment)->getSucc0();
  }
  
  if(assignment->isDrain()) return const_cast<ADDBaseNode*>(node);
  
  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(node,BooleanConstants::COF_OP),assignment)) return comp;
  
  //do it the hard way
  ADDBaseNode* tempNode;
  if(assignment->getRealLevel()==node->getRealLevel()){
    const bool value=(static_cast<const ADDInnerNode*>(assignment)->getSucc0()==ADD::zeroDrain)?true:false;
    tempNode=cofactor(static_cast<const ADDInnerNode*>(node)->getSucc(value),static_cast<const ADDInnerNode*>(assignment)->getSucc(value),computedTable,memPool);
  } else {
    ADDBaseNode* w0=cofactor(static_cast<const ADDInnerNode*>(node)->getSucc0(),assignment,computedTable,memPool);
    ADDBaseNode* w1=cofactor(static_cast<const ADDInnerNode*>(node)->getSucc1(),assignment,computedTable,memPool);
    
    tempNode=(w0==w1)?w0:findOrAdd(memPool,node->getUniqueTable(),w0,w1);
  }
  
  computedTable.insert(Common::calcPtr(node,BooleanConstants::COF_OP),assignment,tempNode);
  
  return tempNode;
}

ADDFunction ADDFunction::cofactor(const ADDFunction& assignment) const {
  ADDData& data=getADDData();
  return ADDFunction(::cofactor(rootNode,assignment.rootNode,*data.computedTables.booleanHashMap,data.memPools.innerNodeMemPool));
}

ADDFunction ADDFunction::subSet(const ADDFunction& assignment) const {
  return ((*this)*assignment);
}

//****************************************************************************************************************
ADDFunction& ADDFunction::operator^=(const ADDFunction& function){
  return *this=ITE(*this,!function,function);
}

bool ADDFunction::operator==(const ADDFunction& function) const {
  return (function.rootNode==rootNode);
}

bool ADDFunction::operator<(const ADDFunction& function) const {
  return (rootNode<function.rootNode);
}

bool ADDFunction::isConstant() const {
  return rootNode->isDrain();
}

double ADDFunction::getValue() const {
  return isConstant()?static_cast<const ADDTerminalNode*>(rootNode)->getValue():0.0;
}

VariableIndexType ADDFunction::getFirstVariableIndex() const {
  return rootNode->getLevel();
}

// ADDFunction ADDFunction::equivalenceClasses(const ADDFunction& pattern, const ADDFunction& cubeSet) const {
//   return ADDFunction::BDD->equivalenceClasses(*this,pattern,cubeSet);
// }
//
// ADDFunction ADDFunction::constrain(const ADDFunction& g) const {
//   return ADDFunction::BDD->constrain(*this,g);
// }

ADDFunction ADDFunction::compose(const VariableIndexType index, const ADDFunction& function) const {
  if(index>=ADDVarOrder::size()) return (*this);
  else return ITE(function,cofactor(ADDFunction::projection(index,1)),cofactor(ADDFunction::projection(index,0)));
}

template <int N>
ADDBaseNode* moveUp(const ADDBaseNode* u, PtrHashMap3<ADDBaseNode>& computedTable, MemPool<ADDInnerNode>& memPool){
  if(u->isDrain()) return const_cast<ADDBaseNode*>(u);

  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(u,MoveConstants::MUP_OP),0,reinterpret_cast<ADDBaseNode*>(N))) return comp;

  ADDBaseNode* node=findOrAdd(memPool,ADD::variableOrdering.getData(u->getLevel()-N),moveUp<N>(static_cast<const ADDInnerNode*>(u)->getSucc0(),computedTable,memPool),moveUp<N>(static_cast<const ADDInnerNode*>(u)->getSucc1(),computedTable,memPool));
  computedTable.insert(Common::calcPtr(u,MoveConstants::MUP_OP),0,reinterpret_cast<ADDBaseNode*>(N),node);

  return node;
}

ADDFunction ADDFunction::moveUp() const {
  if(isConstant()) return ADDFunction(rootNode);

  ADDData& data=getADDData();
  return ADDFunction(::moveUp<1>(rootNode,*data.computedTables.moveHashMap,data.memPools.innerNodeMemPool));
}

template <int N>
ADDBaseNode* moveUp(const ADDBaseNode* u, const ADDBaseNode* v, PtrHashMap3<ADDBaseNode>& computedTable, MemPool<ADDInnerNode>& memPool){
  if(u->isDrain()) return const_cast<ADDBaseNode*>(u);

  VariableIndexType uLevel=u->getLevel();
  while(v->getLevel()<uLevel) v=static_cast<const ADDInnerNode*>(v)->getSucc1();

  if(v->isDrain()) return const_cast<ADDBaseNode*>(u);
  if((uLevel<N)&&(uLevel==v->getLevel())) return const_cast<ADDBaseNode*>(u);

  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(u,MoveConstants::MUP_OP),v,reinterpret_cast<ADDBaseNode*>(N))) return comp;

  ADDBaseNode* w0;
  ADDBaseNode* w1;
  ADDBaseNode* node;
  if(uLevel==v->getLevel()){
    w0=moveUp<N>(static_cast<const ADDInnerNode*>(u)->getSucc0(),static_cast<const ADDInnerNode*>(v)->getSucc1(),computedTable,memPool);
    w1=moveUp<N>(static_cast<const ADDInnerNode*>(u)->getSucc1(),static_cast<const ADDInnerNode*>(v)->getSucc1(),computedTable,memPool);

    node=findOrAdd(memPool,ADD::variableOrdering.getData(uLevel-N),w0,w1);
  } else {
    w0=moveUp<N>(static_cast<const ADDInnerNode*>(u)->getSucc0(),v,computedTable,memPool);
    w1=moveUp<N>(static_cast<const ADDInnerNode*>(u)->getSucc1(),v,computedTable,memPool);

    if((w0->getLevel()<=uLevel)||(w1->getLevel()<=uLevel)){
      node=const_cast<ADDBaseNode*>(u);
    } else node=findOrAdd(memPool,ADD::variableOrdering.getData(uLevel),w0,w1);
  }

  computedTable.insert(Common::calcPtr(u,MoveConstants::MUP_OP),v,reinterpret_cast<ADDBaseNode*>(N),node);

  return node;
}

ADDFunction ADDFunction::moveUp(const ADDFunction& varSet) const {
  if(isConstant()) return ADDFunction(rootNode);

  ADDData& data=getADDData();
  return ADDFunction(::moveUp<1>(rootNode,const_cast<ADDBaseNode*>(varSet.rootNode),*data.computedTables.moveHashMap,data.memPools.innerNodeMemPool));
}

template <int N>
ADDBaseNode* moveDown(const ADDBaseNode* u, PtrHashMap3<ADDBaseNode>& computedTable, MemPool<ADDInnerNode>& memPool){
  if(u->isDrain()) return const_cast<ADDBaseNode*>(u);

  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(u,MoveConstants::MDO_OP),0,reinterpret_cast<ADDBaseNode*>(N))) return comp;

  ADDBaseNode* w0=moveDown<N>(static_cast<const ADDInnerNode*>(u)->getSucc0(),computedTable,memPool);
  ADDBaseNode* w1=moveDown<N>(static_cast<const ADDInnerNode*>(u)->getSucc1(),computedTable,memPool);

  ADDBaseNode* node=((w0==w1) || (u->getLevel()+N>=ADD::variableOrdering.size()))?static_cast<ADDBaseNode*>(ADD::zeroDrain):findOrAdd(memPool,ADD::variableOrdering.getData(u->getLevel()+N),w0,w1);
  computedTable.insert(Common::calcPtr(u,MoveConstants::MDO_OP),0,reinterpret_cast<ADDBaseNode*>(N),node);

  return node;
}

ADDFunction ADDFunction::moveDown() const {
  if(isConstant()) return ADDFunction(rootNode);

  ADDData& data=getADDData();
  return ADDFunction(::moveDown<1>(rootNode,*data.computedTables.moveHashMap,data.memPools.innerNodeMemPool));
}

template <int N>
ADDBaseNode* moveDown(const ADDBaseNode* u, const ADDBaseNode* v, PtrHashMap3<ADDBaseNode>& computedTable, MemPool<ADDInnerNode>& memPool){
  if(u->isDrain()) return const_cast<ADDBaseNode*>(u);

  VariableIndexType uLevel=u->getLevel();
  while(v->getLevel()<uLevel) v=static_cast<const ADDInnerNode*>(v)->getSucc1();

  if(v->isDrain()) return const_cast<ADDBaseNode*>(u);

  if(ADDBaseNode* comp=computedTable.find(Common::calcPtr(u,MoveConstants::MDO_OP),v,reinterpret_cast<ADDBaseNode*>(N))) return comp;

  ADDBaseNode* w0;
  ADDBaseNode* w1;
  ADDBaseNode* node;
  if(uLevel==v->getLevel()){
    w0=moveDown<N>(static_cast<const ADDInnerNode*>(u)->getSucc0(),static_cast<const ADDInnerNode*>(v)->getSucc1(),computedTable,memPool);
    w1=moveDown<N>(static_cast<const ADDInnerNode*>(u)->getSucc1(),static_cast<const ADDInnerNode*>(v)->getSucc1(),computedTable,memPool);

    if((uLevel+N>=w0->getLevel())||(uLevel+N>=w1->getLevel())) node=findOrAdd(memPool,ADD::variableOrdering.getData(uLevel),w0,w1);
    else node=findOrAdd(memPool,ADD::variableOrdering.getData(uLevel+N),w0,w1);
  } else {
    w0=moveDown<N>(static_cast<const ADDInnerNode*>(u)->getSucc0(),v,computedTable,memPool);
    w1=moveDown<N>(static_cast<const ADDInnerNode*>(u)->getSucc1(),v,computedTable,memPool);

    node=findOrAdd(memPool,u->getUniqueTable(),w0,w1);
  }

  computedTable.insert(Common::calcPtr(u,MoveConstants::MDO_OP),v,reinterpret_cast<ADDBaseNode*>(N),node);

  return node;
}

ADDFunction ADDFunction::moveDown(const ADDFunction& varSet) const {
  if(isConstant()) return ADDFunction(rootNode);

  ADDData& data=getADDData();
  return ADDFunction(::moveDown<1>(rootNode,const_cast<ADDBaseNode*>(varSet.rootNode),*data.computedTables.moveHashMap,data.memPools.innerNodeMemPool));
}

boost::dynamic_bitset<> ADDFunction::getEssentialVariables() const {
  return ::getEssentialVariables<ADDBaseNode,ADD>(rootNode);
}

ADDFunction ITE(const ADDFunction& p_if, const ADDFunction& p_then, const ADDFunction& p_else){
  return ADDFunction((p_if&p_then)|(!p_if&p_else));
}

ADDBaseNode* MM(const ADDBaseNode* A, const ADDBaseNode* B, const ADDBaseNode* varSet, const VariableIndexType m, ADDComputedTables& computedTables, ADDMemPools& pools){
  if((A==ADD::zeroDrain) || (B==ADD::zeroDrain)) return ADD::zeroDrain;
  if((A->isDrain() && B->isDrain()) || (m==0)) return findOrAddDrain(pools.terminalNodeMemPool,static_cast<const ADDTerminalNode*>(A)->getValue()*static_cast<const ADDTerminalNode*>(B)->getValue()*static_cast<double>(1<<m));

  if(ADDBaseNode* comp=computedTables.matrixHashMap->find(Common::calcPtr(A,MatrixConstants::MMA_OP),B,varSet)) return comp;

  ADDUniqueTable* uniqueTable=varSet->getUniqueTable();
  ADDUniqueTable* nextUniqueTable=ADD::variableOrdering.getData(uniqueTable->getLevel()+1);

  ADDBaseNode* A00, *A01, *A10, *A11;
  if(uniqueTable==A->getUniqueTable()){
    ADDBaseNode* A0=static_cast<const ADDInnerNode*>(A)->getSucc0();
    ADDBaseNode* A1=static_cast<const ADDInnerNode*>(A)->getSucc1();
    if(A0->getUniqueTable()==nextUniqueTable){
      A00=static_cast<ADDInnerNode*>(A0)->getSucc0();
      A01=static_cast<ADDInnerNode*>(A0)->getSucc1();
    } else {
      A00=A01=A0;
    }
    if(A1->getUniqueTable()==nextUniqueTable){
      A10=static_cast<ADDInnerNode*>(A1)->getSucc0();
      A11=static_cast<ADDInnerNode*>(A1)->getSucc1();
    } else {
      A10=A11=A1;
    }
  } else {
    if(A->getUniqueTable()==nextUniqueTable){
      A00=A10=static_cast<const ADDInnerNode*>(A)->getSucc0();
      A01=A11=static_cast<const ADDInnerNode*>(A)->getSucc1();
    } else {
      A00=A01=A10=A11=const_cast<ADDBaseNode*>(A);
    }
  }

  ADDBaseNode* B00, *B01, *B10, *B11;
  if(uniqueTable==B->getUniqueTable()){
    ADDBaseNode* B0=static_cast<const ADDInnerNode*>(B)->getSucc0();
    ADDBaseNode* B1=static_cast<const ADDInnerNode*>(B)->getSucc1();
    if(B0->getUniqueTable()==nextUniqueTable){
      B00=static_cast<ADDInnerNode*>(B0)->getSucc0();
      B01=static_cast<ADDInnerNode*>(B0)->getSucc1();
    } else {
      B00=B01=B0;
    }
    if(B1->getUniqueTable()==nextUniqueTable){
      B10=static_cast<ADDInnerNode*>(B1)->getSucc0();
      B11=static_cast<ADDInnerNode*>(B1)->getSucc1();
    } else {
      B10=B11=B1;
    }
  } else {
    if(B->getUniqueTable()==nextUniqueTable){
      B00=B10=static_cast<const ADDInnerNode*>(B)->getSucc0();
      B01=B11=static_cast<const ADDInnerNode*>(B)->getSucc1();
    } else {
      B00=B01=B10=B11=const_cast<ADDBaseNode*>(B);
    }
  }

  ADDBaseNode* newVarSet=static_cast<const ADDInnerNode*>(varSet)->getSucc1();

  ADDBaseNode* w0000=::MM(A00,B00,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w0110=::MM(A01,B10,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w0001=::MM(A00,B01,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w0111=::MM(A01,B11,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w1000=::MM(A10,B00,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w1110=::MM(A11,B10,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w1001=::MM(A10,B01,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w1111=::MM(A11,B11,newVarSet,m-1,computedTables,pools);

  ADDBaseNode* c00=::comp<ADD_ADDITION_2>(w0000,w0110,*computedTables.arithmeticHashMap,pools);
  ADDBaseNode* c01=::comp<ADD_ADDITION_2>(w0001,w0111,*computedTables.arithmeticHashMap,pools);
  ADDBaseNode* c10=::comp<ADD_ADDITION_2>(w1000,w1110,*computedTables.arithmeticHashMap,pools);
  ADDBaseNode* c11=::comp<ADD_ADDITION_2>(w1001,w1111,*computedTables.arithmeticHashMap,pools);

  ADDBaseNode* result0=(c00==c01)?c00:findOrAdd(pools.innerNodeMemPool,nextUniqueTable,c00,c01);
  ADDBaseNode* result1=(c10==c11)?c11:findOrAdd(pools.innerNodeMemPool,nextUniqueTable,c10,c11);
  ADDBaseNode* result=(result0==result1)?result1:findOrAdd(pools.innerNodeMemPool,uniqueTable,result0,result1);

  computedTables.matrixHashMap->insert(Common::calcPtr(A,MatrixConstants::MMA_OP),B,varSet,result);

  return result;
}

ADDFunction MM(const ADDFunction& A, const ADDFunction& B, const ADDFunction& varSet){
  VariableIndexType m=0;
  ADDBaseNode* node=varSet.rootNode;
  while(!node->isDrain()){
    node=static_cast<ADDInnerNode*>(node)->getSucc1();
    ++m;
  }

  ADDData& data=getADDData();
  return ADDFunction(::MM(A.rootNode,B.rootNode,varSet.rootNode,m,data.computedTables,data.memPools));
}

ADDBaseNode* MV(const ADDBaseNode* A, const ADDBaseNode* b, const ADDBaseNode* varSet, const VariableIndexType m, ADDComputedTables& computedTables, ADDMemPools& pools){
  if((A==ADD::zeroDrain) || (b==ADD::zeroDrain)) return ADD::zeroDrain;
  if((A->isDrain() && b->isDrain()) || (m==0)){
    return findOrAddDrain(pools.terminalNodeMemPool,static_cast<const ADDTerminalNode*>(A)->getValue()*static_cast<const ADDTerminalNode*>(b)->getValue()*static_cast<double>(1<<m));
  }

  if(ADDBaseNode* comp=computedTables.matrixHashMap->find(Common::calcPtr(A,MatrixConstants::MAV_OP),b,varSet)) return comp;

  ADDUniqueTable* uniqueTable=varSet->getUniqueTable();
  ADDUniqueTable* nextUniqueTable=ADD::variableOrdering.getData(uniqueTable->getLevel()+1);

  ADDBaseNode* A00, *A01, *A10, *A11;
  if(uniqueTable==A->getUniqueTable()){
    ADDBaseNode* A0=static_cast<const ADDInnerNode*>(A)->getSucc0();
    ADDBaseNode* A1=static_cast<const ADDInnerNode*>(A)->getSucc1();
    if(A0->getUniqueTable()==nextUniqueTable){
      A00=static_cast<ADDInnerNode*>(A0)->getSucc0();
      A01=static_cast<ADDInnerNode*>(A0)->getSucc1();
    } else {
      A00=A01=A0;
    }
    if(A1->getUniqueTable()==nextUniqueTable){
      A10=static_cast<ADDInnerNode*>(A1)->getSucc0();
      A11=static_cast<ADDInnerNode*>(A1)->getSucc1();
    } else {
      A10=A11=A1;
    }
  } else {
    if(A->getUniqueTable()==nextUniqueTable){
      A00=A10=static_cast<const ADDInnerNode*>(A)->getSucc0();
      A01=A11=static_cast<const ADDInnerNode*>(A)->getSucc1();
    } else {
      A00=A01=A10=A11=const_cast<ADDBaseNode*>(A);
    }
  }

  ADDBaseNode* b0, *b1;
  if(uniqueTable==b->getUniqueTable()){
    b0=static_cast<const ADDInnerNode*>(b)->getSucc0();
    b1=static_cast<const ADDInnerNode*>(b)->getSucc1();
  } else {
    b0=b1=const_cast<ADDBaseNode*>(b);
  }

  ADDBaseNode* newVarSet=static_cast<const ADDInnerNode*>(varSet)->getSucc1();

  ADDBaseNode* w00=::MV(A00,b0,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w01=::MV(A01,b1,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w10=::MV(A10,b0,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w11=::MV(A11,b1,newVarSet,m-1,computedTables,pools);

  ADDBaseNode* c0=::comp<ADD_ADDITION_2>(w00,w01,*computedTables.arithmeticHashMap,pools);
  ADDBaseNode* c1=::comp<ADD_ADDITION_2>(w10,w11,*computedTables.arithmeticHashMap,pools);

  ADDBaseNode* result=(c0==c1)?c1:findOrAdd(pools.innerNodeMemPool,uniqueTable,c0,c1);

  computedTables.matrixHashMap->insert(Common::calcPtr(A,MatrixConstants::MAV_OP),b,varSet,result);

  return result;
}

ADDFunction MV(const ADDFunction& A, const ADDFunction& b, const ADDFunction& varSet){
  VariableIndexType m=0;
  ADDBaseNode* node=varSet.rootNode;
  while(!node->isDrain()){
    node=static_cast<ADDInnerNode*>(node)->getSucc1();
    ++m;
  }

  ADDData& data=getADDData();
  return ADDFunction(::MV(A.rootNode,b.rootNode,varSet.rootNode,m,data.computedTables,data.memPools));
}

ADDBaseNode* VM(const ADDBaseNode* b, const ADDBaseNode* A, const ADDBaseNode* varSet, const VariableIndexType m, ADDComputedTables& computedTables, ADDMemPools& pools){
  if((A==ADD::zeroDrain) || (b==ADD::zeroDrain)) return ADD::zeroDrain;
  if((A->isDrain() && b->isDrain()) || (m==0)){
    return findOrAddDrain(pools.terminalNodeMemPool,static_cast<const ADDTerminalNode*>(A)->getValue()*static_cast<const ADDTerminalNode*>(b)->getValue()*static_cast<double>(1<<m));
  }

  if(ADDBaseNode* comp=computedTables.matrixHashMap->find(Common::calcPtr(b,MatrixConstants::VMA_OP),A,varSet)) return comp;

  ADDUniqueTable* uniqueTable=varSet->getUniqueTable();
  ADDUniqueTable* nextUniqueTable=ADD::variableOrdering.getData(uniqueTable->getLevel()+1);

  ADDBaseNode* A00, *A01, *A10, *A11;
  if(uniqueTable==A->getUniqueTable()){
    ADDBaseNode* A0=static_cast<const ADDInnerNode*>(A)->getSucc0();
    ADDBaseNode* A1=static_cast<const ADDInnerNode*>(A)->getSucc1();
    if(A0->getUniqueTable()==nextUniqueTable){
      A00=static_cast<ADDInnerNode*>(A0)->getSucc0();
      A01=static_cast<ADDInnerNode*>(A0)->getSucc1();
    } else {
      A00=A01=A0;
    }
    if(A1->getUniqueTable()==nextUniqueTable){
      A10=static_cast<ADDInnerNode*>(A1)->getSucc0();
      A11=static_cast<ADDInnerNode*>(A1)->getSucc1();
    } else {
      A10=A11=A1;
    }
  } else {
    if(A->getUniqueTable()==nextUniqueTable){
      A00=A10=static_cast<const ADDInnerNode*>(A)->getSucc0();
      A01=A11=static_cast<const ADDInnerNode*>(A)->getSucc1();
    } else {
      A00=A01=A10=A11=const_cast<ADDBaseNode*>(A);
    }
  }

  ADDBaseNode* b0, *b1;
  if(uniqueTable==b->getUniqueTable()){
    b0=static_cast<const ADDInnerNode*>(b)->getSucc0();
    b1=static_cast<const ADDInnerNode*>(b)->getSucc1();
  } else {
    b0=b1=const_cast<ADDBaseNode*>(b);
  }

  ADDBaseNode* newVarSet=static_cast<const ADDInnerNode*>(varSet)->getSucc1();

  ADDBaseNode* w00=::VM(b0,A00,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w01=::VM(b0,A01,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w10=::VM(b1,A10,newVarSet,m-1,computedTables,pools);
  ADDBaseNode* w11=::VM(b1,A11,newVarSet,m-1,computedTables,pools);

  ADDBaseNode* c0=::comp<ADD_ADDITION_2>(w00,w10,*computedTables.arithmeticHashMap,pools);
  ADDBaseNode* c1=::comp<ADD_ADDITION_2>(w01,w11,*computedTables.arithmeticHashMap,pools);

  ADDBaseNode* result=(c0==c1)?c1:findOrAdd(pools.innerNodeMemPool,uniqueTable,c0,c1);

  computedTables.matrixHashMap->insert(Common::calcPtr(b,MatrixConstants::VMA_OP),A,varSet,result);

  return result;
}

ADDFunction VM(const ADDFunction& A, const ADDFunction& b, const ADDFunction& varSet){
  VariableIndexType m=0;
  ADDBaseNode* node=varSet.rootNode;
  while(!node->isDrain()){
    node=static_cast<ADDInnerNode*>(node)->getSucc1();
    ++m;
  }

  ADDData& data=getADDData();
  return ADDFunction(::VM(b.rootNode,A.rootNode,varSet.rootNode,m,data.computedTables,data.memPools));
}

