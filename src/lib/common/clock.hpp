/***************************************************************************
                          clock.hpp  -  description
                             -------------------
    begin                : Wed Jul 8 2008
    copyright            : (C) 2008-2009 by Joern Ossowski
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

#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <sys/time.h>

struct Clock {
  struct timeval startTime;
  struct timeval stopTime;

  Clock() {
    gettimeofday(&startTime,0);
    stopTime=startTime;
  };
  void restart(){
    gettimeofday(&startTime,0);
    stopTime=startTime;
  };
  double time(){
    struct timeval temp;
    gettimeofday(&temp,0);
    return static_cast<double>(temp.tv_sec  - startTime.tv_sec ) + static_cast<double>(temp.tv_usec - startTime.tv_usec)/1000000.0;
  };
  double stop(){
    gettimeofday(&stopTime,0);
    return static_cast<double>(stopTime.tv_sec  - startTime.tv_sec ) + static_cast<double>(stopTime.tv_usec - startTime.tv_usec)/1000000.0;
  };
};

#endif
