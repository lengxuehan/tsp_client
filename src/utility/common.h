/**
* @file common.h
* @brief common defines.
* @author		wuting.xu
* @date		    2023/10/30
* @par Copyright(c): 	2023 megatronix. All rights reserved.
*/

#pragma once

#undef DISALLOW_EVIL_CONSTRUCTORS
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
	TypeName(const TypeName&);                           \
	void operator=(const TypeName&)

// A macro to disallow all the implicit constructors, namely the
// default constructor, copy constructor and operator= functions.
//
// This should be used in the private: declarations for a class
// that wants to prevent anyone from instantiating it. This is
// especially useful for classes containing only static methods.
#undef DISALLOW_IMPLICIT_CONSTRUCTORS
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName)	\
	TypeName();										\
	DISALLOW_EVIL_CONSTRUCTORS(TypeName)
