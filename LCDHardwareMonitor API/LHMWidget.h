#ifndef LHM_WIDGET
#define LHM_WIDGET

struct Widget
{
	// TODO: Should plugins or the renderer be responsible for creating a matrix from this information?
	v2  position;
	//v2  scale;
	v2  pivot;
	r32 depth;
};

#endif
