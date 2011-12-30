//----------------------------------------------------------------------------------
// File:   Voxelizer.fx
// Author: Ignacio Llamas
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
Texture2D<uint2>      stencilbufferTex2D;

//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------
float4x4    WorldViewProjection : WORLDVIEWPROJECTION;

float2 projSpacePixDim; // the dimensions of a pixel in projection space i.e. (2.0/rtWidth, 2.0/rtHeight)
float3 gridDim;
float recTimeStep;

int sliceIdx;       // index of the slice we want to output into
float sliceZ;       // z in the range [-0.5, 0.5] for the slice we are outputting into
float velocityMultiplier = 1.0; //if rendering fire we reduce the effect of the mesh on the velocity field to 20%

//--------------------------------------------------------------------------------------
// Pipeline State definitions
//--------------------------------------------------------------------------------------

// RasterizerState 
RasterizerState RS_CullDisabled
{
  MultiSampleEnable = False;
  CullMode = None;
  ScissorEnable = true;
};


// BlendState
BlendState BS_NoBlending
{
  BlendEnable[0] = false;
  RenderTargetWriteMask[0] = 0x0F;
};


// DepthStencilState
DepthStencilState DSS_NonZeroRule
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
    
    //stencil
    StencilEnable = true;
    StencilReadMask = 0x00;
    StencilWriteMask = 0xFF;
    FrontFaceStencilFunc = Always;
    FrontFaceStencilPass = Decr;
    FrontFaceStencilFail = Keep;
    BackFaceStencilFunc = Always;
    BackFaceStencilPass = Incr;
    BackFaceStencilFail = Keep;
    
};

DepthStencilState DSS_Disabled
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
    
    //stencil
    StencilEnable = FALSE;
    StencilReadMask = 0x00;
    StencilWriteMask = 0x00;
};


//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------

// For Technique VoxelizeNZ
struct VsVoxInput
{
    float3 Pos          : POSITION;
};

struct VsVoxOutput
{
    float4 Pos	        : SV_Position;
};


// For technique VoxelizeResolveWithPS
struct VsResInput
{
    float3 Pos          : POSITION;
    float3 Tex          : TEXCOORD;
};

struct VsResOutput
{
    float4 Pos          : POSITION;
    float3 Tex          : TEXCOORD;
};

struct GsResOutput
{
    float4 Pos          : SV_Position;
    float3 Tex          : TEXCOORD;
    uint   RTIndex      : SV_RenderTargetArrayIndex;
};


//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------

VsVoxOutput VS_VOXELIZE( VsVoxInput input )
{
    VsVoxOutput output;
    output.Pos = mul( float4(input.Pos,1.0f), WorldViewProjection );
    return output;
}

VsResOutput VS_RESOLVE( VsResInput input )
{
    VsResOutput output;
    output.Pos = float4(input.Pos,1);
    output.Tex = input.Tex;
    return output;
}



//--------------------------------------------------------------------------------------
// Geometry Shaders
//--------------------------------------------------------------------------------------

[maxvertexcount (3)]
void GS_RESOLVE(triangle VsResOutput input[3], inout TriangleStream<GsResOutput> triStream)
{
    GsResOutput output;
    output.RTIndex = input[0].Tex.z;
    for(int v=0; v<3; v++)
    {
        output.Pos = input[v].Pos;
        output.Tex = input[v].Tex;
        triStream.Append( output );
    }
    triStream.RestartStrip( );
}


//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------

float4 PS_RESOLVE( GsResOutput input ) : SV_Target
{
    if( stencilbufferTex2D.Load(int3(input.Tex.x, input.Tex.y,0)).g )
        return 0.5;
    return 0;
}


//--------------------------------------------------------------------------------------

technique10 VoxelizeNZ
{
    pass NonZeroRule
    {
        SetVertexShader( CompileShader(vs_4_0, VS_VOXELIZE()) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
        SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DSS_NonZeroRule, 0 );
    }
}

technique10 VoxelizeResolveWithPS
{
    pass ResolveWithPS
    {
        SetVertexShader( CompileShader( vs_4_0, VS_RESOLVE()) );
        SetGeometryShader ( CompileShader(gs_4_0, GS_RESOLVE()) );
        SetPixelShader(CompileShader( ps_4_0, PS_RESOLVE()) );
        SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DSS_Disabled, 0 );
    }
}

