//## begin module%1.3%.codegen_version preserve=yes
//   Read the documentation to learn more about C++ code generator
//   versioning.
//## end module%1.3%.codegen_version

//## begin module%3A9419890089.cm preserve=no
//	  %X% %Q% %Z% %W%
//## end module%3A9419890089.cm

//## begin module%3A9419890089.cp preserve=no
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
//## end module%3A9419890089.cp

//## Module: FreeFunctor%3A9419890089; Subprogram specification
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Source file: D:\Data\Private\DeferCall\Functors\FreeFunctor.h

#ifndef FreeFunctor_h
#define FreeFunctor_h 1

//## begin module%3A9419890089.additionalIncludes preserve=no
//## end module%3A9419890089.additionalIncludes

//## begin module%3A9419890089.includes preserve=yes
//## end module%3A9419890089.includes

// Functor
#include "Functor.h"
//## begin module%3A9419890089.declarations preserve=no
//## end module%3A9419890089.declarations

//## begin module%3A9419890089.additionalDeclarations preserve=yes
//## end module%3A9419890089.additionalDeclarations


//## begin FreeFunctor0%3A944B62034F.preface preserve=yes
//## end FreeFunctor0%3A944B62034F.preface

//## Class: FreeFunctor0%3A944B62034F; Parameterized Class
//	This family of class templates can store the following types of function calls for later
//	execution:
//	- calls to static member functions
//	- calls to non-member (free) functions
//	- calls to functors, i.e. instances of classes or structs that overload operator()
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename Fun>
class FreeFunctor0 : public Functor  //## Inherits: <unnamed>%3A98EB3D0311
{
  //## begin FreeFunctor0%3A944B62034F.initialDeclarations preserve=yes
  //## end FreeFunctor0%3A944B62034F.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: FreeFunctor0%3A944B620351
      FreeFunctor0 (const Fun &aFun);


    //## Other Operations (specified)
      //## Operation: operator()%3A944B620353
      virtual void operator () ();

    // Additional Public Declarations
      //## begin FreeFunctor0%3A944B62034F.public preserve=yes
      //## end FreeFunctor0%3A944B62034F.public

  protected:
    // Additional Protected Declarations
      //## begin FreeFunctor0%3A944B62034F.protected preserve=yes
      //## end FreeFunctor0%3A944B62034F.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: aFun%3A944B620354
      //## begin FreeFunctor0::aFun%3A944B620354.attr preserve=no  private: Fun {U} 
      Fun _aFun;
      //## end FreeFunctor0::aFun%3A944B620354.attr

    // Additional Private Declarations
      //## begin FreeFunctor0%3A944B62034F.private preserve=yes
      //## end FreeFunctor0%3A944B62034F.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin FreeFunctor0%3A944B62034F.implementation preserve=yes
      //## end FreeFunctor0%3A944B62034F.implementation

};

//## begin FreeFunctor0%3A944B62034F.postscript preserve=yes
//## end FreeFunctor0%3A944B62034F.postscript

//## begin FreeFunctor1%3A944BCF034C.preface preserve=yes
//## end FreeFunctor1%3A944BCF034C.preface

//## Class: FreeFunctor1%3A944BCF034C; Parameterized Class
//	This family of class templates can store the following types of function calls for later
//	execution:
//	- calls to static member functions
//	- calls to non-member (free) functions
//	- calls to functors, i.e. instances of classes or structs that overload operator()
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename Fun, typename Parm1>
class FreeFunctor1 : public Functor  //## Inherits: <unnamed>%3A98EB4B034D
{
  //## begin FreeFunctor1%3A944BCF034C.initialDeclarations preserve=yes
  //## end FreeFunctor1%3A944BCF034C.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: FreeFunctor1%3A944BCF034E
      FreeFunctor1 (const Fun &aFun, Parm1 aParm1);


    //## Other Operations (specified)
      //## Operation: operator()%3A944BCF0350
      virtual void operator () ();

    // Additional Public Declarations
      //## begin FreeFunctor1%3A944BCF034C.public preserve=yes
      //## end FreeFunctor1%3A944BCF034C.public

