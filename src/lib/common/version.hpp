/***************************************************************************
                          version.hpp  -  description
                             -------------------
    begin                : Wed Feb 16 2004
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

#ifndef VERSION_HPP
#define VERSION_HPP

/**
 * \file version.h
 * \brief This file contains the version number of the current release.
 *
 * This file can be used to check if the headers of the library are installed.
 * \code
 * AC_CHECK_HEADERS([jinc/common/version.hpp], ,
 *    [AC_MSG_ERROR([unable to find the jinc header files])])
 * \endcode
 * The example above shows how this can be done with automake.
 * How to check if the library is installed is shown in the example below.
 * \code
 * AC_CHECK_LIB([add],main, ,
 *    [AC_MSG_ERROR([cannot find the jinc library])])
 * \endcode
 */

const short JINC_VERSION_MAJOR=2;
const short JINC_VERSION_MINOR=2;

#endif
