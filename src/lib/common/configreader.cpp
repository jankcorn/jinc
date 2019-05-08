/***************************************************************************
                          configreader.cpp  -  description
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

#include <common/configreader.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>

ConfigReader::ConfigReader(const string& filename){
	readConfigFile(filename);
}

unsigned long ConfigReader::getULong(const string& key, const unsigned long defaultValue){
	if(valueMap.find(key)==valueMap.end()) return defaultValue;
	else {
		cout << "[Config Reader] " << key << "=" << atol(valueMap[key].c_str()) << " [" << defaultValue << "]" << endl;
		return atol(valueMap[key].c_str());
  }
}

float ConfigReader::getFloat(const string& key, const float defaultValue){
	if(valueMap.find(key)==valueMap.end()) return defaultValue;
	else {
		cout << "[Config Reader] " << key << "=" << atof(valueMap[key].c_str()) << " [" << defaultValue << "]" << endl;
		return atof(valueMap[key].c_str());
  }
}

double ConfigReader::getDouble(const string& key, const double defaultValue){
  char* temp;
	if(valueMap.find(key)==valueMap.end()) return defaultValue;
	else {
		cout << "[Config Reader] " << key << "=" << strtod(valueMap[key].c_str(),&temp) << " [" << defaultValue << "]" << endl;
		return strtod(valueMap[key].c_str(),&temp);
  }
}

void ConfigReader::readConfigFile(const string& filename){
	string fileName=(filename=="")?(string(getenv("HOME"))+"/.jinc/config"):filename;
	ifstream file(fileName.c_str(),ios::in);
	if(!file) return;
	
	string line;
	while(!file.eof()){
	  getline(file,line);
	  if(line.find("=")!=string::npos) valueMap[line.substr(0,line.find("="))]=line.substr(line.find("=")+1);
	}
	file.close();
}
