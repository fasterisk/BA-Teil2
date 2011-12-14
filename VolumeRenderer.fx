//----------------------------------------------------------------------------------
// File:   VolumeRenderer.fx
// Author: Ignacio Llamas and Chris (Wei-Tae) Kim and Sarah Tariq 
// Email:  sdkfeedback@nvidia.com
// 
// Copyright (c) 2007 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA OR ITS SUPPLIERS
// BE  LIABLE  FOR  ANY  SPECIAL,  INCIDENTAL,  INDIRECT,  OR  CONSEQUENTIAL DAMAGES
// WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS OF BUSINESS PROFITS,
// BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
// ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
//
//----------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------
Texture3D   colorTex;
Texture2D   rayDataTex;
Texture2D   rayDataTexSmall;
Texture2D   rayCastTex;
Texture2D   sceneDepthTex;
Texture2D   edgeTex;
Texture2D   jitterTex;
Texture2D   fireTransferFunction;
Texture2D   glowTex;

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


struct VS_OUTPUT_EDGE
{
    // There's no textureUV11 because its weight is zero.
    float4 position      : SV_Position;   // vertex position
    float2 textureUV00   : TEXCOORD0;  // kernel tap texture coords 
    float2 textureUV01   : TEXCOORD1;  // kernel tap texture coords 
    float2 textureUV02   : TEXCOORD2;  // kernel tap texture coords 
    float2 textureUV10   : TEXCOORD3;  // kernel tap texture coords 
    float2 textureUV12   : TEXCOORD4;  // kernel tap texture coords 
    float2 textureUV20   : TEXCOORD5;  // kernel tap texture coords 
    float2 textureUV21   : TEXCOORD6;  // kernel tap texture coords 
    float2 textureUV22   : TEXCOORD7;  // kernel tap texture coords 
};

