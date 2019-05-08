/***************************************************************************
                          taddterminalnode.hpp  -  description
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

#ifndef TADDTERMINALNODE_HPP
#define TADDTERMINALNODE_HPP

#include <tadd/taddbasenode.hpp>

class TADDTerminalNode : public TADDBaseNode {
public:
  TADDTerminalNode() : value(0.0) {setSymmetry(true);}
  TADDTerminalNode* getNext() const {return static_cast<TADDTerminalNode*>(next);}
  void setNext(const TADDTerminalNode* next){this->next=static_cast<TADDBaseNode*>(const_cast<TADDTerminalNode*>(next));}
  void setValue(double value){
    preNodeCount=0;
    this->value=value;
  }
  double getValue() const {return value;}
private:
  double value;
};

#endif
