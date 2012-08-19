//--------------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------------

matrix ModelViewProjectionMatrix;
matrix NormalMatrix;

Texture2D SurfaceTexture;
Texture2D ColorSliceTex2D;
Texture2D DistSliceTex2D;

float4 vBBMin;
float4 vBBMax;

int iSliceIndex;
float3 vTextureSize;

float fIsoSurfaceVal;

//--------------------------------------------------------------------------------------
// Sampler
//--------------------------------------------------------------------------------------

SamplerState linearSamplerBorder
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Border;
	AddressV = Border;
};

SamplerState linearSamplerClamp
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
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
	float2 tex		: TEXCOORD;
};

struct GS_VORONOI_INPUT
{
	float4 pos		: POSITION;
	float3 pos2		: TEXTURE0;
	float2 tex		: TEXCOORD;
};

struct GS_TRIANGLE_VORONOI_OUTPUT
{
	float4 pos		: SV_Position;
	float3 normal	: NORMAL;
	float2 tex		: TEXCOORD;
	float4 pos2			: TEXTURE0;
	float4 trianglepoint : TEXTURE1;
};

struct GS_EDGE_VORONOI_OUTPUT
{
	float4 pos		: SV_Position;
	float2 tex		: TEXCOORD;
	float4 vec1		: TEXTURE0;
	float4 vec2		: TEXTURE1;
	float4 pos2		: TEXTURE2;
};

struct GS_VERTEX_VORONOI_OUTPUT
{
	float4 pos		: SV_Position;
	float2 tex		: TEXCOORD;
	float4 vertex	: TEXTURE0;
	float4 pos2		: TEXTURE1;
};

struct PS_VORONOI_OUTPUT
{
	float4 color	 : SV_Target0;
	float4 distdir	 : SV_Target1;
	float depth		 : SV_Depth;
};

struct VS_2DTO3D_INPUT
{
	float3 pos		: POSITION;
	float2 tex		: TEXCOORD;
};

struct GS_2DTO3D_INPUT
{
	float4 pos		: SV_Position;
	float2 tex		: TEXCOORD;
};

struct PS_2DTO3D_INPUT
{
	float4 pos		: SV_Position;
	float2 tex		: TEXCOORD;
	uint RTIndex	: SV_RenderTargetArrayIndex;
};

struct PS_2DTO3D_OUTPUT
{
	float4 color	 : SV_Target0;
	float4 distdir	 : SV_Target1;
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

float2 interpolateTexCoord(float4 p1, float4 p2, float2 tex1, float2 tex2, float sliceDepth)
{
	float zDist = p2.z - p1.z;
	float zWeight = (sliceDepth - p1.z)/zDist;
	return lerp(tex1, tex2, float2(zWeight, zWeight));
}


void TriangleCalcDistanceAndAppend(triangle GS_TRIANGLE_VORONOI_OUTPUT vertices[3], inout TriangleStream<GS_TRIANGLE_VORONOI_OUTPUT> tStream, float sliceDepth, bool bSliceDepthGreater)
{
	GS_TRIANGLE_VORONOI_OUTPUT output;

	float3 normal = vertices[0].normal;

	float3 reltex = vTextureSize / vTextureSize.z;

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
		output.normal = normalize(normal);
		output.pos = float4(vertices[v].pos.x+normalToPosAtSlice.x*2/reltex.x, vertices[v].pos.y+normalToPosAtSlice.y*2/reltex.y,  sliceDepth, 1.0f);
		output.pos2 = output.pos;
		output.trianglepoint = vertices[0].pos;
		output.tex = vertices[v].tex;
		tStream.Append(output);
	}
	tStream.RestartStrip();
}


