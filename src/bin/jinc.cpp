/***************************************************************************
                          jinc.cpp  -  description
                             -------------------
    begin                : Fri Nov 25 2005
    copyright            : (C) 2005 by Joern Ossowski
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

#include <add/add.hpp>
#include <add/addinnernode.hpp>
#include <add/addterminalnode.hpp>
#include <add/addvarorder.hpp>
#include <add/addfunction.hpp>
#include <add/addtraversalhelper.hpp>
#include <add/addgenericapply.hpp>

#include <tadd/tadd.hpp>
#include <tadd/taddinnernode.hpp>
#include <tadd/taddterminalnode.hpp>
#include <tadd/taddvarorder.hpp>
#include <tadd/taddfunction.hpp>
#include <tadd/taddtraversalhelper.hpp>

// #include <nadd/nadd.h>
// #include <fevbdd/fevbdd.h>
// #include <tadd/tadd.h>
// #include <ztadd/ztadd.h>
// #include <zadd/zadd.h>
#include <common/assignment_iterator.hpp>
#include <common/ptrhashmap.hpp>
#include <common/constants.hpp>
#include <common/reordering.hpp>
#include <common/clock.hpp>
#include <common/commondata.hpp>
#include <common/elementpool.hpp>
#include <common/future.hpp>
#include <common/threadpool.hpp>
#include <common/version.hpp>

#include <string>
#include <iostream>

unsigned short n=8;
unsigned short m=8;

#define LIBCHECK(cond)    \
                           do { \
                             if( !(cond) ){\
                               std::cout << "Error in line " << __LINE__ << " (" << __FILE__ << ")" << std::endl;\
                               throw 0;\
                             }\
                           } while(0)

template <typename BDDFunction, typename BDDVarOrder, typename BDD>
void checkVariant(){
  //constant functions
  {
    LIBCHECK(BDDFunction::ZERO().getValue()==0.0);
    LIBCHECK(BDDFunction::ONE().getValue()==1.0);
    LIBCHECK(BDDFunction::createConstantFunction(42.0).getValue()==42.0);
  }

  //reference counting
  {
    BDDFunction f1=BDDFunction::createConstantFunction(42.0);
    {
      BDDFunction f2=BDDFunction::createConstantFunction(42.0);
      LIBCHECK(f1.rootNode->preNodeCount==2);
    }
    LIBCHECK(f1.rootNode->preNodeCount==1);
    {
      BDDFunction f2=f1;
      LIBCHECK(f1.rootNode->preNodeCount==2);
    }
    LIBCHECK(f1.rootNode->preNodeCount==1);
  }
  
  //basic operators
  {
    BDDVarOrder::insertVariables(0,2);
    {
      BDDFunction f1=BDDFunction::projection(0,1);

      LIBCHECK(!!f1==f1);
      LIBCHECK((f1&!f1)==BDDFunction::ZERO());
      LIBCHECK((f1|!f1)==BDDFunction::ONE());
      {
        BDDFunction fTemp=f1;
        LIBCHECK((fTemp++)==f1);
        LIBCHECK((fTemp)==(f1+1.0));
      }

      BDDFunction f2=BDDFunction::projection(1,1);
      LIBCHECK(f1!=f2);
      LIBCHECK((f1^f2)==(((!f1)&f2) | (f1&(!f2))));

      BDDFunction f3=f1&f2;
      LIBCHECK(f3.cofactor(BDDFunction::projection(0,1))==f2);
      LIBCHECK(f3.cofactor(BDDFunction::projection(0,0))==0.0);
      LIBCHECK(f3.cofactor(BDDFunction::projection(1,1))==f1);
      LIBCHECK(f3.cofactor(BDDFunction::projection(1,0))==0.0);
      LIBCHECK(BDD::size()>0);
    }
    BDD::garbageCollect();
    LIBCHECK(BDD::size()==0);
    BDD::clear();
    LIBCHECK(BDDVarOrder::size()==0);
  }
}

template <typename BDDVarOrder>
void createVars(const unsigned short n){
  BDDVarOrder::insertVariables(0,n*n);
}

template <>
void createVars<TADDVarOrder>(const unsigned short n){
  TADDVarOrder::insertVariables(0,(n%2)?(n*n)/2+1:(n*n)/2);
}

template <typename BDDFunction>
Path<2> createPath(const unsigned short n){
  return Path<2>(n*n);
}

template <>
Path<2> createPath<TADDFunction>(const unsigned short n){
  if(n%2){
    Path<2> p(n*n+1);
    p[n*n]=0;
    return p;
  } else return Path<2>(n*n);
}

template <typename BDDFunction>
Path<2> createPos(const unsigned short i, const unsigned short j, const unsigned short n){
  Path<2> path=createPath<BDDFunction>(n);

  path[i*n+n-j-1]=1;

  //creating vertical moves
  for(short k=0;k<n;++k){
    if(k!=j) path[i*n+(n-k-1)]=0;
  }
  //create horizontal moves
  for(short k=0;k<n;++k){
    if(k!=i) path[k*n+(n-j-1)]=0;
  }
  //create diagonal moves (lower left to upper right)
  {
    short minStart=-(i<j?i:j);
    short minEnd=n-i<n-j?n-i:n-j;
    for(short k=minStart;k<minEnd;++k){
      if(k!=0) path[(i+k)*n+(n-j-k-1)]=0;
    }
  }
  //create diagonal moves (upper left to lower right)
  {
    short minStart=-(i<n-j-1?i:n-j-1);
    short minEnd=n-i-1<j?n-i-1:j;
    for(short k=minStart;k<=minEnd;++k){
      if(k!=0) path[(i+k)*n+(n-j+k-1)]=0;
    }
  }
  return path;
}

template <typename BDDFunction>
BDDFunction createRow(const unsigned short i, const unsigned short n){
  BDDFunction row=BDDFunction::ZERO();
  if(i>=n) return row;
  for(unsigned short j=0;j<n;++j){
    row|=BDDFunction(createPos<BDDFunction>(i,j,n));
  }
  return row;
}

template <typename BDDFunction>
boost::function<BDDFunction ()> createRowFunction(unsigned short n){
  static unsigned short i=0;
  return boost::bind(createRow<BDDFunction>,i++,n);
}

template <typename BDDFunction, typename BDDGroup, typename BDDVarOrder, typename BDD>
void testNQueensProblem(const unsigned short n, const bool messages, const bool printBoard){
  BDDFunction nqueensproblem;

  createVars<BDDVarOrder>(n);

  Clock time;

  {
    Helper::FutureContainer<BDDFunction> rows(n,boost::bind(createRowFunction<BDDFunction>,n));
    nqueensproblem=BDDFunction::ONE();
    {
      for(unsigned short i=0;i<n;++i){
        nqueensproblem&=rows[i].getValue();
        std::cout << "#" << std::flush;
      }
    }
  }
  std::cout << std::endl;
  BDD::garbageCollect();
  ReorderHelper<BDD> rH;
  Reordering<ReorderHelper<BDD> > reord(&rH);
  //reord.sifting(1.3);
  //reord.geneticMinimize(true);

  unsigned long size=BDD::size();

  //checks size calculation in combination with garbage collection
  std::cout << size << ":" << nqueensproblem.size() << std::endl;
  LIBCHECK(size==nqueensproblem.size());
  
  unsigned long count=0;
  Assignment_Iterator<typename BDDFunction::iterator> it=nqueensproblem.begin();
  while(it!=nqueensproblem.end()){
    LIBCHECK((*it).getValue()==1.0);
    ++count;
    ++it;
  }

  if(messages){
    if(printBoard){
      Assignment_Iterator<typename BDDFunction::iterator> it=nqueensproblem.begin();
      for(unsigned short i=0;i<n;++i){
        for(unsigned short j=n;j>0;--j){
          std::cout << (((*it)[i*n+j-1]==1)?"X ":"0 ");
        }
        std::cout << std::endl;
      }
      std::cout << std::endl;
    }
    
    std::cout << "BDD nodes: " << size << std::endl;
    std::cout << "Paths    : " << static_cast<unsigned long>(BDDFunction::iterator::count(nqueensproblem.begin())) << std::endl;
    std::cout << "Solutions: " << count << std::endl;
    std::cout << "Time     : " << time.stop() << std::endl;
    std::cout << std::endl;
  }

  nqueensproblem=BDDFunction::ZERO();

  BDD::clear();
  //zero drain is reachable
  LIBCHECK(BDD::size()==1);
  LIBCHECK(BDDVarOrder::size()==0);
}

template <typename BDDFunction, typename BDDVarOrder, typename BDD>
void testArithmetic(const unsigned short m, bool exactCheck=true){
  double factor=1.0;
  unsigned int i, j;
  BDDFunction testFunction=0.0;
  Clock time;
  BDDVarOrder::insertVariables(0,m);

  for(i=0;i<m;++i){
    BDDFunction temp=BDDFunction(factor)*BDDFunction::projection(i,1);
    testFunction+=temp;
    factor*=2.0;
  }
  BDDFunction tempFunction;
  for(i=0;i<static_cast<unsigned long>(1<<m);++i){
    tempFunction=testFunction;
    for(j=0;j<m;++j){
      tempFunction=tempFunction.cofactor(j,i&(1<<j));
    }
    if(exactCheck) LIBCHECK(static_cast<double>(i)==tempFunction.getValue());
    else LIBCHECK(ABS(static_cast<double>(i)-tempFunction.getValue())<=0.00001);
  }
  tempFunction=BDDFunction::ZERO();
  Clock gcTime;
  BDD::garbageCollect();
  std::cout << "BDD nodes: " << (testFunction.size()) << std::endl;
  std::cout << "BDD nodes: " << (BDD::size()) << std::endl;
  std::cout << "Time     : " << time.stop() << std::endl;
  std::cout << "Time GC  : " << gcTime.stop() << std::endl;
  testFunction=BDDFunction::ZERO();
  BDD::clear();
  std::cout << std::endl;
}

int main(int argc, char** argv){

  std::cout << "JINC " << JINC_VERSION_MAJOR << "." << JINC_VERSION_MINOR << " Testsuite (Compiled at " << __DATE__ << " " << __TIME__ << ")" << std::endl;
  std::cout << std::endl;
  
  if(argc>1) n=atoi(argv[1]);
  if(argc>2) m=atoi(argv[2]);

  std::cout << "############# ADD ##############" << std::endl;
  checkVariant<ADDFunction,ADDVarOrder,ADD>();
  std::cout << std::endl;
  
  std::cout << n << "-queens problem " << std::endl;
  std::cout << "ADD    " << std::flush;
  testNQueensProblem<ADDFunction,ADDGroup,ADDVarOrder,ADD>(n,true,true);
  
  std::cout << "############# TADD ##############" << std::endl;
  checkVariant<TADDFunction,TADDVarOrder,TADD>();
  std::cout << std::endl;
  
  std::cout << n << "-queens problem " << std::endl;
  std::cout << "TADD    " << std::flush;
  testNQueensProblem<TADDFunction,TADDGroup,TADDVarOrder,TADD>(n,true,true);


  /*
  std::cout << "Arithmetic check " << std::endl;
  std::cout << "ADD    " << std::flush;
  libasserttestArithmetic<ADDFunction,ADDVarOrder,ADD>(m,true);
  std::cout << "ADD    " << std::flush;
  libasserttestArithmetic2<ADDFunction,ADDVarOrder,ADD>(10);
  std::cout << "TADD   " << std::flush;
  libasserttestArithmetic<TADDFunction,TADDVarOrder,TADD>(m,true);
  std::cout << std::endl;
  //---------------------------------------------------------------------------------------------
  std::cout << "ADD    " << std::endl;
  ADD::BoolComputedTable.printStat("bool");
  ADD::ArithmeticComputedTable.printStat("arithmetic");
  std::cout << (ADDInnerNode::PEAK()+ADDTerminalNode::PEAK()) << std::endl;
  std::cout << std::endl;
  //std::cout << "TADD   " << std::endl;
  //TADD::BoolComputedTable.printStat("bool");
  //TADD::ArithmeticComputedTable.printStat("arithmetic");
  //std::cout << (TADDInnerNode::PEAK()+TADDTerminalNode::PEAK()) << std::endl;
  std::cout << std::endl;
  std::cout << (LIBCHECK?"Library is working fine.":"Some errors occured.") << std::endl;
  return 0;*/
}



