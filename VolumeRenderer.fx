//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------
Texture3D   colorTex;
Texture2D   rayDataTex;
Texture2D   rayDataTexSmall;
Texture2D   rayCastTex;
Texture2D   sceneDepthTex;

//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------
float       RTWidth;
float       RTHeight;

float4x4    WorldViewProjection;
float4x4    InvWorldViewProjection; 
float4x4    WorldView;

float       ZNear = 0.05f;
float       ZFar = 1000.0f;

float3      gridDim;
float3      recGridDim;
float       maxGridDim;
float       gridScaleFactor = 1.0;
float3      eyeOnGrid;

float       edgeThreshold = 0.2;

float       tan_FovXhalf;
float       tan_FovYhalf;

//gaussian with a sigma of 3, and a miu of 0
float gaussian_3[5] =
{
    0.132981, 0.125794, 0.106483, 0.080657, 0.05467,
};

bool useGlow               = true;
float glowContribution     = 0.81f;
float finalIntensityScale  = 22.0f;
float finalAlphaScale      = 0.95f;
float smokeColorMultiplier = 2.0f;   
float smokeAlphaMultiplier = 0.1f; 
float fireAlphaMultiplier  = 0.4; 
int   RednessFactor        = 5; 

//--------------------------------------------------------------------------------------
// Pipeline State definitions
//--------------------------------------------------------------------------------------
SamplerState samPointClamp
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};             

SamplerState samLinearClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

SamplerState samRepeat
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

DepthStencilState DisableDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
};


BlendState FireBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = ONE;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = ONE;
    DestBlendAlpha = ONE;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
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

BlendState SubtractiveBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = ONE;
    DestBlend = ZERO;
    BlendOp = REV_SUBTRACT;         // DST - SRC
    SrcBlendAlpha = ONE;
    DestBlendAlpha = ONE;
    BlendOpAlpha = REV_SUBTRACT;    // DST - SRC
    RenderTargetWriteMask[0] = 0x0F;
};

RasterizerState CullNone
{
    MultiSampleEnable = False;
    CullMode = None;
};

RasterizerState CullFront
{
    MultiSampleEnable = False;
    CullMode = Front;
};

RasterizerState CullBack
{
    MultiSampleEnable = False;
    CullMode = Back;
};


//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float3 pos      : POSITION;
};

struct PS_INPUT_RAYDATA_BACK
{
    float4 pos      : SV_Position;
    float3 worldViewPos : TEXCOORD0;

};

struct PS_INPUT_RAYDATA_FRONT
{
    float4 pos      : SV_Position;
    float3 posInGrid: POSITION;
    float3 worldViewPos : TEXCOORD0;
};

struct PS_INPUT_RAYCAST
{
    float4 pos      : SV_Position;
    float3 posInGrid: POSITION;
};


//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------
PS_INPUT_RAYDATA_BACK VS_RAYDATA_BACK(VS_INPUT input)
{
    PS_INPUT_RAYDATA_BACK output = (PS_INPUT_RAYDATA_BACK)0;
    output.pos = mul(float4(input.pos,1), WorldViewProjection);
    output.worldViewPos = mul(float4(input.pos,1), WorldView).xyz;
    return output;
}

PS_INPUT_RAYDATA_FRONT VS_RAYDATA_FRONT(VS_INPUT input)
{
    PS_INPUT_RAYDATA_FRONT output = (PS_INPUT_RAYDATA_FRONT)0;
    output.pos =  mul(float4(input.pos,1), WorldViewProjection);
    output.posInGrid = input.pos;
    output.worldViewPos = mul(float4(input.pos,1), WorldView).xyz;
    return output;
}

