#if __cplusplus
	#define cbuffer struct
	#define float4  v4
#endif

cbuffer PSInitialize
{
	float4 color;
};

#if __cplusplus
	#undef cbuffer
	#undef float4
#endif
