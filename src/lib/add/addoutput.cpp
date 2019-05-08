/***************************************************************************
                          addoutput.cpp  -  description
                             -------------------
    begin                : Tue Jul 13 2004
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

#include <add/addoutput.hpp>
#include <add/addbasenode.hpp>
#include <add/addinnernode.hpp>
#include <add/addterminalnode.hpp>
#include <add/addfunction.hpp>
#include <add/addvarorder.hpp>

bool ADDOutput::storeFunction(const std::string& filename, const ADDFunction& function){
  return ADDOutput::storeFunctions(filename,ADDFunctionContainer(function));
}

bool ADDOutput::storeFunctions(const std::string& filename, const ADDFunctionContainer& container){
  ADDOutput output(filename);
  return output.storeBDD(container);
}

ADDOutput::ADDOutput(const std::string& filename){
  file.open(filename.c_str(),std::ios::out);
}

ADDOutput::~ADDOutput(){
  if(file) file.close();
}

bool ADDOutput::storeBDD(const ADDFunctionContainer& container){
  unsigned long i;

  //file open?
  if(!file) return false;
  //empty container?
  if(container.size()==0) return false;

  file << "graph BDD {" << std::endl;
  for(i=0;i<container.size();++i){
    ADDBaseNode* node=container[i].rootNode;
    //file << "rootnode {" << std::endl;
    //file << "  id=" << node << std::endl;
    //file << "}" << std::endl;
    storeBDD(node);
  }
  file << "}" << std::endl;

  return true;
}

void ADDOutput::storeBDD(ADDBaseNode* node){
  if(visitedSet.find(node)!=visitedSet.end()) return;
  unsigned long n=reinterpret_cast<unsigned long>(node);
  if(!node->isDrain()){
    unsigned long s0=reinterpret_cast<unsigned long>(static_cast<ADDInnerNode*>(node)->getSucc0());
    unsigned long s1=reinterpret_cast<unsigned long>(static_cast<ADDInnerNode*>(node)->getSucc1());
    file << n << " -- " << s0 << " [style=dotted];" << std::endl;
    file << n << " -- " << s1 << ";" << std::endl;
    file << n << " [label=" << ADDVarOrder::getVariableName(node->getLevel())  << "];" << std::endl;
    visitedSet.insert(node);
    storeBDD(static_cast<ADDInnerNode*>(node)->getSucc0());
    storeBDD(static_cast<ADDInnerNode*>(node)->getSucc1());
  } else {
    file << n << " [shape=box,label=" << (static_cast<ADDTerminalNode*>(node)->getValue())  << "];" << std::endl;
    visitedSet.insert(node);
  }
}
