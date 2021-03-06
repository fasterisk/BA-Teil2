//------------------------------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------------------------------

matrix WorldViewProjection;

Texture2D FrontTexture;
Texture2D BackTexture;
Texture3D VolumeTexture;

float3 vStepSize;
int iIterations;

float3 vBBMin;
float3 vBBMax;

bool bLinearSampling;
bool bShowIsoSurface;

//------------------------------------------------------------------------------------------------------
// States
//------------------------------------------------------------------------------------------------------

DepthStencilState DisableDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
	StencilEnable = FALSE;
};

DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
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

RasterizerState CullNone
{
    MultiSampleEnable = True;
    CullMode = None;
};

RasterizerState RasterizerWireframe
{
	FillMode = WIREFRAME;
	CullMode = Back;
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


//------------------------------------------------------------------------------------------------------
// Sampler
//------------------------------------------------------------------------------------------------------

SamplerState linearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border;
    AddressV = Border;
	AddressW = Border;
    BorderColor = float4(0,0,0,0);
};

SamplerState pointSampler
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Border;
    AddressV = Border;
	AddressW = Border;
    BorderColor = float4(0,0,0,0);
};

//------------------------------------------------------------------------------------------------------
// Structs
//------------------------------------------------------------------------------------------------------

struct VsBBInput
{
	float3 pos : POSITION;
};

struct VsBBOutput
{
	float4 pos : SV_POSITION;
	float3 texC : TEXCOORD;
};

struct VsSQInput
{
	float3 pos : POSITION;
};

struct VsSQOutput
{
	float4 pos : SV_POSITION;
	float4 pos2 : TEXCOORD;
};

struct PsOutput
{
    float4 color : SV_Target;
};

//------------------------------------------------------------------------------------------------------
// Vertex Shaders
//------------------------------------------------------------------------------------------------------

VsBBOutput VS_BB_POSITION(VsBBInput input)
{
	VsBBOutput output;
	output.pos = mul(float4(input.pos, 1.0f), WorldViewProjection);
	output.texC = input.pos;
	return output;
}

VsSQOutput VS_RAYCAST(VsSQInput input)
{
	VsSQOutput output;
	output.pos = mul(float4(input.pos, 1.0f), WorldViewProjection);
	output.pos2 = output.pos;
	return output;
}

//------------------------------------------------------------------------------------------------------
// Pixel Shaders
//------------------------------------------------------------------------------------------------------

PsOutput PS_BB_WIREFRAME(VsBBOutput input)
{
	PsOutput output;
	output.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
	return output;
}

PsOutput PS_BB_POSITION(VsBBOutput input)
{
	PsOutput output;
	float3 col = (input.texC - vBBMin) / (vBBMax - vBBMin);
	output.color = float4(col, 1.0f);
	return output;
}

PsOutput PS_RAYCAST(VsSQOutput input)
{
	PsOutput output;
	float2 texC = input.pos2.xy /= input.pos2.w;
    texC.x = 0.5f*texC.x + 0.5f;
    texC.y = -0.5f*texC.y + 0.5f;
	float3 front = FrontTexture.Sample(linearSampler, texC).rgb;
	float3 back = BackTexture.Sample(linearSampler, texC).rgb;
    
    float3 dir = normalize(back - front);
	
	
    float4 pos = float4(front, 0);
    
    output.color = float4(0, 0, 0, 0);
    float4 src = 0;
    
	float3 Step = dir * vStepSize;
    
    for(int i = 0; i < iIterations; i++)
    {
		float4 pos2 = pos;
		pos2.y = 1 - pos2.y;
		if(bLinearSampling)
			src = VolumeTexture.SampleLevel(linearSampler, pos2, 0).rgba;
		else
			src = VolumeTexture.SampleLevel(pointSampler, pos2, 0).rgba;
		
		//if(!bShowIsoSurface)
		//	src.a *= 0.01;
		output.color = output.color + src;
		
		if(output.color.a >= 1.0f)
		{
			output.color.rgb /= output.color.a;
			output.color.a = 1.0f;
			break;
		}

		//advance the current position
		pos.xyz += Step;
		
		//break if the position is greater than <1, 1, 1>
		if(pos.x > 1.0f || pos.y > 1.0f || pos.z > 1.0f)
			break;
    }
    return output;
}

//------------------------------------------------------------------------------------------------------
// Techniques
//------------------------------------------------------------------------------------------------------

technique10 VolumeRendering
{
	pass BoundingBoxFront
	{
		SetVertexShader(CompileShader(vs_4_0, VS_BB_POSITION()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_BB_POSITION()));

		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(CullBack);
		SetDepthStencilState( DisableDepth, 0 );
	}

	pass BoundingBoxBack
	{
		SetVertexShader(CompileShader(vs_4_0, VS_BB_POSITION()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_BB_POSITION()));

		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(CullFront);
		SetDepthStencilState( DisableDepth, 0 );
	}

	pass RayCast
	{
		SetVertexShader(CompileShader(vs_4_0, VS_RAYCAST()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_RAYCAST()));

		SetBlendState(AlphaBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(CullBack);
		SetDepthStencilState( EnableDepth, 0 );
	}

	pass Wireframe
	{
		SetVertexShader(CompileShader(vs_4_0, VS_BB_POSITION()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_BB_WIREFRAME()));

		SetBlendState(AlphaBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetRasterizerState(RasterizerWireframe);
		SetDepthStencilState( DisableDepth, 0 );
	}
}