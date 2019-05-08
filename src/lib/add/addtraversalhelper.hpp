/***************************************************************************
                          addtraversalhelper.hpp  -  description
                             -------------------
    begin                : Sat Jan 24 2009
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

#ifndef ADDTRAVERSALHELPER_HPP
#define ADDTRAVERSALHELPER_HPP

#include <common/constants.hpp>
#include <common/traversalhelper.hpp>

#include <add/addbasenode.hpp>
#include <add/addinnernode.hpp>
#include <add/addterminalnode.hpp>

template <>
struct TraversalHelper<ADDBaseNode> {
  TraversalHelper(const ADDBaseNode* n) : node(n) {}
  TraversalHelper getSucc(const unsigned short succX) const {return TraversalHelper(static_cast<const ADDInnerNode*>(node)->getSucc(succX));}
  const ADDBaseNode* getSuccPtr(const unsigned short succX) const {return static_cast<const ADDInnerNode*>(node)->getSucc(succX);}
  bool isDrain() const {return node->isDrain();}
  double getValue() const {return node->isDrain()?static_cast<const ADDTerminalNode*>(node)->getValue():0.0;}
  const ADDBaseNode* getPtr() const {return node;}
  VariableIndexType getLevel() const {return node->getLevel();}

  static const unsigned short N=2;
  const ADDBaseNode* node;
};

#endif
