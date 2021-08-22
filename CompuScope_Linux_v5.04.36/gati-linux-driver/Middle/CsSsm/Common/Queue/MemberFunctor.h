//## begin module%1.3%.codegen_version preserve=yes
//   Read the documentation to learn more about C++ code generator
//   versioning.
//## end module%1.3%.codegen_version

//## begin module%3A94196B03C5.cm preserve=no
//	  %X% %Q% %Z% %W%
//## end module%3A94196B03C5.cm

//## begin module%3A94196B03C5.cp preserve=no
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
//## end module%3A94196B03C5.cp

//## Module: MemberFunctor%3A94196B03C5; Subprogram specification
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Source file: D:\Data\Private\DeferCall\Functors\MemberFunctor.h

#ifndef MemberFunctor_h
#define MemberFunctor_h 1

//## begin module%3A94196B03C5.additionalIncludes preserve=no
//## end module%3A94196B03C5.additionalIncludes

//## begin module%3A94196B03C5.includes preserve=yes
//## end module%3A94196B03C5.includes

// Functor
#include "Functor.h"
//## begin module%3A94196B03C5.declarations preserve=no
//## end module%3A94196B03C5.declarations

//## begin module%3A94196B03C5.additionalDeclarations preserve=yes
//## end module%3A94196B03C5.additionalDeclarations


//## begin MemberFunctor0%3A9417400150.preface preserve=yes
//## end MemberFunctor0%3A9417400150.preface

//## Class: MemberFunctor0%3A9417400150; Parameterized Class
//	This family of class templates can store calls to non-static member functions for later
//	execution.
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename CalleePtr, typename MemFunPtr>
class MemberFunctor0 : public Functor  //## Inherits: <unnamed>%3A98EB4601B5
{
  //## begin MemberFunctor0%3A9417400150.initialDeclarations preserve=yes
  //## end MemberFunctor0%3A9417400150.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: MemberFunctor0%3A942CE800E4
      MemberFunctor0 (const CalleePtr &pCallee, const MemFunPtr &pMemFun);


    //## Other Operations (specified)
      //## Operation: operator()%3A94373201A9
      virtual void operator () ();

    // Additional Public Declarations
      //## begin MemberFunctor0%3A9417400150.public preserve=yes
      //## end MemberFunctor0%3A9417400150.public

  protected:
    // Additional Protected Declarations
      //## begin MemberFunctor0%3A9417400150.protected preserve=yes
      //## end MemberFunctor0%3A9417400150.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: pCallee%3A942AAB018A
      //## begin MemberFunctor0::pCallee%3A942AAB018A.attr preserve=no  private: CalleePtr {U} 
      CalleePtr _pCallee;
      //## end MemberFunctor0::pCallee%3A942AAB018A.attr

      //## Attribute: pMemFun%3A942AB50012
      //## begin MemberFunctor0::pMemFun%3A942AB50012.attr preserve=no  private: MemFunPtr {U} 
      MemFunPtr _pMemFun;
      //## end MemberFunctor0::pMemFun%3A942AB50012.attr

    // Additional Private Declarations
      //## begin MemberFunctor0%3A9417400150.private preserve=yes
      //## end MemberFunctor0%3A9417400150.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin MemberFunctor0%3A9417400150.implementation preserve=yes
      //## end MemberFunctor0%3A9417400150.implementation

};

//## begin MemberFunctor0%3A9417400150.postscript preserve=yes
//## end MemberFunctor0%3A9417400150.postscript

//## begin MemberFunctor1%3A94477F0240.preface preserve=yes
//## end MemberFunctor1%3A94477F0240.preface

