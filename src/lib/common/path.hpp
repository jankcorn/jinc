/***************************************************************************
                          path.hpp  -  description
                             -------------------
    begin                : Sat Jan 24 2009
    copyright            : (C) 2009 by Joern Ossowski
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

#ifndef PATH_HPP
#define PATH_HPP

#include <common/constants.hpp>

#include <vector>
#include <iostream>
#include <tr1/tuple>

template <unsigned short N>
class Path{
public:
  template <unsigned short M> friend std::ostream& operator<<(std::ostream& o, const Path<M>  & p);
  typedef std::vector<short> vector;
  Path() : value(0.0) {}
  Path(int size) : value(1.0) {path.resize(size,-1);}
  Path& operator=(const double v){
    value=v;
    return *this;
  }
  double getValue() const {return value;}
  short& operator[](const VariableIndexType index){return path[index];}
  const short& operator[](const VariableIndexType index) const {return path[index];}
  VariableIndexType size() const {return path.size();}
  bool operator==(const Path& p) const {return ((value==p.value) && (path==p.path));}
  bool operator!=(const Path& p) const {return !(*this==p);}
private:
  double value;
  vector path;
};

template <unsigned short N>
Path<N> interleave(const Path<N>& p1, const Path<N>& p2){
  VariableIndexType size=p1.size()>=p2.size()?2*p1.size():p2.size();
  Path<N> p(size);
  for(VariableIndexType i=0;i<size;++i){
    if(i%2==0){
      if(i/2<p1.size()) p[i]=p1[i/2];
    } else {
      if(i/2<p2.size()) p[i]=p2[i/2];
    }
  }
  return p;
}

template <unsigned short N>
std::tr1::tuple<Path<N>,Path<N> > divide(const Path<N>& p){
  VariableIndexType size=(p.size()+1)/2;
  Path<N> p1(size);
  Path<N> p2(size);
  for(VariableIndexType i=0;i<2*size;++i){
    if(i<p.size()){
      if(i%2==0){
	p1[i/2]=p[i];
      } else {
	p2[i/2]=p[i];
      }
    }
  }
  return std::tr1::tuple<Path<N>,Path<N> >(p1,p2);
}

template <unsigned short N>
inline std::ostream& operator<<(std::ostream& o, const Path<N>& p){
  typename Path<N>::vector::const_iterator it=p.path.begin();
  while(it!=p.path.end()){
    if((*it)==-1) o << '-';
    else o << (*it);
    ++it;
  }
  o << "|" << p.value;
  return o;
}

#endif
