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
	// TODO: How do we get this? (Do the UV calculation in CPU, I think)
	float2 size;
	float2 padding1;

	float4 borderColor = Color32(47, 112, 22, 255);
	float1 borderSize  = 1.0f;
	float1 borderBlur  = 0.0f;
	float2 padding2;

	float4 fillColor  = Color32(47, 112, 22, 255);
	float1 fillAmount = 0.5f;
	float1 fillBlur   = 0.0f;
	float2 padding3;

	float4 backgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
};
