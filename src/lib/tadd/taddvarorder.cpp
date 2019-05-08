/***************************************************************************
                          taddvarorder.cpp  -  description
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

#include <tadd/tadd.hpp>
#include <tadd/taddvarorder.hpp>

void TADDVarOrder::clear(){
  TADD::garbageCollect();
  TADD::variableOrdering.clear();
}

VariableIndexType TADDVarOrder::size(){
  return TADD::variableOrdering.size();
}

std::string TADDVarOrder::getVarOrder(){
  return TADD::variableOrdering.getVariableOrdering();
}

std::string TADDVarOrder::getVariableName(const VariableIndexType index){
  return TADD::variableOrdering.getVariable(index)->getName();
}

bool TADDVarOrder::insertVariable(const VariableIndexType index){
  return TADD::variableOrdering.insertVariable(index);
}

bool TADDVarOrder::insertVariables(const VariableIndexType index, const VariableIndexType count){
  return TADD::variableOrdering.insertVariables(index,count);
}

bool TADDVarOrder::insertVariable(const std::string& label, const VariableIndexType index){
  return TADD::variableOrdering.insertVariable(label,index);
}

bool TADDVarOrder::addVariable(){
  return TADD::variableOrdering.appendVariable();
}

bool TADDVarOrder::addVariable(const std::string& label){
  return TADD::variableOrdering.appendVariable(label);
}

bool TADDVarOrder::prependVariable(){
  return TADD::variableOrdering.prependVariable();
}

bool TADDVarOrder::prependVariable(const std::string& label){
  return TADD::variableOrdering.prependVariable(label);
}

bool TADDVarOrder::removeVariable(const VariableIndexType index){
  TADD::garbageCollect();  
  return TADD::variableOrdering.removeVariable(index);
}

bool TADDVarOrder::removeVariable(const std::string& label){
  return TADDVarOrder::removeVariable(TADDVarOrder::getVariable(label));
}

VariableIndexType TADDVarOrder::getVariable(const std::string& variable){
  return TADD::variableOrdering[variable];
}

bool TADDVarOrder::isGroupMember(const VariableIndexType index){
  return (index>=size())?false:TADD::variableOrdering.getVariable(index)->getGroup();
}

VariableIndexType TADDVarOrder::groupCount(){
  return TADD::variableOrdering.groupCount();
}
