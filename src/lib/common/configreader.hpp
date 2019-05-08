/***************************************************************************
                          configreader.hpp  -  description
                             -------------------
    begin                : Fri Apr 21 2006
    copyright            : (C) 2006 by Joern Ossowski
    email                : ossowski@cs.uni-bonn.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CONFIG_READER_HPP
#define CONFIG_READER_HPP

#include <string>
#include <map>

using namespace std;

class ConfigReader{
public:
	ConfigReader(const string&);
  unsigned long getULong(const string&, const unsigned long);
  float getFloat(const string&, const float);
  double getDouble(const string&, const double);
private:
  map<string,string> valueMap;

  void readConfigFile(const string&);
};

#endif