  protected:
    // Additional Protected Declarations
      //## begin FreeFunctor1%3A944BCF034C.protected preserve=yes
      //## end FreeFunctor1%3A944BCF034C.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: aFun%3A944BCF0351
      //## begin FreeFunctor1::aFun%3A944BCF0351.attr preserve=no  private: Fun {U} 
      Fun _aFun;
      //## end FreeFunctor1::aFun%3A944BCF0351.attr

      //## Attribute: aParm1%3A98EC610194
      //## begin FreeFunctor1::aParm1%3A98EC610194.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end FreeFunctor1::aParm1%3A98EC610194.attr

    // Additional Private Declarations
      //## begin FreeFunctor1%3A944BCF034C.private preserve=yes
      //## end FreeFunctor1%3A944BCF034C.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin FreeFunctor1%3A944BCF034C.implementation preserve=yes
      //## end FreeFunctor1%3A944BCF034C.implementation

};

//## begin FreeFunctor1%3A944BCF034C.postscript preserve=yes
//## end FreeFunctor1%3A944BCF034C.postscript

//## begin FreeFunctor2%3A94CFA5028A.preface preserve=yes
//## end FreeFunctor2%3A94CFA5028A.preface

//## Class: FreeFunctor2%3A94CFA5028A; Parameterized Class
//	This family of class templates can store the following types of function calls for later
//	execution:
//	- calls to static member functions
//	- calls to non-member (free) functions
//	- calls to functors, i.e. instances of classes or structs that overload operator()
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename Fun, typename Parm1, typename Parm2>
class FreeFunctor2 : public Functor  //## Inherits: <unnamed>%3A98EB570110
{
  //## begin FreeFunctor2%3A94CFA5028A.initialDeclarations preserve=yes
  //## end FreeFunctor2%3A94CFA5028A.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: FreeFunctor2%3A94CFA5028B
      FreeFunctor2 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2);


    //## Other Operations (specified)
      //## Operation: operator()%3A94CFA5028E
      virtual void operator () ();

    // Additional Public Declarations
      //## begin FreeFunctor2%3A94CFA5028A.public preserve=yes
      //## end FreeFunctor2%3A94CFA5028A.public

  protected:
    // Additional Protected Declarations
      //## begin FreeFunctor2%3A94CFA5028A.protected preserve=yes
      //## end FreeFunctor2%3A94CFA5028A.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: aFun%3A94CFA5028F
      //## begin FreeFunctor2::aFun%3A94CFA5028F.attr preserve=no  private: Fun {U} 
      Fun _aFun;
      //## end FreeFunctor2::aFun%3A94CFA5028F.attr

      //## Attribute: aParm1%3A98EC57015E
      //## begin FreeFunctor2::aParm1%3A98EC57015E.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end FreeFunctor2::aParm1%3A98EC57015E.attr

      //## Attribute: aParm2%3A98EC570168
      //## begin FreeFunctor2::aParm2%3A98EC570168.attr preserve=no  private: Parm2 {U} 
      Parm2 _aParm2;
      //## end FreeFunctor2::aParm2%3A98EC570168.attr

    // Additional Private Declarations
      //## begin FreeFunctor2%3A94CFA5028A.private preserve=yes
      //## end FreeFunctor2%3A94CFA5028A.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin FreeFunctor2%3A94CFA5028A.implementation preserve=yes
      //## end FreeFunctor2%3A94CFA5028A.implementation

};

//## begin FreeFunctor2%3A94CFA5028A.postscript preserve=yes
//## end FreeFunctor2%3A94CFA5028A.postscript

//## begin FreeFunctor3%3A953E850010.preface preserve=yes
//## end FreeFunctor3%3A953E850010.preface

