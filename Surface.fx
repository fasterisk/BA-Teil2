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


matrix    ModelViewProjectionMatrix;

//--------------------------------------------------------------------------------------

struct VsInput
{
    float3 Pos      : POSITION;
    float4 Color    : COLOR;
};


struct VsOutput
{
	float4 Pos		: SV_POSITION;
	float4 Color	: COLOR;
    float  Depth   : TEXCOORD0;
};

struct VsWOutput
{
	float4 Pos		: SV_POSITION;
};

struct PsOutput
{
    float4 Color    : SV_Target0;
    float  Depth    : SV_Target1;
};

struct PsWOutput
{
	float4 Color    : SV_Target0;
};

//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------

VsOutput VS_COLOR_AND_DEPTH(VsInput input)
{
    VsOutput output;

    output.Pos = mul(float4(input.Pos, 1.0f), ModelViewProjectionMatrix);
	output.Color = input.Color;
    output.Depth = output.Pos.z;

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


PsOutput PS_COLOR_AND_DEPTH( VsOutput input )
{
    PsOutput output;
    output.Color = input.Color;
    output.Depth = input.Depth;

	output.Color.a = 0.5f;
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

technique10 RenderColorAndDepth
{     
    pass
    {
        SetVertexShader( CompileShader( vs_4_0, VS_COLOR_AND_DEPTH() ) );
        SetGeometryShader(NULL);
        SetPixelShader( CompileShader( ps_4_0, PS_COLOR_AND_DEPTH() ) );

		SetBlendState(AlphaBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetDepthStencilState( WriteDepthTest, 0 );
        SetRasterizerState( CullBack );
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
