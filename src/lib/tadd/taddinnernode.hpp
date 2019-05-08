/***************************************************************************
                          taddinnernode.hpp  -  description
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

#ifndef TADDINNERNODE_HPP
#define TADDINNERNODE_HPP

#include <common/helper.hpp>

#include <tadd/taddbasenode.hpp>
#include <tadd/taddterminalnode.hpp>

class TADDInnerNode : public TADDBaseNode {
public:
  TADDInnerNode(){
    succs[0]=0;
    succs[1]=0;
    succs[2]=0;
    succs[3]=0;
  }
  TADDInnerNode* getNext() const {return static_cast<TADDInnerNode*>(next);}
  void setNext(const TADDInnerNode* next){this->next=static_cast<TADDBaseNode*>(const_cast<TADDInnerNode*>(next));}
  TADDBaseNode* getSucc0() const {return succs[0];}
  TADDBaseNode* getSucc1() const {return succs[1];}
  TADDBaseNode* getSucc2() const {return succs[2];}
  TADDBaseNode* getSucc3() const {return succs[3];}
  TADDBaseNode* getSucc(const short succX) const { return succs[succX%4];}
  void initNode(TADDUniqueTable* uTable, TADDBaseNode** s){
    const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(s[0]).getPtr())->incNodeCount();
    const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(s[1]).getPtr())->incNodeCount();
    const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(s[2]).getPtr())->incNodeCount();
    const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(s[3]).getPtr())->incNodeCount();

    succs[0]=s[0];
    succs[1]=s[1];
    succs[2]=s[2];
    succs[3]=s[3];
    uniqueTable=uTable;

    preNodeCount=0;
  }

  template <class IMP, class TMP>
  void inline checkDeleteNode(TADDBaseNode* node, IMP& innerMemPool, TMP& terminalMemPool){
    if UNLIKELY(Common::atomicDecAndTest(node->preNodeCount)){
      if UNLIKELY(node->isDrain()){
        TADD::drainUniqueTable.remove(static_cast<TADDTerminalNode*>(node));
        terminalMemPool.free(static_cast<TADDTerminalNode*>(node));
      } else {
        Common::SmartAccess<TADDBaseNode> n(node);
        const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(static_cast<const TADDInnerNode*>(n.getPtr())->getSucc0()).getPtr())->deleteNode();
        const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(static_cast<const TADDInnerNode*>(n.getPtr())->getSucc1()).getPtr())->deleteNode();
        const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(static_cast<const TADDInnerNode*>(n.getPtr())->getSucc2()).getPtr())->deleteNode();
        const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(static_cast<const TADDInnerNode*>(n.getPtr())->getSucc3()).getPtr())->deleteNode();
        n->uniqueTable->remove(static_cast<const TADDInnerNode*>(n.getPtr()));
        innerMemPool.free(static_cast<TADDInnerNode*>(const_cast<TADDBaseNode*>(n.getPtr())));
      }
    }
  }

  template <class IMP, class TMP>
  void setNode(TADDUniqueTable* uTable, TADDBaseNode* s0, TADDBaseNode* s1, TADDBaseNode* s2, TADDBaseNode* s3, IMP& innerMemPool, TMP& terminalMemPool){
    const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(s0).getPtr())->incNodeCount();
    const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(s1).getPtr())->incNodeCount();
    const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(s2).getPtr())->incNodeCount();
    const_cast<TADDBaseNode*>(Common::SmartAccess<TADDBaseNode>(s3).getPtr())->incNodeCount();
    
    checkDeleteNode(succs[0],innerMemPool,terminalMemPool);
    checkDeleteNode(succs[1],innerMemPool,terminalMemPool);
    checkDeleteNode(succs[2],innerMemPool,terminalMemPool);
    checkDeleteNode(succs[3],innerMemPool,terminalMemPool);

    succs[0]=s0;
    succs[1]=s1;
    succs[2]=s2;
    succs[3]=s3;
    uniqueTable=uTable;
    uniqueTable->add(this);
  }
private:
  TADDBaseNode* succs[4];
};

#endif