//## Class: FreeFunctor3%3A953E850010; Parameterized Class
//	This family of class templates can store the following types of function calls for later
//	execution:
//	- calls to static member functions
//	- calls to non-member (free) functions
//	- calls to functors, i.e. instances of classes or structs that overload operator()
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename Fun, typename Parm1, typename Parm2, typename Parm3>
class FreeFunctor3 : public Functor  //## Inherits: <unnamed>%3A98EB5F0021
{
  //## begin FreeFunctor3%3A953E850010.initialDeclarations preserve=yes
  //## end FreeFunctor3%3A953E850010.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: FreeFunctor3%3A953E850011
      FreeFunctor3 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3);


    //## Other Operations (specified)
      //## Operation: operator()%3A953E850015
      virtual void operator () ();

    // Additional Public Declarations
      //## begin FreeFunctor3%3A953E850010.public preserve=yes
      //## end FreeFunctor3%3A953E850010.public

  protected:
    // Additional Protected Declarations
      //## begin FreeFunctor3%3A953E850010.protected preserve=yes
      //## end FreeFunctor3%3A953E850010.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: aFun%3A953E850016
      //## begin FreeFunctor3::aFun%3A953E850016.attr preserve=no  private: Fun {U} 
      Fun _aFun;
      //## end FreeFunctor3::aFun%3A953E850016.attr

      //## Attribute: aParm1%3A98EC450393
      //## begin FreeFunctor3::aParm1%3A98EC450393.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end FreeFunctor3::aParm1%3A98EC450393.attr

      //## Attribute: aParm2%3A98EC45039D
      //## begin FreeFunctor3::aParm2%3A98EC45039D.attr preserve=no  private: Parm2 {U} 
      Parm2 _aParm2;
      //## end FreeFunctor3::aParm2%3A98EC45039D.attr

      //## Attribute: aParm3%3A98EC4503B1
      //## begin FreeFunctor3::aParm3%3A98EC4503B1.attr preserve=no  private: Parm3 {U} 
      Parm3 _aParm3;
      //## end FreeFunctor3::aParm3%3A98EC4503B1.attr

    // Additional Private Declarations
      //## begin FreeFunctor3%3A953E850010.private preserve=yes
      //## end FreeFunctor3%3A953E850010.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin FreeFunctor3%3A953E850010.implementation preserve=yes
      //## end FreeFunctor3%3A953E850010.implementation

};

//## begin FreeFunctor3%3A953E850010.postscript preserve=yes
//## end FreeFunctor3%3A953E850010.postscript

//## begin FreeFunctor4%3A955FF5025B.preface preserve=yes
//## end FreeFunctor4%3A955FF5025B.preface

//## Class: FreeFunctor4%3A955FF5025B; Parameterized Class
//	This family of class templates can store the following types of function calls for later
//	execution:
//	- calls to static member functions
//	- calls to non-member (free) functions
//	- calls to functors, i.e. instances of classes or structs that overload operator()
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4>
class FreeFunctor4 : public Functor  //## Inherits: <unnamed>%3A98EB6A004F
{
  //## begin FreeFunctor4%3A955FF5025B.initialDeclarations preserve=yes
  //## end FreeFunctor4%3A955FF5025B.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: FreeFunctor4%3A955FF5025C
      FreeFunctor4 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4);


    //## Other Operations (specified)
      //## Operation: operator()%3A955FF50261
      virtual void operator () ();

    // Additional Public Declarations
      //## begin FreeFunctor4%3A955FF5025B.public preserve=yes
      //## end FreeFunctor4%3A955FF5025B.public

  protected:
    // Additional Protected Declarations
      //## begin FreeFunctor4%3A955FF5025B.protected preserve=yes
      //## end FreeFunctor4%3A955FF5025B.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: aFun%3A955FF50262
      //## begin FreeFunctor4::aFun%3A955FF50262.attr preserve=no  private: Fun {U} 
      Fun _aFun;
      //## end FreeFunctor4::aFun%3A955FF50262.attr

      //## Attribute: aParm1%3A98EC3C014B
      //## begin FreeFunctor4::aParm1%3A98EC3C014B.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end FreeFunctor4::aParm1%3A98EC3C014B.attr

      //## Attribute: aParm2%3A98EC3C015F
      //## begin FreeFunctor4::aParm2%3A98EC3C015F.attr preserve=no  private: Parm2 {U} 
      Parm2 _aParm2;
      //## end FreeFunctor4::aParm2%3A98EC3C015F.attr

      //## Attribute: aParm3%3A98EC3C0173
      //## begin FreeFunctor4::aParm3%3A98EC3C0173.attr preserve=no  private: Parm3 {U} 
      Parm3 _aParm3;
      //## end FreeFunctor4::aParm3%3A98EC3C0173.attr

      //## Attribute: aParm4%3A98EC3C017D
      //## begin FreeFunctor4::aParm4%3A98EC3C017D.attr preserve=no  private: Parm4 {U} 
      Parm4 _aParm4;
      //## end FreeFunctor4::aParm4%3A98EC3C017D.attr

    // Additional Private Declarations
      //## begin FreeFunctor4%3A955FF5025B.private preserve=yes
      //## end FreeFunctor4%3A955FF5025B.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin FreeFunctor4%3A955FF5025B.implementation preserve=yes
      //## end FreeFunctor4%3A955FF5025B.implementation

};

