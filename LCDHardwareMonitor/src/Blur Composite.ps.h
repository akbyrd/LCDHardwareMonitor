#if __cplusplus
	#define cbuffer struct
	#define float3 v3
	#define float4 v4
	namespace BlurComposite {
#endif

cbuffer PSPerObject
{
	bool   useSourceColor;
	float3 padding;
	float4 compositeColor;
};

#if __cplusplus
	}
	#undef cbuffer
	#undef float3
	#undef float4
#endif