//## Class: MemberFunctor1%3A94477F0240; Parameterized Class
//	This family of class templates can store calls to non-static member functions for later
//	execution.
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename CalleePtr, typename MemFunPtr, typename Parm1>
class MemberFunctor1 : public Functor  //## Inherits: <unnamed>%3A98EB500200
{
  //## begin MemberFunctor1%3A94477F0240.initialDeclarations preserve=yes
  //## end MemberFunctor1%3A94477F0240.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: MemberFunctor1%3A94477F0242
      MemberFunctor1 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1);


    //## Other Operations (specified)
      //## Operation: operator()%3A94477F0245
      virtual void operator () ();

    // Additional Public Declarations
      //## begin MemberFunctor1%3A94477F0240.public preserve=yes
      //## end MemberFunctor1%3A94477F0240.public

  protected:
    // Additional Protected Declarations
      //## begin MemberFunctor1%3A94477F0240.protected preserve=yes
      //## end MemberFunctor1%3A94477F0240.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: pCallee%3A94477F0246
      //## begin MemberFunctor1::pCallee%3A94477F0246.attr preserve=no  private: CalleePtr {U} 
      CalleePtr _pCallee;
      //## end MemberFunctor1::pCallee%3A94477F0246.attr

      //## Attribute: pMemFun%3A94477F0247
      //## begin MemberFunctor1::pMemFun%3A94477F0247.attr preserve=no  private: MemFunPtr {U} 
      MemFunPtr _pMemFun;
      //## end MemberFunctor1::pMemFun%3A94477F0247.attr

      //## Attribute: aParm1%3A98EC080313
      //## begin MemberFunctor1::aParm1%3A98EC080313.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end MemberFunctor1::aParm1%3A98EC080313.attr

    // Additional Private Declarations
      //## begin MemberFunctor1%3A94477F0240.private preserve=yes
      //## end MemberFunctor1%3A94477F0240.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin MemberFunctor1%3A94477F0240.implementation preserve=yes
      //## end MemberFunctor1%3A94477F0240.implementation

};

//## begin MemberFunctor1%3A94477F0240.postscript preserve=yes
//## end MemberFunctor1%3A94477F0240.postscript

//## begin MemberFunctor2%3A94CFA50294.preface preserve=yes
//## end MemberFunctor2%3A94CFA50294.preface

//## Class: MemberFunctor2%3A94CFA50294; Parameterized Class
//	This family of class templates can store calls to non-static member functions for later
//	execution.
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2>
class MemberFunctor2 : public Functor  //## Inherits: <unnamed>%3A98EB5B002F
{
  //## begin MemberFunctor2%3A94CFA50294.initialDeclarations preserve=yes
  //## end MemberFunctor2%3A94CFA50294.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: MemberFunctor2%3A94CFA50295
      MemberFunctor2 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1, Parm2 aParm2);


    //## Other Operations (specified)
      //## Operation: operator()%3A94CFA50299
      virtual void operator () ();

    // Additional Public Declarations
      //## begin MemberFunctor2%3A94CFA50294.public preserve=yes
      //## end MemberFunctor2%3A94CFA50294.public

  protected:
    // Additional Protected Declarations
      //## begin MemberFunctor2%3A94CFA50294.protected preserve=yes
      //## end MemberFunctor2%3A94CFA50294.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: pCallee%3A94CFA5029A
      //## begin MemberFunctor2::pCallee%3A94CFA5029A.attr preserve=no  private: CalleePtr {U} 
      CalleePtr _pCallee;
      //## end MemberFunctor2::pCallee%3A94CFA5029A.attr

      //## Attribute: pMemFun%3A94CFA5029B
      //## begin MemberFunctor2::pMemFun%3A94CFA5029B.attr preserve=no  private: MemFunPtr {U} 
      MemFunPtr _pMemFun;
      //## end MemberFunctor2::pMemFun%3A94CFA5029B.attr

      //## Attribute: aParm1%3A98EBF80356
      //## begin MemberFunctor2::aParm1%3A98EBF80356.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end MemberFunctor2::aParm1%3A98EBF80356.attr

      //## Attribute: aParm2%3A98EBF8036A
      //## begin MemberFunctor2::aParm2%3A98EBF8036A.attr preserve=no  private: Parm2 {U} 
      Parm2 _aParm2;
      //## end MemberFunctor2::aParm2%3A98EBF8036A.attr

    // Additional Private Declarations
      //## begin MemberFunctor2%3A94CFA50294.private preserve=yes
      //## end MemberFunctor2%3A94CFA50294.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin MemberFunctor2%3A94CFA50294.implementation preserve=yes
      //## end MemberFunctor2%3A94CFA50294.implementation

};

//## begin MemberFunctor2%3A94CFA50294.postscript preserve=yes
//## end MemberFunctor2%3A94CFA50294.postscript

