#ifndef LHM_COMPILER
#define LHM_COMPILER

#if _MSC_VER
	// Some warnings are just fucking stupid
	#pragma warning (disable: 4061) // Enum not explicitly handle in switch
	#pragma warning (disable: 4201) // Nameless struct/union
	#pragma warning (disable: 4324) // Struct padded due to alignment specifier
	#pragma warning (disable: 4339) // Undefined type in CLR metadata
	#pragma warning (disable: 4464) // Relative include path
	#pragma warning (disable: 4472) // Native enum
	#pragma warning (disable: 4514) // Unreferenced inline removed
	#pragma warning (disable: 4626) // Assignment operator implicitly deleted
	#pragma warning (disable: 4668) // Macro not defined
	#pragma warning (disable: 4710) // Function not inlined
	#pragma warning (disable: 4711) // Function inlined
	#pragma warning (disable: 4774) // printf format not a literal
	#pragma warning (disable: 4820) // Struct padding added
	#pragma warning (disable: 5039) // Throwing function passed to extern C function

	#define EXPORT extern "C" __declspec(dllexport)
	#define __FUNCTION_FULL_NAME__ __FUNCSIG__
#endif

#if __cplusplus_cli
	// NOTE: This warning is spuriously raised when using #pragma make_public on
	// a nested native type.
	#pragma warning (disable: 4692) // Non-private member contains private type
#endif

#endif