struct VS_OUTPUT_GLOW_9
{
    float4 position    : SV_Position;   // vertex position
    float2 textureM4   : TEXCOORD0;  // kernel tap texture coords 
    float2 textureM3   : TEXCOORD1;  // kernel tap texture coords 
    float2 textureM2   : TEXCOORD2;  // kernel tap texture coords 
    float2 textureM1   : TEXCOORD3;  // kernel tap texture coords 
    float2 texture0    : TEXCOORD4;  // kernel tap texture coords 
    float2 textureP1   : TEXCOORD5;  // kernel tap texture coords 
    float2 textureP2   : TEXCOORD6;  // kernel tap texture coords 
    float2 textureP3   : TEXCOORD7;  // kernel tap texture coords
    float2 textureP4   : TEXCOORD8;  // kernel tap texture coords  
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


// A full-screen edge detection pass to locate artifacts
VS_OUTPUT_EDGE VS_EDGE_DETECT( VS_INPUT input )
{
    VS_OUTPUT_EDGE output = (VS_OUTPUT_EDGE)0;
    output.position = float4(input.pos,1);

    float2 texelSize = 1.0 / float2(RTWidth,RTHeight);
    float2 center = float2( (input.pos.x+1)/2.0 , 1.0 - (input.pos.y+1)/2.0 );

    // Eight nearest neighbours needed for Sobel.
    output.textureUV00 = center + float2(-texelSize.x, -texelSize.y);
    output.textureUV01 = center + float2(-texelSize.x,  0);
    output.textureUV02 = center + float2(-texelSize.x,  texelSize.y);

    output.textureUV10 = center + float2(0, -texelSize.y);
    output.textureUV12 = center + float2(0,  texelSize.y);

    output.textureUV20 = center + float2(texelSize.x, -texelSize.y);
    output.textureUV21 = center + float2(texelSize.x,  0);
    output.textureUV22 = center + float2(texelSize.x,  texelSize.y);

    return output;
}


// A full-screen glow pass in the horizontal direction
VS_OUTPUT_GLOW_9 VS_GLOW_H_9( VS_INPUT input )
{
    VS_OUTPUT_GLOW_9 output;
    output.position = float4(input.pos,1);

    float texelSize = 1.0 / RTWidth;
    float2 center = float2( (input.pos.x+1)/2.0 , 1.0 - (input.pos.y+1)/2.0 );

    // 9 taps for a guassian filter with sigma of 3
    output.textureM4 = center + float2(-texelSize*4.0f, 0);
    output.textureM3 = center + float2(-texelSize*3.0f, 0);
    output.textureM2 = center + float2(-texelSize*2.0f, 0);
    output.textureM1 = center + float2(-texelSize, 0);
    output.texture0  = center;
    output.textureP1 = center + float2(texelSize, 0);
    output.textureP2 = center + float2(texelSize*2.0f, 0);
    output.textureP3 = center + float2(texelSize*3.0f, 0);
    output.textureP4 = center + float2(texelSize*4.0f, 0);

    
    return output;
}

// A full-screen glow pass in the vertical direction
VS_OUTPUT_GLOW_9 VS_GLOW_V_9( VS_INPUT input )
{
    VS_OUTPUT_GLOW_9 output;
    output.position = float4(input.pos,1);

    float texelSize = 1.0 / RTHeight;
    float2 center = float2( (input.pos.x+1)/2.0 , 1.0 - (input.pos.y+1)/2.0 );

    // 9 taps for a guassian filter with sigma of 3
    output.textureM4 = center + float2(0,-texelSize*4.0f);
    output.textureM3 = center + float2(0,-texelSize*3.0f);
    output.textureM2 = center + float2(0,-texelSize*2.0f);
    output.textureM1 = center + float2(0,-texelSize);
    output.texture0  = center;
    output.textureP1 = center + float2(0,texelSize);
    output.textureP2 = center + float2(0,texelSize*2.0f);
    output.textureP3 = center + float2(0,texelSize*3.0f);
    output.textureP4 = center + float2(0,texelSize*4.0f);
    
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
    
    if(!renderFire)
    {    
        //render smoke with front to back blending
        float t;
        float4 sample = weight * colorTex.SampleLevel(samLinearClamp, texcoords, 0);
        sample.a = (sample.r) * 0.1;
        t = sample.a * (1.0-color.a);
        color.rgb += t * sample.r;
        color.a += t;
    }
    else
    {
        //render fire and smoke with back to front blending 
        
        //dont render the area below where the fire originates
        if(O.z < OBSTACLE_MAX_HEIGHT/gridDim.z)
            return;
        
        //this is the threshold at which we decide whether to render fire or smoke
        float threshold = 1.4;
        float maxValue = 3;
        
        float s = colorTex.SampleLevel(samLinearClamp, texcoords, 0).x;
        s = clamp(s,0,maxValue);
          
        if(s>threshold)   
        {   
            //render fire
            float lookUpVal = ( (s-threshold)/(maxValue-threshold) );
            lookUpVal = 1.0 - pow(lookUpVal,RednessFactor);
            lookUpVal = clamp(lookUpVal,0,1);
            float3 interpColor = fireTransferFunction.SampleLevel(samLinearClamp,float2(lookUpVal,0),0); 
            float mult = (s-threshold);
            color += float4(weight*interpColor.rgb,weight*mult*mult*fireAlphaMultiplier); 
        }
        else    
        {    
             //render smoke
             float4 sample = weight*s;
             sample.a = sample.r*0.1*smokeAlphaMultiplier;
             float3 smokeColor = float3(0.9,0.35,0.055);
             color.rgb = (1 - sample.a) * color.rgb + sample.a * sample.rrr * smokeColor * smokeColorMultiplier * 5.0; 
             color.a = (1 - sample.a) * color.a + sample.a;
        }        
    }
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
    float Offset = jitterTex.Sample( samRepeat, input.pos.xy / 256.0 ).r;
    float rayLength = rayData.w;

    // Sample twice per voxel
    float fSamples = ( rayLength / gridScaleFactor * maxGridDim ) * 2.0;
    int nSamples = floor(fSamples);
    float3 stepVec = normalize( (rayOrigin - eyeOnGrid) * gridDim ) * recGridDim * 0.5;
   
    float3 O = rayOrigin + stepVec*Offset;
    
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

        if(!renderFire)
        {
            // If doing front-to-back blending we can do early exit when opacity saturates
            if( color.a > 0.99 )
                break;
        }
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

float EdgeDetectScalar(float sx, float sy, float threshold)
{
    float dist = (sx*sx+sy*sy);
    float e = (dist > threshold*ZFar)? 1: 0;
    return e;
}

//
// A full-screen edge detection pass to locate artifacts
//  these artifacts are located on a downsized version of the rayDataTexture
// We use a smaller texture both to accurately find all the depth artifacts 
//  when raycasting to this smaller size and to save on the cost of this pass
// Use col.a to find depth edges of objects occluding the smoke
// Use col.g to find the edges where the camera near plane cuts the smoke volume
//
float4 PS_EDGE_DETECT(VS_OUTPUT_EDGE vIn) : SV_Target
{

    // We need eight samples (the centre has zero weight in both kernels).
    float4 col;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV00); 
    float g00 = col.a;
    if(col.g < 0)
        g00 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV01); 
    float g01 = col.a;
    if(col.g < 0)
        g01 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV02); 
    float g02 = col.a;
    if(col.g < 0)
        g02 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV10); 
    float g10 = col.a;
    if(col.g < 0)
        g10 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV12); 
    float g12 = col.a;
    if(col.g < 0)
        g12 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV20); 
    float g20 = col.a;
    if(col.g < 0)
        g20 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV21); 
    float g21 = col.a;
    if(col.g < 0)
        g21 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV22); 
    float g22 = col.a;
    if(col.g < 0)
        g22 *= -1;
    	
    // Sobel in horizontal dir.
    float sx = 0;
    sx -= g00;
    sx -= g01 * 2;
    sx -= g02;
    sx += g20;
    sx += g21 * 2;
    sx += g22;
    // Sobel in vertical dir - weights are just rotated 90 degrees.
    float sy = 0;
    sy -= g00;
    sy += g02;
    sy -= g10 * 2;
    sy += g12 * 2;
    sy -= g20;
    sy += g22;

    float e = EdgeDetectScalar(sx, sy, edgeThreshold);
    return float4(e,e,e,1);

}


