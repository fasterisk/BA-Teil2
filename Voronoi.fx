//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------

matrix ModelViewProjectionMatrix;
matrix NormalMatrix;

Texture2D flatColorTexture;
Texture2D flatDistTexture;

float4 vBBMin;
float4 vBBMax;

int iSliceIndex;
float3 vTextureSize;

//--------------------------------------------------------------------------------------
// Sampler
//--------------------------------------------------------------------------------------

SamplerState linearSamplerBorder
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border;
	AddressV = Border;
};

//--------------------------------------------------------------------------------------
// States
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

DepthStencilState EnableDepth
{
	DepthEnable = TRUE;
	Depthfunc = LESS_EQUAL;
	DepthWriteMask = ALL;
	StencilEnable = FALSE;
};

DepthStencilState DSS_Disabled
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
    
    //stencil
    //StencilEnable = FALSE;
    //StencilReadMask = 0x00;
    //StencilWriteMask = 0x00;
};

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------

struct VS_VORONOI_INPUT
{
	float3 pos		: POSITION;
	float3 normal	: NORMAL;
	float4 color	: COLOR;
};

struct GS_VORONOI_INPUT
{
	float4 pos		: POSITION;
	float3 normal	: NORMAL;
	float4 color	: COLOR;
};

struct GS_EDGE_VORONOI_INPUT
{
	float4 pos		: POSITION;
	float3 pos2		: TEXTURE0;
	float4 color	: COLOR;
};

struct GS_VORONOI_OUTPUT
{
	float4 pos		: SV_POSITION; //former vertex positions + vertex positions generated by the geometry shader
	float4 color	: COLOR;
	float4 dist		: TEXTURE0;		// analog zum 2d? (bei VolSurfaces10 Effect_Undistort.fx)
};

struct PS_VORONOI_OUTPUT
{
	float4 color : SV_Target0;
	float4 dist	 : SV_Target1;
};

struct VS_RESOLVE_INPUT
{
	float3 pos : POSITION;
	float3 tex : TEXCOORD;
};

struct GS_RESOLVE_INPUT
{
	float4 pos : POSITION;
	float3 tex : TEXCOORD;
};

struct GS_RESOLVE_OUTPUT
{
	float4 pos : SV_Position;
	float3 tex : TEXCOORD;
	uint RTIndex : SV_RenderTargetArrayIndex;
};

struct PS_RESOLVE_OUTPUT
{
	float4 color : SV_Target0;
	float4 dist  : SV_Target1;
};

//--------------------------------------------------------------------------------------
// Helper Functions
//--------------------------------------------------------------------------------------
float4 interpolate(float4 p1, float4 p2, float sliceDepth)
{
	float zDist = p2.z - p1.z;
	float zWeight = (sliceDepth - p1.z)/zDist;
	return lerp(p1, p2, float4(zWeight, zWeight, zWeight, zWeight));
}


void TriangleCalcDistanceAndAppend(triangle GS_VORONOI_INPUT vertices[3], inout TriangleStream<GS_VORONOI_OUTPUT> tStream, float sliceDepth, bool bSliceDepthGreater)
{
	GS_VORONOI_OUTPUT output;
	output.color = vertices[0].color;//assumed, that all 3 vertices have the same color
	
	float3 normal = normalize(vertices[0].normal);

	//check if normal is not parallel to the slice
	if(normal.z == 0)
		return;

	if((bSliceDepthGreater && normal.z < 0)||(!bSliceDepthGreater && normal.z > 0))
		normal = -normal;

	//normalize the normal for the z-value, so we can easily
	//compute the distance-normal vector between the point
	//and the slice
	normal /= abs(normal.z);
	
	for(int v = 0; v < 3; v++)
	{
		//distance of the point to the slice
		float distPosToSliceZ = abs(sliceDepth - vertices[v].pos.z);
	
		//distance-normal between point of the triangle and slice
		float3 normalToPosAtSlice =	normal*distPosToSliceZ;
		output.pos = float4(vertices[v].pos.x+normalToPosAtSlice.x*2, vertices[v].pos.y+normalToPosAtSlice.y*2,  length(normalToPosAtSlice), 1.0f);

		output.dist = float4(normal.xyz, 1.0f);
		tStream.Append(output);
	}
	tStream.RestartStrip();
}

