/***************************************************************************
                          taddtraversalhelper.hpp  -  description
                             -------------------
    begin                : Tue Feb 3 2009
    copyright            : (C) 2009 by Joern Ossowski
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

#ifndef TADDTRAVERSALHELPER_HPP
#define TADDTRAVERSALHELPER_HPP

#include <common/constants.hpp>
#include <common/helper.hpp>
#include <common/traversalhelper.hpp>

#include <tadd/taddbasenode.hpp>
#include <tadd/taddinnernode.hpp>
#include <tadd/taddterminalnode.hpp>

struct TADDBaseNode_2;

template <>
struct TraversalHelper<TADDBaseNode> {
  TraversalHelper() : node(0) {}
  TraversalHelper(const TADDBaseNode* n) : node(const_cast<TADDBaseNode*>(n)) {}
  TraversalHelper getSucc(const unsigned short succX) const {
    if(Common::bitSet(node.ptr)){
      TADDBaseNode* succ=((succX==1) || (succX==2))?static_cast<const TADDInnerNode*>(node.getPtr())->getSucc(3-succX):static_cast<const TADDInnerNode*>(node.getPtr())->getSucc(succX);
      return TraversalHelper((Common::SmartAccess<TADDBaseNode>(succ)->isSymmetric())?succ:Common::flipBit(succ));
    } else {
      return TraversalHelper(static_cast<const TADDInnerNode*>(node.getPtr())->getSucc(succX));
    }
  }
  const TADDBaseNode* getSuccPtr(const unsigned short succX) const {return getSucc(succX).getPtr();}
  bool isDrain() const {return node->isDrain();}
  double getValue() const {return node->isDrain()?static_cast<const TADDTerminalNode*>(node.getPtr())->getValue():0.0;}
  const TADDBaseNode* getPtr() const {return node.getPtr();}
  VariableIndexType getLevel() const {return node->getLevel();}

  static const unsigned short N=4;
  Common::SmartAccess<TADDBaseNode> node;
};

template <>
struct TraversalHelper<TADDBaseNode_2> {
  TraversalHelper(const TADDBaseNode* n) : helper(n), succ(true) {
    if(n==0) evenLevel=true;
    else evenLevel=((helper.getSucc(0).node.ptr!=helper.getSucc(2).node.ptr) || (helper.getSucc(1).node.ptr!=helper.getSucc(3).node.ptr));
  }
  TraversalHelper(TraversalHelper<TADDBaseNode> h, bool even, bool s) : helper(h), evenLevel(even), succ(s){
  }
  TraversalHelper getSucc(const bool succX) const {
    if(evenLevel){
      if(helper.getSucc(2*succX).node.ptr==helper.getSucc(2*succX+1).node.ptr) return TraversalHelper(helper.getSucc(2*succX).node.ptr);
      else return TraversalHelper(helper,false,succX);
    } else return TraversalHelper(helper.getSucc(2*succ+succX).node.ptr);
  }
  bool isDrain() const {return helper.isDrain();}
  double getValue() const {return helper.getValue();}
  VariableIndexType getLevel() const {
    return 2*helper.getLevel()+(1-evenLevel);
  }

  static const unsigned short N=2;
  TraversalHelper<TADDBaseNode>  helper;
  bool evenLevel;
  bool succ;
};

#endif
