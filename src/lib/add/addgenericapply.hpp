/***************************************************************************
                          addgenericapply.hpp  -  description
                             -------------------
    begin                : Tue Apr 13 2010
    copyright            : (C) 2010 by Joern Ossowski
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

#ifndef ADD_GENERIC_APPLY_HPP
#define ADD_GENERIC_APPLY_HPP

#include <add/add.hpp>
#include <add/addbasenode.hpp>
#include <common/ptrhashmap.hpp>
#include <common/mempool.hpp>

namespace GenericApply {
  namespace ADD {
    inline ADDTerminalNode* findOrAddDrain(MemPool<ADDTerminalNode>& memPool, const double value){
      if UNLIKELY(ABS(value)<0.0000001) return ::ADD::zeroDrain;
      return ::ADD::drainUniqueTable.findOrAddValue(value, memPool);
    }

    inline ADDInnerNode* findOrAdd(MemPool<ADDInnerNode>& memPool, ADDUniqueTable* uTable, ADDBaseNode* succ0, ADDBaseNode* succ1){
      const ADDBaseNode* nodes[] = { succ0, succ1 };
      return uTable->findOrAdd(nodes, memPool);
    }


    template <typename T>
    struct Expression {
      enum {size=T::size};

      Expression() : value() {}
      Expression(const T& t) : value(t) {}
      T value;
  
      void fill(ADDBaseNode** nodes) const {
        value.fill(nodes);
      }
      static ADDBaseNode* terminalCase(ADDMemPools& pools, ADDBaseNode** nodes){
        return T::terminalCase(pools,nodes);
      }
      static ADDUniqueTable* getMinUniqueTable(ADDBaseNode** nodes){
        return T::getMinUniqueTable(nodes);
      }
      static void fillSuccs(ADDBaseNode** nodes, ADDBaseNode** succs, ADDUniqueTable* table, bool succX){
        T::fillSuccs(nodes,succs,table,succX);
      }
    };

    struct Symbol {
      enum {size=1};
  
      Symbol() : node(0) {}
      Symbol(ADDFunction& f) : node(f.rootNode) {}
      ADDBaseNode* node;

      void fill(ADDBaseNode** nodes) const {
        (*nodes)=node;
      }
      static ADDBaseNode* terminalCase(ADDMemPools&, ADDBaseNode** nodes){
        return (*nodes)->isDrain()?*nodes:0;
      }
      static ADDUniqueTable* getMinUniqueTable(ADDBaseNode** nodes){
        return (*nodes)->getUniqueTable();
      }
      static void fillSuccs(ADDBaseNode** nodes, ADDBaseNode** succs, ADDUniqueTable* table, bool succX){
        if((*nodes)->getUniqueTable()==table) (*succs)=static_cast<const ADDInnerNode*>(*nodes)->getSucc(succX);
        else (*succs)=(*nodes);
      }
      static ADDBaseNode* find(ADDData&, ADDBaseNode**){
        throw "unreachable";
        return 0;
      }
      static void insert(ADDData&, ADDBaseNode**, ADDBaseNode*){
        throw "unreachable";
      }
    };

    template <typename T>
    struct SingleOperatorBase {
      enum {size=T::size};
  
      SingleOperatorBase(const T& a) : arg(a) {}
      T arg;
  
      void fill(ADDBaseNode** nodes) const {
        arg.fill(nodes);
      }
      static ADDUniqueTable* getMinUniqueTable(ADDBaseNode** nodes){
        return T::getMinUniqueTable(nodes);
      }
      static void fillSuccs(ADDBaseNode** nodes, ADDBaseNode** succs, ADDUniqueTable* table, bool succX){
        T::fillSuccs(nodes,succs,table,succX);
      }  
    };

    template <typename T1, typename T2>
    struct OperatorBase {
      enum {size=T1::size+T2::size};
  
      OperatorBase(const T1& l, const T2& r) : lhs(l), rhs(r) {}
      T1 lhs;
      T2 rhs;
  
      void fill(ADDBaseNode** nodes) const {
        lhs.fill(nodes);
        rhs.fill(nodes+T1::size);
      }
      static ADDUniqueTable* getMinUniqueTable(ADDBaseNode** nodes){
        ADDUniqueTable* table1=T1::getMinUniqueTable(nodes);
        if(table1==0) return T2::getMinUniqueTable(nodes+T1::size);
        ADDUniqueTable* table2=T2::getMinUniqueTable(nodes+T1::size);
        if(table2==0) return table1;

        return (table1->getRealLevel()<=table2->getRealLevel()?table1:table2);
      }
      static void fillSuccs(ADDBaseNode** nodes, ADDBaseNode** succs, ADDUniqueTable* table, bool succX){
        T1::fillSuccs(nodes,succs,table,succX);
        T2::fillSuccs(nodes+T1::size,succs+T1::size,table,succX);
      }
  
    };

    template <typename T1, typename T2>
    struct Multiply : OperatorBase<T1,T2> {
      Multiply(const T1& l, const T2& r) : OperatorBase<T1,T2>(l,r){}
  
      static ADDBaseNode* terminalCase(ADDMemPools& pools, ADDBaseNode** nodes){
        ADDBaseNode* node=T1::terminalCase(pools,nodes);
        if(node==::ADD::zeroDrain) return ::ADD::zeroDrain;
        ADDBaseNode* node2=T2::terminalCase(pools,nodes+T1::size);
        if(node2==::ADD::zeroDrain) return ::ADD::zeroDrain;
        if(node && node2 && node->isDrain() && node2->isDrain()){
          return findOrAddDrain(pools.terminalNodeMemPool,static_cast<const ADDTerminalNode*>(node)->getValue()*static_cast<const ADDTerminalNode*>(node2)->getValue());
        }
    
        return 0;
      }  
      static ADDBaseNode* find(ADDData& data, ADDBaseNode** nodes){
        return data.computedTables.arithmeticHashMap->find(Common::calcPtr(nodes[0],ArithmeticConstants::MUL_OP),nodes[1]);
      }
      static void insert(ADDData& data, ADDBaseNode** nodes, ADDBaseNode* result){
        data.computedTables.arithmeticHashMap->insert(Common::calcPtr(nodes[0],ArithmeticConstants::MUL_OP),nodes[1],result);
      }
    };

    template <typename T1, typename T2>
    struct LogicalOr : OperatorBase<T1,T2> {
      LogicalOr(const T1& l, const T2& r) : OperatorBase<T1,T2>(l,r){}
  
      static ADDBaseNode* terminalCase(ADDMemPools& pools, ADDBaseNode** nodes){
        ADDBaseNode* node=T1::terminalCase(pools,nodes);
        if(node && node->isDrain()){
          return (node==::ADD::zeroDrain)?T2::terminalCase(pools,nodes+T1::size): ::ADD::oneDrain; 
        }
        ADDBaseNode* node2=T2::terminalCase(pools,nodes+T1::size);
        if(node2 && node2->isDrain()){
          return (node2==::ADD::zeroDrain)?node: ::ADD::oneDrain; 
        }
        if(node && node==node2) return node;
    
        return 0;
      }  
      static ADDBaseNode* find(ADDData& data, ADDBaseNode** nodes){
        return data.computedTables.arithmeticHashMap->find(Common::calcPtr(nodes[0],BooleanConstants::OR_OP),nodes[1]);
      }
      static void insert(ADDData& data, ADDBaseNode** nodes, ADDBaseNode* result){
        data.computedTables.arithmeticHashMap->insert(Common::calcPtr(nodes[0],BooleanConstants::OR_OP),nodes[1],result);
      }
    };

    template <typename T1, typename T2>
    struct LogicalAnd : OperatorBase<T1,T2> {
      LogicalAnd(const T1& l, const T2& r) : OperatorBase<T1,T2>(l,r){}
  
      static ADDBaseNode* terminalCase(ADDMemPools& pools, ADDBaseNode** nodes){
        ADDBaseNode* node=T1::terminalCase(pools,nodes);
        if(node && node->isDrain()){
          return (node==::ADD::zeroDrain)?::ADD::zeroDrain:T2::terminalCase(pools,nodes+T1::size); 
        }
        ADDBaseNode* node2=T2::terminalCase(pools,nodes+T1::size);
        if(node2 && node2->isDrain()){
          return (node2==::ADD::zeroDrain)?::ADD::zeroDrain:node;
        }
        if(node && node==node2) return node;
    
        return 0;
      }  
      static ADDBaseNode* find(ADDData& data, ADDBaseNode** nodes){
        return data.computedTables.arithmeticHashMap->find(Common::calcPtr(nodes[0],BooleanConstants::AND_OP),nodes[1]);
      }
      static void insert(ADDData& data, ADDBaseNode** nodes, ADDBaseNode* result){
        data.computedTables.arithmeticHashMap->insert(Common::calcPtr(nodes[0],BooleanConstants::AND_OP),nodes[1],result);
      }
    };
    
    template <typename T>
    struct LogicalNot : SingleOperatorBase<T> {
      LogicalNot(const T& arg) : SingleOperatorBase<T>(arg){}
  
      static ADDBaseNode* terminalCase(ADDMemPools& pools, ADDBaseNode** nodes){
        ADDBaseNode* node=T::terminalCase(pools,nodes);
        if(node && node->isDrain()){
          return (node==::ADD::zeroDrain)?::ADD::oneDrain: ::ADD::zeroDrain; 
        }
    
        return 0;
      }  
      static ADDBaseNode* find(ADDData& data, ADDBaseNode** nodes){
        return 0;
      }
      static void insert(ADDData& data, ADDBaseNode** nodes, ADDBaseNode* result){}
    };

    template <typename T>
    Expression<LogicalNot<Expression<T> > > operator!(const Expression<T>& t){
      return Expression<LogicalNot<Expression<T> > >(LogicalNot<Expression<T> >(t));
    }

    template <typename T1, typename T2>
    Expression<Multiply<Expression<T1>,Expression<T2> > > operator*(const Expression<T1>& t1, const Expression<T2>& t2){
      return Expression<Multiply<Expression<T1>,Expression<T2> > >(Multiply<Expression<T1>,Expression<T2> >(t1,t2));
    }

    template <typename T1, typename T2>
    Expression<LogicalOr<Expression<T1>,Expression<T2> > > operator|(const Expression<T1>& t1, const Expression<T2>& t2){
      return Expression<LogicalOr<Expression<T1>,Expression<T2> > >(LogicalOr<Expression<T1>,Expression<T2> >(t1,t2));
    }

    template <typename T1, typename T2>
    Expression<LogicalAnd<Expression<T1>,Expression<T2> > > operator&(const Expression<T1>& t1, const Expression<T2>& t2){
      return Expression<LogicalAnd<Expression<T1>,Expression<T2> > >(LogicalAnd<Expression<T1>,Expression<T2> >(t1,t2));
    }

    template <typename T>
    ADDBaseNode* apply_impl(ADDBaseNode** nodes, ADDData& data){
      if(ADDBaseNode* comp=T::terminalCase(data.memPools,nodes)) return comp;
  
      if(T::size==2){
        if(ADDBaseNode* comp=T::find(data,nodes)) return comp;
      }
  
      ADDUniqueTable* table=T::getMinUniqueTable(nodes);

      ADDBaseNode* succs[T::size];
      T::fillSuccs(nodes,succs,table,false);
      ADDBaseNode* w0=apply_impl<T>(succs,data);
  
      T::fillSuccs(nodes,succs,table,true);
      ADDBaseNode* w1=apply_impl<T>(succs,data);
  
      ADDBaseNode* result=(w0==w1)?w0:findOrAdd(data.memPools.innerNodeMemPool,table,w0,w1);

      if(T::size==2){
        T::insert(data,nodes,result);
      }

      return result;
    }

    template <typename T>
    ADDFunction apply(const Expression<T>& s){
      ADDData& data=getADDData();
      ADDBaseNode* nodes[T::size];
      s.fill(nodes);
      ADDFunction temp(apply_impl<T>(nodes,data));
      return temp;
    }
  }
}
#endif
