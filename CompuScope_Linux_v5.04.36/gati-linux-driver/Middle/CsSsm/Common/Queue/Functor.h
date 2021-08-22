//## begin module%1.3%.codegen_version preserve=yes
//   Read the documentation to learn more about C++ code generator
//   versioning.
//## end module%1.3%.codegen_version

//## begin module%3A941F4000F1.cm preserve=no
//	  %X% %Q% %Z% %W%
//## end module%3A941F4000F1.cm

//## begin module%3A941F4000F1.cp preserve=no
////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright:      2001/03/01; Andreas Huber <andreas@huber.net>; Zurich; Switzerland
////////////////////////////////////////////////////////////////////////////////////////////////////
// License:        This file is part of the downloadable accompanying material for the article
//                 "Elegant Function Call Wrappers" in the May 2001 issue of the C/C++ Users Journal
//                 (www.cuj.com). The material may be used, copied, distributed, modified and sold
//                 free of charge, provided that this entire notice (copyright, license and
//                 feedback) appears unaltered in all copies of the material.
//                 All material is provided "as is", without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////////////////////////
// Feedback:       Please send comments to andreas@huber.net.
////////////////////////////////////////////////////////////////////////////////////////////////////
//## end module%3A941F4000F1.cp

//## Module: Functor%3A941F4000F1; Subprogram specification
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Source file: D:\Data\Private\DeferCall\Functors\Functor.h

#ifndef Functor_h
#define Functor_h 1

//## begin module%3A941F4000F1.additionalIncludes preserve=no
//## end module%3A941F4000F1.additionalIncludes

//## begin module%3A941F4000F1.includes preserve=yes
//## end module%3A941F4000F1.includes

//## begin module%3A941F4000F1.declarations preserve=no
//## end module%3A941F4000F1.declarations

//## begin module%3A941F4000F1.additionalDeclarations preserve=yes
//## end module%3A941F4000F1.additionalDeclarations


//## begin Functor%3A941F25025B.preface preserve=yes
//## end Functor%3A941F25025B.preface

//## Class: Functor%3A941F25025B; Abstract
//	Provides the minimal interface to execute a call. Users normally only handle pointers to this
//	class.
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

class Functor 
{
  //## begin Functor%3A941F25025B.initialDeclarations preserve=yes
  //## end Functor%3A941F25025B.initialDeclarations

  public:
    //## Destructor (generated)
      virtual ~Functor()
      {
        //## begin Functor::~Functor%3A941F25025B_dest.body preserve=yes
        //## end Functor::~Functor%3A941F25025B_dest.body
      }


    //## Other Operations (specified)
      //## Operation: operator()%3A942032023A
      virtual void operator () () = 0;

    // Additional Public Declarations
      //## begin Functor%3A941F25025B.public preserve=yes
      //## end Functor%3A941F25025B.public

  protected:
    // Additional Protected Declarations
      //## begin Functor%3A941F25025B.protected preserve=yes
      //## end Functor%3A941F25025B.protected

  private:
    // Additional Private Declarations
      //## begin Functor%3A941F25025B.private preserve=yes
      //## end Functor%3A941F25025B.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin Functor%3A941F25025B.implementation preserve=yes
      //## end Functor%3A941F25025B.implementation

};

//## begin Functor%3A941F25025B.postscript preserve=yes
//## end Functor%3A941F25025B.postscript

// Class Functor 

// Additional Declarations
  //## begin Functor%3A941F25025B.declarations preserve=yes
  //## end Functor%3A941F25025B.declarations

//## begin module%3A941F4000F1.epilog preserve=yes
//## end module%3A941F4000F1.epilog


#endif
