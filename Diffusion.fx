//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------

Texture3D ColorTexture;
Texture3D DistTexture;

float3 vTextureSize;
float fIsoValue;
float fPolySize;
int iSliceIndex;

bool bShowIsoColor;

//--------------------------------------------------------------------------------------
// Sampler
//--------------------------------------------------------------------------------------

SamplerState pointSamplerClamp
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;				// clamp sampling in U
    AddressV = Clamp;				// clamp sampling in V
	AddressW = Clamp;				// clamp sampling in W
};

SamplerState linearSamplerClamp
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;				// clamp sampling in U
    AddressV = Clamp;				// clamp sampling in V
	AddressW = Clamp;				// clamp sampling in W
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
	uint sliceindex : SLICEINDEX;
};

struct PS_DIFFUSION_INPUT
{
	float4 pos		: SV_Position;
	float3 tex		: TEXCOORD0;
	uint sliceindex : SLICEINDEX;
};

struct PS_DIFFUSION_OUTPUT
{
	float4 color	: SV_Target0;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

PS_DIFFUSION_INPUT DiffusionVS(VS_DIFFUSION_INPUT input)
{
	PS_DIFFUSION_INPUT output;
	output.pos = float4(input.pos, 1.0f);
	output.tex = input.tex;
	output.sliceindex = input.sliceindex;
	return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------


PS_DIFFUSION_OUTPUT DiffusionPS(PS_DIFFUSION_INPUT input)
{
	PS_DIFFUSION_OUTPUT output;

	float rawKernel = 0.92387*DistTexture.SampleLevel(pointSamplerClamp, input.tex, 0).x;
	float kernel = rawKernel*vTextureSize.x;
	kernel *= fPolySize;
	kernel -= 0.5;
	kernel = max(0,kernel);
	output.color = ColorTexture.SampleLevel(pointSamplerClamp, input.tex+float3(-kernel/vTextureSize.x,0,0), 0);
	output.color += ColorTexture.SampleLevel(pointSamplerClamp, input.tex+float3( kernel/vTextureSize.x,0,0), 0);
	kernel = rawKernel*vTextureSize.y;
	kernel *= fPolySize;
	kernel -= 0.5;
	kernel = max(0,kernel);
	output.color += ColorTexture.SampleLevel(pointSamplerClamp, input.tex+float3(0,-kernel/vTextureSize.y, 0), 0);
	output.color += ColorTexture.SampleLevel(pointSamplerClamp, input.tex+float3(0, kernel/vTextureSize.y, 0), 0);
	kernel = rawKernel*vTextureSize.z;
	kernel *= fPolySize;
	kernel -= 0.5;
	kernel = max(0,kernel);
	output.color += ColorTexture.SampleLevel(pointSamplerClamp, input.tex+float3(0, 0, -kernel/vTextureSize.z), 0);
	output.color += ColorTexture.SampleLevel(pointSamplerClamp, input.tex+float3(0, 0, kernel/vTextureSize.z), 0);
	
	output.color /= 6;
	return output;
}

PS_DIFFUSION_OUTPUT OneSlicePS(PS_DIFFUSION_INPUT input)
{
	PS_DIFFUSION_OUTPUT output;
	if(input.sliceindex == iSliceIndex)
	{
		output.color = ColorTexture.SampleLevel(pointSamplerClamp, input.tex, 0);
		output.color.a = 1.0f;
	}
	else
	{
		output.color = float4(0.0f,0.0f,0.0f,0.0f);
	}
	return output;
}

PS_DIFFUSION_OUTPUT IsoSurfacePS(PS_DIFFUSION_INPUT input)
{
	PS_DIFFUSION_OUTPUT output;

	output.color = ColorTexture.SampleLevel(linearSamplerClamp, input.tex, 0);
	
	if(output.color.a >= fIsoValue)
	{
		if(!bShowIsoColor)
			output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
		output.color.a = 1.0f;
	}
	else
	{
		output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	
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
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, DiffusionPS()));
		SetRasterizerState( CullNone );
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DisableDepth, 0 );
	}

	pass RenderOneSlice
	{
		SetVertexShader(CompileShader(vs_4_0, DiffusionVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, OneSlicePS()));
		SetRasterizerState( CullNone );
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DisableDepth, 0 );
	}

	pass RenderIsoSurface
	{
		SetVertexShader(CompileShader(vs_4_0, DiffusionVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, IsoSurfacePS()));
		SetRasterizerState( CullNone );
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DisableDepth, 0 );
	}

}