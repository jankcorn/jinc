/***************************************************************************
                          add.hpp  -  description
                             -------------------
    begin                : Wed Apr 14 2004
    copyright            : (C) 2004-2008 by Joern Ossowski
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

#ifndef ADD_HPP
#define ADD_HPP

#include <common/mempool.hpp>
#include <add/addvarorder.hpp>

#include <vector>

template <typename T> class PtrHashMap2;
template <typename T> class PtrHashMap3;

typedef PtrHashMap2<ADDBaseNode> ADDHashMap2;
typedef PtrHashMap3<ADDBaseNode> ADDHashMap3;

struct ADDComputedTables {
  ADDHashMap2* arithmeticHashMap;
  ADDHashMap2* booleanHashMap;
  ADDHashMap2* extremaHashMap;
  ADDHashMap2* compareHashMap;
  ADDHashMap3* moveHashMap;
  ADDHashMap3* matrixHashMap;
};

struct ADDMemPools {
  MemPool<ADDInnerNode> innerNodeMemPool;
  MemPool<ADDTerminalNode> terminalNodeMemPool;
};

struct ADDData {
  ADDComputedTables computedTables;
  ADDMemPools memPools;
};

extern ADDData& getADDData();

struct ADD {
  static ADDTerminalNodeUniqueTable drainUniqueTable;

  static std::vector<ADDHashMap2*> hashMaps2Params;
  static std::vector<ADDHashMap3*> hashMaps3Params;
  static std::vector<MemPool<ADDInnerNode>*> innerMemPools;

  static ADDVariableOrdering variableOrdering;
  static ADDTerminalNode* zeroDrain;
  static ADDTerminalNode* oneDrain;

  static void garbageCollect();
  static unsigned long size();
  static unsigned long deadNodes();
  static bool clear();
  static bool swap(VariableIndexType);
};

#endif
