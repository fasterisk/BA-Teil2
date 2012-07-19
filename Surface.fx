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
    float2 TexCoord : TEXCOORD;
};


struct VsOutput
{
	float4 Pos		: SV_POSITION;
	float3 Normal	: NORMAL;
	float2 TexCoord	: TEXCOORD;
};

struct VsNormalOutput
{
	float4 Pos		: SV_POSITION;
	float3 Normal	: NORMAL;
	float2 TexCoord	: TEXCOORD;
	float3 Pos2		: TEXTURE0;
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
	output.Normal = mul(input.Normal, (float3x3)NormalMatrix);
	output.TexCoord = input.TexCoord;

    return output;
}

VsWOutput VS_WIREFRAME(VsInput input)
{
	VsWOutput output;
	output.Pos = mul(float4(input.Pos, 1.0f), ModelViewProjectionMatrix);
	return output;
}

VsNormalOutput VS_NORMAL(VsInput input)
{
	VsNormalOutput output;
	output.Pos = float4(input.Pos, 1.0f);//mul(float4(input.Pos, 1.0f), ModelViewProjectionMatrix);
	output.Normal =  input.Normal;//mul(input.Normal, (float3x3)NormalMatrix);
	output.TexCoord = input.TexCoord;
	output.Pos2 = input.Pos;
	return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shaders
//--------------------------------------------------------------------------------------
[maxvertexcount(6)]
void GS_NORMAL( triangle VsOutput input[3], inout LineStream<VsOutput> tStream)
{
	VsOutput output;
	output.Pos = mul(input[0].Pos, ModelViewProjectionMatrix);
	output.Normal = mul(input[0].Normal, (float3x3)NormalMatrix);
	output.TexCoord = input[0].TexCoord;
	tStream.Append(output);

	output.Pos = mul(float4(input[0].Pos.xyz + input[0].Normal.xyz*5, 1.0f), ModelViewProjectionMatrix);
	tStream.Append(output);
	tStream.RestartStrip();

	output.Pos = mul(input[1].Pos, ModelViewProjectionMatrix);
	output.Normal = mul(input[1].Normal, (float3x3)NormalMatrix);
	tStream.Append(output);

	output.Pos = mul(float4(input[1].Pos.xyz + input[1].Normal.xyz*5, 1.0f), ModelViewProjectionMatrix);
	tStream.Append(output);
	tStream.RestartStrip();

	output.Pos = mul(input[2].Pos, ModelViewProjectionMatrix);
	output.Normal = mul(input[2].Normal, (float3x3)NormalMatrix);
	tStream.Append(output);

	output.Pos = mul(float4(input[2].Pos.xyz + input[2].Normal.xyz*5, 1.0f), ModelViewProjectionMatrix);
	tStream.Append(output);
	tStream.RestartStrip();
}


//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------


PsOutput PS_COLOR( VsOutput input )
{
    PsOutput output;
    output.Color = SurfaceTexture.Sample(textureSampler, input.TexCoord);

	output.Color.a = 0.1f;
    return output;
}

PsWOutput PS_WIREFRAME(VsWOutput input)
{
	PsWOutput output;
	output.Color = float4(0.0, 1.0, 0.0, 1.0);
	return output;
}

PsOutput PS_NORMAL(VsOutput input)
{
	PsOutput output;
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
        SetDepthStencilState( DisableDepth, 0 );
        SetRasterizerState(CullNone);
    }

	/*pass
	{
		SetVertexShader(CompileShader(vs_4_0, VS_WIREFRAME()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_WIREFRAME()));

		SetDepthStencilState( DisableDepth, 0);
		SetRasterizerState(Wireframe);
	}

	pass
	{
		SetVertexShader(CompileShader(vs_4_0, VS_NORMAL()));
		SetGeometryShader(CompileShader(gs_4_0, GS_NORMAL()));
		SetPixelShader(CompileShader(ps_4_0, PS_NORMAL()));

		SetDepthStencilState( EnableDepth, 0);
		SetRasterizerState(CullNone);
	}*/
}