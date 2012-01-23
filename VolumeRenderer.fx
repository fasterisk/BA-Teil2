//------------------------------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------------------------------

matrix WorldViewProjection;

Texture2D FrontTexture;
Texture2D BackTexture;
Texture3D VolumeTexture;

float3 vStepSize;
int iIterations;

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

RasterizerState CullNone
{
    MultiSampleEnable = True;
    CullMode = None;
};

RasterizerState RasterizerWireframe
{
	FillMode = WIREFRAME;
};


//------------------------------------------------------------------------------------------------------
// Sampler
//------------------------------------------------------------------------------------------------------

SamplerState linearSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border;				// border sampling in U
    AddressV = Border;				// border sampling in V
    BorderColor = float4(0,0,0,0);	// outside of border should be black
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

VsOutput VS_POSITION(VsInput input)
{
	VsOutput output;
	output.pos = mul(float4(input.pos, 1), WorldViewProjection);
	output.posb = float4(input.pos, 1);
	return output;
}

VsOutput VS_RAYCAST(VsInput input)
{
	VsOutput output;
	output.pos = float4(input.pos, 1);
	output.posb = float4(input.pos, 1);
	return output;
}

//------------------------------------------------------------------------------------------------------
// Pixel Shaders
//------------------------------------------------------------------------------------------------------

PsOutput PS_WIREFRAME(VsOutput input)
{
	PsOutput output;
	output.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
	return output;
}

PsOutput PS_POSITION(VsOutput input)
{
	PsOutput output;
	output.color = input.posb;
	return output;
}

PsOutput PS_DIRECTION(VsOutput input)
{
	PsOutput output;
	float2 texC = input.posb.xy / input.posb.w;
	texC.x = 0.5f*texC.x + 0.5f;
	texC.y = -0.5f*texC.y + 0.5f;

	float3 front = FrontTexture.Sample(linearSampler, texC).rgb;
	float3 back = BackTexture.Sample(linearSampler, texC).rgb;

	//output.color = float4(front, 1.0f);
	//output.color = float4(back, 1.0f);
	output.color = float4(abs(back - front), 1.0f);
	return output;
}

PsOutput PS_RAYCAST(VsOutput input)
{

	PsOutput output;

	//calculate projective texture coordinates
	//used to project the front and back position textures onto the cube
	float2 texC = input.posb.xy / input.posb.w;
	texC.x =  0.5f*texC.x + 0.5f; 
	texC.y = -0.5f*texC.y + 0.5f;  
	
    float3 front = FrontTexture.Sample(linearSampler, texC).rgb;
	float3 back = BackTexture.Sample(linearSampler, texC).rgb;
    
    float3 dir = normalize(back - front);
    float4 pos = float4(front, 0);
    
    output.color = float4(0, 0, 0, 0);
    float4 src = 0;
    
    float value = 0;
	
	float3 Step = dir * vStepSize;
    
    for(int i = 0; i < iIterations; i++)
    {
		value = VolumeTexture.SampleLevel(linearSampler, front, 0).r;
				
		src = (float4)value;
		src.a *= .1f; //reduce the alpha to have a more transparent result
					  //this needs to be adjusted based on the step size
					  //i.e. the more steps we take, the faster the alpha will grow	
			
		//Front to back blending
		// dst.rgb = dst.rgb + (1 - dst.a) * src.a * src.rgb
		// dst.a   = dst.a   + (1 - dst.a) * src.a		
		src.rgb *= src.a;
		output.color = (1.0f - output.color.a)*src + output.color;		
		
		//break from the loop when alpha gets high enough
		if(output.color.a >= .95f)
			break;	
		
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
		SetVertexShader(CompileShader(vs_4_0, VS_POSITION()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_POSITION()));

		SetRasterizerState(CullBack);
		SetDepthStencilState(DisableDepth, 0);
	}

	pass BoundingBoxBack
	{
		SetVertexShader(CompileShader(vs_4_0, VS_POSITION()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_POSITION()));

		SetRasterizerState(CullFront);
		SetDepthStencilState(DisableDepth, 0);
	}

	pass Direction
	{
		SetVertexShader(CompileShader(vs_4_0, VS_RAYCAST()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_DIRECTION()));

		SetRasterizerState(CullBack);
	}

	pass RayCast
	{
		SetVertexShader(CompileShader(vs_4_0, VS_RAYCAST()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_RAYCAST()));

		SetRasterizerState(CullBack);
	}

	pass Wireframe
	{
		SetVertexShader(CompileShader(vs_4_0, VS_POSITION()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_WIREFRAME()));

		SetRasterizerState(RasterizerWireframe);
	}
}