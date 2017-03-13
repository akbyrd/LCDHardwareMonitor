#include <cstdint>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  r32;
typedef double r64;

typedef char    c8;
typedef wchar_t c16;

typedef int b32;

typedef size_t index;

#define Kilobyte 1024LL
#define Megabyte 1024LL * Kilobyte
#define Gigabyte 1024LL * Megabyte

#define Assert(condition) if (!(condition)) { *((u8 *) 0) = 0; }
#define ArrayCount(x) sizeof(x) / sizeof(x[0])
#define ArraySize(x) sizeof(x);

/* NOTE: Raise a compiler error when switching over
 * an enum and any enum values are missing a case.
 * https://msdn.microsoft.com/en-us/library/fdt9w8tf.aspx
 */
#pragma warning (error: 4062)

#include <memory>
using std::unique_ptr;