//## begin MemberFunctor3%3A953E85001D.preface preserve=yes
//## end MemberFunctor3%3A953E85001D.preface

//## Class: MemberFunctor3%3A953E85001D; Parameterized Class
//	This family of class templates can store calls to non-static member functions for later
//	execution.
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3>
class MemberFunctor3 : public Functor  //## Inherits: <unnamed>%3A98EB650369
{
  //## begin MemberFunctor3%3A953E85001D.initialDeclarations preserve=yes
  //## end MemberFunctor3%3A953E85001D.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: MemberFunctor3%3A953E85001E
      MemberFunctor3 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3);


    //## Other Operations (specified)
      //## Operation: operator()%3A953E850023
      virtual void operator () ();

    // Additional Public Declarations
      //## begin MemberFunctor3%3A953E85001D.public preserve=yes
      //## end MemberFunctor3%3A953E85001D.public

  protected:
    // Additional Protected Declarations
      //## begin MemberFunctor3%3A953E85001D.protected preserve=yes
      //## end MemberFunctor3%3A953E85001D.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: pCallee%3A953E850024
      //## begin MemberFunctor3::pCallee%3A953E850024.attr preserve=no  private: CalleePtr {U} 
      CalleePtr _pCallee;
      //## end MemberFunctor3::pCallee%3A953E850024.attr

      //## Attribute: pMemFun%3A953E850025
      //## begin MemberFunctor3::pMemFun%3A953E850025.attr preserve=no  private: MemFunPtr {U} 
      MemFunPtr _pMemFun;
      //## end MemberFunctor3::pMemFun%3A953E850025.attr

      //## Attribute: aParm1%3A98EBEF0064
      //## begin MemberFunctor3::aParm1%3A98EBEF0064.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end MemberFunctor3::aParm1%3A98EBEF0064.attr

      //## Attribute: aParm2%3A98EBEF0078
      //## begin MemberFunctor3::aParm2%3A98EBEF0078.attr preserve=no  private: Parm2 {U} 
      Parm2 _aParm2;
      //## end MemberFunctor3::aParm2%3A98EBEF0078.attr

      //## Attribute: aParm3%3A98EBEF008C
      //## begin MemberFunctor3::aParm3%3A98EBEF008C.attr preserve=no  private: Parm3 {U} 
      Parm3 _aParm3;
      //## end MemberFunctor3::aParm3%3A98EBEF008C.attr

    // Additional Private Declarations
      //## begin MemberFunctor3%3A953E85001D.private preserve=yes
      //## end MemberFunctor3%3A953E85001D.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin MemberFunctor3%3A953E85001D.implementation preserve=yes
      //## end MemberFunctor3%3A953E85001D.implementation

};

//## begin MemberFunctor3%3A953E85001D.postscript preserve=yes
//## end MemberFunctor3%3A953E85001D.postscript

//## begin MemberFunctor4%3A955FF5026A.preface preserve=yes
//## end MemberFunctor4%3A955FF5026A.preface

