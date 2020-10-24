#ifndef LHM_COMPILER
#define LHM_COMPILER

#if _MSC_VER
	// Some warnings are just fucking stupid
	#pragma warning (disable: 4201) // Nameless struct/union
	#pragma warning (disable: 4514) // Unreferenced inline removed
	#pragma warning (disable: 4710) // Function not inlined
	#pragma warning (disable: 4711) // Function inlined
	#pragma warning (disable: 4738) // Storing float in memory
	#pragma warning (disable: 4774) // printf format not a literal
	#pragma warning (disable: 4820) // Struct padding added
	#pragma warning (disable: 5045) // Spectre mitigations

	#define EXPORT extern "C" __declspec(dllexport)
	#define __FUNCTION_FULL_NAME__ __FUNCSIG__

	#if defined(_DEBUG)
		#define DEBUG 1
	#else
		#define DEBUG 0
	#endif
#endif

#if defined(__cplusplus_cli)
	// NOTE: This warning is spuriously raised when using #pragma make_public on
	// a nested native type.
	#pragma warning (disable: 4692) // Non-private member contains private type
	#pragma warning (disable: 4339) // Undefined type in CLR metadata
#endif

#endif
