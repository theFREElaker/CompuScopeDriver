//## begin module%1.3%.codegen_version preserve=yes
//   Read the documentation to learn more about C++ code generator
//   versioning.
//## end module%1.3%.codegen_version

//## begin module%3A98F644031A.cm preserve=no
//	  %X% %Q% %Z% %W%
//## end module%3A98F644031A.cm

//## begin module%3A98F644031A.cp preserve=no
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
//## end module%3A98F644031A.cp

//## Module: SimpleReferencer%3A98F644031A; Subprogram specification
//## Subsystem: DeferCall::Simple%3A971FA802FA
//## Source file: D:\Data\Private\DeferCall\Simple\SimpleReferencer.h

#ifndef SimpleReferencer_h
#define SimpleReferencer_h 1

//## begin module%3A98F644031A.additionalIncludes preserve=no
//## end module%3A98F644031A.additionalIncludes

//## begin module%3A98F644031A.includes preserve=yes
//## end module%3A98F644031A.includes

//## begin module%3A98F644031A.declarations preserve=no
//## end module%3A98F644031A.declarations

//## begin module%3A98F644031A.additionalDeclarations preserve=yes
//## end module%3A98F644031A.additionalDeclarations


//## begin SimpleReferencer%3A98F3860040.preface preserve=yes
//## end SimpleReferencer%3A98F3860040.preface

//## Class: SimpleReferencer%3A98F3860040; Parameterized Class
//	This class stores a reference to the object passed to its constructor. It also defines a
//	conversion operator, so that it can be implicitly converted to a reference pointing to the
//	original object.
//## Category: DeferCall::Simple%3A971F9303C2
//## Subsystem: DeferCall::Simple%3A971FA802FA
//## Persistence: Transient
//## Cardinality/Multiplicity: n

template <typename Type>
class SimpleReferencer 
{
  //## begin SimpleReferencer%3A98F3860040.initialDeclarations preserve=yes
  //## end SimpleReferencer%3A98F3860040.initialDeclarations

  public:
    //## Constructors (specified)
      //## Operation: SimpleReferencer%3A98F5B901D0
      explicit SimpleReferencer (Type &Object);


    //## Other Operations (specified)
      //## Operation: operator Type &%3A98F3CC031B
      operator Type & () const;

    // Additional Public Declarations
      //## begin SimpleReferencer%3A98F3860040.public preserve=yes
      //## end SimpleReferencer%3A98F3860040.public

  protected:
    // Additional Protected Declarations
      //## begin SimpleReferencer%3A98F3860040.protected preserve=yes
      //## end SimpleReferencer%3A98F3860040.protected

  private:
    // Data Members for Class Attributes

      //## Attribute: ObjectRef%3A98F3E200D8
      //## begin SimpleReferencer::ObjectRef%3A98F3E200D8.attr preserve=no  private: Type & {U} 
      Type &_ObjectRef;
      //## end SimpleReferencer::ObjectRef%3A98F3E200D8.attr

    // Additional Private Declarations
      //## begin SimpleReferencer%3A98F3860040.private preserve=yes
      //## end SimpleReferencer%3A98F3860040.private

  private: //## implementation
    // Additional Implementation Declarations
      //## begin SimpleReferencer%3A98F3860040.implementation preserve=yes
      //## end SimpleReferencer%3A98F3860040.implementation

};

//## begin SimpleReferencer%3A98F3860040.postscript preserve=yes
//## end SimpleReferencer%3A98F3860040.postscript

// Parameterized Class SimpleReferencer 

template <typename Type>
inline SimpleReferencer<Type>::SimpleReferencer (Type &Object)
  //## begin SimpleReferencer::SimpleReferencer%3A98F5B901D0.hasinit preserve=no
  //## end SimpleReferencer::SimpleReferencer%3A98F5B901D0.hasinit
  //## begin SimpleReferencer::SimpleReferencer%3A98F5B901D0.initialization preserve=yes
  : _ObjectRef( Object )
  //## end SimpleReferencer::SimpleReferencer%3A98F5B901D0.initialization
{
  //## begin SimpleReferencer::SimpleReferencer%3A98F5B901D0.body preserve=yes
  //## end SimpleReferencer::SimpleReferencer%3A98F5B901D0.body
}



//## Other Operations (inline)
template <typename Type>
inline SimpleReferencer<Type>::operator Type & () const
{
  //## begin SimpleReferencer::operator Type &%3A98F3CC031B.body preserve=yes
  return _ObjectRef;
  //## end SimpleReferencer::operator Type &%3A98F3CC031B.body
}

// Parameterized Class SimpleReferencer 


// Additional Declarations
  //## begin SimpleReferencer%3A98F3860040.declarations preserve=yes
  //## end SimpleReferencer%3A98F3860040.declarations

//## begin module%3A98F644031A.epilog preserve=yes
//## end module%3A98F644031A.epilog


#endif
