/* NOTE:
	Problems we are solving:
		- Releasing resources at the end of the current scope
		- Without complicated control flow
		- Without separating creation and release code
		- Without adding a ton of complexity

	Problems we are *not* solving:
		- Exception safety
		- Thread safety
		- Perfect forwarding
		- Bad programmers

	Tradeoffs we accept:
		- Potentially low quality code generation due to using lambdas (that may
		  capture) and templated structs.
		- Small performance hit for checking resource validity in the generalized
		  cleanup code that we may not have needed to do in specific instances.
 */

template <typename TResource, typename TCleanup, TCleanup Cleanup>
struct Scoped
{
	//NOTE: Const members have to be set in the initializer list.
	Scoped(TResource resource) : resource(resource) { }
	~Scoped() { Cleanup(resource); }

	const TResource resource;
	b32 operator== (TResource other) { return resource == other; }
	operator TResource() { return resource; }
};



template<typename TLambda>
struct deferred final
{
	TLambda lambda;
	//NOTE: Lambdas cannot be default constructed, so the initializer list must be used.
	deferred(TLambda&& _lambda) : lambda(_lambda) {}
	~deferred() { lambda(); }
};

struct
{
	//NOTE: Template classes can't deduce parameters but functions can.
	//NOTE: Using operator<< lets us keep the actual lambda body out of the defer macro.
	template<typename TLambda>
	deferred<TLambda>
	operator<< (TLambda&& lambda)
	{
		return deferred<TLambda>(std::forward<TLambda>(lambda));
	}
} make_deferred;

#define defer auto CONCAT(__defer_, __COUNTER__) = make_deferred << [&]
