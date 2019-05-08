/***************************************************************************
                          varorder.hpp  -  description
                             -------------------
    begin                : Tue Nov 29 2005
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

#ifndef VARORDER_HPP
#define VARORDER_HPP

#include <map>
#include <string>
#include <cstring>

#include <common/constants.hpp>
#include <common/helper.hpp>

template <typename T> class Variable;
template <typename T> class Group;
template <typename T> class VarOrder;

//******************Variable*****************
//*this template stores the information for *
//*one variable                             *
//*******************************************
//*NOTE: the object which is used with this *
//*      template must implement:           *
//*       -T(VariableIndexType)                  *
//*       -void setLevel(VariableIndexType)      *
//*       -VariableIndexType getLevel()          *
//*       -bool deleteable()                *
//*******************************************
template <typename T>
class Variable{
public:
  Variable(const VariableIndexType level) : data(new T(level)), group(0), name() {}
  Variable(const std::string& n , const VariableIndexType level) : data(new T(level)), group(0), name(n) {}
  ~Variable(){delete data;}
  void setName(const std::string& newName){name=newName;}
  std::string getName() const {return name;}
  T* getData() const {return data;}
  void setLevel(const VariableIndexType level){data->setLevel(level);}
  VariableIndexType getLevel() const {return data->getLevel();}
  bool deleteable() const {return data->deleteable();}
  void setGroup(Group<T>* newGroup){group=newGroup;}
  Group<T>* getGroup() const {return group;}
  const Variable* getFirst() const {return (group?group->getFirst():this);}
  const Variable* getLast() const {return (group?group->getLast():this);}
  VariableIndexType size() const {return (group?group->size():1);}
private:
  T* data;
  Group<T>* group;
  std::string name;
};

template <typename T>
class Group{
public:
  Group() : firstElement(0), lastElement(0) {}
  ~Group(){clear();}
  VariableIndexType size() const {
    if(firstElement) return (lastElement->getLevel()-firstElement->getLevel())+1;
    else return 0;
  }
  void clear();
  bool groupVariable(const VariableIndexType);
  bool ungroupVariable(const VariableIndexType);
  std::string getVariableOrdering() const;
  Variable<T>* getFirst() const {return firstElement;}
  Variable<T>* getLast() const {return lastElement;}
  void setFirst(Variable<T>* first){firstElement=first;}
  void setLast(Variable<T>* last){lastElement=last;}

  static VarOrder<T>* varOrder;
private:
  Variable<T>* firstElement;
  Variable<T>* lastElement;
};
template <typename T>
VarOrder<T>* Group<T>::varOrder=0;

template <typename T>
void Group<T>::clear(){
  if(firstElement){
    for(VariableIndexType i=firstElement->getLevel();i<=lastElement->getLevel();++i) varOrder->getVariable(i)->setGroup(0);
    firstElement=lastElement=0;
    varOrder->decGroupCount();
  }
}

template <typename T>
bool Group<T>::groupVariable(const VariableIndexType index){
  //you can only group a variable if
  //1. the variable exists
  //2. the variable belongs to no other group
  //3. the group is empty or the variable is next to the group
  //    (a,[b,c])->([a,b,c]) or ([a,b],c)->([a,b,c])

  Variable<T>* tempVariable=varOrder->getVariable(index);
  if(tempVariable==0) return false;
  if(tempVariable->getGroup()) return (tempVariable->getGroup()==this);
  if(!firstElement){
    firstElement=lastElement=tempVariable;
    varOrder->incGroupCount();
  } else {
    if(index+1==firstElement->getLevel()) firstElement=tempVariable;
    else if(index==lastElement->getLevel()+1) lastElement=tempVariable;
    else return false;
  }
  tempVariable->setGroup(this);

  return true;
}

template <typename T>
bool Group<T>::ungroupVariable(const VariableIndexType index){
  //you can only ungroup a variable if
  //1. the variable exists
  //2. the variable belongs to no other group
  //3. the variable is the first or last variable in the group
  //    ([a,b,c])->(a,[b,c]) or ([a,b,c])->([a,b],c)

  Variable<T>* tempVariable=varOrder->getVariable(index);
  if(tempVariable==0) return false;

  if(firstElement==tempVariable){
    if(lastElement==tempVariable){
      firstElement=lastElement=0;
      varOrder->decGroupCount();
    } else firstElement=varOrder->getVariable(index+1);
  } else {
    if(lastElement==tempVariable) lastElement=varOrder->getVariable(index-1);
    else return false;
  }

  tempVariable->setGroup(0);

  return true;
}

template <typename T>
std::string Group<T>::getVariableOrdering() const {
  std::string ret="[";
  if(firstElement){
    VariableIndexType i=firstElement->getLevel();
    ret+=varOrder->getVariable(i)->getName();
    ++i;
    while(i<=lastElement->getLevel()){
      ret+=",";
      ret+=varOrder->getVariable(i)->getName();
      ++i;
    }
  }
  ret+="]";

  return ret;
}

//********************VarOrder**********************
//*this template implements a common variable      *
//*ordering                                        *
//**************************************************
template <typename T>
class VarOrder{
public:
  VarOrder();
  ~VarOrder();
  VariableIndexType size() const {return count;}
  VariableIndexType groupCount() const;
  bool prependVariable(){return insertVariables(0,1);}
  bool prependVariable(const std::string& name){return insertVariable(name,0);}
  bool appendVariable(){return insertVariables(count,1);}
  bool appendVariable(const std::string& name){return insertVariable(name,count);}
  bool insertVariable(const VariableIndexType index){return insertVariables(index,1);}
  bool insertVariables(const VariableIndexType index, const VariableIndexType count);
  bool insertVariable(const std::string&, const VariableIndexType);
  bool removeVariable(const VariableIndexType);
  Variable<T>* getVariable(const VariableIndexType) const;
  bool swapVariables(const VariableIndexType);
  T* getData(const VariableIndexType) const;
  Variable<T>* getGroup(const VariableIndexType) const;
  Variable<T>* getNextGroup(const Variable<T>*) const;
  Variable<T>* getPrevGroup(const Variable<T>*) const;
  void incGroupCount(){++groupCounter;}
  void decGroupCount(){--groupCounter;}
  std::string getVariableOrdering() const;
  void clear();
  VariableIndexType operator[](const std::string&) const;
private:
  VariableIndexType count;
  VariableIndexType groupCounter;
  VariableIndexType reservedVariables;
  VariableIndexType skip;
  
  //TODO: replace ptr array with std::vector<Variable<T>* >
  Variable<T>** varArray;
  std::map<std::string,Variable<T>*> nameMap;
};

template <typename T>
VarOrder<T>::VarOrder() : count(0), groupCounter(0) {
  reservedVariables=1000;
  varArray=new Variable<T>*[reservedVariables];
  skip=reservedVariables/2;
  T::setSkip(skip);
}

template <typename T>
VarOrder<T>::~VarOrder(){
  if(count>0){
    for(VariableIndexType i=skip;i<skip+count;++i) delete varArray[i];
  }
  delete[] varArray;
}

template <typename T>
VariableIndexType VarOrder<T>::groupCount() const {
  if(groupCounter>0){
    VariableIndexType counter=0;
    VariableIndexType index=0;
    while(index<count){
      index=varArray[skip+index]->getLast()->getLevel()+1;
      ++counter;
    }
    return counter;
  } else return count;
}

template <typename T>
bool VarOrder<T>::insertVariables(const VariableIndexType pos, const VariableIndexType varCount){
  if(pos>count) return false;

  //check if resize is needed
  bool resizeNeeded=false;
  bool moveUp;
  if(pos<=count/2){
    moveUp=true;
    if(skip<varCount) resizeNeeded=true;
  } else {
    moveUp=false;
    if(skip+count+varCount>reservedVariables) resizeNeeded=true;
  }

  if UNLIKELY(resizeNeeded){
    unsigned int resizeValue=1000>2*varCount?1000:2*varCount;
    reservedVariables+=resizeValue;
    unsigned int newSkip=(reservedVariables-count)/2;
    Variable<T>** tempVarArray=new Variable<T>*[reservedVariables];

    //copy old values (before pos)
    if(pos>0) memcpy(tempVarArray+newSkip,varArray+skip,sizeof(Variable<T>*)*pos);
    for(VariableIndexType i=0;i<pos;++i) tempVarArray[newSkip+i]->setLevel(newSkip+i);

    //insert vars
    for(VariableIndexType i=0;i<varCount;++i) tempVarArray[newSkip+pos+i]=new Variable<T>(newSkip+pos+i);

    //copy old values (after pos)
    if(pos<count) memcpy(tempVarArray+(newSkip+pos+varCount),varArray+(skip+pos),sizeof(Variable<T>*)*(count-pos));
    for(VariableIndexType i=pos;i<count;++i) tempVarArray[newSkip+varCount+i]->setLevel(newSkip+varCount+i);

    skip=newSkip;
    T::setSkip(skip);

    //delete old varArray
    delete[] varArray;

    //update varArray
    varArray=tempVarArray;
  } else {
    if(moveUp){
      //copy old values (before pos)
      for(VariableIndexType i=0;i<pos;++i){
        varArray[skip+i-varCount]=varArray[skip+i];
        varArray[skip+i-varCount]->setLevel(skip+i-varCount);
      }
      //insert vars
      for(VariableIndexType i=0;i<varCount;++i) varArray[skip+pos+i-varCount]=new Variable<T>(skip+pos+i-varCount);
      skip-=varCount;
      T::setSkip(skip);
    } else {
      //copy old values (after pos)
      for(VariableIndexType i=count;i>pos;--i){
        varArray[skip+i+varCount-1]=varArray[skip+i-1];
        varArray[skip+i+varCount-1]->setLevel(skip+i+varCount-1);
      }
      //insert vars
      for(VariableIndexType i=0;i<varCount;++i) varArray[skip+pos+i]=new Variable<T>(skip+pos+i);
    }
  }

  //check if positions are inside a group
  if((pos>0) && (pos<count)){
    if(varArray[skip+pos-1]->getGroup()){
      if(varArray[skip+pos-1]->getGroup()==varArray[skip+pos+varCount]->getGroup()){
	for(VariableIndexType i=0;i<varCount;++i) varArray[skip+pos+i]->setGroup(varArray[skip+pos-1]->getGroup());
      }
    }
  }
  count+=varCount;
  return true;
}

template <typename T>
bool VarOrder<T>::insertVariable(const std::string& name, const VariableIndexType pos){
  if(name=="") return false;
  if(nameMap.find(name)!=nameMap.end()) return false;

  if(insertVariables(pos,1)){
    varArray[skip+pos]->setName(name);
    typedef std::map<std::string,Variable<T>*> nameMapType;
    nameMap.insert(typename nameMapType::value_type(name,varArray[skip+pos]));
    return true;
  }

  return false;
}

template <typename T>
bool VarOrder<T>::removeVariable(const VariableIndexType index){
  if(index>=count) return false;
  if(!varArray[skip+index]->deleteable()) return false;

  if(varArray[skip+index]->getGroup()) varArray[skip+index]->getGroup()->ungroupVariable(varArray[skip+index]->getLevel());
  nameMap.erase(varArray[skip+index]->getName());
  delete varArray[skip+index];

  if(count==1){
    skip=reservedVariables/2;
    T::setSkip(skip);
  } else {
    //check if first variable is removed (in this case it is only necessary to increase skip)
    if(index==0){
      ++skip;
      T::setSkip(skip);
    } else {
      VariableIndexType i;
      //check if move down or move up
      if(index<=count/2){
        //copy old variables (before index)
        Variable<T>** target=&varArray[skip+index];
        Variable<T>** source=target-1;
        Variable<T>** end=&varArray[skip-1];
        i=skip+index;
        while(source!=end){
	  (*target)=(*source);
          (*target)->setLevel(i);
          target=(source--);
          --i;
        }
        ++skip;
        T::setSkip(skip);
      } else {
        //copy old variables (after index)
        if(index+1<count){
          Variable<T>** target=&varArray[skip+index];
          Variable<T>** source=target+1;
          Variable<T>** end=&varArray[skip+count];
          i=skip+index;
          while(source!=end){
	          (*target)=(*source);
	          (*target)->setLevel(i);
            target=(source++);
            ++i;
          }
        }
      }
    }
  }

  --count;

  return true;
}

template <typename T>
Variable<T>* VarOrder<T>::getVariable(const VariableIndexType index) const {
  return LIKELY(index<count)?varArray[skip+index]:0;
}

template <typename T>
bool VarOrder<T>::swapVariables(const VariableIndexType index){
  if(index+1>=count) return false;

  std::swap(varArray[skip+index],varArray[skip+index+1]);
  varArray[skip+index]->setLevel(skip+index);
  varArray[skip+index+1]->setLevel(skip+index+1);

  //be careful with this function
  //following situations could occure
  //([a,b],{c,d})->([a,{c,b],d})
  //(a,[b,c])->([b,a,c])
  //this behaviour is needed for reordering algorithms on groups
  Group<T>* group=varArray[skip+index]->getGroup();
  if(group){
    if(group==varArray[skip+index+1]->getGroup()){
      //check for ([a,b,c])->([b,a,c]), ([a,b,c])->([a,c,b]) or ([a,b])->([b,a])
      if(varArray[skip+index+1]==group->getFirst()) group->setFirst(varArray[skip+index]);
      if(varArray[skip+index]==group->getLast()) group->setLast(varArray[skip+index+1]);
    }
  }

  return true;
}

template <typename T>
T* VarOrder<T>::getData(const VariableIndexType index) const {
  if UNLIKELY(index>=count) return 0;
  else return varArray[skip+index]->getData();
}

template <typename T>
Variable<T>* VarOrder<T>::getGroup(const VariableIndexType index) const {
  if(groupCounter==0){
    if UNLIKELY(index>=count) return 0;
    else return varArray[skip+index];
  }

  VariableIndexType i=0;
  VariableIndexType groupIndex=0;
  while(i<count){
    if(groupIndex==index){
      return varArray[skip+i];
    }
    if(varArray[skip+i]->getGroup()) i=varArray[skip+i]->getGroup()->getLast()->getLevel();
    ++i;
    ++groupIndex;
  }

  return 0;
}

template <typename T>
Variable<T>* VarOrder<T>::getNextGroup(const Variable<T>* var) const {
  VariableIndexType index=var->getGroup()?var->getGroup()->getLast()->getLevel()+1:var->getLevel()+1;
  if(index>=count) return 0;
  
  return varArray[skip+index];
}

template <typename T>
Variable<T>* VarOrder<T>::getPrevGroup(const Variable<T>* var) const {
  VariableIndexType index=var->getGroup()?var->getGroup()->getFirst()->getLevel():var->getLevel();
  if(index==0) return 0;
  --index;
  
  return varArray[skip+index];
}

template <typename T>
std::string VarOrder<T>::getVariableOrdering() const {
  std::string ret="(";

  VariableIndexType i=0;
  while(i<count){
    if(!varArray[skip+i]->getGroup()){
      ret+=varArray[skip+i]->getName();
    } else {
      ret+=varArray[skip+i]->getGroup()->getVariableOrdering();
      i=varArray[skip+i]->getGroup()->getLast()->getLevel();
    }
    if(i+1<count) ret+=",";
    ++i;
  }
  ret+=")";

  return ret;
}

template <typename T>
void VarOrder<T>::clear(){
  //remove all unused variables
  VariableIndexType newIndex=0;
  VariableIndexType newCount=count;
  VariableIndexType newSkip=skip;
  for(VariableIndexType index=0;index<count;++index){
    //check if variable isn't deletable
    if(!varArray[skip+index]->deleteable()){
      if(index!=newIndex){
        //first variables have been removed (otherwise index would be zero) so skip must be updated
        if(newIndex==0) newSkip=skip+index;
        varArray[newSkip+newIndex]=varArray[skip+index];
	      varArray[newSkip+newIndex]->setLevel(newSkip+newIndex);
      }
      ++newIndex;
    } else {
      //ungroup variable if necessary
      if(varArray[skip+index]->getGroup()) varArray[skip+index]->getGroup()->ungroupVariable(varArray[skip+index]->getLevel());

      //remove name from nameMap if neccessary
      nameMap.erase(varArray[skip+index]->getName());

      //delete variable
      delete varArray[skip+index];

      --newCount;
    }
  }

  count=newCount;
  skip=newSkip;
  T::setSkip(skip);
}

template <typename T>
VariableIndexType VarOrder<T>::operator[](const std::string& name) const {
  if(nameMap.find(name)==nameMap.end()) return VARIABLE_INDEX_MAX;
  else return nameMap.find(name)->second->getLevel();
}
#endif
