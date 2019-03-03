enum struct GUIConnectionState
{
	Null,
	Connected,
	Disconnected,
};

enum struct GUIMessage
{
	Null,
	Handshake,
};

struct Handshake
{
	int one;
	int two;
	int three;
};

#if false
enum struct MessageType
{
	Null,
	Handshake,
};

struct Handshake
{
	static const MessageType Type = MessageType::Handshake;

	MessageType type;
	u64         size;
	int         value;
};

template<typename T, typename... Args>
static inline T
Simulation_CreateMessage(Args... args)
{
	T message = {
		T::Type,
		sizeof(T),
		args...
	};
	return message;
}
#endif

// TODO: Ensure GUI can handle sim closing and reopening