//## Class: MemberFunctor4%3A955FF5026A; Parameterized Class
//	This family of class templates can store calls to non-static member functions for later
//	execution.
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3, typename Parm4>
class MemberFunctor4 : public Functor  //## Inherits: <unnamed>%3A98EB710009
{
  //## begin MemberFunctor4%3A955FF5026A.initialDeclarations preserve=yes
  //## end MemberFunctor4%3A955FF5026A.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: MemberFunctor4%3A955FF5026B
      MemberFunctor4 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4);


    //## Other Operations (specified)
      //## Operation: operator()%3A955FF50271
      virtual void operator () ();

    // Additional Public Declarations
      //## begin MemberFunctor4%3A955FF5026A.public preserve=yes
      //## end MemberFunctor4%3A955FF5026A.public

  protected:
    // Additional Protected Declarations
      //## begin MemberFunctor4%3A955FF5026A.protected preserve=yes
      //## end MemberFunctor4%3A955FF5026A.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: pCallee%3A955FF50272
      //## begin MemberFunctor4::pCallee%3A955FF50272.attr preserve=no  private: CalleePtr {U} 
      CalleePtr _pCallee;
      //## end MemberFunctor4::pCallee%3A955FF50272.attr

      //## Attribute: pMemFun%3A955FF50273
      //## begin MemberFunctor4::pMemFun%3A955FF50273.attr preserve=no  private: MemFunPtr {U} 
      MemFunPtr _pMemFun;
      //## end MemberFunctor4::pMemFun%3A955FF50273.attr

      //## Attribute: aParm1%3A98EBD302D1
      //## begin MemberFunctor4::aParm1%3A98EBD302D1.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end MemberFunctor4::aParm1%3A98EBD302D1.attr

      //## Attribute: aParm2%3A98EBD302E5
      //## begin MemberFunctor4::aParm2%3A98EBD302E5.attr preserve=no  private: Parm2 {U} 
      Parm2 _aParm2;
      //## end MemberFunctor4::aParm2%3A98EBD302E5.attr

      //## Attribute: aParm3%3A98EBD302EF
      //## begin MemberFunctor4::aParm3%3A98EBD302EF.attr preserve=no  private: Parm3 {U} 
      Parm3 _aParm3;
      //## end MemberFunctor4::aParm3%3A98EBD302EF.attr

      //## Attribute: aParm4%3A98EBD30303
      //## begin MemberFunctor4::aParm4%3A98EBD30303.attr preserve=no  private: Parm4 {U} 
      Parm4 _aParm4;
      //## end MemberFunctor4::aParm4%3A98EBD30303.attr

    // Additional Private Declarations
      //## begin MemberFunctor4%3A955FF5026A.private preserve=yes
      //## end MemberFunctor4%3A955FF5026A.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin MemberFunctor4%3A955FF5026A.implementation preserve=yes
      //## end MemberFunctor4%3A955FF5026A.implementation

};

//## begin MemberFunctor4%3A955FF5026A.postscript preserve=yes
//## end MemberFunctor4%3A955FF5026A.postscript

//## begin MemberFunctor5%3A957B0D02A7.preface preserve=yes
//## end MemberFunctor5%3A957B0D02A7.preface

//## Class: MemberFunctor5%3A957B0D02A7; Parameterized Class
//	This family of class templates can store calls to non-static member functions for later
//	execution.
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5>
class MemberFunctor5 : public Functor  //## Inherits: <unnamed>%3A98EB79028B
{
  //## begin MemberFunctor5%3A957B0D02A7.initialDeclarations preserve=yes
  //## end MemberFunctor5%3A957B0D02A7.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: MemberFunctor5%3A957B0D02A8
      MemberFunctor5 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5);


    //## Other Operations (specified)
      //## Operation: operator()%3A957B0D02AF
      virtual void operator () ();

    // Additional Public Declarations
      //## begin MemberFunctor5%3A957B0D02A7.public preserve=yes
      //## end MemberFunctor5%3A957B0D02A7.public

  protected:
    // Additional Protected Declarations
      //## begin MemberFunctor5%3A957B0D02A7.protected preserve=yes
      //## end MemberFunctor5%3A957B0D02A7.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: pCallee%3A957B0D02B0
      //## begin MemberFunctor5::pCallee%3A957B0D02B0.attr preserve=no  private: CalleePtr {U} 
      CalleePtr _pCallee;
      //## end MemberFunctor5::pCallee%3A957B0D02B0.attr

      //## Attribute: pMemFun%3A957B0D02B1
      //## begin MemberFunctor5::pMemFun%3A957B0D02B1.attr preserve=no  private: MemFunPtr {U} 
      MemFunPtr _pMemFun;
      //## end MemberFunctor5::pMemFun%3A957B0D02B1.attr

      //## Attribute: aParm1%3A98EBC20182
      //## begin MemberFunctor5::aParm1%3A98EBC20182.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end MemberFunctor5::aParm1%3A98EBC20182.attr

      //## Attribute: aParm2%3A98EBC20196
      //## begin MemberFunctor5::aParm2%3A98EBC20196.attr preserve=no  private: Parm2 {U} 
      Parm2 _aParm2;
      //## end MemberFunctor5::aParm2%3A98EBC20196.attr

      //## Attribute: aParm3%3A98EBC201AA
      //## begin MemberFunctor5::aParm3%3A98EBC201AA.attr preserve=no  private: Parm3 {U} 
      Parm3 _aParm3;
      //## end MemberFunctor5::aParm3%3A98EBC201AA.attr

      //## Attribute: aParm4%3A98EBC201B4
      //## begin MemberFunctor5::aParm4%3A98EBC201B4.attr preserve=no  private: Parm4 {U} 
      Parm4 _aParm4;
      //## end MemberFunctor5::aParm4%3A98EBC201B4.attr

      //## Attribute: aParm5%3A98EBC201C8
      //## begin MemberFunctor5::aParm5%3A98EBC201C8.attr preserve=no  private: Parm5 {U} 
      Parm5 _aParm5;
      //## end MemberFunctor5::aParm5%3A98EBC201C8.attr

    // Additional Private Declarations
      //## begin MemberFunctor5%3A957B0D02A7.private preserve=yes
      //## end MemberFunctor5%3A957B0D02A7.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin MemberFunctor5%3A957B0D02A7.implementation preserve=yes
      //## end MemberFunctor5%3A957B0D02A7.implementation

};