void EdgeProjectOntoSlice(GS_EDGE_VORONOI_INPUT vec1, GS_EDGE_VORONOI_INPUT vec2, inout TriangleStream<GS_VORONOI_OUTPUT> tStream, float fSliceDepth, bool bSliceDepthGreater)
{
	
	GS_VORONOI_OUTPUT output;

	float3 a = float3(0.0f, 0.0f, 0.0f);
	
	vec1.pos /= vec1.pos.w;
	vec2.pos /= vec2.pos.w;

	if(vec1.pos.z == vec2.pos.z)
	{
		/*float3 newEdge = vec2.pos.xyz - vec1.pos.xyz;

		float3 nL = float3(-newEdge.y, newEdge.x, 0.0);
		float3 nR = float3(newEdge.y, -newEdge.x, 0.0);

		float3 vec1L, vec2L, vec1R, vec2R;
		vec1L = vec1.pos.xyz + 3 * nL;
		vec2L = vec2.pos.xyz + 3 * nL;
		vec1R = vec1.pos.xyz + 3 * nR;
		vec2R = vec2.pos.xyz + 3 * nR;
	
		
		output.color = vec1.color;
		output.dist = vec1.color;
		output.pos = float4(vec2L.xy, 2.0f, 1.0f);
		tStream.Append(output);
		output.pos = float4(vec1L.xy, 2.0f, 1.0f);
		tStream.Append(output);
		output.pos = float4(vec1R.xy, 2.0f, 1.0f);
		tStream.Append(output);
		output.pos = float4(vec2R.xy, 2.0f, 1.0f);
		tStream.Append(output);
		output.pos = float4(vec1R.xy, 2.0f, 1.0f);
		tStream.Append(output);
		output.pos = float4(vec2L.xy, 2.0f, 1.0f);
		tStream.Append(output);
		tStream.RestartStrip();
		return;
		*/
		output.pos = vec1.pos;
		output.color = vec1.color;
		output.dist = vec1.color;
		tStream.Append(output);
		output.pos = vec2.pos;
		output.color = vec2.color;
		output.dist = vec2.color;
		tStream.Append(output);
		tStream.RestartStrip();
		return;

	}
	else if(vec1.pos.z < vec2.pos.z)
	{
		if(bSliceDepthGreater)
			a = vec1.pos2 - vec2.pos2;
		else
			a = vec2.pos2 - vec1.pos2;
	}
	else if(vec1.pos.z > vec2.pos.z)
	{
		if(bSliceDepthGreater)
			a = vec2.pos2 - vec1.pos2;
		else
			a = vec1.pos2 - vec2.pos2;
	}

	a = mul(a, (float3x3)NormalMatrix);

	float3 b = float3(a.x, a.y, 0);

	float3 b_a = dot(a,b)/dot(a,a)*a;

	float3 normalToSlice = b - b_a;
	
	normalToSlice /= abs(normalToSlice.z);
	
	//project edge to the slice
	float dist1 = abs(vec1.pos.z - fSliceDepth);
	float dist2 = abs(vec2.pos.z - fSliceDepth);

	float3 newVec1 = vec1.pos.xyz + normalToSlice*dist1*2;
	float3 newVec2 = vec2.pos.xyz + normalToSlice*dist2*2;


	float3 newEdge = newVec2 - newVec1;
	float3 nL = normalize(float3(-newEdge.y, newEdge.x, 0.0));
	float3 nR = normalize(float3(newEdge.y, -newEdge.x, 0.0));

	float3 vec1L, vec2L, vec1R, vec2R;
	vec1L = newVec1 + 3 * nL;
	vec2L = newVec2 + 3 * nL;
	vec1R = newVec1 + 3 * nR;
	vec2R = newVec2 + 3 * nR;


	output.color = vec1.color;
	output.dist = vec1.color;
	output.pos = float4(vec2L.xy, 0.0f, 1.0f);
	tStream.Append(output);
	output.pos = float4(vec1L.xy, 0.0f, 1.0f);
	tStream.Append(output);
	output.pos = float4(vec1R.xy, 0.0f, 1.0f);
	tStream.Append(output);
	output.pos = float4(vec2R.xy, 0.0f, 1.0f);
	tStream.Append(output);
	output.pos = float4(vec1R.xy, 0.0f, 1.0f);
	tStream.Append(output);
	output.pos = float4(vec2L.xy, 0.0f, 1.0f);
	tStream.Append(output);
	tStream.RestartStrip();
	
	/*
	output.pos = float4(newVec1.xy, 0.0f, 1.0f);
	output.color = vec1.color;
	output.dist = vec1.color;
	tStream.Append(output);
	output.pos = float4(newVec2.xy, 0.0f, 1.0f);
	output.color = vec2.color;
	output.dist = vec2.color;
	tStream.Append(output);
	tStream.RestartStrip();
	*/

	// draw triangles as distance function normal to the edge, z-value of every pixel is then calculated 
	// in the pixel shader
	// give the edge coordinates through input parameters to the pixel shader
	// check which point is the higher one, so that my algorithm works; 
	// if they have the same z-value, it gets simple and i dont have to apply my algorithm

}


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

