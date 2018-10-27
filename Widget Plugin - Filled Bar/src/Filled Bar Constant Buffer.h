#if __cplusplus
	#define cbuffer     struct
	#define cbPerObject PSConstants
	#define float1      r32
	#define float2      v2
	#define float4      v4
#else
	#define Color32(r, g, b, a) 0
#endif

cbuffer cbPerObject
{
	float4 borderColor;
	float2 borderSizeUV;
	float2 borderBlurUV;

	float4 fillColor;
	float1 fillAmount;
	float1 fillBlur;
	float2 padding3;

	float4 backgroundColor;
};
