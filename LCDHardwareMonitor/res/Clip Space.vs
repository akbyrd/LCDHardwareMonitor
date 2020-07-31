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

	pOut.PosH  = float4(vIn.PosL, 1.0);
	pOut.Color = vIn.Color;
	pOut.UV    = vIn.UV;

	return pOut;
}
