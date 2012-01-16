#include "Globals.h"
#include "SDKmisc.h"
#include "Voxelizer.h"
#include <stddef.h>


Voxelizer::Voxelizer(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pVoxelizerEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pVoxelizerEffect = pVoxelizerEffect;
	m_pDstInOutTexture3D = NULL;
	m_pDstInOutTexRTView = NULL;
	m_width = 0;
	m_height = 0;
	m_depth = 0;
	m_cols = 0;
	m_rows = 0;
	m_initialized = false;
	m_pDSTex2D = NULL;
	m_pDSTex2DDSView = NULL;
	m_pDSTex2DSRView = NULL;
	m_pNZTech = NULL;
	m_pResolveWithPSTech = NULL;
	m_pWorldViewProjectionVar = NULL;
	m_pDSTex2DSRVar = NULL;
	m_pSlicesLayout = NULL;
	m_pSlicesVB = NULL;
}

Voxelizer::~Voxelizer()
{
    Cleanup();
}

void Voxelizer::Cleanup()
{
    SAFE_RELEASE(m_pd3dDevice);

    SAFE_RELEASE(m_pDstInOutTexture3D);
    SAFE_RELEASE(m_pDstInOutTexRTView);

    m_initialized = false;

    SAFE_RELEASE(m_pDSTex2D);
    SAFE_RELEASE(m_pDSTex2DDSView);
    SAFE_RELEASE(m_pDSTex2DSRView);

    m_pNZTech = NULL;
    m_pResolveWithPSTech = NULL;
    m_pWorldViewProjectionVar = NULL;
    m_pDSTex2DSRVar = NULL;

    SAFE_RELEASE(m_pSlicesLayout);
    SAFE_RELEASE(m_pSlicesVB);

    SAFE_RELEASE(m_pInputLayout);
}

HRESULT Voxelizer::SetDestination(ID3D11Texture3D *pDstInOutTexture3D)
{
	m_pDstInOutTexture3D = pDstInOutTexture3D;

    return Initialize();
}

HRESULT Voxelizer::Initialize()
{
    HRESULT hr(S_OK);

    m_initialized = false;
    SAFE_RELEASE(m_pDstInOutTexRTView);

    // Assert inputs are valid
    assert(m_pd3dDevice);
    assert(m_pDstInOutTexture3D != NULL);

    {
        // Create a rendertarget view for the InOut 3D texture
        D3D11_TEXTURE3D_DESC tex3Ddesc;
        m_pDstInOutTexture3D->GetDesc(&tex3Ddesc);
        D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
        rtDesc.Format = tex3Ddesc.Format;
        rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
        rtDesc.Texture3D.MipSlice = 0;
        rtDesc.Texture3D.FirstWSlice = 0;
        rtDesc.Texture3D.WSize = tex3Ddesc.Depth;
        V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pDstInOutTexture3D, &rtDesc, &m_pDstInOutTexRTView));

        // Get witdh, height and depth
        m_width = tex3Ddesc.Width;
        m_height = tex3Ddesc.Height;
        m_depth = tex3Ddesc.Depth;

		
		/*OutputDebugString(L"############################################################\n");
		char bla[100];
		sprintf(bla, "%d | %d | %d \n", m_width, m_height, m_depth);
		OutputDebugStringA(bla); 
		*/
        ComputeRowColsForFlat3DTexture(m_depth, &m_cols, &m_rows);
    }

    assert((m_width > 0) && (m_height > 0) && (m_depth > 0));
    assert((m_cols > 0) && (m_rows > 0));
    assert(UINT(m_cols * m_rows) >= m_depth);

    // Initialize internal texture resources
    hr = InitTextures();
    if(FAILED(hr))
    {
        Cleanup();
        return hr;
    }

    // Load Voxelizer.fx, and get techniques and variables to use (if needed)
    hr = InitShaders();
    if(FAILED(hr))
    {
        Cleanup();
        return hr;
    }

    // Init vertex buffer for a m_depth quads (to convert a "flat 3D texture" to a "3D texture");
    hr = InitSlices();
    if(FAILED(hr))
    {
        Cleanup();
        return hr;
    }

    m_initialized = true;
    return hr;
}