PS_INPUT_RAYCAST VS_RAYCAST_QUAD (VS_INPUT input)
{
    PS_INPUT_RAYCAST output = (PS_INPUT_RAYCAST)0;
    output.pos = float4(input.pos,1);
    output.posInGrid = mul( float4( input.pos.xy*ZNear, 0, ZNear ), InvWorldViewProjection );
    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------
#define OCCLUDED_PIXEL_RAYVALUE     float4(1, 0, 0, 0)
#define NEARCLIPPED_PIXEL_RAYPOS    float3(0, -1, 0)

float4 PS_RAYDATA_BACK(PS_INPUT_RAYDATA_BACK input) : SV_Target
{
    float4 output;
    
    //get the distance from the eye to the scene point
    //we do this by unprojecting the scene point to view space and then taking the length of the vector
    float2 normalizedInputPos = float2(input.pos.x/RTWidth, input.pos.y/RTHeight);
    float sceneZ = sceneDepthTex.SampleLevel( samLinearClamp, normalizedInputPos ,0).r;
    float2 inputPos = float2((normalizedInputPos.x*2.0)-1.0,(normalizedInputPos.y*2.0)-1.0);
    sceneZ = length(float3( inputPos.x * sceneZ * tan_FovXhalf, inputPos.y * sceneZ * tan_FovYhalf, sceneZ ));
    
    float inputDepth = length(input.worldViewPos);

    // This value will only remain if no fragments get blended on top in the next pass (front-faces)
    //  which would happen if the front faces of the box get clipped by the near plane of the camera
    output.xyz = NEARCLIPPED_PIXEL_RAYPOS;

    output.w = min(inputDepth, sceneZ);
    return output;
}

float4 PS_RAYDATA_FRONT(PS_INPUT_RAYDATA_FRONT input) : SV_Target
{
    float4 output;
    
    float2 normalizedInputPos = float2(input.pos.x/RTWidth, input.pos.y/RTHeight);
    float sceneZ = sceneDepthTex.SampleLevel( samLinearClamp, normalizedInputPos, 0).r;
    float2 inputPos = float2((normalizedInputPos.x*2.0)-1.0,(normalizedInputPos.y*2.0)-1.0);
    sceneZ = length(float3( inputPos.x * sceneZ * tan_FovXhalf, inputPos.y * sceneZ * tan_FovYhalf, sceneZ ));
    
    float inputDepth = length(input.worldViewPos);
    
    if(sceneZ < inputDepth)
    {
        // If the scene occludes intersection point we want to kill the pixel early in PS
        return OCCLUDED_PIXEL_RAYVALUE;
    }
    // We negate input.posInGrid because we use subtractive blending in front faces
    //  Note that we set xyz to 0 when rendering back faces
    output.xyz = -input.posInGrid;
    output.w = inputDepth;
    return output;
}

float4 PS_RAYDATA_FRONT_NOBLEND(PS_INPUT_RAYDATA_FRONT input) : SV_Target
{
    float4 output;
    
    float2 normalizedInputPos = float2(input.pos.x/RTWidth, input.pos.y/RTHeight);
    float sceneZ = sceneDepthTex.SampleLevel(samLinearClamp, normalizedInputPos,0).r;
    float2 inputPos = float2((normalizedInputPos.x*2.0)-1.0,(normalizedInputPos.y*2.0)-1.0);
    sceneZ = length(float3( inputPos.x * sceneZ * tan_FovXhalf, inputPos.y * sceneZ * tan_FovYhalf, sceneZ ));
    
    float inputDepth = length(input.worldViewPos);
    
    if(sceneZ < inputDepth)
    {
        // If the scene occludes intersection point we want to kill the pixel early in PS
        return OCCLUDED_PIXEL_RAYVALUE;
    }

    float4 rayDataBackDepth = rayDataTex.SampleLevel(samLinearClamp, float2(input.pos.x/RTWidth, input.pos.y/RTHeight),0).w;

    // We negate input.posInGrid because we use subtractive blending in front faces
    //  Note that we set xyz to 0 when rendering back faces
    output.xyz = input.posInGrid;
    output.w = rayDataBackDepth - inputDepth;

    return output;
}


#define OBSTACLE_MAX_HEIGHT 4
void DoSample(float weight, float3 O, inout float4 color, uniform bool renderFire )
{
    
    float3 texcoords;
    texcoords = float3( O.x, 1 - O.y, O.z) ;
    
    float t;
    float4 sample = weight * colorTex.SampleLevel(samLinearClamp, texcoords, 0);
    sample.a = (sample.r) * 0.1;
    t = sample.a * (1.0-color.a);
    color.rgb += t * sample.r;
    color.a += t;
}


float4 Raycast( PS_INPUT_RAYCAST input,uniform bool renderFire )
{
    float4 color = 0;
    float2 normalizedInputPos = float2(input.pos.x/RTWidth,input.pos.y/RTHeight);
    float4 rayData = rayDataTex.Sample(samLinearClamp, normalizedInputPos);

    // Don't raycast if the starting position is negative 
    //   (see use of OCCLUDED_PIXEL_RAYVALUE in PS_RAYDATA_FRONT)
    if(rayData.x < 0)
        return color;

    // If the front face of the box was clipped here by the near plane of the camera
    //   (see use of NEARCLIPPED_PIXEL_RAYPOS in PS_RAYDATA_BACK)
    if(rayData.y < 0)
    {
       // Initialize the position of the fragment and adjust the depth
       rayData.xyz = input.posInGrid;
       float2 inputPos = float2((normalizedInputPos.x*2.0)-1.0,(normalizedInputPos.y*2.0)-1.0);
       float distanceToNearPlane = length(float3( inputPos.x * ZNear * tan_FovXhalf, inputPos.y * ZNear * tan_FovYhalf, ZNear ));
       rayData.w = rayData.w - distanceToNearPlane;
    }

    float3 rayOrigin = rayData.xyz;
    float rayLength = rayData.w;

    // Sample twice per voxel
    float fSamples = ( rayLength / gridScaleFactor * maxGridDim ) * 2.0;
    int nSamples = floor(fSamples);
    float3 stepVec = normalize( (rayOrigin - eyeOnGrid) * gridDim ) * recGridDim * 0.5;
   
    float3 O = rayOrigin + stepVec;
    
    if(renderFire)
    {
        // we render fire with back to front ray marching 
        // In back-to-front blending we start raycasting from the surface point and step towards the eye
        O += fSamples * stepVec;
        stepVec = -stepVec;
    }

    for( int i=0; i<nSamples ; i++ )
    {
        DoSample(1, O, color, renderFire);
        O += stepVec;

    }

    // The last sample is weighted by the fractional part of the ray length in voxel 
    //  space (fSamples), thus avoiding banding artifacts when the smoke is blended against the scene
    if( i == nSamples )
    {
        DoSample(frac(fSamples), O, color, renderFire);
    }
    
    return color;
}


float4 PS_RAYDATACOPY_QUAD(PS_INPUT_RAYCAST input) : SV_Target
{
    return rayDataTex.Sample(samPointClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));
}