void TriangleCalcDistanceAndAppendNormalParallel(GS_TRIANGLE_VORONOI_OUTPUT interVec1, GS_TRIANGLE_VORONOI_OUTPUT interVec2, inout TriangleStream<GS_TRIANGLE_VORONOI_OUTPUT> tStream)
{
	GS_TRIANGLE_VORONOI_OUTPUT output;
	output.trianglepoint = interVec1.pos;
	output.normal = normalize(interVec1.normal);

	float sliceDepth = iSliceIndex/vTextureSize.z;

	float3 nL = normalize(interVec1.normal);
	float3 nR = -nL;

	float3 vec1L, vec2L, vec1R, vec2R;
	vec1L = interVec1.pos.xyz + 3 * nL;
	vec2L = interVec2.pos.xyz + 3 * nL;
	vec1R = interVec1.pos.xyz + 3 * nR;
	vec2R = interVec2.pos.xyz + 3 * nR;

	output.pos = float4(vec2L.xy, sliceDepth, 1.0f);
	output.tex = interVec2.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(vec1L.xy, sliceDepth, 1.0f);
	output.tex = interVec1.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(interVec1.pos.xy, sliceDepth, 1.0f);
	output.tex = interVec1.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	tStream.RestartStrip();
	output.pos = float4(vec2L.xy, sliceDepth, 1.0f);
	output.tex = interVec2.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(interVec1.pos.xy, sliceDepth, 1.0f);
	output.tex = interVec1.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(interVec2.pos.xy, sliceDepth, 1.0f);
	output.tex = interVec2.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	tStream.RestartStrip();

	output.pos = float4(interVec2.pos.xy, sliceDepth, 1.0f);
	output.tex = interVec2.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(interVec1.pos.xy, sliceDepth, 1.0f);
	output.tex = interVec1.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(vec1R.xy, sliceDepth, 1.0f);
	output.tex = interVec1.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	tStream.RestartStrip();
	output.pos = float4(interVec2.pos.xy, sliceDepth, 1.0f);
	output.tex = interVec2.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(vec1R.xy, sliceDepth, 1.0f);
	output.tex = interVec1.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(vec2R.xy, sliceDepth, 1.0f);
	output.tex = interVec2.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	tStream.RestartStrip();
}



