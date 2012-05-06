//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------

Texture3D ColorTexture;
Texture3D DistTexture;

float3 vTextureSize;
float fIsoValue;
float fPolySize;

//--------------------------------------------------------------------------------------
// Sampler
//--------------------------------------------------------------------------------------

SamplerState linearSamplerBorder
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border;				// border sampling in U
    AddressV = Border;				// border sampling in V
	AddressW = Border;				// border sampling in W
    BorderColor = float4(0,0,0,0);	// outside of border should be black
};

//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------

RasterizerState CullNone
{
    MultiSampleEnable = True;
    CullMode = None;
};

DepthStencilState DisableDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
	StencilEnable = FALSE;
};

BlendState NoBlending
{
  BlendEnable[0] = false;
  RenderTargetWriteMask[0] = 0x0F;
};

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------

struct VS_DIFFUSION_INPUT
{
	float3 pos		: POSITION;
	float3 tex		: TEXCOORD;
};

struct GS_DIFFUSION_INPUT
{
	float4 pos		: POSITION;
	float3 tex		: TEXCOORD;
};

struct GS_DIFFUSION_OUTPUT
{
	float4 pos		: SV_Position;
	float3 tex		: TEXCOORD0;
	float4 pos2		: TEXCOORD1;
	uint RTIndex	: SV_RenderTargetArrayIndex;
};

struct PS_DIFFUSION_OUTPUT
{
	float4 color	: SV_Target0;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

GS_DIFFUSION_INPUT DiffusionVS(VS_DIFFUSION_INPUT input)
{
	GS_DIFFUSION_INPUT output;
	output.pos = float4(input.pos, 1.0f);
	output.tex = input.tex;
	return output;
}


//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------

[maxvertexcount(3)]
void DiffusionGS(triangle GS_DIFFUSION_INPUT input[3], inout TriangleStream<GS_DIFFUSION_OUTPUT> tStream)
{
	GS_DIFFUSION_OUTPUT output;
	output.RTIndex = (uint)input[0].tex.z;
	for(int v = 0; v < 3; v++)
	{
		output.pos = input[v].pos;
		output.pos2 = input[v].pos;
		output.tex = input[v].tex;
		tStream.Append(output);
	}
	tStream.RestartStrip();
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------


PS_DIFFUSION_OUTPUT DiffusionPS(GS_DIFFUSION_OUTPUT input)
{
	PS_DIFFUSION_OUTPUT output;
	float3 tex = float3(input.tex.xy, input.tex.z / (vTextureSize.z - 1));

	float rawKernel = 0.92387*DistTexture.SampleLevel(linearSamplerBorder, tex, 0).x*3;
	float kernel = rawKernel*vTextureSize.x;
	kernel *= fPolySize;
	kernel -= 0.5;
	kernel = max(0,kernel);
	output.color = ColorTexture.SampleLevel(linearSamplerBorder, tex+float3(-kernel/vTextureSize.x,0,0), 0);
	output.color += ColorTexture.SampleLevel(linearSamplerBorder, tex+float3( kernel/vTextureSize.x,0,0), 0);
	kernel = rawKernel*vTextureSize.y;
	kernel *= fPolySize;
	kernel -= 0.5;
	kernel = max(0,kernel);
	output.color += ColorTexture.SampleLevel(linearSamplerBorder, tex+float3(0,-kernel/vTextureSize.y, 0), 0);
	output.color += ColorTexture.SampleLevel(linearSamplerBorder, tex+float3(0, kernel/vTextureSize.y, 0), 0);
	kernel = rawKernel*vTextureSize.z;
	kernel *= fPolySize;
	kernel -= 0.5;
	kernel = max(0,kernel);
	output.color += ColorTexture.SampleLevel(linearSamplerBorder, tex+float3(0,-kernel/vTextureSize.z, 0), 0);
	output.color += ColorTexture.SampleLevel(linearSamplerBorder, tex+float3(0, kernel/vTextureSize.z, 0), 0);
	
	output.color /= 6;
	//output.color = ColorTexture.SampleLevel(linearSamplerBorder, tex, 0);
	return output;
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------


technique10 Diffusion
{
	pass DiffuseTexture
	{
		SetVertexShader(CompileShader(vs_4_0, DiffusionVS()));
		SetGeometryShader(CompileShader(gs_4_0, DiffusionGS()));
		SetPixelShader(CompileShader(ps_4_0, DiffusionPS()));
		SetRasterizerState( CullNone );
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DisableDepth, 0 );
	}
}