float4 PS_RAYCAST_QUAD(PS_INPUT_RAYCAST input, uniform bool renderFire) : SV_Target
{
    return Raycast(input,renderFire);
}


float4 PS_RAYCASTCOPY_QUAD_SMOKE(PS_INPUT_RAYCAST input) : SV_Target
{
    float4 tex = rayCastTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));
    
    if(tex.a > 0)
        return Raycast(input,false);
    else
        return tex;
}


float4 PS_RAYCASTCOPY_QUAD_FIRE(PS_INPUT_RAYCAST input) : SV_Target
{
    float4 tex = rayCastTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));
         
    float4 color;
    
    if(tex.a > 0)
        color = Raycast(input,true);
    else
        color = tex;

    return color;
}

//------------------------------------------------------------------------------------------------------
//techniques
//------------------------------------------------------------------------------------------------------
 
technique10 VolumeRenderer
{
    pass CompRayData_Back
    {
        SetVertexShader(CompileShader( vs_4_0, VS_RAYDATA_BACK() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_RAYDATA_BACK() ));
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetRasterizerState(CullFront);
        SetDepthStencilState( DisableDepth, 0 );
    }

    pass CompRayData_Front
    {
        SetVertexShader(CompileShader( vs_4_0, VS_RAYDATA_FRONT() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_RAYDATA_FRONT() ));
        SetBlendState (SubtractiveBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
        SetRasterizerState(CullBack);
        SetDepthStencilState( DisableDepth, 0 );
    }

    pass CompRayData_FrontNOBLEND
    {
        SetVertexShader(CompileShader( vs_4_0, VS_RAYDATA_FRONT() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_RAYDATA_FRONT_NOBLEND() ));
        SetBlendState (NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
        SetRasterizerState(CullBack);
        SetDepthStencilState( DisableDepth, 0 );
    }
        
    pass QuadDownSampleRayDataTexture
    {
        SetVertexShader(CompileShader( vs_4_0, VS_RAYCAST_QUAD() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_RAYDATACOPY_QUAD() ));
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetRasterizerState(CullBack);
        SetDepthStencilState( DisableDepth, 0 );
    }

    pass QuadRaycastSmoke
    {
        SetVertexShader(CompileShader( vs_4_0, VS_RAYCAST_QUAD() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_RAYCAST_QUAD(false) ));
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetRasterizerState(CullBack);
        SetDepthStencilState( DisableDepth, 0 );
    }  
    
    pass QuadRaycastFire
    {
        SetVertexShader(CompileShader( vs_4_0, VS_RAYCAST_QUAD() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_RAYCAST_QUAD(true) ));
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetRasterizerState(CullBack);
        SetDepthStencilState( DisableDepth, 0 );
    }  
         
    pass QuadRaycastCopySmoke
    {
        SetVertexShader(CompileShader( vs_4_0, VS_RAYCAST_QUAD() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_RAYCASTCOPY_QUAD_SMOKE() ));
        SetBlendState( AlphaBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetRasterizerState(CullBack);
        SetDepthStencilState( DisableDepth, 0 );
    }
 
    pass QuadRaycastCopyFire
    {
        SetVertexShader(CompileShader( vs_4_0, VS_RAYCAST_QUAD() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_RAYCASTCOPY_QUAD_FIRE() ));
        SetBlendState( FireBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetRasterizerState(CullBack);
        SetDepthStencilState( DisableDepth, 0 );
    }
}