//## begin FreeFunctor4%3A955FF5025B.postscript preserve=yes
//## end FreeFunctor4%3A955FF5025B.postscript

//## begin FreeFunctor5%3A957B0D0296.preface preserve=yes
//## end FreeFunctor5%3A957B0D0296.preface

//## Class: FreeFunctor5%3A957B0D0296; Parameterized Class
//	This family of class templates can store the following types of function calls for later
//	execution:
//	- calls to static member functions
//	- calls to non-member (free) functions
//	- calls to functors, i.e. instances of classes or structs that overload operator()
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5>
class FreeFunctor5 : public Functor  //## Inherits: <unnamed>%3A98EB750267
{
  //## begin FreeFunctor5%3A957B0D0296.initialDeclarations preserve=yes
  //## end FreeFunctor5%3A957B0D0296.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: FreeFunctor5%3A957B0D0297
      FreeFunctor5 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5);


    //## Other Operations (specified)
      //## Operation: operator()%3A957B0D029D
      virtual void operator () ();

    // Additional Public Declarations
      //## begin FreeFunctor5%3A957B0D0296.public preserve=yes
      //## end FreeFunctor5%3A957B0D0296.public

  protected:
    // Additional Protected Declarations
      //## begin FreeFunctor5%3A957B0D0296.protected preserve=yes
      //## end FreeFunctor5%3A957B0D0296.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: aFun%3A957B0D029E
      //## begin FreeFunctor5::aFun%3A957B0D029E.attr preserve=no  private: Fun {U} 
      Fun _aFun;
      //## end FreeFunctor5::aFun%3A957B0D029E.attr

      //## Attribute: aParm1%3A98EC2F00FC
      //## begin FreeFunctor5::aParm1%3A98EC2F00FC.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end FreeFunctor5::aParm1%3A98EC2F00FC.attr

      //## Attribute: aParm2%3A98EC2F0106
      //## begin FreeFunctor5::aParm2%3A98EC2F0106.attr preserve=no  private: Parm2 {U} 
      Parm2 _aParm2;
      //## end FreeFunctor5::aParm2%3A98EC2F0106.attr

      //## Attribute: aParm3%3A98EC2F011A
      //## begin FreeFunctor5::aParm3%3A98EC2F011A.attr preserve=no  private: Parm3 {U} 
      Parm3 _aParm3;
      //## end FreeFunctor5::aParm3%3A98EC2F011A.attr

      //## Attribute: aParm4%3A98EC2F0124
      //## begin FreeFunctor5::aParm4%3A98EC2F0124.attr preserve=no  private: Parm4 {U} 
      Parm4 _aParm4;
      //## end FreeFunctor5::aParm4%3A98EC2F0124.attr

      //## Attribute: aParm5%3A98EC2F0138
      //## begin FreeFunctor5::aParm5%3A98EC2F0138.attr preserve=no  private: Parm5 {U} 
      Parm5 _aParm5;
      //## end FreeFunctor5::aParm5%3A98EC2F0138.attr

    // Additional Private Declarations
      //## begin FreeFunctor5%3A957B0D0296.private preserve=yes
      //## end FreeFunctor5%3A957B0D0296.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin FreeFunctor5%3A957B0D0296.implementation preserve=yes
      //## end FreeFunctor5%3A957B0D0296.implementation

};

//## begin FreeFunctor5%3A957B0D0296.postscript preserve=yes
//## end FreeFunctor5%3A957B0D0296.postscript

//## begin FreeFunctor6%3A95855A0383.preface preserve=yes
//## end FreeFunctor6%3A95855A0383.preface

