#ifndef BDDALGORITHMS_HPP
#define BDDALGORITHMS_HPP

#include <common/constants.hpp>
#include <common/traversalhelper.hpp>
#include <common/iterator.hpp>

#include <set>
//C++0x
//#include <unordered_set>
#include <stack>

#include <boost/dynamic_bitset.hpp>

template <typename B, typename BDD>
boost::dynamic_bitset<> getEssentialVariables(const B* root){
  const VariableIndexType size=BDD::variableOrdering.size();
  boost::dynamic_bitset<> bitSet(size,false);
  Iterator<B> it(root,size);
  Iterator<B> end;
  while(it!=end){
    for(VariableIndexType i=0;i<size;++i){
      if((*it)[i]>=0) bitSet[i]=true;
    }
    ++it;
  }
  
  return bitSet;
}

template <typename B>
unsigned long size(const B* root){
  //C++0x
  //std::unordered_set<const B*> visited; c++0x
  std::set<const B*> visited;
  std::stack<const B*> nodeStack;

  TraversalHelper<B> node=root;
  visited.insert(node.getPtr());
  nodeStack.push(node.getPtr());
  while(nodeStack.size()>0){
    node=nodeStack.top();
    nodeStack.pop();
    if LIKELY(!node.isDrain()){
      for(unsigned short i=0;i<TraversalHelper<B>::N;++i){
        const B* succ=node.getSuccPtr(i);
        if(visited.find(succ)==visited.end()){
          visited.insert(succ);
          nodeStack.push(succ);
        }
      }
    }
  }

  return visited.size();
}

#endif