/*

  const int N=8;
  const int M=15;

  ADDVarOrder::insertVariables(0,N*N);
    
  ADDFunction fields[M][M];
  GenericApply::ADD::Expression<GenericApply::ADD::Symbol> s_fields[M][M];
  for(int i=0;i<M;++i){
    for(int j=0;j<M;++j){
      if((i>=N) || (j>=N)){
        if(i<N) fields[i][j]=ADDFunction::ZERO();
        else fields[i][j]=ADDFunction::ONE();
      } else fields[i][j]=createPos<ADDFunction>(i,j,N);
      s_fields[i][j]=GenericApply::ADD::Expression<GenericApply::ADD::Symbol>(fields[i][j]);
    }
  }
  
  std::cout << ADD::size() << std::endl;
  Clock time;
  
 
  ADDFunction z=apply( ( s_fields[0][0]|s_fields[0][1]|s_fields[0][2]|s_fields[0][3]|s_fields[0][4]|s_fields[0][5]|s_fields[0][6]|s_fields[0][7]|s_fields[0][8]|s_fields[0][9]|s_fields[0][10]|s_fields[0][11]|s_fields[0][12]|s_fields[0][13]|s_fields[0][14] ) &
                       ( s_fields[1][0]|s_fields[1][1]|s_fields[1][2]|s_fields[1][3]|s_fields[1][4]|s_fields[1][5]|s_fields[1][6]|s_fields[1][7]|s_fields[1][8]|s_fields[1][9]|s_fields[1][10]|s_fields[1][11]|s_fields[1][12]|s_fields[1][13]|s_fields[1][14] ) &
                       ( s_fields[2][0]|s_fields[2][1]|s_fields[2][2]|s_fields[2][3]|s_fields[2][4]|s_fields[2][5]|s_fields[2][6]|s_fields[2][7]|s_fields[2][8]|s_fields[2][9]|s_fields[2][10]|s_fields[2][11]|s_fields[2][12]|s_fields[2][13]|s_fields[2][14] ) &
                       ( s_fields[3][0]|s_fields[3][1]|s_fields[3][2]|s_fields[3][3]|s_fields[3][4]|s_fields[3][5]|s_fields[3][6]|s_fields[3][7]|s_fields[3][8]|s_fields[3][9]|s_fields[3][10]|s_fields[3][11]|s_fields[3][12]|s_fields[3][13]|s_fields[3][14] ) &
                       ( s_fields[4][0]|s_fields[4][1]|s_fields[4][2]|s_fields[4][3]|s_fields[4][4]|s_fields[4][5]|s_fields[4][6]|s_fields[4][7]|s_fields[4][8]|s_fields[4][9]|s_fields[4][10]|s_fields[4][11]|s_fields[4][12]|s_fields[4][13]|s_fields[4][14] ) &
                       ( s_fields[5][0]|s_fields[5][1]|s_fields[5][2]|s_fields[5][3]|s_fields[5][4]|s_fields[5][5]|s_fields[5][6]|s_fields[5][7]|s_fields[5][8]|s_fields[5][9]|s_fields[5][10]|s_fields[5][11]|s_fields[5][12]|s_fields[5][13]|s_fields[5][14] ) &
                       ( s_fields[6][0]|s_fields[6][1]|s_fields[6][2]|s_fields[6][3]|s_fields[6][4]|s_fields[6][5]|s_fields[6][6]|s_fields[6][7]|s_fields[6][8]|s_fields[6][9]|s_fields[6][10]|s_fields[6][11]|s_fields[6][12]|s_fields[6][13]|s_fields[6][14] ) &
                       ( s_fields[7][0]|s_fields[7][1]|s_fields[7][2]|s_fields[7][3]|s_fields[7][4]|s_fields[7][5]|s_fields[7][6]|s_fields[7][7]|s_fields[7][8]|s_fields[7][9]|s_fields[7][10]|s_fields[7][11]|s_fields[7][12]|s_fields[7][13]|s_fields[7][14] ) &
                       ( s_fields[8][0]|s_fields[8][1]|s_fields[8][2]|s_fields[8][3]|s_fields[8][4]|s_fields[8][5]|s_fields[8][6]|s_fields[8][7]|s_fields[8][8]|s_fields[8][9]|s_fields[8][10]|s_fields[8][11]|s_fields[8][12]|s_fields[8][13]|s_fields[8][14] ) &
                       ( s_fields[9][0]|s_fields[9][1]|s_fields[9][2]|s_fields[9][3]|s_fields[9][4]|s_fields[9][5]|s_fields[9][6]|s_fields[9][7]|s_fields[9][8]|s_fields[9][9]|s_fields[9][10]|s_fields[9][11]|s_fields[9][12]|s_fields[9][13]|s_fields[9][14] ) &
                       ( s_fields[10][0]|s_fields[10][1]|s_fields[10][2]|s_fields[10][3]|s_fields[10][4]|s_fields[10][5]|s_fields[10][6]|s_fields[10][7]|s_fields[10][8]|s_fields[10][9]|s_fields[10][10]|s_fields[10][11]|s_fields[10][12]|s_fields[10][13]|s_fields[10][14] ) &
                       ( s_fields[11][0]|s_fields[11][1]|s_fields[11][2]|s_fields[11][3]|s_fields[11][4]|s_fields[11][5]|s_fields[11][6]|s_fields[11][7]|s_fields[11][8]|s_fields[11][9]|s_fields[11][10]|s_fields[11][11]|s_fields[11][12]|s_fields[11][13]|s_fields[11][14] ) &
                       ( s_fields[12][0]|s_fields[12][1]|s_fields[12][2]|s_fields[12][3]|s_fields[12][4]|s_fields[12][5]|s_fields[12][6]|s_fields[12][7]|s_fields[12][8]|s_fields[12][9]|s_fields[12][10]|s_fields[12][11]|s_fields[12][12]|s_fields[12][13]|s_fields[12][14] ) &
                       ( s_fields[13][0]|s_fields[13][1]|s_fields[13][2]|s_fields[13][3]|s_fields[13][4]|s_fields[13][5]|s_fields[13][6]|s_fields[13][7]|s_fields[13][8]|s_fields[13][9]|s_fields[13][10]|s_fields[13][11]|s_fields[13][12]|s_fields[13][13]|s_fields[13][14] ) &
                       ( s_fields[14][0]|s_fields[14][1]|s_fields[14][2]|s_fields[14][3]|s_fields[14][4]|s_fields[14][5]|s_fields[14][6]|s_fields[14][7]|s_fields[14][8]|s_fields[14][9]|s_fields[14][10]|s_fields[14][11]|s_fields[14][12]|s_fields[14][13]|s_fields[14][14] )
                     );
  std::cout << z.size() << std::endl;
  std::cout << "Paths    : " << static_cast<unsigned long>(ADDFunction::iterator::count(z.begin())) << std::endl;
  std::cout << time.stop() << std::endl;
  std::cout << ADD::size() << std::endl;
  ADD::garbageCollect();
  time.restart();

  ADDFunction z2=( ( fields[0][0]|fields[0][1]|fields[0][2]|fields[0][3]|fields[0][4]|fields[0][5]|fields[0][6]|fields[0][7]|fields[0][8]|fields[0][9]|fields[0][10]|fields[0][11]|fields[0][12]|fields[0][13]|fields[0][14] ) &
                       ( fields[1][0]|fields[1][1]|fields[1][2]|fields[1][3]|fields[1][4]|fields[1][5]|fields[1][6]|fields[1][7]|fields[1][8]|fields[1][9]|fields[1][10]|fields[1][11]|fields[1][12]|fields[1][13]|fields[1][14] ) &
                       ( fields[2][0]|fields[2][1]|fields[2][2]|fields[2][3]|fields[2][4]|fields[2][5]|fields[2][6]|fields[2][7]|fields[2][8]|fields[2][9]|fields[2][10]|fields[2][11]|fields[2][12]|fields[2][13]|fields[2][14] ) &
                       ( fields[3][0]|fields[3][1]|fields[3][2]|fields[3][3]|fields[3][4]|fields[3][5]|fields[3][6]|fields[3][7]|fields[3][8]|fields[3][9]|fields[3][10]|fields[3][11]|fields[3][12]|fields[3][13]|fields[3][14] ) &
                       ( fields[4][0]|fields[4][1]|fields[4][2]|fields[4][3]|fields[4][4]|fields[4][5]|fields[4][6]|fields[4][7]|fields[4][8]|fields[4][9]|fields[4][10]|fields[4][11]|fields[4][12]|fields[4][13]|fields[4][14] ) &
                       ( fields[5][0]|fields[5][1]|fields[5][2]|fields[5][3]|fields[5][4]|fields[5][5]|fields[5][6]|fields[5][7]|fields[5][8]|fields[5][9]|fields[5][10]|fields[5][11]|fields[5][12]|fields[5][13]|fields[5][14] ) &
                       ( fields[6][0]|fields[6][1]|fields[6][2]|fields[6][3]|fields[6][4]|fields[6][5]|fields[6][6]|fields[6][7]|fields[6][8]|fields[6][9]|fields[6][10]|fields[6][11]|fields[6][12]|fields[6][13]|fields[6][14] ) &
                       ( fields[7][0]|fields[7][1]|fields[7][2]|fields[7][3]|fields[7][4]|fields[7][5]|fields[7][6]|fields[7][7]|fields[7][8]|fields[7][9]|fields[7][10]|fields[7][11]|fields[7][12]|fields[7][13]|fields[7][14] ) &
                       ( fields[8][0]|fields[8][1]|fields[8][2]|fields[8][3]|fields[8][4]|fields[8][5]|fields[8][6]|fields[8][7]|fields[8][8]|fields[8][9]|fields[8][10]|fields[8][11]|fields[8][12]|fields[8][13]|fields[8][14] ) &
                       ( fields[9][0]|fields[9][1]|fields[9][2]|fields[9][3]|fields[9][4]|fields[9][5]|fields[9][6]|fields[9][7]|fields[9][8]|fields[9][9]|fields[9][10]|fields[9][11]|fields[9][12]|fields[9][13]|fields[9][14] ) &
                       ( fields[10][0]|fields[10][1]|fields[10][2]|fields[10][3]|fields[10][4]|fields[10][5]|fields[10][6]|fields[10][7]|fields[10][8]|fields[10][9]|fields[10][10]|fields[10][11]|fields[10][12]|fields[10][13]|fields[10][14] ) &
                       ( fields[11][0]|fields[11][1]|fields[11][2]|fields[11][3]|fields[11][4]|fields[11][5]|fields[11][6]|fields[11][7]|fields[11][8]|fields[11][9]|fields[11][10]|fields[11][11]|fields[11][12]|fields[11][13]|fields[11][14] ) &
                       ( fields[12][0]|fields[12][1]|fields[12][2]|fields[12][3]|fields[12][4]|fields[12][5]|fields[12][6]|fields[12][7]|fields[12][8]|fields[12][9]|fields[12][10]|fields[12][11]|fields[12][12]|fields[12][13]|fields[12][14] ) &
                       ( fields[13][0]|fields[13][1]|fields[13][2]|fields[13][3]|fields[13][4]|fields[13][5]|fields[13][6]|fields[13][7]|fields[13][8]|fields[13][9]|fields[13][10]|fields[13][11]|fields[13][12]|fields[13][13]|fields[13][14] ) &
                       ( fields[14][0]|fields[14][1]|fields[14][2]|fields[14][3]|fields[14][4]|fields[14][5]|fields[14][6]|fields[14][7]|fields[14][8]|fields[14][9]|fields[14][10]|fields[14][11]|fields[14][12]|fields[14][13]|fields[14][14] )
                     );
  
  std::cout << z2.size() << std::endl;
  std::cout << "Paths    : " << static_cast<unsigned long>(ADDFunction::iterator::count(z2.begin())) << std::endl;
  std::cout << ADD::size() << std::endl;
  std::cout << time.stop() << std::endl;
  
  return 0;
*/

