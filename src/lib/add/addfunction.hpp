/***************************************************************************
                          addfunction.hpp  -  description
                             -------------------
    begin                : Thu Apr 15 2004
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

#ifndef ADDFUNCTION_HPP
#define ADDFUNCTION_HPP

class ADDBaseNode;
template <unsigned short N> class Path;

#include <common/constants.hpp>
#include <common/iterator.hpp>

#include <add/addtraversalhelper.hpp>

#include <boost/operators.hpp>
#include <boost/dynamic_bitset.hpp>

#include <string>

class ADDFunction : public boost::operators<ADDFunction>{
public:
  typedef Iterator<ADDBaseNode> iterator;
  iterator begin() const;
  iterator end() const;

  ADDFunction(const ADDFunction&);
  ADDFunction(const Path<2>&);
  ADDFunction(const std::string&, bool);
  ADDFunction(const double);
  explicit ADDFunction(const ADDBaseNode*);
  ADDFunction();
  ~ADDFunction();
  static ADDFunction ONE();
  static ADDFunction ZERO();
  static ADDFunction createConstantFunction(const double);
  static ADDFunction projection(const std::string&, bool);
  static ADDFunction projection(const VariableIndexType, bool);
  unsigned long size() const;
  ADDFunction& operator=(const ADDFunction&);
  ADDFunction& operator+=(const ADDFunction&);
  ADDFunction& operator-=(const ADDFunction&);
  ADDFunction& operator*=(const ADDFunction&);
  ADDFunction& operator|=(const ADDFunction&);
  ADDFunction& operator&=(const ADDFunction&);
  ADDFunction& operator^=(const ADDFunction&);
  ADDFunction& operator++();
  ADDFunction& operator--();
  ADDFunction operator!() const;
  bool operator==(const ADDFunction&) const;
  bool operator<(const ADDFunction&) const;
  bool isConstant() const;
  double getValue() const;
  VariableIndexType getFirstVariableIndex() const;
  ADDFunction cofactor(const ADDFunction&) const;
  ADDFunction subSet(const ADDFunction&) const;
  ADDFunction change() const;
  ADDFunction change(const ADDFunction&) const;
  ADDFunction exists(const ADDFunction&) const;
  ADDFunction forall(const ADDFunction&) const;
  ADDFunction equivalenceClasses(const ADDFunction&, const ADDFunction&) const;
  ADDFunction constrain(const ADDFunction&) const;
  ADDFunction sum(const ADDFunction&) const;
  ADDFunction product(const ADDFunction&) const;
  ADDFunction maximum(const ADDFunction&) const;
  double max() const;
  ADDFunction minimum(const  ADDFunction&) const;
  double min() const;
  ADDFunction oneOver() const;
  ADDFunction compose(const VariableIndexType, const ADDFunction&) const;
  ADDFunction moveUp() const;
  ADDFunction moveUp(const ADDFunction&) const;
  ADDFunction moveDown() const;
  ADDFunction moveDown(const ADDFunction&) const;
  boost::dynamic_bitset<> getEssentialVariables() const;

  ADDBaseNode* rootNode;
};

inline ADDFunction operator+(const double value, const ADDFunction& function){return ADDFunction(value)+function;}
inline ADDFunction operator-(const double value, const ADDFunction& function){return ADDFunction(value)-function;}
inline ADDFunction operator*(const double value, const ADDFunction& function){return ADDFunction(value)*function;}
ADDFunction operator-(const ADDFunction&);
inline ADDFunction EQUIV(const ADDFunction& function1, const ADDFunction& function2){return !(function1^function2);}
inline ADDFunction IMPLIES(const ADDFunction& function1, const ADDFunction& function2){return !function1|function2;}
ADDFunction ITE(const ADDFunction&, const ADDFunction&, const ADDFunction&);
ADDFunction LESS(const ADDFunction&, const ADDFunction&);
ADDFunction LESS_EQUAL(const ADDFunction&, const ADDFunction&);
ADDFunction EQUAL(const ADDFunction&, const ADDFunction&);
inline ADDFunction GREATER(const ADDFunction& function1, const ADDFunction& function2){return LESS(function2,function1);}
inline ADDFunction GREATER_EQUAL(const ADDFunction& function1, const ADDFunction& function2){return LESS_EQUAL(function2,function1);}
ADDFunction MAXIMUM(const ADDFunction&, const ADDFunction&);
ADDFunction MINIMUM(const ADDFunction&, const ADDFunction&);

ADDFunction MM(const ADDFunction&, const ADDFunction&, const ADDFunction&);
ADDFunction MV(const ADDFunction&, const ADDFunction&, const ADDFunction&);
ADDFunction VM(const ADDFunction&, const ADDFunction&, const ADDFunction&);

ADDFunction addition(const ADDFunction&, const ADDFunction&);
ADDFunction multiplication(const ADDFunction&, const ADDFunction&);

#endif
