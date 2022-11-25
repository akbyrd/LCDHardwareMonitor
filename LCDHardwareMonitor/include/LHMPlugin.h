#ifndef LHM_Plugin
#define LHM_Plugin

struct PluginDesc
{
	StringView name;
	StringView author;
	u32        version;
	u32        lhmVersion;
};

struct PluginContext;

// TODO: Should be able to make this private once we copy sensor values to widgets
template <typename T>
struct Handle
{
	u32 value = 0;

	static const Handle Null;
	explicit operator b8() { return *this != Null; }
};

template<typename T>
const Handle<T> Handle<T>::Null = {};

template <typename T>
inline b8 operator== (Handle<T> lhs, Handle<T> rhs) { return lhs.value == rhs.value; }

template <typename T>
inline b8 operator!= (Handle<T> lhs, Handle<T> rhs) { return lhs.value != rhs.value; }

#endif
