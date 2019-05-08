/***************************************************************************
                          addoutput.hpp  -  description
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

#ifndef ADDOUTPUT_HPP
#define ADDOUTPUT_HPP

#include <set>
#include <string>
#include <fstream>

#include <add/addfunctioncontainer.hpp>

class ADDBaseNode;
class ADDFunction;

class ADDOutput{
public:
  static bool storeFunction(const std::string&, const ADDFunction&);
  static bool storeFunctions(const std::string&, const ADDFunctionContainer&);
private:
  ADDOutput(const std::string&);
  ~ADDOutput();

  bool storeBDD(const ADDFunctionContainer&);
  void storeBDD(ADDBaseNode*);

  std::set<ADDBaseNode*> visitedSet;
  std::ofstream file;
};

#endif
