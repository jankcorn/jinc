/***************************************************************************
                          taddfunction.hpp  -  description
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

#ifndef TADDFUNCTION_HPP
#define TADDFUNCTION_HPP

class TADDBaseNode;
struct TADDBaseNode_2;
template <unsigned short N> class Path;

#include <common/constants.hpp>
#include <common/iterator.hpp>

#include <tadd/taddtraversalhelper.hpp>

#include <boost/operators.hpp>
#include <boost/dynamic_bitset.hpp>

#include <string>

class TADDFunction : public boost::operators<TADDFunction>{
public:
  typedef Iterator<TADDBaseNode> iterator_4;
  typedef Iterator<TADDBaseNode_2,TADDBaseNode> iterator;
  iterator_4 begin_4() const;
  iterator_4 end_4() const;
  iterator begin() const;
  iterator end() const;
  
  TADDFunction(const TADDFunction&);
  TADDFunction(const Path<2>&);
  TADDFunction(const Path<4>&);
  TADDFunction(const std::string&, unsigned short);
  TADDFunction(const double);
  explicit TADDFunction(const TADDBaseNode*);
  TADDFunction();
  ~TADDFunction();
  static TADDFunction ONE();
  static TADDFunction ZERO();
  static TADDFunction createConstantFunction(const double);
  static TADDFunction projection(const std::string&, unsigned short);
  static TADDFunction projection(const VariableIndexType, unsigned short);
  static TADDFunction projection_2(const VariableIndexType, bool);
  unsigned long size() const;
  TADDFunction& operator=(const TADDFunction&);
  TADDFunction& operator+=(const TADDFunction&);
  TADDFunction& operator-=(const TADDFunction&);
  TADDFunction& operator*=(const TADDFunction&);
  TADDFunction& operator|=(const TADDFunction&);
  TADDFunction& operator&=(const TADDFunction&);
  TADDFunction& operator^=(const TADDFunction&);
  TADDFunction& operator++();
  TADDFunction& operator--();
  TADDFunction operator!() const;
  bool operator==(const TADDFunction&) const;
  bool operator<(const TADDFunction&) const;
  bool isConstant() const;
  double getValue() const;
  VariableIndexType getFirstVariableIndex() const;
  TADDFunction cofactor(const TADDFunction&) const;
  TADDFunction subSet(const TADDFunction&) const;
  TADDFunction change() const;
  TADDFunction change(const TADDFunction&) const;
  TADDFunction exists(const TADDFunction&) const;
  TADDFunction forall(const TADDFunction&) const;
  TADDFunction equivalenceClasses(const TADDFunction&, const TADDFunction&) const;
  TADDFunction constrain(const TADDFunction&) const;
  TADDFunction sum(const TADDFunction&) const;
  TADDFunction product(const TADDFunction&) const;
  TADDFunction maximum(const TADDFunction&) const;
  double max() const;
  TADDFunction minimum(const  TADDFunction&) const;
  double min() const;
  TADDFunction oneOver() const;
  TADDFunction compose(const VariableIndexType, const TADDFunction&) const;
  TADDFunction toggle() const;
  TADDFunction moveUp() const;
  TADDFunction moveUp(const TADDFunction&) const;
  TADDFunction moveDown() const;
  TADDFunction moveDown(const TADDFunction&) const;
  boost::dynamic_bitset<> getEssentialVariables() const;

  TADDBaseNode* rootNode;
};

inline TADDFunction operator+(const double value, const TADDFunction& function){return TADDFunction(value)+function;}
inline TADDFunction operator-(const double value, const TADDFunction& function){return TADDFunction(value)-function;}
inline TADDFunction operator*(const double value, const TADDFunction& function){return TADDFunction(value)*function;}
TADDFunction operator-(const TADDFunction&);
inline TADDFunction EQUIV(const TADDFunction& function1, const TADDFunction& function2){return !(function1^function2);}
inline TADDFunction IMPLIES(const TADDFunction& function1, const TADDFunction& function2){return !function1|function2;}
TADDFunction ITE(const TADDFunction&, const TADDFunction&, const TADDFunction&);
TADDFunction LESS(const TADDFunction&, const TADDFunction&);
TADDFunction LESS_EQUAL(const TADDFunction&, const TADDFunction&);
TADDFunction EQUAL(const TADDFunction&, const TADDFunction&);
inline TADDFunction GREATER(const TADDFunction& function1, const TADDFunction& function2){return LESS(function2,function1);}
inline TADDFunction GREATER_EQUAL(const TADDFunction& function1, const TADDFunction& function2){return LESS_EQUAL(function2,function1);}
TADDFunction MAXIMUM(const TADDFunction&, const TADDFunction&);
TADDFunction MINIMUM(const TADDFunction&, const TADDFunction&);

TADDFunction MM(const TADDFunction&, const TADDFunction&, const TADDFunction&);
TADDFunction MV(const TADDFunction&, const TADDFunction&, const TADDFunction&);
TADDFunction VM(const TADDFunction&, const TADDFunction&, const TADDFunction&);

TADDFunction addition(const TADDFunction&, const TADDFunction&);
TADDFunction multiplication(const TADDFunction&, const TADDFunction&);

#endif