//## begin MemberFunctor5%3A957B0D02A7.postscript preserve=yes
//## end MemberFunctor5%3A957B0D02A7.postscript

//## begin MemberFunctor6%3A95855A0396.preface preserve=yes
//## end MemberFunctor6%3A95855A0396.preface

//## Class: MemberFunctor6%3A95855A0396; Parameterized Class
//	This family of class templates can store calls to non-static member functions for later
//	execution.
//## Category: DeferCall::Functors%3A9416E203AE
//## Subsystem: DeferCall::Functors%3A9416F20253
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6>
class MemberFunctor6 : public Functor  //## Inherits: <unnamed>%3A98EB87000A
{
  //## begin MemberFunctor6%3A95855A0396.initialDeclarations preserve=yes
  //## end MemberFunctor6%3A95855A0396.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: MemberFunctor6%3A95855A0397
      MemberFunctor6 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5, Parm6 aParm6);


    //## Other Operations (specified)
      //## Operation: operator()%3A95855A039F
      virtual void operator () ();

    // Additional Public Declarations
      //## begin MemberFunctor6%3A95855A0396.public preserve=yes
      //## end MemberFunctor6%3A95855A0396.public

  protected:
    // Additional Protected Declarations
      //## begin MemberFunctor6%3A95855A0396.protected preserve=yes
      //## end MemberFunctor6%3A95855A0396.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: pCallee%3A95855A03A0
      //## begin MemberFunctor6::pCallee%3A95855A03A0.attr preserve=no  private: CalleePtr {U} 
      CalleePtr _pCallee;
      //## end MemberFunctor6::pCallee%3A95855A03A0.attr

      //## Attribute: pMemFun%3A95855A03A1
      //## begin MemberFunctor6::pMemFun%3A95855A03A1.attr preserve=no  private: MemFunPtr {U} 
      MemFunPtr _pMemFun;
      //## end MemberFunctor6::pMemFun%3A95855A03A1.attr

      //## Attribute: aParm1%3A98E77602E9
      //## begin MemberFunctor6::aParm1%3A98E77602E9.attr preserve=no  private: Parm1 {U} 
      Parm1 _aParm1;
      //## end MemberFunctor6::aParm1%3A98E77602E9.attr

      //## Attribute: aParm2%3A98E7760307
      //## begin MemberFunctor6::aParm2%3A98E7760307.attr preserve=no  private: Parm2 {U} 
      Parm2 _aParm2;
      //## end MemberFunctor6::aParm2%3A98E7760307.attr

      //## Attribute: aParm3%3A98E7760325
      //## begin MemberFunctor6::aParm3%3A98E7760325.attr preserve=no  private: Parm3 {U} 
      Parm3 _aParm3;
      //## end MemberFunctor6::aParm3%3A98E7760325.attr

      //## Attribute: aParm4%3A98E7760343
      //## begin MemberFunctor6::aParm4%3A98E7760343.attr preserve=no  private: Parm4 {U} 
      Parm4 _aParm4;
      //## end MemberFunctor6::aParm4%3A98E7760343.attr

      //## Attribute: aParm5%3A98E7760361
      //## begin MemberFunctor6::aParm5%3A98E7760361.attr preserve=no  private: Parm5 {U} 
      Parm5 _aParm5;
      //## end MemberFunctor6::aParm5%3A98E7760361.attr

      //## Attribute: aParm6%3A98E776037F
      //## begin MemberFunctor6::aParm6%3A98E776037F.attr preserve=no  private: Parm6 {U} 
      Parm6 _aParm6;
      //## end MemberFunctor6::aParm6%3A98E776037F.attr

    // Additional Private Declarations
      //## begin MemberFunctor6%3A95855A0396.private preserve=yes
      //## end MemberFunctor6%3A95855A0396.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin MemberFunctor6%3A95855A0396.implementation preserve=yes
      //## end MemberFunctor6%3A95855A0396.implementation

};

