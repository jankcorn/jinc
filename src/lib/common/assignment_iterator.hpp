/***************************************************************************
                          assignment_iterator.hpp  -  description
                             -------------------
    begin                : Tue Jan 27 2009
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

#ifndef ASSIGNMENT_ITERATOR_HPP
#define ASSIGNMENT_ITERATOR_HPP

#include <common/constants.hpp>
#include <common/path.hpp>

#include <vector>
#include <cmath>

template <typename Iterator>
class Assignment_Iterator{
public:
  static Assignment_Iterator end(){
    return Assignment_Iterator(Iterator());
  }

  Assignment_Iterator(const Iterator& i) : iterator(i), assignment(*i) {
    for(VariableIndexType i=0;i<assignment.size();++i){
      if(assignment[i]<0){
        assignment[i]=0;
        dontCarePositions.push_back(i);
      }
    }
  }
  Assignment_Iterator(const Path<Iterator::N>& p) : iterator(p), assignment(p) {
    for(VariableIndexType i=0;i<assignment.size();++i){
      if(assignment[i]<0){
        assignment[i]=0;
        dontCarePositions.push_back(i);
      }
    }
  }
  bool operator==(const Assignment_Iterator& a) const {
    if(iterator.isEnd() && a.iterator.isEnd()) return true;
    return ((!iterator.isEnd()) && (!a.iterator.isEnd()) && (assignment==a.assignment));
  }
  bool operator!=(const Assignment_Iterator& a) const {return !(*this==a);}
  Path<Iterator::N>& operator*(){return assignment;}
  Assignment_Iterator& operator++(){
    const unsigned short N=Iterator::N;
    bool incrementIterator=true;
    VariableIndexType index=0;
    for(VariableIndexType i=dontCarePositions.size();(i>0) && incrementIterator;--i){
      if(static_cast<unsigned short>(assignment[dontCarePositions[i-1]])<(N-1)){
        index=i-1;
        incrementIterator=false;
      }
    }
    if(incrementIterator){
      ++iterator;
      dontCarePositions.clear();
      assignment=*iterator;
      if(iterator.isEnd()) return *this;
      for(VariableIndexType i=0;i<assignment.size();++i){
        if(assignment[i]<0){
          assignment[i]=0;
          dontCarePositions.push_back(i);
        }
      }
    } else {
      ++assignment[dontCarePositions[index]];
      while((++index)<dontCarePositions.size()){
        assignment[dontCarePositions[index]]=0;
      }
    }
    return *this;
  }
  static double count(const Iterator& i){
    Iterator it=i;
    Iterator end;
    double ret=0.0;
    while(it!=end){
      VariableIndexType dontCares=0;
      for(VariableIndexType i=0;i<(*it).size();++i){
        if((*it)[i]<0) ++dontCares;
      }
      ret+=pow(static_cast<double>(Iterator::N),static_cast<double>(dontCares));
      ++it;
    }
    return ret;
  }
private:
  Iterator iterator;
  Path<Iterator::N> assignment;
  std::vector<VariableIndexType> dontCarePositions;
};

#endif
