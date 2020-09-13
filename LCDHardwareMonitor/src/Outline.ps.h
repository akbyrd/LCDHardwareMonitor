#if __cplusplus
	#define cbuffer struct
	#define float2 v2
	#define float4 v4
	#define uint2  v2u
	namespace Outline {
#endif

cbuffer PSPerPass
{
	uint2  textureSize;
	float2 blurDirection;
	float4 outlineColor;
};

#if __cplusplus
	}
	#undef cbuffer
	#undef float2
	#undef float4
	#undef uint2
#endif
