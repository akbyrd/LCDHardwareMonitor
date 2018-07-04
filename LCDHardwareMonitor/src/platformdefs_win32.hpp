// NOTE: Raise a compiler error when switching over
// an enum and any enum values are missing a case.
// https://msdn.microsoft.com/en-us/library/fdt9w8tf.aspx
#pragma warning (error: 4062)

#define L_WIDEHELPER(x) L##x
#define L_WIDE(x) L_WIDEHELPER(x)

#define WFILE L_WIDE(__FILE__)
#define LINE  __LINE__
#define WFUNC L_WIDE(__FUNCTION__)
