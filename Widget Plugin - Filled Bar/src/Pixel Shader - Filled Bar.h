#if __cplusplus
	#define cbuffer struct
	#define float1  r32
	#define float2  v2
	#define float3  v3
	#define float4  v4
	#define matrix  Matrix
#endif

cbuffer PSInitialize
{
	float4 borderColor;
	float2 borderSizeUV;
	float2 borderBlurUV;

	float4 backgroundColor;
	float4 fillColor;
	float1 fillBlur;
	float3 padding1;
};

cbuffer PSPerObject
{
	float1 fillAmount;
	float3 padding2;
};

#if __cplusplus
	#undef cbuffer
	#undef float1
	#undef float2
	#undef float3
	#undef float4
	#undef matrix
#endif