HRESULT Voxelizer::InitTextures()
{
    HRESULT hr(S_OK);

    // release the textures if they were allocated before
    SAFE_RELEASE(m_pDSTex2D);
    SAFE_RELEASE(m_pDSTex2DDSView);
    SAFE_RELEASE(m_pDSTex2DSRView);

    // create DXGI_FORMAT_R24G8_TYPELESS depth-stencil buffer and view
    D3D11_TEXTURE2D_DESC dsTexDesc;
    dsTexDesc.Width = m_width * m_cols;
    dsTexDesc.Height = m_height * m_rows;
    dsTexDesc.MipLevels = 1;
    dsTexDesc.ArraySize = 1;
    dsTexDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    dsTexDesc.SampleDesc.Count = 1;
    dsTexDesc.SampleDesc.Quality = 0;
    dsTexDesc.Usage = D3D11_USAGE_DEFAULT;
    dsTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    dsTexDesc.CPUAccessFlags = 0;
    dsTexDesc.MiscFlags = 0;
    V_RETURN(m_pd3dDevice->CreateTexture2D( &dsTexDesc, NULL, &m_pDSTex2D ));

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC dsViewDesc;
	dsViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsViewDesc.Texture2D.MipSlice = 0;
	dsViewDesc.Flags = 0;
    V_RETURN(m_pd3dDevice->CreateDepthStencilView( m_pDSTex2D, &dsViewDesc, &m_pDSTex2DDSView ));

    // Create the shader resource view for the depth-stencil buffer
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    
    V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pDSTex2D, &srvDesc, &m_pDSTex2DSRView));
    
    return hr;
}

HRESULT Voxelizer::InitShaders()
{
    HRESULT hr(S_OK);

    m_pNZTech = m_pVoxelizerEffect->GetTechniqueByName( "VoxelizeNZ" );
    m_pResolveWithPSTech = m_pVoxelizerEffect->GetTechniqueByName( "VoxelizeResolveWithPS" );

    m_pWorldViewProjectionVar = m_pVoxelizerEffect->GetVariableByName("WorldViewProjection")->AsMatrix();
    m_pDSTex2DSRVar = m_pVoxelizerEffect->GetVariableByName("stencilbufferTex2D")->AsShaderResource();

	assert(m_pNZTech && m_pResolveWithPSTech && m_pWorldViewProjectionVar && m_pDSTex2DSRVar);
    
	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pNZTech->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	// Create our vertex input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

	V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));

    return hr;
}

struct SliceVertex
{
    D3DXVECTOR3 pos;
    D3DXVECTOR3 tex;
};

