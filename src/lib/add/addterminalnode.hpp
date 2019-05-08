/***************************************************************************
                          addterminalnode.hpp  -  description
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

#ifndef ADDTERMINALNODE_HPP
#define ADDTERMINALNODE_HPP

#include <add/addbasenode.hpp>

class ADDTerminalNode : public ADDBaseNode {
public:
  ADDTerminalNode() : value(0.0) {}
  ADDTerminalNode* getNext() const {return static_cast<ADDTerminalNode*>(next);}
  void setNext(const ADDTerminalNode* next){this->next=static_cast<ADDBaseNode*>(const_cast<ADDTerminalNode*>(next));}
  void setValue(double value){
    preNodeCount=0;
    this->value=value;
  }
  double getValue() const {return value;}
private:
  double value;
};

#endif
