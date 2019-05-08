/***************************************************************************
                          addvarorder.hpp  -  description
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

#ifndef ADDVARORDER_HPP
#define ADDVARORDER_HPP

#include <common/constants.hpp>

class ADDBaseNode;
class ADDInnerNode;
class ADDTerminalNode;
//definition for uniquetable just change type of data structure and everything changes automatically
#include <common/varorder.hpp>
#include <common/hashuniquetable.hpp>
#include <common/hashdrainuniquetable.hpp>
typedef HashUniqueTable<ADDInnerNode,ADDBaseNode,2> ADDUniqueTable;
typedef HashDrainUniqueTable<ADDTerminalNode,262144,NUMBER_OF_THREADS> ADDTerminalNodeUniqueTable;
typedef VarOrder<ADDUniqueTable> ADDVariableOrdering;
typedef Group<ADDUniqueTable> ADDGroup;
typedef Variable<ADDUniqueTable> ADDVariable;

#include <string>

struct ADDVarOrder{
  static void clear();
  static VariableIndexType size();
  static std::string getVarOrder();
  static bool addVariable();
  static bool addVariable(const std::string& name);
  static bool prependVariable();
  static bool prependVariable(const std::string& name);
  static bool removeVariable(const VariableIndexType index);
  static bool removeVariable(const std::string& name);
  static bool insertVariable(const VariableIndexType index);
  static bool insertVariables(const VariableIndexType index, const VariableIndexType count);
  static bool insertVariable(const std::string&, const VariableIndexType index);
  static std::string getVariableName(const VariableIndexType index);
  static VariableIndexType getVariable(const std::string&);
  static bool isGroupMember(const VariableIndexType index);
  static VariableIndexType groupCount();
};

#endif