HRESULT Voxelizer::InitSlices(void)
{
    HRESULT hr(S_OK);

    if( m_pSlicesVB != NULL )
    {
        assert(m_pSlicesLayout != NULL);
        return hr;
    }

    // Create full-screen quad input layout
    const D3D11_INPUT_ELEMENT_DESC slicesLayout[] =
    {
        { "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "Texcoord", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    // Create the input layout
    D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pResolveWithPSTech->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;
		
	V_RETURN(m_pd3dDevice->CreateInputLayout(slicesLayout, _countof(slicesLayout), vsCodePtr, vsCodeLen, &m_pSlicesLayout));
	
#define SLICEQUAD_VTXCNT 6
    
    // Create a vertex buffers of quads, one per slice, wit texcoords to lookup from a flat 3D texture
    //  and with homogenous coordinates to cover a fullscreen quad
    SliceVertex *slicesVertices(NULL);
    try {
         slicesVertices = new SliceVertex[SLICEQUAD_VTXCNT*m_depth];
    }
    catch(...) {
        return E_OUTOFMEMORY;
    }

    SliceVertex sliceVtx[4];
    int row, col;
    float x, y;
    int vtxIdx = 0;
    for(UINT z= 0; z<m_depth; z++) {
        row = z / m_cols;
        col = z % m_cols;
        x = float(col) * m_width;
        y = float(row) * m_height;
        vtxIdx = z*SLICEQUAD_VTXCNT;

        sliceVtx[0].pos = D3DXVECTOR3(-1.0f, 1.0f, 0.5f);
        sliceVtx[0].tex = D3DXVECTOR3(x, y, float(z));
        
        sliceVtx[1].pos = D3DXVECTOR3(-1.0f, -1.0f, 0.5f);
        sliceVtx[1].tex = D3DXVECTOR3(x, y+m_height, float(z));
        
        sliceVtx[2].pos = D3DXVECTOR3(1.0f, -1.0f, 0.5f);
        sliceVtx[2].tex = D3DXVECTOR3(x+m_width, y+m_height, float(z));
        
        sliceVtx[3].pos = D3DXVECTOR3(1.0f, 1.0f, 0.5f);
        sliceVtx[3].tex = D3DXVECTOR3(x+m_width, y, float(z));

        slicesVertices[vtxIdx+0] = sliceVtx[0];
        slicesVertices[vtxIdx+1] = sliceVtx[1];
        slicesVertices[vtxIdx+2] = sliceVtx[2];
        slicesVertices[vtxIdx+3] = sliceVtx[0];
        slicesVertices[vtxIdx+4] = sliceVtx[2];
        slicesVertices[vtxIdx+5] = sliceVtx[3];
    }

    D3D11_BUFFER_DESC vbDesc =
    {
        SLICEQUAD_VTXCNT*m_depth*sizeof(SliceVertex),
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER,
        0,
        0
    };

    D3D11_SUBRESOURCE_DATA initialData;
    initialData.pSysMem = slicesVertices;
    initialData.SysMemPitch = 0;
    initialData.SysMemSlicePitch = 0;
    V_RETURN(m_pd3dDevice->CreateBuffer( &vbDesc, &initialData, &m_pSlicesVB ));

    delete []slicesVertices;

    return hr;
}

void Voxelizer::DrawSlices(void)
{
    assert(m_pSlicesLayout);
    assert(m_pSlicesVB);

    UINT strides = sizeof(SliceVertex);
    UINT offsets = 0;

    m_pd3dImmediateContext->IASetInputLayout( m_pSlicesLayout );
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pSlicesVB, &strides, &offsets );
    m_pd3dImmediateContext->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    m_pd3dImmediateContext->Draw(SLICEQUAD_VTXCNT*m_depth, 0);
}


HRESULT Voxelizer::Voxelize(Surface *pSurface1, Surface *pSurface2)
{
    m_pSurface1 = pSurface1;
	m_pSurface2 = pSurface2;

    HRESULT hr = DoVoxelization();

    m_pSurface1 = NULL;
	m_pSurface2 = NULL;
    
    return hr;
}

HRESULT Voxelizer::DoVoxelization(void)
{
    HRESULT hr(S_OK);

    // Do the actual voxelization
    hr = StencilClipVolume();

    m_pd3dImmediateContext->ClearState();

    return hr;
}

void Voxelizer::UpdateMinMax()
{
	for(int i = 0; i < m_pSurface1->m_vNum; i++)
	{
		VERTEX tempver = m_pSurface1->m_pVertices[i];
		D3DXVECTOR4 temp;
		D3DXVec3Transform(&temp, &D3DXVECTOR3(tempver.x, tempver.y, tempver.z), &m_pSurface1->m_mModel);
		if(i == 0)
		{
			m_xMin = temp.x;
			m_xMax = temp.x;
			m_yMin = temp.y;
			m_yMax = temp.y;
			m_zMin = temp.z;
			m_zMax = temp.z;
		}

		if(temp.x < m_xMin)
			m_xMin = temp.x;
		if(temp.x > m_xMax)
			m_xMax = temp.x;
		if(temp.y < m_yMin)
			m_yMin = temp.y;
		if(temp.y > m_yMax)
			m_yMax = temp.y;
		if(temp.z < m_zMin)
			m_zMin = temp.z;
		if(temp.z > m_zMax)
			m_zMax = temp.z;
	}

	/*for(int i = 0; i < m_pSurface2->m_vNum; i++)
	{
		VERTEX tempver = m_pSurface2->m_pVertices[i];
		D3DXVECTOR4 temp;
		D3DXVec3Transform(&temp, &D3DXVECTOR3(tempver.x, tempver.y, tempver.z), &m_pSurface2->m_mModel);
		if(temp.x < xMin)
			xMin = temp.x;
		if(temp.x > xMax)
			xMax = temp.x;
		if(temp.z < yMin)
			yMin = temp.z;
		if(temp.z > yMax)
			yMax = temp.z;
	}*/
}

//
// StencilClipVolume algorithm summary:
// ====================================
// 
// function DrawClippedMesh():
//   set a vertex shader that transforms the vertices to the volume space
//   set stencil functions for NZ rule (incr on back and decr on front)
//   set the near plane to be aligned with each slice 
//   render the mesh
// done
//
// function StencilClipVolume():
//   clear depthstencil buffer
//   bind as rendertarget a NULL color buffer and the depthstencil buffer
//
//   for each slice:
//     set the viewport and cliprect to match the slice's 2D region
//     DrawClippedMesh
//   done
//
//   bind as rendertarget the 3D-texture and set viewport and scissor to match a slice
//   do a resolve pass to write out 1 for inside voxels on each slice in the 3D-texture
// done

HRESULT Voxelizer::StencilClipVolume(void)
{
    HRESULT hr(S_OK);
    int x, y;

	UpdateMinMax();

    assert(m_initialized);
    
    // clear depthstencil buffer to 0
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDSTex2DDSView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 0, 0);        
    // set the depthstencil buffer as rendertarget (no color buffer)
    m_pd3dImmediateContext->OMSetRenderTargets(0, NULL, m_pDSTex2DDSView);
    for( UINT z=0; z<m_depth; z++)
    {
        // compute x and y coordinates for the TOP-LEFT corner of the slice in the flat 3D texture
        x = (z % m_cols) * m_width;
        y = (z / m_cols) * m_height;

        // set viewport and scissor to match the size of single slice
        D3D11_VIEWPORT viewport = { float(x), float(y), float(m_width), float(m_height), 0.0f, 1.0f };
        m_pd3dImmediateContext->RSSetViewports(1, &viewport);
        D3D11_RECT scissorRect = { x, y, x+m_width, y+m_height };
        m_pd3dImmediateContext->RSSetScissorRects(1, &scissorRect);

		float distFromZMinToZMax = m_zMax - m_zMin;
        V_RETURN(RenderClippedMesh((float)z/m_depth*distFromZMinToZMax + m_zMin, m_zMax, m_pNZTech));
    }

    // set texture as rendertarget
    m_pd3dImmediateContext->OMSetRenderTargets(1, &m_pDstInOutTexRTView, NULL );
    // Set a resolve PixelShader (instead of using stencil test)
    //  to resolve the stencil buffer into the final texture
    V_RETURN(m_pDSTex2DSRVar->SetResource(m_pDSTex2DSRView));
    V_RETURN(m_pResolveWithPSTech->GetPassByIndex(0)->Apply(0, m_pd3dImmediateContext));

    // Set viewport and scissor to match the size of a single slice 
    D3D11_VIEWPORT viewport = { 0, 0, float(m_width), float(m_height), 0.0f, 1.0f };
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);
    D3D11_RECT scissorRect = { 0, 0, m_width, m_height };
    m_pd3dImmediateContext->RSSetScissorRects(1, &scissorRect);

    DrawSlices();

    V_RETURN(m_pDSTex2DSRVar->SetResource(NULL));
    m_pd3dImmediateContext->OMSetRenderTargets(0, NULL, NULL );

    return hr;
}

HRESULT Voxelizer::RenderClippedMesh(float zNear, float zFar, ID3DX11EffectTechnique *pTechnique)
{
    HRESULT hr(S_OK);
    D3DXMATRIX proj;
    D3DXMATRIX worldViewProj;
	D3DXMATRIX modelViewProj;

	

	


	D3DXMatrixOrthoOffCenterLH(&proj, m_xMin, m_xMax, m_yMin, m_yMax, zNear, zFar);
	D3DXMatrixMultiply(&worldViewProj, &m_pSurface1->m_mModel, &proj);
    V_RETURN(m_pWorldViewProjectionVar->SetMatrix(reinterpret_cast<float*>(&worldViewProj)));
    
	m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout);
	
	//Render surface
	m_pSurface1->Render(pTechnique);

    return hr;
}