//## Class: FreeFunctor6%3A95855A0383; Parameterized Class
//	This family of class templates can store the following types of function calls for later
//	execution:
//	- calls to static member functions
//	- calls to non-member (free) functions
//	- calls to functors, i.e. instances of classes or structs that overload operator()
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6>
class FreeFunctor6 : public Functor  //## Inherits: <unnamed>%3A98EB8201A8
{
  //## begin FreeFunctor6%3A95855A0383.initialDeclarations preserve=yes
  //## end FreeFunctor6%3A95855A0383.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: FreeFunctor6%3A95855A0384
      FreeFunctor6 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5, Parm6 aParm6);


    //## Other Operations (specified)
      //## Operation: operator()%3A95855A038B
      virtual void operator () ();

    // Additional Public Declarations
      //## begin FreeFunctor6%3A95855A0383.public preserve=yes
      //## end FreeFunctor6%3A95855A0383.public

  protected:
    // Additional Protected Declarations
      //## begin FreeFunctor6%3A95855A0383.protected preserve=yes
      //## end FreeFunctor6%3A95855A0383.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: aFun%3A95855A038C
      //## begin FreeFunctor6::aFun%3A95855A038C.attr preserve=no  private: Fun {U} 
      Fun _aFun;
      //## end FreeFunctor6::aFun%3A95855A038C.attr

      //## Attribute: aParm1%3A98E7DF0113
      //## begin FreeFunctor6::aParm1%3A98E7DF0113.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end FreeFunctor6::aParm1%3A98E7DF0113.attr

      //## Attribute: aParm2%3A98E7DF0131
      //## begin FreeFunctor6::aParm2%3A98E7DF0131.attr preserve=no  private: Parm2 {U} 
      Parm2 _aParm2;
      //## end FreeFunctor6::aParm2%3A98E7DF0131.attr

      //## Attribute: aParm3%3A98E7DF014F
      //## begin FreeFunctor6::aParm3%3A98E7DF014F.attr preserve=no  private: Parm3 {U} 
      Parm3 _aParm3;
      //## end FreeFunctor6::aParm3%3A98E7DF014F.attr

      //## Attribute: aParm4%3A98E7DF016D
      //## begin FreeFunctor6::aParm4%3A98E7DF016D.attr preserve=no  private: Parm4 {U} 
      Parm4 _aParm4;
      //## end FreeFunctor6::aParm4%3A98E7DF016D.attr

      //## Attribute: aParm5%3A98E7DF018B
      //## begin FreeFunctor6::aParm5%3A98E7DF018B.attr preserve=no  private: Parm5 {U} 
      Parm5 _aParm5;
      //## end FreeFunctor6::aParm5%3A98E7DF018B.attr

      //## Attribute: aParm6%3A98E7DF01A9
      //## begin FreeFunctor6::aParm6%3A98E7DF01A9.attr preserve=no  private: Parm6 {U} 
      Parm6 _aParm6;
      //## end FreeFunctor6::aParm6%3A98E7DF01A9.attr

    // Additional Private Declarations
      //## begin FreeFunctor6%3A95855A0383.private preserve=yes
      //## end FreeFunctor6%3A95855A0383.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin FreeFunctor6%3A95855A0383.implementation preserve=yes
      //## end FreeFunctor6%3A95855A0383.implementation

};

//## begin FreeFunctor6%3A95855A0383.postscript preserve=yes
//## end FreeFunctor6%3A95855A0383.postscript




template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7>
class FreeFunctor7 : public Functor
{
  public:
      FreeFunctor7 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5, Parm6 aParm6, Parm7 aParm7);

      virtual void operator () ();

    // Additional Public Declarations

  protected:
    // Additional Protected Declarations

  private:
    // Data Members for Class Attributes
      Fun _aFun;

      Parm1 _aParm1;
 
      Parm2 _aParm2;

      Parm3 _aParm3;

      Parm4 _aParm4;

      Parm5 _aParm5;
  
      Parm6 _aParm6;
   
	  Parm7 _aParm7;

    // Additional Private Declarations

  private: 
    // Additional Implementation Declarations

};


// Parameterized Class FreeFunctor0 

