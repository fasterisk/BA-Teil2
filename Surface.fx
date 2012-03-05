DepthStencilState WriteDepthTest
{
    // depth
    DepthWriteMask = ALL;
    DepthEnable = true;
    DepthFunc = Less;

    //stencil
    StencilEnable = false;
    StencilReadMask = 0xFF;
    StencilWriteMask = 0x00;
};

DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};

RasterizerState CullBack
{
    MultiSampleEnable = True;
    CullMode = Back;
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

BlendState NoBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
    RenderTargetWriteMask[0] = 0x0F;
};


matrix	ModelViewProjectionMatrix;
matrix	NormalMatrix;

//--------------------------------------------------------------------------------------

struct VsInput
{
    float3 Pos      : POSITION;
	float3 Normal	: NORMAL;
    float4 Color    : COLOR;
};


struct VsOutput
{
	float4 Pos		: SV_POSITION;
	float3 Normal	: NORMAL;
	float4 Color	: COLOR;
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
	output.Color = input.Color;

    return output;
}

VsWOutput VS_WIREFRAME(VsInput input)
{
	VsWOutput output;
	output.Pos = mul(float4(input.Pos, 1.0f), ModelViewProjectionMatrix);
	return output;
}


VsOutput VS_NORMAL(VsInput input)
{
	VsOutput output;
	output.Pos = float4(input.Pos, 1.0f);//mul(float4(input.Pos, 1.0f), ModelViewProjectionMatrix);
	output.Normal =  input.Normal;//mul(input.Normal, (float3x3)NormalMatrix);
	output.Color = input.Color;
	return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shaders
//--------------------------------------------------------------------------------------
[maxvertexcount(4)]
void GS_NORMAL( line VsOutput input[2], inout LineStream<VsOutput> tStream)
{
	VsOutput output;
	output.Pos = mul(input[0].Pos, ModelViewProjectionMatrix);
	output.Color = input[0].Color;
	output.Normal = mul(input[0].Normal, (float3x3)NormalMatrix);
	tStream.Append(output);
	
	output.Pos = mul(float4(input[0].Pos.xyz + input[0].Normal.xyz, 1.0f), ModelViewProjectionMatrix);
	tStream.Append(output);
	tStream.RestartStrip();
	
	output.Pos = mul(input[1].Pos, ModelViewProjectionMatrix);
	output.Color = input[1].Color;
	output.Normal = mul(input[1].Normal, (float3x3)NormalMatrix);
	tStream.Append(output);
	
	output.Pos = mul(float4(input[1].Pos.xyz + input[1].Normal.xyz, 1.0f), ModelViewProjectionMatrix);
	tStream.Append(output);
	tStream.RestartStrip();
}

//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------


PsOutput PS_COLOR( VsOutput input )
{
    PsOutput output;
    output.Color = input.Color;

	output.Color.a = 0.5f;
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
	output.Color = float4(abs(input.Normal), 1.0f);
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
        SetDepthStencilState( EnableDepth, 0 );
        SetRasterizerState( CullNone );
    }

	pass
	{
		SetVertexShader(CompileShader(vs_4_0, VS_WIREFRAME()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_WIREFRAME()));

		SetDepthStencilState( EnableDepth, 0);
		SetRasterizerState(Wireframe);
	}
}

technique10 RenderNormals
{
	pass
	{
		SetVertexShader(CompileShader(vs_4_0, VS_NORMAL()));
		SetGeometryShader(CompileShader(gs_4_0, GS_NORMAL()));
		SetPixelShader(CompileShader(ps_4_0, PS_NORMAL()));

		SetDepthStencilState( EnableDepth, 0);
		SetRasterizerState(CullNone);
	}
}