float4 PS_GLOW_9(VS_OUTPUT_GLOW_9 vIn) : SV_Target
{
    float4 col = float4(0,0,0,0);
    float4 tex;
    float threshold = 1.4;

    tex = glowTex.Sample(samPointClamp, vIn.textureM4); 
    if(tex.r > threshold)
        col += tex*gaussian_3[4];
    
    tex = glowTex.Sample(samPointClamp, vIn.textureM3); 
    if(tex.r > threshold)
        col += tex*gaussian_3[3];
        
    tex = glowTex.Sample(samPointClamp, vIn.textureM2); 
    if(tex.r > threshold)
        col += tex*gaussian_3[2]; 
        
    tex = glowTex.Sample(samPointClamp, vIn.textureM1); 
    if(tex.r > threshold)
        col += tex*gaussian_3[1]; 
        
    tex = glowTex.Sample(samPointClamp, vIn.texture0); 
    if(tex.r > threshold)
        col += tex*gaussian_3[0];
        
    tex = glowTex.Sample(samPointClamp, vIn.textureP1); 
    if(tex.r > threshold)
        col += tex*gaussian_3[1]; 
      
    tex = glowTex.Sample(samPointClamp, vIn.textureP2); 
    if(tex.r > threshold)
        col += tex*gaussian_3[2]; 

    tex = glowTex.Sample(samPointClamp, vIn.textureP3); 
    if(tex.r > threshold)
        col += tex*gaussian_3[3];
       
    tex = glowTex.Sample(samPointClamp, vIn.textureP4); 
    if(tex.r > threshold)
        col += tex*gaussian_3[4];
       
    return col;
}



float4 PS_RAYCASTCOPY_QUAD_SMOKE(PS_INPUT_RAYCAST input) : SV_Target
{
    float edge = edgeTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight)).r;
    float4 tex = rayCastTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));
    
    if(edge > 0 && tex.a > 0)
        return Raycast(input,false);
    else
        return tex;
}


float4 PS_RAYCASTCOPY_QUAD_FIRE(PS_INPUT_RAYCAST input) : SV_Target
{
    float edge = edgeTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight)).r;
    float4 tex = rayCastTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));
         
    float4 color;
    
    if(edge > 0 && tex.a > 0)
        color = Raycast(input,true);
    else
        color = tex;

    if(useGlow)    
    {
        float4 glow = glowTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));
        color.rgba += glowContribution*glow.rgba;
    }

    //tone map
    color.rgb /= finalIntensityScale;

    color.a = clamp(color.a,0,1);
    color.rgb *= color.a; 
    color.a = color.a*finalAlphaScale;
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
         
    pass QuadEdgeDetect
    {              
        SetVertexShader(CompileShader( vs_4_0, VS_EDGE_DETECT() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_EDGE_DETECT() ));
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetRasterizerState(CullNone);
        SetDepthStencilState( DisableDepth, 0 );

    }
    
     pass GlowHorizontal
    {
        SetVertexShader(CompileShader( vs_4_0, VS_GLOW_H_9() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_GLOW_9() ));
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetRasterizerState(CullBack);
        SetDepthStencilState( DisableDepth, 0 );
    } 
    
     pass GlowVertical
    {
        SetVertexShader(CompileShader( vs_4_0, VS_GLOW_V_9() ));
        SetGeometryShader ( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_GLOW_9() ));
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

