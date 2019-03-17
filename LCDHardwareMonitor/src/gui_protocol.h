struct Null
{
	static const u32 Id = 0;
	u32 id;
};

struct Handshake
{
	static constexpr u32 Id = IdOf<Handshake>();
	u32 id;

	int        one;
	int        two;
	int        three;
};
