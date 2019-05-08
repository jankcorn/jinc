/***************************************************************************
                          functioncontainer.hpp  -  description
                             -------------------
    begin                : Fri Nov 25 2005
    copyright            : (C) 2005-2008 by Joern Ossowski
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

#ifndef FUNCTION_CONTAINER_HPP
#define FUNCTION_CONTAINER_HPP

#include <vector>

template <typename T>
class FunctionContainer{
public:
  FunctionContainer();
  FunctionContainer(const T&);
  FunctionContainer(const FunctionContainer&);
  ~FunctionContainer();
  FunctionContainer& operator=(const FunctionContainer&);
  FunctionContainer& operator=(const T&);

  bool operator==(const FunctionContainer&) const;
  bool operator==(const T&) const;
  bool operator!=(const FunctionContainer&) const;
  bool operator!=(const T&) const;
  T& operator[](const unsigned short);
  const T& operator[](const unsigned short) const;
  void push(const T&);
  void push(const FunctionContainer&);
  bool pop();
  bool remove(unsigned long);
  unsigned long size() const;
  void clear();
private:
  std::vector<T> functions;
};

template <typename T>
FunctionContainer<T>::FunctionContainer(){}

template <typename T>
FunctionContainer<T>::FunctionContainer(const T& function){functions.push_back(function);}

template <typename T>
FunctionContainer<T>::FunctionContainer(const FunctionContainer& container){
  for(unsigned int i=0;i<container.functions.size();++i) functions.push_back(container.functions[i]);
}

template <typename T>
FunctionContainer<T>::~FunctionContainer(){}

template <typename T>
FunctionContainer<T>& FunctionContainer<T>::operator=(const FunctionContainer& container){
  if(&container!=this) functions=container.functions;

  return *this;
}

template <typename T>
FunctionContainer<T>& FunctionContainer<T>::operator=(const T& function){
  functions.clear();
  functions.push_back(function);

  return *this;
}

template <typename T>
bool FunctionContainer<T>::operator==(const FunctionContainer& container) const {
  if(functions.size()!=container.functions.size()) return false;
  for(unsigned int i=0;i<functions.size();++i) if(functions[i]!=container.functions[i]) return false;

  return true;
}

template <typename T>
bool FunctionContainer<T>::operator==(const T& function) const {
  return *this==FunctionContainer<T>(function);
}

template <typename T>
bool FunctionContainer<T>::operator!=(const FunctionContainer& container) const {
  return !((*this)==container);
}

template <typename T>
bool FunctionContainer<T>::operator!=(const T& function) const {
  return *this!=FunctionContainer<T>(function);
}

template <typename T>
T& FunctionContainer<T>::operator[](const unsigned short index){
  return functions[index];
}

template <typename T>
const T& FunctionContainer<T>::operator[](const unsigned short index) const {
  return functions[index];
}

template <typename T>
void FunctionContainer<T>::push(const T& function){
  functions.push_back(function);
}

template <typename T>
void FunctionContainer<T>::push(const FunctionContainer& container){
  for(unsigned short i=0;i<container.functions.size();++i) functions.push_back(container.functions[i]);
}

template <typename T>
bool FunctionContainer<T>::pop(){
  if(!functions.size()) return false;

  functions.pop_back();

  return true;
}

template <typename T>
bool FunctionContainer<T>::remove(const unsigned long i){
  if(i<functions.size()){
    functions.erase(functions.begin()+i);
    return true;
  } else return false;
}

template <typename T>
unsigned long FunctionContainer<T>::size() const {
  return functions.size();
}

template <typename T>
void FunctionContainer<T>::clear(){
  functions.clear();
}

#endif