GS_VORONOI_INPUT VoronoiTriangleVS(VS_VORONOI_INPUT input)
{
	GS_VORONOI_INPUT output;
	output.pos = mul(float4(input.pos, 1.0f), ModelViewProjectionMatrix);
	output.normal = mul(input.normal, (float3x3)NormalMatrix);
	output.color = input.color;
	return output;
}

GS_EDGE_VORONOI_INPUT VoronoiEdgeVS(VS_VORONOI_INPUT input)
{
	GS_EDGE_VORONOI_INPUT output;
	output.pos = mul(float4(input.pos, 1.0f), ModelViewProjectionMatrix);
	output.pos2 = input.pos;
	output.color = input.color;
	return output;
}

GS_RESOLVE_INPUT ResolveVS(VS_RESOLVE_INPUT input)
{
	GS_RESOLVE_INPUT output;
	output.pos = float4(input.pos, 1.0f);
	output.tex = input.tex;
	return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------

[maxvertexcount(9)]
void VoronoiTriangleGS( triangle GS_VORONOI_INPUT input[3], inout TriangleStream<GS_VORONOI_OUTPUT> tStream)
{
	GS_VORONOI_INPUT triangle1[3] = input;
	
	//z-distance of the bounding box
	float zBBDist = vBBMax.z - vBBMin.z;

	//Calculate depth of the current slice
	float sliceDepth = (iSliceIndex/vTextureSize.z)*zBBDist+vBBMin.z;
	

	// check if all points of the triangle have a higher/lower z value as the sliceindex-depth
	if(input[0].pos.z <= sliceDepth && input[1].pos.z <= sliceDepth && input[2].pos.z <= sliceDepth)
	{
		TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
	}
	else if(input[0].pos.z >= sliceDepth && input[1].pos.z >= sliceDepth && input[2].pos.z >= sliceDepth)
	{
		TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
	}
	else
	{
		//divide triangle into 3 triangle, divided by the slice
		//calculate distance function for each polygon


		//calculate interpolated vectors and create the 3 triangles
		if(input[0].pos.z < sliceDepth)
		{
			//case 2.3
			if(input[1].pos.z < sliceDepth)
			{
				float4 interVec1 = interpolate(input[0].pos, input[2].pos, sliceDepth);
				float4 interVec2 = interpolate(input[1].pos, input[2].pos, sliceDepth);

				//triangle 1: 0,1,interVec1
				triangle1[0].pos = input[0].pos;
				triangle1[1].pos = input[1].pos;
				triangle1[2].pos = interVec1;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
				//triangle 2: 1,iV1,iV2
				triangle1[0].pos = input[1].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
				//triangle 3: 2,iV1,iV2
				triangle1[0].pos = input[2].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);		
			}
			//case 2.2
			else if(input[2].pos.z < sliceDepth)
			{
				//interpolate between 0 and 1 and between 1 and 2
				float4 interVec1 = interpolate(input[0].pos, input[1].pos, sliceDepth);
				float4 interVec2 = interpolate(input[2].pos, input[1].pos, sliceDepth);

				//triangle 1: 0,2,iV1
				triangle1[0].pos = input[0].pos;
				triangle1[1].pos = input[2].pos;
				triangle1[2].pos = interVec1;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
				//triangle 2: 2,iV1,iV2
				triangle1[0].pos = input[2].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
				//triangle 3: 1,iV1,iV2
				triangle1[0].pos = input[1].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
			}
			//case 1.1
			else
			{
				//interpolate between 0 and 1 and between 0 and 2
				float4 interVec1 = interpolate(input[0].pos, input[1].pos, sliceDepth);
				float4 interVec2 = interpolate(input[0].pos, input[2].pos, sliceDepth);

				//triangle 1: 1,2,iV2
				triangle1[0].pos = input[1].pos;
				triangle1[1].pos = input[2].pos;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
				//triangle 2: 1,iv1,iv2
				triangle1[0].pos = input[1].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
				//triangle 3: 0,iv1,iv2
				triangle1[0].pos = input[0].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
			}
		}
		else
		{
			//case 1.3
			if(input[1].pos.z > sliceDepth)
			{
				//interpolate between 0 and 2 and between 1 and 2
				float4 interVec1 = interpolate(input[0].pos, input[2].pos, sliceDepth);
				float4 interVec2 = interpolate(input[1].pos, input[2].pos, sliceDepth);

				//triangle 1: 0, iv1,iv2
				triangle1[0].pos = input[0].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
				//triangle 2: 0,1,iv2
				triangle1[0].pos = input[0].pos;
				triangle1[1].pos = input[1].pos;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
				//triangle 3: 2, iv1,iv2
				triangle1[0].pos = input[2].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
			}
			//case 1.2
			else if(input[2].pos.z > sliceDepth)
			{
				//interpolate between 0 and 1 and between 1 and 2
				float4 interVec1 = interpolate(input[0].pos, input[1].pos, sliceDepth);
				float4 interVec2 = interpolate(input[1].pos, input[2].pos, sliceDepth);

				//triangle 1: 0, iv1, iv2
				triangle1[0].pos = input[0].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
				//triangle 2: 0,2,iv2
				triangle1[0].pos = input[0].pos;
				triangle1[1].pos = input[2].pos;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
				//triangle 3: 1,iv1,iv2
				triangle1[0].pos = input[1].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
			}
			//case 2.1
			else
			{
				//interpolate between 0 and 1 and between 0 and 2
				float4 interVec1 = interpolate(input[0].pos, input[1].pos, sliceDepth);
				float4 interVec2 = interpolate(input[0].pos, input[2].pos, sliceDepth);

				//triangle 1: 0,iv1,iv2
				triangle1[0].pos = input[0].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
				//triangle 2: 1,2,iv1
				triangle1[0].pos = input[1].pos;
				triangle1[1].pos = input[2].pos;
				triangle1[2].pos = interVec1;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
				//triangle 3: 2,iv1,iv2
				triangle1[0].pos = input[2].pos;
				triangle1[1].pos = interVec1;
				triangle1[2].pos = interVec2;
				TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
			}
		}
	}
	
}

