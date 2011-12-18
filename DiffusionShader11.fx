//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------
Texture3D Texture_diffuse0;
Texture3D Texture_diffuse1;

//--------------------------------------------------------------------------------------
// Variables / Matrices
//--------------------------------------------------------------------------------------
matrix g_mModelViewProjection;

float textureHeight;
float textureWidth;
float textureDepth;

//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------
SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

BlendState NoBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
    RenderTargetWriteMask[0] = 0x0F;
};

DepthStencilState DisableDepth
{
        DepthEnable = false;
        DepthWriteMask = ZERO;
        DepthFunc = Less;

        //Stencil
        StencilEnable = false;
        StencilReadMask = 0xFF;
        StencilWriteMask = 0x00;
};

RasterizerState CullNone
{
    MultiSampleEnable = False;
    CullMode=None;
};

DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};


//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------

struct VS_INPUT_DIFFUSE
{
    float3 position          : POSITION;    // 2D slice vertex coordinates in clip space
    float3 textureCoords0    : TEXCOORD;    // 3D cell coordinates (x,y,z in 0-dimension range)
};

struct VS_OUTPUT
{
	float4 position	: SV_POSITION;
	float4 color : COLOR;
};

struct VS_OUTPUT_DIFFUSE
{
    float4 pos               : SV_Position;
    float3 cell0             : TEXCOORD0;
    float3 texcoords         : TEXCOORD1;
    float2 LR                : TEXCOORD2;
    float2 BT                : TEXCOORD3;
    float2 DU                : TEXCOORD4;
};

struct GS_OUTPUT_DIFFUSE
{
    float4 pos               : SV_Position; // 2D slice vertex coordinates in homogenous clip space
    float3 cell0             : TEXCOORD0;   // 3D cell coordinates (x,y,z in 0-dimension range)
    float3 texcoords         : TEXCOORD1;   // 3D cell texcoords (x,y,z in 0-1 range)
    float2 LR                : TEXCOORD2;   // 3D cell texcoords for the Left and Right neighbors
    float2 BT                : TEXCOORD3;   // 3D cell texcoords for the Bottom and Top neighbors
    float2 DU                : TEXCOORD4;   // 3D cell texcoords for the Down and Up neighbors
    uint RTIndex             : SV_RenderTargetArrayIndex;  // used to choose the destination slice
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain(float3 position : POSITION, float4 color : COLOR)
{
	VS_OUTPUT Output;
	
	Output.position = mul(float4(position.x, position.y, position.z, 1.0),g_mModelViewProjection);
	Output.color = color;
	
	return Output;
}

VS_OUTPUT_DIFFUSE VS_GRID( VS_INPUT_DIFFUSE input)
{
    VS_OUTPUT_DIFFUSE output = (VS_OUTPUT_DIFFUSE)0;

    output.pos = float4(input.position.x, input.position.y, input.position.z, 1.0);
    output.cell0 = float3(input.textureCoords0.x, input.textureCoords0.y, input.textureCoords0.z);
    output.texcoords = float3( (input.textureCoords0.x)/(textureWidth),
                              (input.textureCoords0.y)/(textureHeight), 
                              (input.textureCoords0.z+0.5)/(textureDepth));

    float x = output.texcoords.x;
    float y = output.texcoords.y;
    float z = output.texcoords.z;

    // compute single texel offsets in each dimension
    float invW = 1.0/textureWidth;
    float invH = 1.0/textureHeight;
    float invD = 1.0/textureDepth;

    output.LR = float2(x - invW, x + invW);
    output.BT = float2(y - invH, y + invH);
    output.DU = float2(z - invD, z + invD);

    return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------
[maxvertexcount (3)]
void GS_ARRAY(triangle VS_OUTPUT_DIFFUSE In[3], inout TriangleStream<GS_OUTPUT_DIFFUSE> triStream)
{
    GS_OUTPUT_DIFFUSE Out;
    // cell0.z of the first vertex in the triangle determines the destination slice index
    Out.RTIndex = In[0].cell0.z;
    for(int v=0; v<3; v++)
    {
        Out.pos          = In[v].pos; 
        Out.cell0        = In[v].cell0;
        Out.texcoords    = In[v].texcoords;
        Out.LR           = In[v].LR;
        Out.BT           = In[v].BT;
        Out.DU           = In[v].DU;
        triStream.Append( Out );
    }
    triStream.RestartStrip( );
}

[maxvertexcount (2)]
void GS_ARRAY_LINE(line VS_OUTPUT_DIFFUSE In[2], inout LineStream<GS_OUTPUT_DIFFUSE> Stream)
{
    GS_OUTPUT_DIFFUSE Out;
    // cell0.z of the first vertex in the line determines the destination slice index
    Out.RTIndex = In[0].cell0.z;
    for(int v=0; v<2; v++)
    {
        Out.pos          = In[v].pos; 
        Out.cell0        = In[v].cell0;
        Out.texcoords    = In[v].texcoords;
        Out.LR           = In[v].LR;
        Out.BT           = In[v].BT;
        Out.DU           = In[v].DU;

        Stream.Append( Out );
    }
    Stream.RestartStrip( );
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain(VS_OUTPUT vsOutput) : SV_TARGET
{
	return vsOutput.color;
}

float4 PS_GRID( GS_OUTPUT_DIFFUSE input ) : SV_Target
{
    return Texture_diffuse0.SampleLevel( samLinear, input.cell0, 0);
}



//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------



technique10 RenderSurfacesToTexture
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_4_0, VSMain()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PSMain()));

		SetDepthStencilState( EnableDepth, 0);
	}
}

technique10 Grid
{
	pass
	{
		SetVertexShader(CompileShader(vs_4_0, VS_GRID()));
		SetGeometryShader(CompileShader(gs_4_0, GS_ARRAY()));
		SetPixelShader(CompileShader(ps_4_0, PS_GRID()));

		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetDepthStencilState(DisableDepth, 0);
		SetRasterizerState(CullNone);
	}
}