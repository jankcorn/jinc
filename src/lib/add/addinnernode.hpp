/***************************************************************************
                        addinnernode.hpp  -  description
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

#ifndef ADDINNERNODE_HPP
#define ADDINNERNODE_HPP

#include <common/helper.hpp>

#include <add/addbasenode.hpp>
#include <add/addterminalnode.hpp>
#include <add/add.hpp>
#include <add/addvarorder.hpp>

class ADDInnerNode : public ADDBaseNode {
public:
  ADDInnerNode(): succ0(0), succ1(0) {}
  ADDInnerNode* getNext() const {return static_cast<ADDInnerNode*>(next);}
  void setNext(const ADDInnerNode* next){this->next=static_cast<ADDBaseNode*>(const_cast<ADDInnerNode*>(next));}
  ADDBaseNode* getSucc0() const {return succ0;}
  ADDBaseNode* getSucc1() const {return succ1;}
  ADDBaseNode* getSucc(const bool succX) const {return succX?succ1:succ0;}
  void initNode(ADDUniqueTable* uTable, ADDBaseNode** succs){
    succs[0]->incNodeCount();
    succs[1]->incNodeCount();
    
    succ0=succs[0];
    succ1=succs[1];
    uniqueTable=uTable;
    
    preNodeCount=0;
  }
  
  template <typename IMP, typename TMP>
  void inline checkDeleteNode(ADDBaseNode* node, IMP& innerMemPool, TMP& terminalMemPool){
    if UNLIKELY(Common::atomicDecAndTest(node->preNodeCount)){
      if UNLIKELY(node->isDrain()){
        ADD::drainUniqueTable.remove(static_cast<ADDTerminalNode*>(node));
        terminalMemPool.free(static_cast<ADDTerminalNode*>(node));
      } else {
        static_cast<ADDInnerNode*>(node)->getSucc0()->deleteNode();
        static_cast<ADDInnerNode*>(node)->getSucc1()->deleteNode();
        node->uniqueTable->remove(static_cast<ADDInnerNode*>(node));
        innerMemPool.free(static_cast<ADDInnerNode*>(node));
      }
    }
  }
  
  template <typename IMP, typename TMP>
  void setNode(ADDUniqueTable* uTable, ADDBaseNode* s0, ADDBaseNode* s1, IMP& innerMemPool, TMP& terminalMemPool){
    s0->incNodeCount();
    s1->incNodeCount();
    
    checkDeleteNode(succ0,innerMemPool,terminalMemPool);
    checkDeleteNode(succ1,innerMemPool,terminalMemPool);
    
    succ0=s0;
    succ1=s1;
    uniqueTable=uTable;
    uniqueTable->add(this);
  }
private:
  ADDBaseNode* succ0;
  ADDBaseNode* succ1;
};

#endif
