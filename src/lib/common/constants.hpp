/***************************************************************************
                           constants.hpp  -  description
                             -------------------
    begin                : Thu Dec 8 2005
    copyright            :(C) 2005-2008 by Joern Ossowski
    email                : mail@jossowski.de
 ***************************************************************************/

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <stdint.h>
#include <limits>

typedef uint32_t VariableIndexType;
const VariableIndexType VARIABLE_INDEX_MAX=std::numeric_limits<VariableIndexType>::max();

//thread configurations
const unsigned short NUMBER_OF_THREADS=3;

//constants for computed tables
struct BooleanConstants {
  static const unsigned short AND_OP= 2;
  static const unsigned short OR_OP = 4;
  static const unsigned short EXI_OP= 6;
  static const unsigned short DIF_OP= 8;
  static const unsigned short COF_OP=10;
  static const unsigned short CHA_OP=12;
};

struct ArithmeticConstants {
  static const unsigned short ADD_OP= 2;
  static const unsigned short SUM_OP= 4;
  static const unsigned short SUB_OP= 6;
  static const unsigned short MUL_OP= 8;
  static const unsigned short PRO_OP=10;
  static const unsigned short OVE_OP=12;
};

struct CompareConstants {
  static const unsigned short LES_OP=2;
  static const unsigned short LEE_OP=4;
  static const unsigned short EQU_OP=6;
};

struct ExtremaConstants {
  static const unsigned short MIN_OP    =2;
  static const unsigned short MINIMUM_OP=4;
  static const unsigned short MAX_OP    =6;
  static const unsigned short MAXIMUM_OP=8;
};

struct MoveConstants {
  static const unsigned short MUP_OP=2;
  static const unsigned short MDO_OP=4;
};

struct MatrixConstants {
  static const unsigned short MAV_OP=2;
  static const unsigned short VMA_OP=4;
  static const unsigned short MMA_OP=6;
};

//TODO: implement as structs

//special constants
#define EQI_OP              2
#define CON_OP              4

//constants for genetic minimize
const unsigned short POPULATION_SIZE=8;
const unsigned short ELITARISM_WINNERS=(POPULATION_SIZE/5>0?POPULATION_SIZE/5:1);

#endif
