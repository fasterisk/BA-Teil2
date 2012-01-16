//------------------------------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------------------------------

matrix WorldViewProjection;

//------------------------------------------------------------------------------------------------------
// States
//------------------------------------------------------------------------------------------------------

DepthStencilState DisableDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
};

RasterizerState CullFront
{
    MultiSampleEnable = True;
    CullMode = Front;
};

RasterizerState CullBack
{
    MultiSampleEnable = True;
    CullMode = Back;
};

//------------------------------------------------------------------------------------------------------
// Structs
//------------------------------------------------------------------------------------------------------

struct VsInput
{
	float3 pos : POSITION;
};

struct VsOutput
{
	float4 pos : SV_POSITION;
	float4 posb : POSITION;
};

struct PsOutput
{
    float4 color : SV_Target;
};

//------------------------------------------------------------------------------------------------------
// Vertex Shaders
//------------------------------------------------------------------------------------------------------

VsOutput VS_BOUNDINGBOX(VsInput input)
{
	VsOutput output;
	output.pos = mul(float4(input.pos, 1), WorldViewProjection);
	output.posb = float4(input.pos, 1);
	return output;
}

//------------------------------------------------------------------------------------------------------
// Pixel Shaders
//------------------------------------------------------------------------------------------------------

PsOutput PS_BOUNDINGBOX(VsOutput input)
{
	PsOutput output;
	output.color = (input.posb+1)/2;
	return output;
}

//------------------------------------------------------------------------------------------------------
// Techniques
//------------------------------------------------------------------------------------------------------

technique10 VolumeRendering
{
	pass BoundingBoxFront
	{
		SetVertexShader(CompileShader(vs_4_0, VS_BOUNDINGBOX()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_BOUNDINGBOX()));

		SetRasterizerState(CullBack);
		SetDepthStencilState(DisableDepth, 0);
	}

	pass BoundingBoxBack
	{
		SetVertexShader(CompileShader(vs_4_0, VS_BOUNDINGBOX()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_BOUNDINGBOX()));

		SetRasterizerState(CullFront);
		SetDepthStencilState(DisableDepth, 0);
	}
}