/***************************************************************************
                           spinlock.hpp  -  description
                             -------------------
    begin                : Mon Jan 12 2009
    copyright            :(C) 2009 by Joern Ossowski
    email                : mail@jossowski.de
 ***************************************************************************/

#ifndef SPINLOCK_HPP
#define SPINLOCK_HPP

#include <cassert>
#include <common/helper.hpp>

struct SpinLock {
  SpinLock(bool& e) : element(e){
    while(!Common::atomicCompareAndSwap<bool>(element,false,true)){}
  }
  ~SpinLock(){
    bool r=Common::atomicCompareAndSwap<bool>(element,true,false);
    assert(r);
  }
  bool& element;
};

#endif
