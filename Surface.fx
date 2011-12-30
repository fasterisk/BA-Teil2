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

RasterizerState CullBack
{
    MultiSampleEnable = True;
    CullMode = Back;
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

struct PsOutput
{
    float4 Color    : SV_Target0;
    float  Depth    : SV_Target1;
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


//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------


PsOutput PS_COLOR_AND_DEPTH( VsOutput input )
{
    PsOutput output;
    output.Color = input.Color;
    output.Depth = input.Depth;
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

        SetDepthStencilState( WriteDepthTest, 0 );
        SetRasterizerState( CullBack );
    }
}

