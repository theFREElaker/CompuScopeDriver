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
#ifndef Reference_h
#define Reference_h 1

#include "SimpleReferencer.h"



////////////////////////////////////////////////////////////////////////////////////////////////////
// Creator function template for the SimpleReferencer class
////////////////////////////////////////////////////////////////////////////////////////////////////
template< class Type >
inline const SimpleReferencer< Type > Reference( Type & Object )
{
  return SimpleReferencer< Type >( Object );
}



#endif
