/***************************************************************************
                          addvarorder.cpp  -  description
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

#include <add/add.hpp>
#include <add/addvarorder.hpp>

void ADDVarOrder::clear(){
  ADD::garbageCollect();
  ADD::variableOrdering.clear();
}

VariableIndexType ADDVarOrder::size(){
  return ADD::variableOrdering.size();
}

std::string ADDVarOrder::getVarOrder(){
  return ADD::variableOrdering.getVariableOrdering();
}

std::string ADDVarOrder::getVariableName(const VariableIndexType index){
  return ADD::variableOrdering.getVariable(index)->getName();
}

bool ADDVarOrder::insertVariable(const VariableIndexType index){
  return ADD::variableOrdering.insertVariable(index);
}

bool ADDVarOrder::insertVariables(const VariableIndexType index, const VariableIndexType count){
  return ADD::variableOrdering.insertVariables(index,count);
}

bool ADDVarOrder::insertVariable(const std::string& label, const VariableIndexType index){
  return ADD::variableOrdering.insertVariable(label,index);
}

bool ADDVarOrder::addVariable(){
  return ADD::variableOrdering.appendVariable();
}

bool ADDVarOrder::addVariable(const std::string& label){
  return ADD::variableOrdering.appendVariable(label);
}

bool ADDVarOrder::prependVariable(){
  return ADD::variableOrdering.prependVariable();
}

bool ADDVarOrder::prependVariable(const std::string& label){
  return ADD::variableOrdering.prependVariable(label);
}

bool ADDVarOrder::removeVariable(const VariableIndexType index){
  ADD::garbageCollect();  
  return ADD::variableOrdering.removeVariable(index);
}

bool ADDVarOrder::removeVariable(const std::string& label){
  return ADDVarOrder::removeVariable(ADDVarOrder::getVariable(label));
}

VariableIndexType ADDVarOrder::getVariable(const std::string& variable){
  return ADD::variableOrdering[variable];
}

bool ADDVarOrder::isGroupMember(const VariableIndexType index){
  return (index>=size())?false:ADD::variableOrdering.getVariable(index)->getGroup();
}

VariableIndexType ADDVarOrder::groupCount(){
  return ADD::variableOrdering.groupCount();
}