void EdgeProjectOntoSlice(GS_EDGE_VORONOI_OUTPUT output, GS_VORONOI_INPUT vec1, GS_VORONOI_INPUT vec2, inout TriangleStream<GS_EDGE_VORONOI_OUTPUT> tStream, float fSliceDepth, bool bSliceDepthGreater)
{
	float3 a = float3(0.0f, 0.0f, 0.0f);

	output.tex = vec1.tex;

	vec1.pos /= vec1.pos.w;
	vec2.pos /= vec2.pos.w;

	if(vec1.pos.z == vec2.pos.z)
	{
		float3 edge = mul((vec1.pos2 - vec2.pos2), (float3x3)NormalMatrix);

		float3 nL = normalize(float3(-edge.y, edge.x, 0.0));
		float3 nR = normalize(float3(edge.y, -edge.x, 0.0));

		float3 vec1L, vec2L, vec1R, vec2R;
		vec1L = vec1.pos.xyz + 3 * nL;
		vec2L = vec2.pos.xyz + 3 * nL;
		vec1R = vec1.pos.xyz + 3 * nR;
		vec2R = vec2.pos.xyz + 3 * nR;

		output.pos = float4(vec2L.xy, fSliceDepth, 1.0f);
		output.tex = vec2.tex;
		output.pos2 = output.pos;
		tStream.Append(output);
		output.pos = float4(vec1L.xy, fSliceDepth, 1.0f);
		output.tex = vec1.tex;
		output.pos2 = output.pos;
		tStream.Append(output);
		output.pos = float4(vec1R.xy, fSliceDepth, 1.0f);
		output.tex = vec1.tex;
		output.pos2 = output.pos;
		tStream.Append(output);
		tStream.RestartStrip();
		output.pos = float4(vec2R.xy, fSliceDepth, 1.0f);
		output.tex = vec2.tex;
		output.pos2 = output.pos;
		tStream.Append(output);
		output.pos = float4(vec1R.xy, fSliceDepth, 1.0f);
		output.tex = vec1.tex;
		output.pos2 = output.pos;
		tStream.Append(output);
		output.pos = float4(vec2L.xy, fSliceDepth, 1.0f);
		output.tex = vec2.tex;
		output.pos2 = output.pos;
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

	//case if edge points only in z-direction and you can not map it to a slice
	if(a.x == 0 && a.y == 0)
	{
		float2 tex = interpolateTexCoord(vec1.pos, vec2.pos, vec1.tex, vec2.tex, fSliceDepth);
		output.tex = tex;

		output.pos = float4(-1.0f, -1.0f, fSliceDepth, 1.0f);
		output.pos2 = output.pos;
		tStream.Append(output);
		output.pos = float4(-1.0f, 1.0f, fSliceDepth, 1.0f);
		output.pos2 = output.pos;
		tStream.Append(output);
		output.pos = float4(1.0f, 1.0f, fSliceDepth, 1.0f);
		output.pos2 = output.pos;
		tStream.Append(output);
		tStream.RestartStrip();
		output.pos = float4(-1.0f, -1.0f, fSliceDepth, 1.0f);
		output.pos2 = output.pos;
		tStream.Append(output);
		output.pos = float4(1.0f, 1.0f, fSliceDepth, 1.0f);
		output.pos2 = output.pos;
		tStream.Append(output);
		output.pos = float4(1.0f, -1.0f, fSliceDepth, 1.0f);
		output.pos2 = output.pos;
		tStream.Append(output);
		tStream.RestartStrip();
		return;
	}

	float3 b = float3(a.x, a.y, 0);

	float3 b_a = dot(a,b)/dot(a,a)*a;

	float3 normalToSlice = normalize(b - b_a);

	normalToSlice /= abs(normalToSlice.z);

	//project edge to the slice
	float dist1 = abs(vec1.pos.z - fSliceDepth);
	float dist2 = abs(vec2.pos.z - fSliceDepth);

	float3 reltex = vTextureSize / vTextureSize.z;


	float3 newVec1 = vec1.pos.xyz + normalToSlice*dist1*2*(1/reltex);
	float3 newVec2 = vec2.pos.xyz + normalToSlice*dist2*2*(1/reltex);

	float3 newEdge = newVec2 - newVec1;
	float3 nL = normalize(float3(-newEdge.y, newEdge.x, 0.0));
	float3 nR = normalize(float3(newEdge.y, -newEdge.x, 0.0));

	float3 vec1L, vec2L, vec1R, vec2R;
	vec1L = newVec1 + 3 * nL;
	vec2L = newVec2 + 3 * nL;
	vec1R = newVec1 + 3 * nR;
	vec2R = newVec2 + 3 * nR;



	output.pos = float4(vec2L.xy, fSliceDepth, 1.0f);
	output.tex = vec2.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(vec1L.xy, fSliceDepth, 1.0f);
	output.tex = vec1.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(vec1R.xy, fSliceDepth, 1.0f);
	output.tex = vec1.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	tStream.RestartStrip();
	output.pos = float4(vec2R.xy, fSliceDepth, 1.0f);
	output.tex = vec2.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(vec1R.xy, fSliceDepth, 1.0f);
	output.tex = vec1.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(vec2L.xy, fSliceDepth, 1.0f);
	output.tex = vec2.tex;
	output.pos2 = output.pos;
	tStream.Append(output);
	tStream.RestartStrip();

}

void CalculateEdgeProjection(GS_VORONOI_INPUT vec1, GS_VORONOI_INPUT vec2, float fSliceDepth, inout TriangleStream<GS_EDGE_VORONOI_OUTPUT> tStream)
{
	GS_EDGE_VORONOI_OUTPUT output;
	output.vec1 = vec1.pos;
	output.vec2 = vec2.pos;

	if(vec1.pos.z <= fSliceDepth && vec2.pos.z <= fSliceDepth)
	{
		if(vec1.pos.x == vec2.pos.x && vec1.pos.y == vec2.pos.y)
			return;

		EdgeProjectOntoSlice(output, vec1, vec2, tStream, fSliceDepth, true);
	}
	else if(vec1.pos.z >= fSliceDepth && vec2.pos.z >= fSliceDepth)
	{
		if(vec1.pos.x == vec2.pos.x && vec1.pos.y == vec2.pos.y)
			return;

		EdgeProjectOntoSlice(output, vec1, vec2, tStream, fSliceDepth, false);
	}
	else// case when slice divides edge in 2 parts
	{
		GS_VORONOI_INPUT interVec;
		interVec.pos = interpolate(vec1.pos, vec2.pos, fSliceDepth);
		interVec.tex = interpolateTexCoord(vec1.pos, vec2.pos, vec1.tex, vec2.tex, fSliceDepth);

		float zDist = vec2.pos.z - vec1.pos.z;
		float zWeight = (fSliceDepth - vec1.pos.z)/zDist;
		interVec.pos2 = lerp(vec1.pos2, vec2.pos2, float3(zWeight, zWeight, zWeight));

		if(vec1.pos.z < fSliceDepth)
		{
			EdgeProjectOntoSlice(output, vec1, interVec, tStream, fSliceDepth, true);
			EdgeProjectOntoSlice(output, vec2, interVec, tStream, fSliceDepth, false);	
		}
		else
		{
			EdgeProjectOntoSlice(output, vec1, interVec, tStream, fSliceDepth, false);
			EdgeProjectOntoSlice(output, vec2, interVec, tStream, fSliceDepth, true);
		}
	}
}

void CalculatePointProjection(GS_VORONOI_INPUT vec, float fSliceDepth, inout TriangleStream<GS_VERTEX_VORONOI_OUTPUT> tStream)
{
	GS_VERTEX_VORONOI_OUTPUT output;

	output.tex = vec.tex;
	output.vertex = vec.pos;
	output.pos = float4(-1.0f, -1.0f, fSliceDepth, 1.0f);
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(-1.0f, 1.0f, fSliceDepth, 1.0f);
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(1.0f, 1.0f, fSliceDepth, 1.0f);
	output.pos2 = output.pos;
	tStream.Append(output);
	tStream.RestartStrip();
	output.pos = float4(-1.0f, -1.0f, fSliceDepth, 1.0f);
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(1.0f, 1.0f, fSliceDepth, 1.0f);
	output.pos2 = output.pos;
	tStream.Append(output);
	output.pos = float4(1.0f, -1.0f, fSliceDepth, 1.0f);
	output.pos2 = output.pos;
	tStream.Append(output);
	tStream.RestartStrip();
}


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

GS_VORONOI_INPUT VoronoiVS(VS_VORONOI_INPUT input)
{
	GS_VORONOI_INPUT output;
	output.pos = mul(float4(input.pos, 1.0f), ModelViewProjectionMatrix);
	output.pos2 = input.pos;
	output.tex = input.tex;
	return output;
}

GS_2DTO3D_INPUT VS_2Dto3D(VS_2DTO3D_INPUT input)
{
	GS_2DTO3D_INPUT output;
	output.pos = float4(input.pos, 1.0f);
	output.tex = input.tex;
	return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------

[maxvertexcount(12)]
void VoronoiTriangleGS( triangle GS_VORONOI_INPUT input[3], inout TriangleStream<GS_TRIANGLE_VORONOI_OUTPUT> tStream)
{
	GS_TRIANGLE_VORONOI_OUTPUT triangle1[3];
	triangle1[0].pos = input[0].pos;
	triangle1[0].tex = input[0].tex;
	triangle1[0].trianglepoint = float4(0.0, 0.0, 0.0, 0.0);//dummy initialization, so that shader compiles
	triangle1[1].pos = input[1].pos;
	triangle1[1].tex = input[1].tex;
	triangle1[1].trianglepoint = float4(0.0, 0.0, 0.0, 0.0);
	triangle1[2].pos = input[2].pos;
	triangle1[2].tex = input[2].tex;
	triangle1[2].trianglepoint = float4(0.0, 0.0, 0.0, 0.0);

	float3 u = input[0].pos2 - input[1].pos2;
	float3 v = input[0].pos2 - input[2].pos2;

	float3 norm = mul(normalize(cross(u, v)), (float3x3)NormalMatrix);
	
	triangle1[0].normal = norm;
	triangle1[1].normal = norm;
	triangle1[2].normal = norm;

	//Calculate depth of the current slice
	float sliceDepth = iSliceIndex/vTextureSize.z;


	// check if all points of the triangle have a higher/lower z value as the sliceindex-depth
	if(input[0].pos.z <= sliceDepth && input[1].pos.z <= sliceDepth && input[2].pos.z <= sliceDepth)
	{
		//return;//DEBUG
		TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
	}
	else if(input[0].pos.z >= sliceDepth && input[1].pos.z >= sliceDepth && input[2].pos.z >= sliceDepth)
	{
		//return;//DEBUG
		TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
	}
	else
	{
		//divide triangle into 3 triangles, divided by the slice
		//calculate distance function for each polygon

		GS_TRIANGLE_VORONOI_OUTPUT interVec1 = triangle1[0];
		GS_TRIANGLE_VORONOI_OUTPUT interVec2 = triangle1[0];

		//calculate interpolated vectors and create the 3 triangles
		if(input[0].pos.z < sliceDepth)
		{
			//case 2.3
			if(input[1].pos.z < sliceDepth)
			{
				interVec1.pos = interpolate(input[0].pos, input[2].pos, sliceDepth);
				interVec2.pos = interpolate(input[1].pos, input[2].pos, sliceDepth);
				interVec1.tex = interpolateTexCoord(input[0].pos, input[2].pos, input[0].tex, input[2].tex, sliceDepth);
				interVec2.tex = interpolateTexCoord(input[1].pos, input[2].pos, input[1].tex, input[2].tex, sliceDepth);

				if(triangle1[0].normal.z == 0)
				{
					TriangleCalcDistanceAndAppendNormalParallel(interVec1, interVec2, tStream);
				}
				else
				{
					//return;//DEBUG

					//triangle 1: 0,1,interVec1
					triangle1[0].pos = input[0].pos;
					triangle1[0].tex = input[0].tex;
					triangle1[1].pos = input[1].pos;
					triangle1[1].tex = input[1].tex;
					triangle1[2].pos = interVec1.pos;
					triangle1[2].tex = interVec1.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
					//triangle 2: 1,iV1,iV2
					triangle1[0].pos = input[1].pos;
					triangle1[0].tex = input[1].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
					//triangle 3: 2,iV1,iV2
					triangle1[0].pos = input[2].pos;
					triangle1[0].tex = input[2].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
				}
			}
			//case 2.2
			else if(input[2].pos.z < sliceDepth)
			{
				//interpolate between 0 and 1 and between 1 and 2
				interVec1.pos = interpolate(input[0].pos, input[1].pos, sliceDepth);
				interVec2.pos = interpolate(input[2].pos, input[1].pos, sliceDepth);
				interVec1.tex = interpolateTexCoord(input[0].pos, input[1].pos, input[0].tex, input[1].tex, sliceDepth);
				interVec2.tex = interpolateTexCoord(input[2].pos, input[1].pos, input[2].tex, input[1].tex, sliceDepth);

				if(triangle1[0].normal.z == 0)
				{
					TriangleCalcDistanceAndAppendNormalParallel(interVec1, interVec2, tStream);
				}
				else
				{
					//return;//DEBUG

					//triangle 1: 0,2,iV1
					triangle1[0].pos = input[0].pos;
					triangle1[0].tex = input[0].tex;
					triangle1[1].pos = input[2].pos;
					triangle1[1].tex = input[2].tex;
					triangle1[2].pos = interVec1.pos;
					triangle1[2].tex = interVec1.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
					//triangle 2: 2,iV1,iV2
					triangle1[0].pos = input[2].pos;
					triangle1[0].tex = input[2].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
					//triangle 3: 1,iV1,iV2
					triangle1[0].pos = input[1].pos;
					triangle1[0].tex = input[1].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
				}
			}
			//case 1.1
			else
			{
				//interpolate between 0 and 1 and between 0 and 2
				interVec1.pos = interpolate(input[0].pos, input[1].pos, sliceDepth);
				interVec2.pos = interpolate(input[0].pos, input[2].pos, sliceDepth);
				interVec1.tex = interpolateTexCoord(input[0].pos, input[1].pos, input[0].tex, input[1].tex, sliceDepth);
				interVec2.tex = interpolateTexCoord(input[0].pos, input[2].pos, input[0].tex, input[2].tex, sliceDepth);

				if(triangle1[0].normal.z == 0) 
				{
					TriangleCalcDistanceAndAppendNormalParallel(interVec1, interVec2, tStream);
				}
				else
				{
					//return;//DEBUG

					//triangle 1: 1,2,iV2
					triangle1[0].pos = input[1].pos;
					triangle1[0].tex = input[1].tex;
					triangle1[1].pos = input[2].pos;
					triangle1[1].tex = input[2].tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
					//triangle 2: 1,iv1,iv2
					triangle1[0].pos = input[1].pos;
					triangle1[0].tex = input[1].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
					//triangle 3: 0,iv1,iv2
					triangle1[0].pos = input[0].pos;
					triangle1[0].tex = input[0].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
				}
			}
		}
		else
		{
			//case 1.3
			if(input[1].pos.z > sliceDepth)
			{
				//interpolate between 0 and 2 and between 1 and 2
				interVec1.pos = interpolate(input[0].pos, input[2].pos, sliceDepth);
				interVec2.pos = interpolate(input[1].pos, input[2].pos, sliceDepth);
				interVec1.tex = interpolateTexCoord(input[0].pos, input[2].pos, input[0].tex, input[2].tex, sliceDepth);
				interVec2.tex = interpolateTexCoord(input[1].pos, input[2].pos, input[1].tex, input[2].tex, sliceDepth);

				if(triangle1[0].normal.z == 0)
				{
					TriangleCalcDistanceAndAppendNormalParallel(interVec1, interVec2, tStream);
				}
				else
				{
					//return;//DEBUG

					//triangle 1: 0, iv1,iv2
					triangle1[0].pos = input[0].pos;
					triangle1[0].tex = input[0].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
					//triangle 2: 0,1,iv2
					triangle1[0].pos = input[0].pos;
					triangle1[0].tex = input[0].tex;
					triangle1[1].pos = input[1].pos;
					triangle1[1].tex = input[1].tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
					//triangle 3: 2, iv1,iv2
					triangle1[0].pos = input[2].pos;
					triangle1[0].tex = input[2].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
				}
			}
			//case 1.2
			else if(input[2].pos.z > sliceDepth)
			{
				//interpolate between 0 and 1 and between 1 and 2
				interVec1.pos = interpolate(input[0].pos, input[1].pos, sliceDepth);
				interVec2.pos = interpolate(input[1].pos, input[2].pos, sliceDepth);
				interVec1.tex = interpolateTexCoord(input[0].pos, input[1].pos, input[0].tex, input[1].tex, sliceDepth);
				interVec2.tex = interpolateTexCoord(input[1].pos, input[2].pos, input[1].tex, input[2].tex, sliceDepth);

				if(triangle1[0].normal.z == 0)
				{
					TriangleCalcDistanceAndAppendNormalParallel(interVec1, interVec2, tStream);
				}
				else
				{
					//return;//DEBUG

					//triangle 1: 0, iv1, iv2
					triangle1[0].pos = input[0].pos;
					triangle1[0].tex = input[0].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
					//triangle 2: 0,2,iv2
					triangle1[0].pos = input[0].pos;
					triangle1[0].tex = input[0].tex;
					triangle1[1].pos = input[2].pos;
					triangle1[1].tex = input[2].tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
					//triangle 3: 1,iv1,iv2
					triangle1[0].pos = input[1].pos;
					triangle1[0].tex = input[1].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
				}
			}
			//case 2.1
			else
			{
				//interpolate between 0 and 1 and between 0 and 2
				interVec1.pos = interpolate(input[0].pos, input[1].pos, sliceDepth);
				interVec2.pos = interpolate(input[0].pos, input[2].pos, sliceDepth);
				interVec1.tex = interpolateTexCoord(input[0].pos, input[1].pos, input[0].tex, input[1].tex, sliceDepth);
				interVec2.tex = interpolateTexCoord(input[0].pos, input[2].pos, input[0].tex, input[2].tex, sliceDepth);

				if(triangle1[0].normal.z == 0) 
				{
					TriangleCalcDistanceAndAppendNormalParallel(interVec1, interVec2, tStream);
				}
				else
				{
					//return;//DEBUG

					//triangle 1: 0,iv1,iv2
					triangle1[0].pos = input[0].pos;
					triangle1[0].tex = input[0].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, false);
					//triangle 2: 1,2,iv1
					triangle1[0].pos = input[1].pos;
					triangle1[0].tex = input[1].tex;
					triangle1[1].pos = input[2].pos;
					triangle1[1].tex = input[2].tex;
					triangle1[2].pos = interVec1.pos;
					triangle1[2].tex = interVec1.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
					//triangle 3: 2,iv1,iv2
					triangle1[0].pos = input[2].pos;
					triangle1[0].tex = input[2].tex;
					triangle1[1].pos = interVec1.pos;
					triangle1[1].tex = interVec1.tex;
					triangle1[2].pos = interVec2.pos;
					triangle1[2].tex = interVec2.tex;
					TriangleCalcDistanceAndAppend(triangle1, tStream, sliceDepth, true);
				}
			}
		}
	}
}

[maxvertexcount(6)]
void VoronoiEdgeGS(line GS_VORONOI_INPUT input[2], inout TriangleStream<GS_EDGE_VORONOI_OUTPUT> tStream)
{
	//Calculate depth of the current slice
	float sliceDepth = iSliceIndex/vTextureSize.z;

	GS_VORONOI_INPUT vec1 = input[0];
	GS_VORONOI_INPUT vec2 = input[1];
	vec1.pos /= vec1.pos.w;
	vec2.pos /= vec2.pos.w;

	CalculateEdgeProjection(vec1, vec2, sliceDepth, tStream);
}

[maxvertexcount(6)]
void VoronoiVertexGS(point GS_VORONOI_INPUT input[1], inout TriangleStream<GS_VERTEX_VORONOI_OUTPUT> tStream)
{
	//Calculate depth of the current slice
	float sliceDepth = iSliceIndex/vTextureSize.z;

	CalculatePointProjection(input[0], sliceDepth, tStream);
}

[maxvertexcount(3)]
void GS_2Dto3D(triangle GS_2DTO3D_INPUT input[3], inout TriangleStream<PS_2DTO3D_INPUT> tStream)
{
	PS_2DTO3D_INPUT output;
	output.RTIndex = iSliceIndex;

	
	
	for(int i = 0; i < 3; i++)
	{
		output.pos = input[i].pos;
		output.tex = input[i].tex;
		tStream.Append(output);
	}
	tStream.RestartStrip();
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

PS_VORONOI_OUTPUT VoronoiTrianglePS(GS_TRIANGLE_VORONOI_OUTPUT input)
{
	PS_VORONOI_OUTPUT output;

	output.color = SurfaceTexture.Sample(linearSamplerClamp, input.tex);
	output.color.a = fIsoSurfaceVal;

	float3 tex = normalize(vTextureSize);

	float3 normal = normalize(input.normal);
	float3 punktaufebene = input.trianglepoint.xyz*tex;
	float3 vertex = input.pos2.xyz * tex;

	punktaufebene.z = punktaufebene.z*2 - 1;
	vertex.z = vertex.z*2 - 1;

	float d = normal.x*punktaufebene.x + normal.y*punktaufebene.y + normal.z*punktaufebene.z;


	float dist = normal.x*vertex.x + normal.y*vertex.y + normal.z*vertex.z - d;
	output.depth = abs(dist)/3;

	output.distdir = float4(abs(dist), 0.0f, 0.0f, 1.0f);

	return output;
}

PS_VORONOI_OUTPUT VoronoiEdgePS(GS_EDGE_VORONOI_OUTPUT input)
{
	PS_VORONOI_OUTPUT output;
	output.color = SurfaceTexture.Sample(linearSamplerClamp, input.tex);
	output.color.a = fIsoSurfaceVal;

	float3 tex = normalize(vTextureSize);

	float3 v1 = input.vec1.xyz * tex;
	float3 v2 = input.vec2.xyz * tex;
	float3 p = input.pos2.xyz * tex;

	v1.z = v1.z*2 - 1;
	v2.z = v2.z*2 - 1;
	p.z = p.z*2 - 1;

	float3 a = v1;
	float3 b = v2 - v1;// gerade: g = a + t*b
	float3 upper = cross(p-a, b);
	float dist = length(upper)/length(b);

	output.depth = dist/3;
	float3 distdir = normalize(cross(b, upper));

	output.distdir = float4(dist, 0.0f, 0.0f, 1.0f);

	return output;
}

PS_VORONOI_OUTPUT VoronoiVertexPS(GS_VERTEX_VORONOI_OUTPUT input)
{
	PS_VORONOI_OUTPUT output;
	output.color = SurfaceTexture.Sample(linearSamplerClamp, input.tex);
	output.color.a = fIsoSurfaceVal;

	float3 tex = normalize(vTextureSize);

	float3 pixelpos = input.pos2.xyz * tex;
	float3 vertex = input.vertex.xyz * tex;
	pixelpos.z = pixelpos.z * 2 - 1;
	vertex.z = vertex.z * 2 - 1;

	float dist = length(pixelpos - vertex);
	output.depth = dist/3;
	float3 distdir = normalize(pixelpos-vertex);

	output.distdir = float4(dist, 0.0f, 0.0f, 1.0f);

	return output;
}

PS_2DTO3D_OUTPUT PS_2Dto3D(PS_2DTO3D_INPUT input)
{
	PS_2DTO3D_OUTPUT output;

	

	output.color = ColorSliceTex2D.Sample(linearSamplerClamp, input.tex);
	output.distdir = DistSliceTex2D.Sample(linearSamplerClamp, input.tex);

	/*if(iSliceIndex > 255)
	{
		output.color = float4(0.0f, 1.0f, 0.0f, 1.0f);
	}*/

	/*if(input.RTIndex < 100)
	{
		output.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	else if(input.RTIndex < 256)
	{
		output.color = float4(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else
	{
		output.color = float4(0.0f, 0.0f, 1.0f, 1.0f);
	}*/
	return output;
}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------



technique10 GenerateVoronoiDiagram
{
	pass Triangle
	{
		SetVertexShader(CompileShader(vs_4_0, VoronoiVS()));
		SetGeometryShader(CompileShader(gs_4_0, VoronoiTriangleGS()));
		SetPixelShader(CompileShader(ps_4_0, VoronoiTrianglePS()));
		SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
	}
	pass Edge
	{
		SetVertexShader(CompileShader(vs_4_0, VoronoiVS()));
		SetGeometryShader(CompileShader(gs_4_0, VoronoiEdgeGS()));
		SetPixelShader(CompileShader(ps_4_0, VoronoiEdgePS()));
		SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
	}
	pass Point
	{
		SetVertexShader(CompileShader(vs_4_0, VoronoiVS()));
		SetGeometryShader(CompileShader(gs_4_0, VoronoiVertexGS()));
		SetPixelShader(CompileShader(ps_4_0, VoronoiVertexPS()));
		SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
	}
}

technique10 Technique2Dto3D
{
	pass
	{
		SetVertexShader(CompileShader(vs_4_0, VS_2Dto3D()));
		SetGeometryShader(CompileShader(gs_4_0, GS_2Dto3D()));
		//SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, PS_2Dto3D()));
		SetRasterizerState( RS_CullDisabled );
        SetBlendState( BS_NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
	}
}