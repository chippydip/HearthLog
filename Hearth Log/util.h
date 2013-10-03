#pragma once

#include <memory>

#ifdef WIN32
	// Use fake variadic template macros for VC++
	#include <xstddef>
	namespace std {
		// TEMPLATE FUNCTIONS make_unique and allocate_shared
		#define _ALLOCATE_MAKE_UNIQUE(TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
			template<class _Ty COMMA LIST(_CLASS_TYPE)> inline \
			unique_ptr<_Ty> make_unique(LIST(_TYPE_REFREF_ARG)) \
			{ \
				return unique_ptr<_Ty>(new _Ty(LIST(_FORWARD_ARG))); \
			}

		_VARIADIC_EXPAND_0X(_ALLOCATE_MAKE_UNIQUE, , , , )
		#undef _ALLOCATE_MAKE_UNIQUE
	}
#else
	// Simple version with variadic template argument list
	namespace std {
		template<typename T, typename ...Args>
		unique_ptr<T> make_unique(Args&& ...args)
		{
			return unique_ptr<T>(new T(forward<Args>(args)...));
		}
	}
#endif

