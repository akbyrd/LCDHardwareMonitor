#if __cplusplus
	#define cbuffer struct
	#define float1  r32
	#define float2  v2
	#define float3  v3
	#define float4  v4
	#define matrix  Matrix
#endif

cbuffer VSPerObject
{
	matrix wvp;
};

#if __cplusplus
	#undef cbuffer
	#undef float1
	#undef float2
	#undef float3
	#undef float4
	#undef matrix
#endif
