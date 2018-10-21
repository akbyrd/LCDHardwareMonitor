struct PixelFragment
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
	float2 UV    : TEXCOORD;
};

float4 main(PixelFragment pIn) : SV_TARGET
{
	return pIn.Color;
}
