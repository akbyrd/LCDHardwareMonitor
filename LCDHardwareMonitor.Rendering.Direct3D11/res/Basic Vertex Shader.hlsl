cbuffer cbPerObject
{
	matrix gWorldViewProj;
};

struct Vertex
{
	float3 PosL  : POSITION;
	float4 Color : COLOR;
};

struct PixelFragment
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
};

PixelFragment main(Vertex vIn)
{
	PixelFragment pOut;

	pOut.PosH  = mul(float4(vIn.PosL, 1.0f), gWorldViewProj);
	pOut.Color = vIn.Color;

	return pOut;
}