struct PixelFragment
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
	float2 UV    : TEXCOORD;
};

float4 main(PixelFragment pIn) : SV_TARGET
{
	float2 res  = floor(pIn.UV * 200);
	float4 mask = (float4) (res.x + res.y) % 2.0;
	return mask * pIn.Color;
}
