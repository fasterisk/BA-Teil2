//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------

struct VS_OUTPUT
{
	float4 position	: SV_POSITION;
	float4 color : COLOR;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain(float4 position : POSITION, float4 color : COLOR)
{
	VS_OUTPUT Output;
	
	Output.position = position;
	Output.color = color;
	
	return Output;
}

