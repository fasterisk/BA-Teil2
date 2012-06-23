DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};

DepthStencilState DisableDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
};

RasterizerState CullNone
{
	MultiSampleEnable = True;
	CullMode = None;
};

RasterizerState Wireframe
{
	FillMode = WIREFRAME;
};

BlendState AlphaBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = SRC_ALPHA;
    DestBlendAlpha = INV_SRC_ALPHA;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

SamplerState textureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
	BorderColor = float4(0,0,0,0);
};


matrix	ModelViewProjectionMatrix;
matrix	NormalMatrix;

Texture2D SurfaceTexture;

//--------------------------------------------------------------------------------------

struct VsInput
{
    float3 Pos      : POSITION;
	float3 Normal	: NORMAL;
    float3 TexCoord : TEXCOORD;
	float4 Color	: COLOR0;
};


struct VsOutput
{
	float4 Pos		: SV_POSITION;
	float3 TexCoord	: TEXCOORD;
	float4 Color	: COLOR0;
};

struct VsWOutput
{
	float4 Pos		: SV_POSITION;
};

struct PsOutput
{
    float4 Color    : SV_Target0;
};

struct PsWOutput
{
	float4 Color    : SV_Target0;
};

//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------

VsOutput VS_COLOR(VsInput input)
{
    VsOutput output;

    output.Pos = mul(float4(input.Pos, 1.0f), ModelViewProjectionMatrix);
	output.TexCoord = input.TexCoord;
	output.Color = input.Color;

    return output;
}

VsWOutput VS_WIREFRAME(VsInput input)
{
	VsWOutput output;
	output.Pos = mul(float4(input.Pos, 1.0f), ModelViewProjectionMatrix);
	return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------


PsOutput PS_COLOR( VsOutput input )
{
    PsOutput output;
	if(input.TexCoord.z == 0.0f)
	{
		output.Color = input.Color;		
	}
	else
	{
		output.Color = SurfaceTexture.Sample(textureSampler, input.TexCoord.xy);
	}

	output.Color.a = 0.1f;
    return output;
}

PsWOutput PS_WIREFRAME(VsWOutput input)
{
	PsWOutput output;
	output.Color = float4(0.0, 1.0, 0.0, 1.0);
	return output;
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique10 RenderColor
{     
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, VS_COLOR() ) );
        SetGeometryShader(NULL);
        SetPixelShader( CompileShader( ps_4_0, PS_COLOR() ) );

		SetBlendState(AlphaBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState(DisableDepth, 0 );
        SetRasterizerState(CullNone);
    }

	pass
	{
		SetVertexShader(CompileShader(vs_4_0, VS_WIREFRAME()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_WIREFRAME()));

		SetDepthStencilState( DisableDepth, 0);
		SetRasterizerState(Wireframe);
	}
}