/***************************************************************************
                          tadd.hpp  -  description
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

#ifndef TADD_HPP
#define TADD_HPP

#include <common/mempool.hpp>
#include <tadd/taddvarorder.hpp>

#include <vector>

template <typename T> class PtrHashMap2;
template <typename T> class PtrHashMap3;

typedef PtrHashMap2<TADDBaseNode> TADDHashMap2;
typedef PtrHashMap3<TADDBaseNode> TADDHashMap3;

struct TADDComputedTables {
  TADDHashMap2* arithmeticHashMap;
  TADDHashMap2* booleanHashMap;
  TADDHashMap2* extremaHashMap;
  TADDHashMap2* compareHashMap;
  TADDHashMap3* moveHashMap;
  TADDHashMap3* matrixHashMap;
};

struct TADDMemPools {
  MemPool<TADDInnerNode> innerNodeMemPool;
  MemPool<TADDTerminalNode> terminalNodeMemPool;
};

struct TADDData {
  TADDComputedTables computedTables;
  TADDMemPools memPools;
};

extern TADDData& getTADDData();

struct TADD {
  static TADDTerminalNodeUniqueTable drainUniqueTable;

  static std::vector<TADDHashMap2*> hashMaps2Params;
  static std::vector<TADDHashMap3*> hashMaps3Params;
  static std::vector<MemPool<TADDInnerNode>*> innerMemPools;

  static TADDVariableOrdering variableOrdering;
  static TADDTerminalNode* zeroDrain;
  static TADDTerminalNode* oneDrain;

  static void garbageCollect();
  static unsigned long size();
  static unsigned long deadNodes();
  static bool clear();
  static bool swap(VariableIndexType);
};

#endif
