#ifndef LHM_DEFER
#define LHM_DEFER

// NOTE:
// Problems we are solving:
// - Releasing resources at the end of the current scope
// - Without complicated control flow
// - Without separating creation and release code
// - Without adding a ton of complexity
//
// Problems we are *not* solving:
// - Exception safety
// - Thread safety
// - Bad programmers
//
// Tradeoffs we accept:
// - Potentially low quality code generation due to using lambdas (that may
//   capture) and templated structs.
// - Small performance hit for checking resource validity in the generalized
//   cleanup code that we may not have needed to do in specific instances.

// NOTE: Lambdas cannot be default constructed, so the initializer list must be used.
// NOTE: Using operator<< lets us keep the actual lambda body out of the defer macro.
// NOTE: The use of a forwarding reference and std:forward avoids unnecessary copies.
// NOTE: Const members have to be set in the initializer list.

template<typename TLambda>
struct Defer final
{
	const TLambda lambda;
	Defer(TLambda&& _lambda) : lambda(_lambda) {}
	~Defer() { lambda(); }

	// NOTE: Avoids warning
	void operator=(const Defer&) = delete;
};

#define DEFER_UNIQUE_NAME2(x, y) x ## y
#define DEFER_UNIQUE_NAME1(x, y) DEFER_UNIQUE_NAME2(x, y)
#define DEFER_UNIQUE_NAME() DEFER_UNIQUE_NAME1(__defer_, __COUNTER__)
#define defer Defer DEFER_UNIQUE_NAME() = [&]

template<typename TLambda>
struct Guard final
{
	const TLambda lambda;
	b8 dismiss;
	Guard(TLambda&& lambda) : lambda(lambda), dismiss(false) {}
	~Guard() { if (!dismiss) lambda(); }

	// NOTE: Avoids warning
	void operator=(const Guard&) = delete;
};

#define guard [&]

template<typename TResource, auto& ReleaseFn>
struct Scoped
{
	~Scoped() { ReleaseFn(resource); }

	TResource resource;
	operator TResource&() { return resource; }
};

#endif
