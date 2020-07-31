#if __cplusplus
	#define cbuffer struct
	#define float2  v2
	#define uint2   v2u
	namespace Blur {
#endif

cbuffer PSPerObject
{
	uint2  textureSize;
	float2 direction;
};

#if __cplusplus
	}
	#undef cbuffer
	#undef float2
	#undef uint2
#endif
