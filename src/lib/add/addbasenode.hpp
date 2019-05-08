/***************************************************************************
                          addbasenode.hpp  -  description
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

#ifndef ADDBASENODE_HPP
#define ADDBASENODE_HPP

#include <common/helper.hpp>
#include <common/constants.hpp>

#include <add/addvarorder.hpp>
#include <add/add.hpp>

class ADDBaseNode{
public:
  ADDBaseNode(): uniqueTable(0), next(0), preNodeCount(0) {}
  bool isDrain() const {return (uniqueTable==0);}
  ADDUniqueTable* getUniqueTable() const {return uniqueTable;}
  void setUniqueTable(const ADDUniqueTable* uTable){uniqueTable=const_cast<ADDUniqueTable*>(uTable);}
  void incNodeCount(){
    if UNLIKELY(Common::atomicIncAndTest(preNodeCount)){
      if UNLIKELY(uniqueTable==0) ADD::drainUniqueTable.decDeadNodeCount();
      else uniqueTable->decDeadNodeCount();
    }
  }
  void deleteNode(){
    if UNLIKELY(Common::atomicDecAndTest(preNodeCount)){
      if UNLIKELY(isDrain()) ADD::drainUniqueTable.incDeadNodeCount();
      else uniqueTable->incDeadNodeCount();
    }
  }
  VariableIndexType getLevel() const {return LIKELY(uniqueTable!=0)?uniqueTable->getLevel():ADD::drainUniqueTable.getLevel();}
  VariableIndexType getRealLevel() const {return LIKELY(uniqueTable!=0)?uniqueTable->getRealLevel():ADD::drainUniqueTable.getRealLevel();}

  ADDUniqueTable* uniqueTable;
  ADDBaseNode* next;
  unsigned long preNodeCount;
};

#endif