[maxvertexcount(18)]
void VoronoiEdgeGS( triangle GS_EDGE_VORONOI_INPUT input[3], inout TriangleStream<GS_VORONOI_OUTPUT> tStream)
{
	//z-distance of the bounding box
	float zBBDist = vBBMax.z - vBBMin.z;

	//Calculate depth of the current slice
	float sliceDepth = (iSliceIndex/vTextureSize.z)*zBBDist+vBBMin.z;

	GS_VORONOI_OUTPUT output;

	for(int i = 0; i < 1; i++)//3; i++)
	{
		GS_VORONOI_INPUT vec1 = input[i];
		GS_VORONOI_INPUT vec2 = input[(i+1)%3];
	
		if(vec1.pos.z <= sliceDepth && vec2.pos.z <= sliceDepth)
		{
			EdgeProjectOntoSlice(vec1, vec2, tStream, sliceDepth, true);
		}
		else if(vec1.pos.z >= sliceDepth && vec2.pos.z >= sliceDepth)
		{
			EdgeProjectOntoSlice(vec1, vec2, tStream, sliceDepth, false);
		}
		else// case when slice divides edge in 2 parts
		{
			// following is just for testing purposes - NOT FINAL
			/*output.pos = vec1.pos;
			output.color = vec1.color;
			output.dist = vec1.color;
			tStream.Append(output);
			output.pos = vec2.pos;
			output.color = vec2.color;
			output.dist = vec2.color;
			tStream.Append(output);
			tStream.RestartStrip();*/
		}
	}

}