template <typename Fun>
inline FreeFunctor0<Fun>::FreeFunctor0 (const Fun &aFun)
  //## begin FreeFunctor0::FreeFunctor0%3A944B620351.hasinit preserve=no
  //## end FreeFunctor0::FreeFunctor0%3A944B620351.hasinit
  //## begin FreeFunctor0::FreeFunctor0%3A944B620351.initialization preserve=yes
  : _aFun( aFun )
  //## end FreeFunctor0::FreeFunctor0%3A944B620351.initialization
{
  //## begin FreeFunctor0::FreeFunctor0%3A944B620351.body preserve=yes
  //## end FreeFunctor0::FreeFunctor0%3A944B620351.body
}


// Parameterized Class FreeFunctor1 

template <typename Fun, typename Parm1>
inline FreeFunctor1<Fun,Parm1>::FreeFunctor1 (const Fun &aFun, Parm1 aParm1)
  //## begin FreeFunctor1::FreeFunctor1%3A944BCF034E.hasinit preserve=no
  //## end FreeFunctor1::FreeFunctor1%3A944BCF034E.hasinit
  //## begin FreeFunctor1::FreeFunctor1%3A944BCF034E.initialization preserve=yes
  : _aFun( aFun ),
    _aParm1( aParm1 )
  //## end FreeFunctor1::FreeFunctor1%3A944BCF034E.initialization
{
  //## begin FreeFunctor1::FreeFunctor1%3A944BCF034E.body preserve=yes
  //## end FreeFunctor1::FreeFunctor1%3A944BCF034E.body
}


// Parameterized Class FreeFunctor2 

template <typename Fun, typename Parm1, typename Parm2>
inline FreeFunctor2<Fun,Parm1,Parm2>::FreeFunctor2 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2)
  //## begin FreeFunctor2::FreeFunctor2%3A94CFA5028B.hasinit preserve=no
  //## end FreeFunctor2::FreeFunctor2%3A94CFA5028B.hasinit
  //## begin FreeFunctor2::FreeFunctor2%3A94CFA5028B.initialization preserve=yes
  : _aFun( aFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 )
  //## end FreeFunctor2::FreeFunctor2%3A94CFA5028B.initialization
{
  //## begin FreeFunctor2::FreeFunctor2%3A94CFA5028B.body preserve=yes
  //## end FreeFunctor2::FreeFunctor2%3A94CFA5028B.body
}


// Parameterized Class FreeFunctor3 

template <typename Fun, typename Parm1, typename Parm2, typename Parm3>
inline FreeFunctor3<Fun,Parm1,Parm2,Parm3>::FreeFunctor3 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3)
  //## begin FreeFunctor3::FreeFunctor3%3A953E850011.hasinit preserve=no
  //## end FreeFunctor3::FreeFunctor3%3A953E850011.hasinit
  //## begin FreeFunctor3::FreeFunctor3%3A953E850011.initialization preserve=yes
  : _aFun( aFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 ),
    _aParm3( aParm3 )
  //## end FreeFunctor3::FreeFunctor3%3A953E850011.initialization
{
  //## begin FreeFunctor3::FreeFunctor3%3A953E850011.body preserve=yes
  //## end FreeFunctor3::FreeFunctor3%3A953E850011.body
}


// Parameterized Class FreeFunctor4 

template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4>
inline FreeFunctor4<Fun,Parm1,Parm2,Parm3,Parm4>::FreeFunctor4 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4)
  //## begin FreeFunctor4::FreeFunctor4%3A955FF5025C.hasinit preserve=no
  //## end FreeFunctor4::FreeFunctor4%3A955FF5025C.hasinit
  //## begin FreeFunctor4::FreeFunctor4%3A955FF5025C.initialization preserve=yes
  : _aFun( aFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 ),
    _aParm3( aParm3 ),
    _aParm4( aParm4 )
  //## end FreeFunctor4::FreeFunctor4%3A955FF5025C.initialization
{
  //## begin FreeFunctor4::FreeFunctor4%3A955FF5025C.body preserve=yes
  //## end FreeFunctor4::FreeFunctor4%3A955FF5025C.body
}


// Parameterized Class FreeFunctor5 