//## begin MemberFunctor6%3A95855A0396.postscript preserve=yes
//## end MemberFunctor6%3A95855A0396.postscript

// Parameterized Class MemberFunctor0 

template <typename CalleePtr, typename MemFunPtr>
inline MemberFunctor0<CalleePtr,MemFunPtr>::MemberFunctor0 (const CalleePtr &pCallee, const MemFunPtr &pMemFun)
  //## begin MemberFunctor0::MemberFunctor0%3A942CE800E4.hasinit preserve=no
  //## end MemberFunctor0::MemberFunctor0%3A942CE800E4.hasinit
  //## begin MemberFunctor0::MemberFunctor0%3A942CE800E4.initialization preserve=yes
  : _pCallee( pCallee ),
    _pMemFun( pMemFun )
  //## end MemberFunctor0::MemberFunctor0%3A942CE800E4.initialization
{
  //## begin MemberFunctor0::MemberFunctor0%3A942CE800E4.body preserve=yes
  //## end MemberFunctor0::MemberFunctor0%3A942CE800E4.body
}


// Parameterized Class MemberFunctor1 

template <typename CalleePtr, typename MemFunPtr, typename Parm1>
inline MemberFunctor1<CalleePtr,MemFunPtr,Parm1>::MemberFunctor1 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1)
  //## begin MemberFunctor1::MemberFunctor1%3A94477F0242.hasinit preserve=no
  //## end MemberFunctor1::MemberFunctor1%3A94477F0242.hasinit
  //## begin MemberFunctor1::MemberFunctor1%3A94477F0242.initialization preserve=yes
  : _pCallee( pCallee ),
    _pMemFun( pMemFun ),
    _aParm1( aParm1 )
  //## end MemberFunctor1::MemberFunctor1%3A94477F0242.initialization
{
  //## begin MemberFunctor1::MemberFunctor1%3A94477F0242.body preserve=yes
  //## end MemberFunctor1::MemberFunctor1%3A94477F0242.body
}


// Parameterized Class MemberFunctor2 

template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2>
inline MemberFunctor2<CalleePtr,MemFunPtr,Parm1,Parm2>::MemberFunctor2 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1, Parm2 aParm2)
  //## begin MemberFunctor2::MemberFunctor2%3A94CFA50295.hasinit preserve=no
  //## end MemberFunctor2::MemberFunctor2%3A94CFA50295.hasinit
  //## begin MemberFunctor2::MemberFunctor2%3A94CFA50295.initialization preserve=yes
  : _pCallee( pCallee ),
    _pMemFun( pMemFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 )
  //## end MemberFunctor2::MemberFunctor2%3A94CFA50295.initialization
{
  //## begin MemberFunctor2::MemberFunctor2%3A94CFA50295.body preserve=yes
  //## end MemberFunctor2::MemberFunctor2%3A94CFA50295.body
}


// Parameterized Class MemberFunctor3 

template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3>
inline MemberFunctor3<CalleePtr,MemFunPtr,Parm1,Parm2,Parm3>::MemberFunctor3 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3)
  //## begin MemberFunctor3::MemberFunctor3%3A953E85001E.hasinit preserve=no
  //## end MemberFunctor3::MemberFunctor3%3A953E85001E.hasinit
  //## begin MemberFunctor3::MemberFunctor3%3A953E85001E.initialization preserve=yes
  : _pCallee( pCallee ),
    _pMemFun( pMemFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 ),
    _aParm3( aParm3 )
  //## end MemberFunctor3::MemberFunctor3%3A953E85001E.initialization
{
  //## begin MemberFunctor3::MemberFunctor3%3A953E85001E.body preserve=yes
  //## end MemberFunctor3::MemberFunctor3%3A953E85001E.body
}


