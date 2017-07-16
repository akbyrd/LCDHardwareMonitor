//TODO: Ensure transparent borders work as expected when blending with the fill/background
//TODO: Doesn't support vertical bars
//TODO: Maybe want to support arbitrary bar direction (to get a slanted end)
//TODO: Corners in the border blur don't look right
//TODO: The border blur size isn't quite correct (give it large numbers, expect it to cap out. Not sure if it does?)

cbuffer cbPerObject
{
	//TODO: How do we get this? (Do the UV calculation in CPU, I think)
	float2 res;

	float4 borderColor = { 1.0f, 0.0f, 0.0f, 1.0f };
	float  borderSize  = 20.0f;
	float  borderBlur  = 20.0f;

	float4 fillColor  = { 1.0f, 1.0f, 1.0f, 0.5f };
	float  fillAmount = 0.5f;
	float  fillBlur   = 10.0f;

	float4 backgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct PixelFragment
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
	float2 UV    : TEXCOORD;
};

float4 main(PixelFragment pIn) : SV_TARGET
{
	//Border
	float2 pixelsPerUV = rcp(res);
	float2 borderSizeUV = borderSize * pixelsPerUV;
	float2 borderBlurUV = borderBlur * pixelsPerUV;
	float2 borderUV = abs(pIn.UV - 0.5f); //[-.5, .5]
	float2 borderThing = smoothstep(0.5f - borderBlurUV - borderSizeUV, 0.5f - borderSizeUV, borderUV);
	float borderMask = max(borderThing.x, borderThing.y);


	//Fill
	pIn.UV = (1.0f + 2.0f * borderSizeUV) * pIn.UV - borderSizeUV;
	float t = smoothstep(fillAmount + fillBlur, fillAmount - fillBlur, pIn.UV.x);
	float4 interiorColor = lerp(fillColor, backgroundColor, t);


	return lerp(interiorColor, borderColor, borderMask);
}