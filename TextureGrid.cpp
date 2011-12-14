#include "Globals.h"
#include "TextureGrid.h"

TextureGrid::TextureGrid(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
}

TextureGrid::~TextureGrid()
{
	SAFE_RELEASE(m_pLayout);
    SAFE_RELEASE(m_pSlicesBuffer);
}

HRESULT TextureGrid::Initialize(int iTextureWidth, int iTextureHeight, int iTextureDepth, ID3DX11EffectTechnique* pTechnique)
{
	HRESULT hr;

	m_iTextureWidth = iTextureWidth;
	m_iTextureHeight = iTextureHeight;
	m_iTextureDepth = iTextureDepth;

	m_iMaxDim = max(max(iTextureWidth, iTextureHeight), iTextureDepth);

	V_RETURN(CreateVertexBuffers(pTechnique));

	return S_OK;
}

void TextureGrid::DrawSlices()
{
    UINT stride[1] = { sizeof(VS_INPUT_DIFFUSION_STRUCT) };
    UINT offset[1] = { 0 };
    DrawPrimitive( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, m_pLayout, &m_pSlicesBuffer,
        stride, offset, 0, m_iNumVerticesSlices );
}

HRESULT TextureGrid::CreateVertexBuffers(ID3DX11EffectTechnique* pTechnique)
{
	HRESULT hr;

	// Create layout
	CreateLayout(pTechnique);
	
	int index(0);
    VS_INPUT_DIFFUSION_STRUCT *slices(NULL);

    m_iNumVerticesSlices = 6 * (m_iTextureDepth - 2);
    slices = new VS_INPUT_DIFFUSION_STRUCT[ m_iNumVerticesSlices ];

    // Vertex buffer for m_iTextureDepth quads to draw all the slices to a 3D texture
    // (a Geometry Shader is used to send each quad to the appropriate slice)
    index = 0;
	for( int z = 1; z < m_iTextureDepth-1; z++ )
        InitSlice( z, &slices, index );
    assert(index==m_iNumVerticesSlices);
    V_RETURN(CreateVertexBuffer(sizeof(VS_INPUT_DIFFUSION_STRUCT)*m_iNumVerticesSlices,
        D3D11_BIND_VERTEX_BUFFER, &m_pSlicesBuffer, slices , m_iNumVerticesSlices));

	return S_OK;
}

void TextureGrid::InitSlice( int z, VS_INPUT_DIFFUSION_STRUCT** vertices, int& index )
{
    VS_INPUT_DIFFUSION_STRUCT tempVertex1;
    VS_INPUT_DIFFUSION_STRUCT tempVertex2;
    VS_INPUT_DIFFUSION_STRUCT tempVertex3;
    VS_INPUT_DIFFUSION_STRUCT tempVertex4;

    int w = m_iTextureWidth;
    int h = m_iTextureHeight;

    tempVertex1.Pos = D3DXVECTOR3( 1*2.0f/w - 1.0f      , -1*2.0f/h + 1.0f      , 0.0f      );
    tempVertex1.Tex = D3DXVECTOR3( 1.0f                 ,  1.0f                 , float(z)  );

    tempVertex2.Pos = D3DXVECTOR3( (w-1.0f)*2.0f/w-1.0f , -1*2.0f/h + 1.0f      , 0.0f      );
    tempVertex2.Tex = D3DXVECTOR3( (w-1.0f)             ,   1.0f                , float(z)  );

    tempVertex3.Pos = D3DXVECTOR3( (w-1.0f)*2.0f/w-1.0f , -(h-1)*2.0f/h+1.0f    , 0.0f      );
    tempVertex3.Tex = D3DXVECTOR3( (w-1.0f)             , (h-1.0f)              , float(z)  );

    tempVertex4.Pos = D3DXVECTOR3( 1*2.0f/w - 1.0f      , -(h-1.0f)*2.0f/h+1.0f , 0.0f      );
    tempVertex4.Tex = D3DXVECTOR3( 1.0f                 , (h-1.0f)              , float(z)  );


    (*vertices)[index++] = tempVertex1;
    (*vertices)[index++] = tempVertex2;
    (*vertices)[index++] = tempVertex3;
    (*vertices)[index++] = tempVertex1;
    (*vertices)[index++] = tempVertex3;
    (*vertices)[index++] = tempVertex4;

}

HRESULT TextureGrid::CreateLayout(ID3DX11EffectTechnique* pTechnique)
{
	HRESULT hr;
	
	D3D11_INPUT_ELEMENT_DESC layoutDesc[] = 
	{
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
    };

	D3DX11_PASS_SHADER_DESC passVsDesc;
	pTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	V_RETURN(m_pd3dDevice->CreateInputLayout(layoutDesc, _countof(layoutDesc), vsCodePtr, vsCodeLen, &m_pLayout));
	
	return S_OK;
}

HRESULT TextureGrid::CreateVertexBuffer( int ByteWidth, UINT bindFlags, ID3D11Buffer** vertexBuffer,
                                 VS_INPUT_DIFFUSION_STRUCT* vertices,int numVertices)
{
    HRESULT hr;

    D3D11_BUFFER_DESC bd;
    bd.ByteWidth = ByteWidth;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
    InitData.pSysMem = vertices;
    InitData.SysMemPitch = ByteWidth/numVertices;

    V_RETURN( m_pd3dDevice->CreateBuffer( &bd, &InitData,vertexBuffer  ) );

    return S_OK;
}

void TextureGrid::DrawPrimitive( D3D11_PRIMITIVE_TOPOLOGY PrimitiveType, ID3D11InputLayout* layout,
                         ID3D11Buffer** vertexBuffer,UINT* stride, UINT* offset, UINT StartVertex,
                         UINT VertexCount )
{
    m_pd3dImmediateContext->IASetPrimitiveTopology( PrimitiveType );
    m_pd3dImmediateContext->IASetInputLayout( layout );
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, vertexBuffer, stride, offset );
    m_pd3dImmediateContext->Draw( VertexCount, StartVertex ); 
}

