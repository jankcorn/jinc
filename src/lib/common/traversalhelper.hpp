/***************************************************************************
                          traversalhelper.hpp  -  description
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

#ifndef TRAVERSALHELPER_HPP
#define TRAVERSALHELPER_HPP

template <typename B>
struct TraversalHelper;

/* specialized struct must implement traversal methods, e.g.
template <>
struct TraversalHelper<BaseNode>{
  TraversalHelper(const BaseNode* n);
  TraversalHelper getSucc(const unsigned short succX) const;
  const BaseNode* getSuccPtr(const unsigned short succX) const;
  bool isDrain() const;
  double getValue() const;
  const BaseNode* getPtr() const;
  VariableIndexType getLevel() const;

  static const unsigned short N=0;
};
*/

#endif
