/***************************************************************************
                          iterator.hpp  -  description
                             -------------------
    begin                : Mon Jan 26 2009
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

#ifndef ITERATOR_HPP
#define ITERATOR_HPP

#include <common/helper.hpp>
#include <common/path.hpp>
#include <common/traversalhelper.hpp>

#include <stack>

//TODO: operator-- ???
template <typename T, typename B=T>
class Iterator{
public:
  static Iterator end(){
    return Iterator();
  }

  Iterator() : valid(false), rootNode(0) {}
  Iterator(const Iterator& it) : valid(it.valid), rootNode(it.rootNode), p(it.p) {
    if(rootNode!=0) const_cast<B*>(Common::SmartAccess<B>(rootNode).getPtr())->incNodeCount();
  }
  Iterator(const B* node, VariableIndexType size) : valid(false), rootNode(const_cast<B*>(node)), p(size) {
    TraversalHelper<T> t=node;
    const_cast<B*>(Common::SmartAccess<B>(rootNode).getPtr())->incNodeCount();
    if(t.isDrain()){
      if(t.getValue()!=0.0) valid=true;
      p=t.getValue();
    } else {
      valid=true;
      while(!t.isDrain()){
        for(unsigned short succ=0;succ<TraversalHelper<T>::N;++succ){
          TraversalHelper<T> temp=t.getSucc(succ);
          if((temp.getValue()!=0.0) || (!temp.isDrain())){
            p[t.getLevel()]=succ;
            t=temp;
            break;
          }
        }
      }
      p=t.getValue();
    }
  }
  Iterator(const Path<TraversalHelper<T>::N>& path) : valid(true), rootNode(0), p(path) {}
  ~Iterator(){
    if(rootNode!=0) const_cast<B*>(Common::SmartAccess<B>(rootNode).getPtr())->deleteNode();
  }
  Iterator& operator=(const Iterator& it){
    if(this==&it) return *this;
    valid=it.valid;
    if(it.rootNode!=0) const_cast<B*>(Common::SmartAccess<B>(it.rootNode).getPtr())->incNodeCount();
    if(rootNode!=0) const_cast<B*>(Common::SmartAccess<B>(rootNode).getPtr())->deleteNode();
    rootNode=it.rootNode;
    p=it.p;
    return *this;
  }
  Iterator& operator++(){
    if(valid){
      if(rootNode==0){
	valid=false;
	return *this;
      }
      std::stack<TraversalHelper<T> > nodeStack;

      //build stack recursively
      {
        TraversalHelper<T> t=rootNode;
        if(!t.isDrain()) {
          nodeStack.push(t);
          for(VariableIndexType i=0;i<p.size();++i){
            if(p[i]>=0){
              TraversalHelper<T> temp=t;
              t=t.getSucc(p[i]);
              if(!t.isDrain()) nodeStack.push(t);
            }
          }
        }
      }

      //search for new path
      bool found=false;
      TraversalHelper<T> newPath=0;
      while((!nodeStack.empty()) && (!found)){
        TraversalHelper<T> t=nodeStack.top();
        nodeStack.pop();
        for(unsigned short i=static_cast<unsigned short>(p[t.getLevel()]+1);i<TraversalHelper<T>::N;++i){
          TraversalHelper<T> temp=t.getSucc(i);
          if((temp.getValue()!=0.0) || (!temp.isDrain())){
            p[t.getLevel()]=i;
            VariableIndexType end=temp.isDrain()?p.size():temp.getLevel();
            for(VariableIndexType j=t.getLevel()+1;j<end;++j) p[j]=-1;
            found=true;
            newPath=temp;
            break;
          }
        }
      }
      
      if(!found){
        valid=false;
        const_cast<B*>(Common::SmartAccess<B>(rootNode).getPtr())->deleteNode();
        rootNode=0;
      } else {
        while(!newPath.isDrain()){
          for(unsigned short succ=0;succ<TraversalHelper<T>::N;++succ){
            TraversalHelper<T> temp=newPath.getSucc(succ);
            if((temp.getValue()!=0.0) || (!temp.isDrain())){
              p[newPath.getLevel()]=succ;
              VariableIndexType end=temp.isDrain()?p.size():temp.getLevel();
              for(VariableIndexType j=newPath.getLevel()+1;j<end;++j) p[j]=-1;
              newPath=temp;
              break;
            }
          }
        }
        p=newPath.getValue();
      }
    }
    return *this;
  }
  bool operator==(const Iterator& i) const {
    if(isEnd() && i.isEnd()) return true;
    return ((!isEnd()) && (!i.isEnd()) && (rootNode==i.rootNode) && (p==i.p));
  }
  bool operator!=(const Iterator& i) const {return !(*this==i);}
  bool isEnd() const {return !valid;}
  Path<TraversalHelper<T>::N>& operator*(){return p;}
  const Path<TraversalHelper<T>::N>& operator*() const {return p;}
  static const unsigned short N=TraversalHelper<T>::N;
  static double count(const Iterator& i){
    Iterator it=i;
    Iterator end;
    double ret=0.0;
    while(it!=end){
      ret+=1.0;
      ++it;
    }
    return ret;
  }
private:
  bool valid;
  B* rootNode;
  Path<TraversalHelper<T>::N> p;
};

#endif