// Parameterized Class MemberFunctor4 

template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3, typename Parm4>
inline MemberFunctor4<CalleePtr,MemFunPtr,Parm1,Parm2,Parm3,Parm4>::MemberFunctor4 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4)
  //## begin MemberFunctor4::MemberFunctor4%3A955FF5026B.hasinit preserve=no
  //## end MemberFunctor4::MemberFunctor4%3A955FF5026B.hasinit
  //## begin MemberFunctor4::MemberFunctor4%3A955FF5026B.initialization preserve=yes
  : _pCallee( pCallee ),
    _pMemFun( pMemFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 ),
    _aParm3( aParm3 ),
    _aParm4( aParm4 )
  //## end MemberFunctor4::MemberFunctor4%3A955FF5026B.initialization
{
  //## begin MemberFunctor4::MemberFunctor4%3A955FF5026B.body preserve=yes
  //## end MemberFunctor4::MemberFunctor4%3A955FF5026B.body
}


// Parameterized Class MemberFunctor5 

template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5>
inline MemberFunctor5<CalleePtr,MemFunPtr,Parm1,Parm2,Parm3,Parm4,Parm5>::MemberFunctor5 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5)
  //## begin MemberFunctor5::MemberFunctor5%3A957B0D02A8.hasinit preserve=no
  //## end MemberFunctor5::MemberFunctor5%3A957B0D02A8.hasinit
  //## begin MemberFunctor5::MemberFunctor5%3A957B0D02A8.initialization preserve=yes
  : _pCallee( pCallee ),
    _pMemFun( pMemFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 ),
    _aParm3( aParm3 ),
    _aParm4( aParm4 ),
    _aParm5( aParm5 )
  //## end MemberFunctor5::MemberFunctor5%3A957B0D02A8.initialization
{
  //## begin MemberFunctor5::MemberFunctor5%3A957B0D02A8.body preserve=yes
  //## end MemberFunctor5::MemberFunctor5%3A957B0D02A8.body
}


// Parameterized Class MemberFunctor6 

template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6>
inline MemberFunctor6<CalleePtr,MemFunPtr,Parm1,Parm2,Parm3,Parm4,Parm5,Parm6>::MemberFunctor6 (const CalleePtr &pCallee, const MemFunPtr &pMemFun, Parm1 aParm1, Parm2 aParm2, Parm3 aParm3, Parm4 aParm4, Parm5 aParm5, Parm6 aParm6)
  //## begin MemberFunctor6::MemberFunctor6%3A95855A0397.hasinit preserve=no
  //## end MemberFunctor6::MemberFunctor6%3A95855A0397.hasinit
  //## begin MemberFunctor6::MemberFunctor6%3A95855A0397.initialization preserve=yes
  : _pCallee( pCallee ),
    _pMemFun( pMemFun ),
    _aParm1( aParm1 ),
    _aParm2( aParm2 ),
    _aParm3( aParm3 ),
    _aParm4( aParm4 ),
    _aParm5( aParm5 ),
    _aParm6( aParm6 )
  //## end MemberFunctor6::MemberFunctor6%3A95855A0397.initialization
{
  //## begin MemberFunctor6::MemberFunctor6%3A95855A0397.body preserve=yes
  //## end MemberFunctor6::MemberFunctor6%3A95855A0397.body
}


// Parameterized Class MemberFunctor0 




//## Other Operations (implementation)
template <typename CalleePtr, typename MemFunPtr>
void MemberFunctor0<CalleePtr,MemFunPtr>::operator () ()
{
  //## begin MemberFunctor0::operator()%3A94373201A9.body preserve=yes
  ( ( *_pCallee ).*_pMemFun )();
  //## end MemberFunctor0::operator()%3A94373201A9.body
}

// Additional Declarations
  //## begin MemberFunctor0%3A9417400150.declarations preserve=yes
  //## end MemberFunctor0%3A9417400150.declarations

