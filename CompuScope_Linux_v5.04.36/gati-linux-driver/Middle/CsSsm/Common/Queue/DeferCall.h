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
#ifndef DeferCall_h
#define DeferCall_h 1


#include "FreeFunctor.h"
#include "MemberFunctor.h"
#include "Reference.h"



////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename Ret >
Functor * const DeferCall( Ret ( *pFun )() )
{
  return new FreeFunctor0< Ret (*)() >( pFun );
}

template< typename CalleePtr, typename Callee, typename Ret >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )() )
{
  return new MemberFunctor0< CalleePtr, Ret ( Callee::* )() >( pCallee, pMemFun );
}

template< typename CalleePtr, typename Callee, typename Ret >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )() const )
{
  return new MemberFunctor0< CalleePtr, Ret ( Callee::* )() const >( pCallee, pMemFun );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename Ret, typename Type1, typename Parm1 >
Functor * const DeferCall( Ret ( *pFun )( Type1 ), const Parm1 & aParm1 )
{
  return new FreeFunctor1< Ret (*)( Type1 ), Parm1 >( pFun, aParm1 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Parm1 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1 ), const Parm1 & aParm1 )
{
  return new MemberFunctor1< CalleePtr, Ret ( Callee::* )( Type1 ), Parm1 >(
    pCallee, pMemFun, aParm1 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Parm1 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1 ) const, const Parm1 & aParm1 )
{
  return new MemberFunctor1< CalleePtr, Ret ( Callee::* )( Type1 ) const, Parm1 >(
    pCallee, pMemFun, aParm1 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename Ret, typename Type1, typename Type2, typename Parm1, typename Parm2 >
Functor * const DeferCall( Ret ( *pFun )( Type1, Type2 ), const Parm1 & aParm1, const Parm2 & aParm2 )
{
  return new FreeFunctor2< Ret (*)( Type1, Type2 ), Parm1, Parm2 >( pFun, aParm1, aParm2 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Parm1, typename Parm2 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2 ), const Parm1 & aParm1, const Parm2 & aParm2 )
{
  return new MemberFunctor2< CalleePtr, Ret ( Callee::* )( Type1, Type2 ), Parm1, Parm2 >(
    pCallee, pMemFun, aParm1, aParm2 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Parm1, typename Parm2 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2 ) const, const Parm1 & aParm1, const Parm2 & aParm2 )
{
  return new MemberFunctor2< CalleePtr, Ret ( Callee::* )( Type1, Type2 ) const, Parm1, Parm2 >(
    pCallee, pMemFun, aParm1, aParm2 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename Ret, typename Type1, typename Type2, typename Type3, typename Parm1, typename Parm2, typename Parm3 >
Functor * const DeferCall( Ret ( *pFun )( Type1, Type2, Type3 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3 )
{
  return new FreeFunctor3< Ret (*)( Type1, Type2, Type3 ), Parm1, Parm2, Parm3 >( pFun, aParm1, aParm2, aParm3 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Parm1, typename Parm2, typename Parm3 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3 )
{
  return new MemberFunctor3< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3 ), Parm1, Parm2, Parm3 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Parm1, typename Parm2, typename Parm3 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3 ) const, const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3 )
{
  return new MemberFunctor3< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3 ) const, Parm1, Parm2, Parm3 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Parm1, typename Parm2, typename Parm3, typename Parm4 >
Functor * const DeferCall( Ret ( *pFun )( Type1, Type2, Type3, Type4 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4 )
{
  return new FreeFunctor4< Ret (*)( Type1, Type2, Type3, Type4 ), Parm1, Parm2, Parm3, Parm4 >( pFun, aParm1, aParm2, aParm3, aParm4 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Parm1, typename Parm2, typename Parm3, typename Parm4 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3, Type4 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4 )
{
  return new MemberFunctor4< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3, Type4 ), Parm1, Parm2, Parm3, Parm4 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3, aParm4 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Parm1, typename Parm2, typename Parm3, typename Parm4 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3, Type4 ) const, const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4 )
{
  return new MemberFunctor4< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3, Type4 ) const, Parm1, Parm2, Parm3, Parm4 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3, aParm4 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5 >
Functor * const DeferCall( Ret ( *pFun )( Type1, Type2, Type3, Type4, Type5 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5 )
{
  return new FreeFunctor5< Ret (*)( Type1, Type2, Type3, Type4, Type5 ), Parm1, Parm2, Parm3, Parm4, Parm5 >( pFun, aParm1, aParm2, aParm3, aParm4, aParm5 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3, Type4, Type5 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5 )
{
  return new MemberFunctor5< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3, Type4, Type5 ), Parm1, Parm2, Parm3, Parm4, Parm5 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3, aParm4, aParm5 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3, Type4, Type5 ) const, const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5 )
{
  return new MemberFunctor5< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3, Type4, Type5 ) const, Parm1, Parm2, Parm3, Parm4, Parm5 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3, aParm4, aParm5 );
}



////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6 >
Functor * const DeferCall( Ret ( *pFun )( Type1, Type2, Type3, Type4, Type5, Type6 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5, const Parm6 & aParm6 )
{
  return new FreeFunctor6< Ret (*)( Type1, Type2, Type3, Type4, Type5, Type6 ), Parm1, Parm2, Parm3, Parm4, Parm5, Parm6 >( pFun, aParm1, aParm2, aParm3, aParm4, aParm5, aParm6 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3, Type4, Type5, Type6 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5, const Parm6 & aParm6 )
{
  return new MemberFunctor6< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3, Type4, Type5, Type6 ), Parm1, Parm2, Parm3, Parm4, Parm5, Parm6 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3, aParm4, aParm5, aParm6 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3, Type4, Type5, Type6 ) const, const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5, const Parm6 & aParm6 )
{
  return new MemberFunctor6< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3, Type4, Type5, Type6 ) const, Parm1, Parm2, Parm3, Parm4, Parm5, Parm6 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3, aParm4, aParm5, aParm6 );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template< typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7 >
Functor * const DeferCall( Ret ( *pFun )( Type1, Type2, Type3, Type4, Type5, Type6, Type7 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5, const Parm6 & aParm6, const Parm7 & aParm7 )
{
  return new FreeFunctor7< Ret (*)( Type1, Type2, Type3, Type4, Type5, Type6, Type7 ), Parm1, Parm2, Parm3, Parm4, Parm5, Parm6, Parm7 >( pFun, aParm1, aParm2, aParm3, aParm4, aParm5, aParm6, aParm7 );
}
/* TODO: RG - ????
template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7  >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3, Type4, Type5, Type6, Type7 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5, const Parm6 & aParm6, const Parm6 & aParm7 )
{
  return new MemberFunctor7< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3, Type4, Type5, Type6, Type7 ), Parm1, Parm2, Parm3, Parm4, Parm5, Parm6, Parm7 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3, aParm4, aParm5, aParm6, aParm7 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3, Type4, Type5, Type6, Type7 ) const, const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5, const Parm6 & aParm6, const Parm7 & aParm7 )
{
  return new MemberFunctor7< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3, Type4, Type5, Type6, Type7 ) const, Parm1, Parm2, Parm3, Parm4, Parm5, Parm6, Parm7 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3, aParm4, aParm5, aParm6, aParm7 );
}
*/
////////////////////////////////////////////////////////////////////////////////////////////////////
/*template< typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Type7, typename Type8, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7, typename Parm8 >
Functor * const DeferCall( Ret ( *pFun )( Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5, const Parm6 & aParm6, const Parm7 & aParm7, const Parm8 & aParm8 )
{
  return new FreeFunctor8< Ret (*)( Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8 ), Parm1, Parm2, Parm3, Parm4, Parm5, Parm6, Parm7, Parm8 >( pFun, aParm1, aParm2, aParm3, aParm4, aParm5, aParm6, aParm7, aParm8 );
}*/
/*
template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6, typename Parm7, typename Parm8 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3, Type4, Type5, Type6 ), const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5, const Parm6 & aParm6, const Parm7 & aParm7, const Parm8 & aParm8 )
{
  return new MemberFunctor6< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3, Type4, Type5, Type6 ), Parm1, Parm2, Parm3, Parm4, Parm5, Parm6 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3, aParm4, aParm5, aParm6 );
}

template< typename CalleePtr, typename Callee, typename Ret, typename Type1, typename Type2, typename Type3, typename Type4, typename Type5, typename Type6, typename Parm1, typename Parm2, typename Parm3, typename Parm4, typename Parm5, typename Parm6 >
Functor * const DeferCall( const CalleePtr & pCallee, Ret ( Callee::*pMemFun )( Type1, Type2, Type3, Type4, Type5, Type6 ) const, const Parm1 & aParm1, const Parm2 & aParm2, const Parm3 & aParm3, const Parm4 & aParm4, const Parm5 & aParm5, const Parm6 & aParm6 )
{
  return new MemberFunctor6< CalleePtr, Ret ( Callee::* )( Type1, Type2, Type3, Type4, Type5, Type6 ) const, Parm1, Parm2, Parm3, Parm4, Parm5, Parm6 >(
    pCallee, pMemFun, aParm1, aParm2, aParm3, aParm4, aParm5, aParm6 );
}*/

#endif
