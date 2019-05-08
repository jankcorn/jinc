/***************************************************************************
                          taddbasenode.hpp  -  description
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

#ifndef TADDBASENODE_HPP
#define TADDBASENODE_HPP

#include <common/helper.hpp>
#include <common/constants.hpp>

#include <tadd/taddvarorder.hpp>
#include <tadd/tadd.hpp>

class TADDBaseNode{
public:
  TADDBaseNode(): uniqueTable(0), next(0), preNodeCount(0), symmetric(false) {}
  bool isDrain() const {return (uniqueTable==0);}
  TADDUniqueTable* getUniqueTable() const {return uniqueTable;}
  void setUniqueTable(const TADDUniqueTable* uTable){uniqueTable=const_cast<TADDUniqueTable*>(uTable);}
  bool isSymmetric() const {return symmetric;}
  void setSymmetry(bool value){symmetric=value;}
  void incNodeCount(){
    if UNLIKELY(Common::atomicIncAndTest(preNodeCount)){
      if UNLIKELY(uniqueTable==0) TADD::drainUniqueTable.decDeadNodeCount();
      else uniqueTable->decDeadNodeCount();
    }
  }
  void deleteNode(){
    if UNLIKELY(Common::atomicDecAndTest(preNodeCount)){
      if UNLIKELY(isDrain()) TADD::drainUniqueTable.incDeadNodeCount();
      else uniqueTable->incDeadNodeCount();
    }
  }
  VariableIndexType getLevel() const {return LIKELY(uniqueTable!=0)?uniqueTable->getLevel():TADD::drainUniqueTable.getLevel();}
  VariableIndexType getRealLevel() const {return LIKELY(uniqueTable!=0)?uniqueTable->getRealLevel():TADD::drainUniqueTable.getRealLevel();}

  TADDUniqueTable* uniqueTable;
  TADDBaseNode* next;
  unsigned long preNodeCount;
  bool symmetric;
};

#endif
