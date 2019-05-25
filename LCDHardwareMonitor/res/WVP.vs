#include "../../LCDHardwareMonitor API/WVP.vs.h"

struct Vertex
{
	float3 PosL  : POSITION;
	float4 Color : COLOR;
	float2 UV    : TEXCOORD;
};

struct PixelFragment
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
	float2 UV    : TEXCOORD;
};

PixelFragment main(Vertex vIn)
{
	PixelFragment pOut;

	pOut.PosH  = mul(float4(vIn.PosL, 1.0f), wvp);
	pOut.Color = vIn.Color;
	pOut.UV    = vIn.UV;

	return pOut;
}
