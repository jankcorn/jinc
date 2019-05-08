/***************************************************************************
                          taddvarorder.hpp  -  description
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

#ifndef TADDVARORDER_HPP
#define TADDVARORDER_HPP

#include <common/constants.hpp>

class TADDBaseNode;
class TADDInnerNode;
class TADDTerminalNode;
//definition for uniquetable just change type of data structure and everything changes automatically
#include <common/varorder.hpp>
#include <common/hashuniquetable.hpp>
#include <common/hashdrainuniquetable.hpp>
typedef HashUniqueTable<TADDInnerNode,TADDBaseNode,4> TADDUniqueTable;
typedef HashDrainUniqueTable<TADDTerminalNode,262144,NUMBER_OF_THREADS> TADDTerminalNodeUniqueTable;
typedef VarOrder<TADDUniqueTable> TADDVariableOrdering;
typedef Group<TADDUniqueTable> TADDGroup;
typedef Variable<TADDUniqueTable> TADDVariable;

#include <string>

struct TADDVarOrder{
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