[maxvertexcount(1)]
void VoronoiVertexGS( point GS_VORONOI_INPUT input[1], inout TriangleStream<GS_VORONOI_OUTPUT> tStream)
{
	// Slice index as per frame variable

	GS_VORONOI_OUTPUT output;
	output.pos = input[0].pos;
	output.color = input[0].color;
	tStream.Append(output);
	// calculate/look up distance function

	tStream.RestartStrip();
}

[maxvertexcount(3)]
void ResolveGS(triangle GS_RESOLVE_INPUT input[3], inout TriangleStream<GS_RESOLVE_OUTPUT> tStream)
{
	GS_RESOLVE_OUTPUT output;
	output.RTIndex = input[0].tex.z;
	for(int v = 0; v < 3; v++)
	{
		output.pos = input[v].pos;
		output.tex = input[v].tex;
		tStream.Append(output);
	}
	tStream.RestartStrip();
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

PS_VORONOI_OUTPUT VoronoiTrianglePS(GS_VORONOI_OUTPUT input)
{
	PS_VORONOI_OUTPUT output;
	output.color = input.color;
	output.dist = input.dist;
	return output;
}

PS_VORONOI_OUTPUT VoronoiEdgePS(GS_VORONOI_OUTPUT input)
{
	PS_VORONOI_OUTPUT output;
	output.color = input.color;
	output.dist = input.dist;
	return output;
}

PS_VORONOI_OUTPUT VoronoiVertexPS(GS_VORONOI_OUTPUT input)
{
	PS_VORONOI_OUTPUT output;
	output.color = input.color;
	output.dist = input.dist;
	return output;
}

PS_RESOLVE_OUTPUT ResolvePS(GS_RESOLVE_OUTPUT input)
{
	PS_RESOLVE_OUTPUT output;
	output.color = flatColorTexture.SampleLevel(linearSamplerBorder, input.tex.xy, 0);
	output.dist = flatDistTexture.SampleLevel(linearSamplerBorder, input.tex.xy, 0);
	return output;
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique10 GenerateVoronoiDiagram
{
	pass Triangle
	{
		SetVertexShader(CompileShader(vs_4_0, VoronoiTriangleVS()));
		SetGeometryShader(CompileShader(gs_4_0, VoronoiTriangleGS()));
		SetPixelShader(CompileShader(ps_4_0, VoronoiTrianglePS()));
		SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
	}
	pass Edge
	{
		SetVertexShader(CompileShader(vs_4_0, VoronoiEdgeVS()));
		SetGeometryShader(CompileShader(gs_4_0, VoronoiEdgeGS()));
		SetPixelShader(CompileShader(ps_4_0, VoronoiEdgePS()));
		SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
	}
	/*pass Point
	{
		SetVertexShader(CompileShader(vs_4_0, VoronoiVS()));
		SetGeometryShader(CompileShader(gs_4_0, VertexGS()));
		SetPixelShader(CompileShader(ps_4_0, VoronoiPS()));
		//TODO: set states
	}*/
}

technique10 Flat2DTextureTo3D
{
	pass F2DTTo3D
	{
		SetVertexShader(CompileShader(vs_4_0, ResolveVS()));
		SetGeometryShader(CompileShader(gs_4_0, ResolveGS()));
		SetPixelShader(CompileShader(ps_4_0, ResolvePS()));
		 SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DSS_Disabled, 0 );
	}
}