// Parameterized Class MemberFunctor1 





//## Other Operations (implementation)
template <typename CalleePtr, typename MemFunPtr, typename Parm1>
void MemberFunctor1<CalleePtr,MemFunPtr,Parm1>::operator () ()
{
  //## begin MemberFunctor1::operator()%3A94477F0245.body preserve=yes
  ( ( *_pCallee ).*_pMemFun )( _aParm1 );
  //## end MemberFunctor1::operator()%3A94477F0245.body
}

// Additional Declarations
  //## begin MemberFunctor1%3A94477F0240.declarations preserve=yes
  //## end MemberFunctor1%3A94477F0240.declarations

// Parameterized Class MemberFunctor2 






//## Other Operations (implementation)
template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2>
void MemberFunctor2<CalleePtr,MemFunPtr,Parm1,Parm2>::operator () ()
{
  //## begin MemberFunctor2::operator()%3A94CFA50299.body preserve=yes
  ( ( *_pCallee ).*_pMemFun )( _aParm1, _aParm2 );
  //## end MemberFunctor2::operator()%3A94CFA50299.body
}

// Additional Declarations
  //## begin MemberFunctor2%3A94CFA50294.declarations preserve=yes
  //## end MemberFunctor2%3A94CFA50294.declarations

// Parameterized Class MemberFunctor3 







//## Other Operations (implementation)
template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3>
void MemberFunctor3<CalleePtr,MemFunPtr,Parm1,Parm2,Parm3>::operator () ()
{
  //## begin MemberFunctor3::operator()%3A953E850023.body preserve=yes
  ( ( *_pCallee ).*_pMemFun )( _aParm1, _aParm2, _aParm3 );
  //## end MemberFunctor3::operator()%3A953E850023.body
}

// Additional Declarations
  //## begin MemberFunctor3%3A953E85001D.declarations preserve=yes
  //## end MemberFunctor3%3A953E85001D.declarations

// Parameterized Class MemberFunctor4 








//## Other Operations (implementation)
template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3, typename Parm4>
void MemberFunctor4<CalleePtr,MemFunPtr,Parm1,Parm2,Parm3,Parm4>::operator () ()
{
  //## begin MemberFunctor4::operator()%3A955FF50271.body preserve=yes
  ( ( *_pCallee ).*_pMemFun )( _aParm1, _aParm2, _aParm3, _aParm4 );
  //## end MemberFunctor4::operator()%3A955FF50271.body
}

// Additional Declarations
  //## begin MemberFunctor4%3A955FF5026A.declarations preserve=yes
  //## end MemberFunctor4%3A955FF5026A.declarations

// Parameterized Class MemberFunctor5 









//## Other Operations (implementation)
template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5>
void MemberFunctor5<CalleePtr,MemFunPtr,Parm1,Parm2,Parm3,Parm4,Parm5>::operator () ()
{
  //## begin MemberFunctor5::operator()%3A957B0D02AF.body preserve=yes
  ( ( *_pCallee ).*_pMemFun )( _aParm1, _aParm2, _aParm3, _aParm4, _aParm5 );
  //## end MemberFunctor5::operator()%3A957B0D02AF.body
}

// Additional Declarations
  //## begin MemberFunctor5%3A957B0D02A7.declarations preserve=yes
  //## end MemberFunctor5%3A957B0D02A7.declarations

// Parameterized Class MemberFunctor6 










//## Other Operations (implementation)
template <typename CalleePtr, typename MemFunPtr, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6>
void MemberFunctor6<CalleePtr,MemFunPtr,Parm1,Parm2,Parm3,Parm4,Parm5,Parm6>::operator () ()
{
  //## begin MemberFunctor6::operator()%3A95855A039F.body preserve=yes
  ( ( *_pCallee ).*_pMemFun )( _aParm1, _aParm2, _aParm3, _aParm4, _aParm5, _aParm6 );
  //## end MemberFunctor6::operator()%3A95855A039F.body
}

// Additional Declarations
  //## begin MemberFunctor6%3A95855A0396.declarations preserve=yes
  //## end MemberFunctor6%3A95855A0396.declarations

//## begin module%3A94196B03C5.epilog preserve=yes
//## end module%3A94196B03C5.epilog


#endif
