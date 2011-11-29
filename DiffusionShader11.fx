//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
matrix g_mModelViewProjection;


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
VS_OUTPUT VSMain(float3 position : POSITION, float4 color : COLOR)
{
	VS_OUTPUT Output;
	
	Output.position = mul(float4(position.x, position.y, position.z, 1.0),g_mModelViewProjection);
	Output.color = color;
	
	return Output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(VS_OUTPUT vsOutput) : SV_TARGET
{
	return vsOutput.color;
}


//--------------------------------------------------------------------------------------
// Techniques/states
//--------------------------------------------------------------------------------------

DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};


technique11 Main
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VSMain()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PSMain()));

		SetDepthStencilState( EnableDepth, 0);
	}
}