template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5>
inline FreeFunctor5<Fun,Parm1,Parm2,Parm3,Parm4,Parm5>::FreeFunctor5 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5)
  //## begin FreeFunctor5::FreeFunctor5%3A957B0D0297.hasinit preserve=no
  //## end FreeFunctor5::FreeFunctor5%3A957B0D0297.hasinit
  //## begin FreeFunctor5::FreeFunctor5%3A957B0D0297.initialization preserve=yes
  : _aFun( aFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 ),
    _aParm3( aParm3 ),
    _aParm4( aParm4 ),
    _aParm5( aParm5 )
  //## end FreeFunctor5::FreeFunctor5%3A957B0D0297.initialization
{
  //## begin FreeFunctor5::FreeFunctor5%3A957B0D0297.body preserve=yes
  //## end FreeFunctor5::FreeFunctor5%3A957B0D0297.body
}


// Parameterized Class FreeFunctor6 

template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6>
inline FreeFunctor6<Fun,Parm1,Parm2,Parm3,Parm4,Parm5,Parm6>::FreeFunctor6 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5, Parm6 aParm6)
  //## begin FreeFunctor6::FreeFunctor6%3A95855A0384.hasinit preserve=no
  //## end FreeFunctor6::FreeFunctor6%3A95855A0384.hasinit
  //## begin FreeFunctor6::FreeFunctor6%3A95855A0384.initialization preserve=yes
  : _aFun( aFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 ),
    _aParm3( aParm3 ),
    _aParm4( aParm4 ),
    _aParm5( aParm5 ),
    _aParm6( aParm6 )
  //## end FreeFunctor6::FreeFunctor6%3A95855A0384.initialization
{
  //## begin FreeFunctor6::FreeFunctor6%3A95855A0384.body preserve=yes
  //## end FreeFunctor6::FreeFunctor6%3A95855A0384.body
}




template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7>
inline FreeFunctor7<Fun,Parm1,Parm2,Parm3,Parm4,Parm5,Parm6,Parm7>::FreeFunctor7 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5, Parm6 aParm6, Parm7 aParm7)
  : _aFun( aFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 ),
    _aParm3( aParm3 ),
    _aParm4( aParm4 ),
    _aParm5( aParm5 ),
    _aParm6( aParm6 ),
	_aParm7( aParm7 )
{
}


// Parameterized Class FreeFunctor0 



//## Other Operations (implementation)
template <typename Fun>
void FreeFunctor0<Fun>::operator () ()
{
  //## begin FreeFunctor0::operator()%3A944B620353.body preserve=yes
  _aFun();
  //## end FreeFunctor0::operator()%3A944B620353.body
}

// Additional Declarations
  //## begin FreeFunctor0%3A944B62034F.declarations preserve=yes
  //## end FreeFunctor0%3A944B62034F.declarations

// Parameterized Class FreeFunctor1 




//## Other Operations (implementation)
template <typename Fun, typename Parm1>
void FreeFunctor1<Fun,Parm1>::operator () ()
{
  //## begin FreeFunctor1::operator()%3A944BCF0350.body preserve=yes
  _aFun( _aParm1 );
  //## end FreeFunctor1::operator()%3A944BCF0350.body
}

// Additional Declarations
  //## begin FreeFunctor1%3A944BCF034C.declarations preserve=yes
  //## end FreeFunctor1%3A944BCF034C.declarations

// Parameterized Class FreeFunctor2 





//## Other Operations (implementation)
template <typename Fun, typename Parm1, typename Parm2>
void FreeFunctor2<Fun,Parm1,Parm2>::operator () ()
{
  //## begin FreeFunctor2::operator()%3A94CFA5028E.body preserve=yes
  _aFun( _aParm1, _aParm2 );
  //## end FreeFunctor2::operator()%3A94CFA5028E.body
}

// Additional Declarations
  //## begin FreeFunctor2%3A94CFA5028A.declarations preserve=yes
  //## end FreeFunctor2%3A94CFA5028A.declarations

// Parameterized Class FreeFunctor3 






//## Other Operations (implementation)
template <typename Fun, typename Parm1, typename Parm2, typename Parm3>
void FreeFunctor3<Fun,Parm1,Parm2,Parm3>::operator () ()
{
  //## begin FreeFunctor3::operator()%3A953E850015.body preserve=yes
  _aFun( _aParm1, _aParm2, _aParm3 );
  //## end FreeFunctor3::operator()%3A953E850015.body
}

// Additional Declarations
  //## begin FreeFunctor3%3A953E850010.declarations preserve=yes
  //## end FreeFunctor3%3A953E850010.declarations

// Parameterized Class FreeFunctor4 







//## Other Operations (implementation)
template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4>
void FreeFunctor4<Fun,Parm1,Parm2,Parm3,Parm4>::operator () ()
{
  //## begin FreeFunctor4::operator()%3A955FF50261.body preserve=yes
  _aFun( _aParm1, _aParm2, _aParm3, _aParm4 );
  //## end FreeFunctor4::operator()%3A955FF50261.body
}

// Additional Declarations
  //## begin FreeFunctor4%3A955FF5025B.declarations preserve=yes
  //## end FreeFunctor4%3A955FF5025B.declarations

// Parameterized Class FreeFunctor5 








//## Other Operations (implementation)
template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5>
void FreeFunctor5<Fun,Parm1,Parm2,Parm3,Parm4,Parm5>::operator () ()
{
  //## begin FreeFunctor5::operator()%3A957B0D029D.body preserve=yes
  _aFun( _aParm1, _aParm2, _aParm3, _aParm4, _aParm5 );
  //## end FreeFunctor5::operator()%3A957B0D029D.body
}

// Additional Declarations
  //## begin FreeFunctor5%3A957B0D0296.declarations preserve=yes
  //## end FreeFunctor5%3A957B0D0296.declarations

// Parameterized Class FreeFunctor6 









//## Other Operations (implementation)
template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6>
void FreeFunctor6<Fun,Parm1,Parm2,Parm3,Parm4,Parm5,Parm6>::operator () ()
{
  //## begin FreeFunctor6::operator()%3A95855A038B.body preserve=yes
  _aFun( _aParm1, _aParm2, _aParm3, _aParm4, _aParm5, _aParm6 );
  //## end FreeFunctor6::operator()%3A95855A038B.body
}

template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7>
void FreeFunctor7<Fun,Parm1,Parm2,Parm3,Parm4,Parm5,Parm6,Parm7>::operator () ()
{
  _aFun( _aParm1, _aParm2, _aParm3, _aParm4, _aParm5, _aParm6, _aParm7 );
}


/*
template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7, typename Parm8>
class FreeFunctor8 : public Functor
{
  public:
      FreeFunctor8 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5, Parm6 aParm6, Parm7 aParm7, Parm8 aParm8);
      virtual void operator () ();

  protected:
 
  private:
      Fun _aFun;
      Parm1 _aParm1;
      Parm2 _aParm2;
      Parm3 _aParm3;
      Parm4 _aParm4;
      Parm5 _aParm5;
      Parm6 _aParm6;
	  Parm7 _aParm7;
	  Parm8 _aParm8;

  private:

};


template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7, typename Parm8>
inline FreeFunctor8<Fun,Parm1,Parm2,Parm3,Parm4,Parm5,Parm6,Parm7,Parm8>::FreeFunctor8 (const Fun &aFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5, Parm6 aParm6, Parm7 aParm7, Parm8 aParm8)
  : _aFun( aFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 ),
    _aParm3( aParm3 ),
    _aParm4( aParm4 ),
    _aParm5( aParm5 ),
    _aParm6( aParm6 ),
	_aParm7( aParm7 ),
	_aParm8( aParm8 )
{
}


template <typename Fun, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7, typename Parm8>
void FreeFunctor8<Fun,Parm1,Parm2,Parm3,Parm4,Parm5,Parm6,Parm7,Parm8>::operator () ()
{
  _aFun( _aParm1, _aParm2, _aParm3, _aParm4, _aParm5, _aParm6, _aParm7, _aParm8 );
}
*/
// Additional Declarations
  //## begin FreeFunctor6%3A95855A0383.declarations preserve=yes
  //## end FreeFunctor6%3A95855A0383.declarations
//## begin module%3A9419890089.epilog preserve=yes
//## end module%3A9419890089.epilog


#endif
