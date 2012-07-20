#include "Globals.h"
#include "Voronoi.h"

/****************************************************************************
 ****************************************************************************/
Voronoi::Voronoi(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pVoronoiEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pVoronoiEffect = pVoronoiEffect;

	m_pDestColorTex3D = NULL;
	m_pDestDistTex3D = NULL;
	m_pDepthStencil = NULL;
	m_pDestColorTex3DRTV = NULL;
	m_pDestDistTex3DRTV = NULL;
	m_pDepthStencilView = NULL;
	m_iTextureWidth = 0;
	m_iTextureHeight = 0;
	m_iTextureDepth = 0;

	m_pSurface1 = NULL;
	m_pSurface2 = NULL;

	m_pFlatColorTex = NULL;
	m_pFlatColorTexRTV = NULL;
	m_pFlatColorTexSRV = NULL;

	m_pFlatDistTex = NULL;
	m_pFlatDistTexRTV = NULL;
	m_pFlatDistTexSRV = NULL;

	m_pSlicesLayout = NULL;
	m_pSlicesVB = NULL;

	m_iCurrentSlice = 0;

	m_bRenderIsoSurface = false;

	m_bRenderToFlatTexture = true;
	m_bRenderFlatTo3DTexture = false;
}

/****************************************************************************
 ****************************************************************************/
Voronoi::~Voronoi()
{
	Cleanup();
}

void Voronoi::Cleanup()
{
	SAFE_RELEASE(m_pDestColorTex3DRTV);
	SAFE_RELEASE(m_pDestDistTex3DRTV);

	SAFE_RELEASE(m_pFlatColorTex);
	SAFE_RELEASE(m_pFlatColorTexRTV);
	SAFE_RELEASE(m_pFlatColorTexSRV);

	SAFE_RELEASE(m_pFlatDistTex);
	SAFE_RELEASE(m_pFlatDistTexRTV);
	SAFE_RELEASE(m_pFlatDistTexSRV);

	SAFE_RELEASE(m_pDepthStencil);
	SAFE_RELEASE(m_pDepthStencilView);

	SAFE_RELEASE(m_pSlicesLayout);
	SAFE_RELEASE(m_pSlicesVB);
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::Initialize()
{
	HRESULT hr;

	assert(m_pd3dDevice);
	assert(m_pd3dImmediateContext);

	//Initialize Techniques and Shadervariables
	V_RETURN(InitShaders());
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::SetDestination(ID3D11Texture3D *pDestColorTex3D, ID3D11Texture3D *pDestDistTex3D)
{
	m_pDestColorTex3D = pDestColorTex3D;
	m_pDestDistTex3D = pDestDistTex3D;

	return Update();
}

/****************************************************************************
 ****************************************************************************/
void Voronoi::SetSurfaces(Surface *pSurface1, Surface *pSurface2)
{
	m_pSurface1 = pSurface1;
	m_pSurface2 = pSurface2;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::Update()
{
	HRESULT hr(S_OK);

	//Initialize Rendertargets for the 3D Textures -- needs to happen before depth stencil initialization
	// because m_iTextureWidth etc. are initialized in InitRendertargets3D
	V_RETURN(InitRendertargets3D());

	ComputeRowColsForFlat3DTexture(m_iTextureDepth, &m_cols, &m_rows);

	//Initialize Flat Textures, their RTVs and SRV; initialize DepthStencil
	V_RETURN(InitFlatTextures());

	//Initialize Slices
	V_RETURN(InitSlices());
	
	//Re-initialize the behaviour variables for incremental voronoi generation
	m_iCurrentSlice = 0;
	m_bRenderToFlatTexture = true;
	m_bRenderFlatTo3DTexture = false;

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::InitFlatTextures()
{
	HRESULT hr;

	//Release old textures
	SAFE_RELEASE(m_pFlatColorTex);
	SAFE_RELEASE(m_pFlatColorTexRTV);
	SAFE_RELEASE(m_pFlatColorTexSRV);

	SAFE_RELEASE(m_pFlatDistTex);
	SAFE_RELEASE(m_pFlatDistTexRTV);
	SAFE_RELEASE(m_pFlatDistTexSRV);

	SAFE_RELEASE(m_pDepthStencil);
	SAFE_RELEASE(m_pDepthStencilView);

	// Create color and dist texture
	D3D11_TEXTURE2D_DESC cdTexDesc;
	cdTexDesc.ArraySize = 1;
	cdTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	cdTexDesc.CPUAccessFlags = 0;
	cdTexDesc.MipLevels = 1;
	cdTexDesc.MiscFlags = 0;
	cdTexDesc.SampleDesc.Count = 1;
	cdTexDesc.SampleDesc.Quality = 0;
	cdTexDesc.Usage = D3D11_USAGE_DEFAULT;
	cdTexDesc.Width = m_iTextureWidth * m_cols;
	cdTexDesc.Height = m_iTextureHeight * m_rows;
	cdTexDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	V_RETURN(m_pd3dDevice->CreateTexture2D(&cdTexDesc, NULL, &m_pFlatColorTex));
	V_RETURN(m_pd3dDevice->CreateTexture2D(&cdTexDesc, NULL, &m_pFlatDistTex));

	DXUT_SetDebugName( m_pFlatColorTex, "Voronoi Flat Texture" );
	DXUT_SetDebugName( m_pFlatDistTex, "Dist Flat Texture" );

	//create RTVs for color and dist texture
	D3D11_RENDER_TARGET_VIEW_DESC cdRTVDesc;
	cdRTVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	cdRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	cdRTVDesc.Texture2D.MipSlice = 0;
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pFlatColorTex, &cdRTVDesc, &m_pFlatColorTexRTV));
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pFlatDistTex, &cdRTVDesc, &m_pFlatDistTexRTV));

	//create SRVs for color and dist texture
	D3D11_SHADER_RESOURCE_VIEW_DESC cdSRVDesc;
	ZeroMemory(&cdSRVDesc, sizeof(cdSRVDesc));
	cdSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	cdSRVDesc.Texture2D.MostDetailedMip = 0;
	cdSRVDesc.Texture2D.MipLevels = 1;
	cdSRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pFlatColorTex, &cdSRVDesc, &m_pFlatColorTexSRV));
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pFlatDistTex, &cdSRVDesc, &m_pFlatDistTexSRV));

	//create depth stencil texture and its RTV
	D3D11_TEXTURE2D_DESC dsTexDesc;
	dsTexDesc.Width = m_iTextureWidth * m_cols;
	dsTexDesc.Height = m_iTextureHeight * m_rows;
	dsTexDesc.MipLevels = 1;
	dsTexDesc.ArraySize = 1;
	dsTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsTexDesc.SampleDesc.Count = 1;
	dsTexDesc.SampleDesc.Quality = 0;
	dsTexDesc.Usage = D3D11_USAGE_DEFAULT;
	dsTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dsTexDesc.CPUAccessFlags = 0;
	dsTexDesc.MiscFlags = 0;
	V_RETURN(m_pd3dDevice->CreateTexture2D(&dsTexDesc, NULL, &m_pDepthStencil));
	V_RETURN(m_pd3dDevice->CreateDepthStencilView(m_pDepthStencil, NULL, &m_pDepthStencilView));

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::InitRendertargets3D()
{
	HRESULT hr;

	assert(m_pDestColorTex3D != NULL);
	assert(m_pDestDistTex3D != NULL);

	//Release old textures
	SAFE_RELEASE(m_pDestColorTex3DRTV);
	SAFE_RELEASE(m_pDestDistTex3DRTV);

	//create color texture
	D3D11_TEXTURE3D_DESC descColorTex3D;
	m_pDestColorTex3D->GetDesc(&descColorTex3D);
	D3D11_RENDER_TARGET_VIEW_DESC descCT3DRTV;
	descCT3DRTV.Format = descColorTex3D.Format;
	descCT3DRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descCT3DRTV.Texture3D.MipSlice = 0;
	descCT3DRTV.Texture3D.FirstWSlice = 0;
	descCT3DRTV.Texture3D.WSize = descColorTex3D.Depth;
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pDestColorTex3D, &descCT3DRTV, &m_pDestColorTex3DRTV));

	//create dist texture
	D3D11_TEXTURE3D_DESC descDistTex3D;
	m_pDestDistTex3D->GetDesc(&descDistTex3D);
	D3D11_RENDER_TARGET_VIEW_DESC descDT3DRTV;
	descDT3DRTV.Format = descDistTex3D.Format;
	descDT3DRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descDT3DRTV.Texture3D.MipSlice = 0;
	descDT3DRTV.Texture3D.FirstWSlice = 0;
	descDT3DRTV.Texture3D.WSize = descDistTex3D.Depth;
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pDestDistTex3D, &descDT3DRTV, &m_pDestDistTex3DRTV));

	m_iTextureWidth = descColorTex3D.Width;
	m_iTextureHeight = descColorTex3D.Height;
	m_iTextureDepth = descColorTex3D.Depth;

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::InitShaders()
{
	HRESULT hr;

	assert(m_pVoronoiEffect);

	// Get Technique and variables
	m_pVoronoiDiagramTechnique	= m_pVoronoiEffect->GetTechniqueByName("GenerateVoronoiDiagram");
	m_pFlatTo3DTexTechnique		= m_pVoronoiEffect->GetTechniqueByName("Flat2DTextureTo3D");

	m_pModelViewProjectionVar	= m_pVoronoiEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();
	m_pNormalMatrixVar			= m_pVoronoiEffect->GetVariableByName("NormalMatrix")->AsMatrix();
	m_pSliceIndexVar			= m_pVoronoiEffect->GetVariableByName("iSliceIndex")->AsScalar();
	m_pTextureSizeVar			= m_pVoronoiEffect->GetVariableByName("vTextureSize")->AsVector();
	m_pBBMinVar					= m_pVoronoiEffect->GetVariableByName("vBBMin")->AsVector();
	m_pBBMaxVar					= m_pVoronoiEffect->GetVariableByName("vBBMax")->AsVector();
	m_pFlatColorTex2DSRVar		= m_pVoronoiEffect->GetVariableByName("flatColorTexture")->AsShaderResource();
	m_pFlatDistTex2DSRVar		= m_pVoronoiEffect->GetVariableByName("flatDistTexture")->AsShaderResource();
	m_pSurfaceTextureVar		= m_pVoronoiEffect->GetVariableByName("SurfaceTexture")->AsShaderResource();
	m_pIsoSurfaceVar			= m_pVoronoiEffect->GetVariableByName("fIsoSurfaceVal")->AsScalar();

	assert(m_pVoronoiDiagramTechnique);
	assert(m_pModelViewProjectionVar);
	assert(m_pSliceIndexVar);
	assert(m_pTextureSizeVar);
	assert(m_pBBMinVar);
	assert(m_pBBMaxVar);
	assert(m_pFlatColorTex2DSRVar);
	assert(m_pFlatDistTex2DSRVar);

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::InitSlices()
{
	HRESULT hr;

	SAFE_RELEASE(m_pSlicesLayout);
	SAFE_RELEASE(m_pSlicesVB);

	//Create full-screen quad input layout
	const D3D11_INPUT_ELEMENT_DESC slicesLayout[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pFlatTo3DTexTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;
	V_RETURN(m_pd3dDevice->CreateInputLayout(slicesLayout, _countof(slicesLayout), vsCodePtr, vsCodeLen, &m_pSlicesLayout));


#define SLICEQUAD_VERTEX_COUNT 6
	// Create a vertex buffers of quads, one per slice, with texcoords to lookup from a flat 3D texture
    // and with homogenous coordinates to cover a fullscreen quad
	SLICE_SCREENQUAD_VERTEX* sliceVertices = new SLICE_SCREENQUAD_VERTEX[SLICEQUAD_VERTEX_COUNT*m_iTextureDepth];
	SLICE_SCREENQUAD_VERTEX sliceVerticesTemp[4];
	int row, col;
	float x1, y1, x2, y2;
	int vertexIndex = 0;

	for(int z = 0; z < m_iTextureDepth; z++)
	{
		row = z / m_cols;
		col = z % m_cols;
		x1 = float(col)/m_cols;
		y1 = float(row)/m_rows;
		x2 = float(col+1)/m_cols;
		y2 = float(row+1)/m_rows;
		
		vertexIndex = z * SLICEQUAD_VERTEX_COUNT;

		sliceVerticesTemp[0].pos = D3DXVECTOR3(-1.0f, 1.0f, 0.5f);
		sliceVerticesTemp[0].tex = D3DXVECTOR3(x1, y2, float(z));

		sliceVerticesTemp[1].pos = D3DXVECTOR3(-1.0f, -1.0f, 0.5f);
		sliceVerticesTemp[1].tex = D3DXVECTOR3(x1, y1, float(z));
        
        sliceVerticesTemp[2].pos = D3DXVECTOR3(1.0f, -1.0f, 0.5f);
		sliceVerticesTemp[2].tex = D3DXVECTOR3(x2, y1, float(z));
        
        sliceVerticesTemp[3].pos = D3DXVECTOR3(1.0f, 1.0f, 0.5f);
		sliceVerticesTemp[3].tex = D3DXVECTOR3(x2, y2, float(z));

		sliceVertices[vertexIndex+0] = sliceVerticesTemp[0];
		sliceVertices[vertexIndex+1] = sliceVerticesTemp[1];
        sliceVertices[vertexIndex+2] = sliceVerticesTemp[2];
        sliceVertices[vertexIndex+3] = sliceVerticesTemp[0];
        sliceVertices[vertexIndex+4] = sliceVerticesTemp[2];
        sliceVertices[vertexIndex+5] = sliceVerticesTemp[3];

	}

	//create vertex buffer for the slices
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = SLICEQUAD_VERTEX_COUNT*m_iTextureDepth*sizeof(SLICE_SCREENQUAD_VERTEX);
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initialData;
	initialData.pSysMem = sliceVertices;
	initialData.SysMemPitch = 0;
	initialData.SysMemSlicePitch = 0;
	V_RETURN(m_pd3dDevice->CreateBuffer(&vbDesc, &initialData, &m_pSlicesVB));

	delete[] sliceVertices;

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
bool Voronoi::RenderVoronoi(D3DXVECTOR3 vBBMin, D3DXVECTOR3 vBBMax, bool bRenderIsoSurface)
{
	//store the old render targets and viewports
    ID3D11RenderTargetView* pOldRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pOldDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	m_pd3dImmediateContext->RSGetViewports( &NumViewports, &pViewports[0]);

	D3DXMATRIX orth, model1Orth, model2Orth;

	// generate orth. matrix with bounding parameters
	D3DXMatrixOrthoOffCenterLH(&orth, vBBMin.x, vBBMax.x, vBBMin.y, vBBMax.y, vBBMin.z, vBBMax.z);
	D3DXMatrixMultiply(&model1Orth, &m_pSurface1->m_mModel, &orth);
	D3DXMatrixMultiply(&model2Orth, &m_pSurface2->m_mModel, &orth);

	//set bounding box parameters
	D3DXVECTOR4 vBBMinOrth, vBBMaxOrth;
	D3DXVec3Transform(&vBBMinOrth, &vBBMin, &orth);
	D3DXVec3Transform(&vBBMaxOrth, &vBBMax, &orth);

	assert(vBBMinOrth);
	assert(vBBMaxOrth);

	//Compute NormalMatrix for both surfaces
	D3DXMATRIX mModel1_3x3 = D3DXMATRIX(m_pSurface1->m_mModel._11, m_pSurface1->m_mModel._12, m_pSurface1->m_mModel._13, 0.0f, 
									  m_pSurface1->m_mModel._21, m_pSurface1->m_mModel._22, m_pSurface1->m_mModel._23, 0.0f, 
									  m_pSurface1->m_mModel._31, m_pSurface1->m_mModel._32, m_pSurface1->m_mModel._33, 0.0f, 
									  0.0f, 0.0f, 0.0f, 1.0f);
	D3DXMATRIX mModel1_3x3Inv, mNormalMatrix1;
	D3DXMatrixInverse(&mModel1_3x3Inv, NULL, &mModel1_3x3);
	D3DXMatrixTranspose(&mNormalMatrix1, &mModel1_3x3Inv);

	D3DXMATRIX mModel2_3x3 = D3DXMATRIX(m_pSurface2->m_mModel._11, m_pSurface2->m_mModel._12, m_pSurface2->m_mModel._13, 0.0f, 
									  m_pSurface2->m_mModel._21, m_pSurface2->m_mModel._22, m_pSurface2->m_mModel._23, 0.0f, 
									  m_pSurface2->m_mModel._31, m_pSurface2->m_mModel._32, m_pSurface2->m_mModel._33, 0.0f, 
									  0.0f, 0.0f, 0.0f, 1.0f);
	D3DXMATRIX mModel2_3x3Inv, mNormalMatrix2;
	D3DXMatrixInverse(&mModel2_3x3Inv, NULL, &mModel2_3x3);
	D3DXMatrixTranspose(&mNormalMatrix2, &mModel2_3x3Inv);
	
	// Set Variables needed for Voronoi Diagram Computation
	m_pBBMinVar->SetFloatVector(vBBMinOrth);
	m_pBBMaxVar->SetFloatVector(vBBMaxOrth);
	m_pTextureSizeVar->SetFloatVector(D3DXVECTOR3((float)m_iTextureWidth, (float)m_iTextureHeight, (float)m_iTextureDepth));

	// Set Flat Textures as Rendertargets
	ID3D11RenderTargetView* destFlatTex2DRTVs[2];
	destFlatTex2DRTVs[0] = m_pFlatColorTexRTV;
	destFlatTex2DRTVs[1] = m_pFlatDistTexRTV;
	m_pd3dImmediateContext->OMSetRenderTargets(2, destFlatTex2DRTVs, m_pDepthStencilView);


	//Render To Flat Texture
	if(m_bRenderToFlatTexture)
	{	
		RenderToFlatTexture(model1Orth, model2Orth, mNormalMatrix1, mNormalMatrix2, m_iCurrentSlice);
		m_iCurrentSlice++;

		//restore old render targets
		m_pd3dImmediateContext->OMSetRenderTargets( 1,  &pOldRTV,  pOldDSV );
		m_pd3dImmediateContext->RSSetViewports( NumViewports, &pViewports[0]);

		if(m_iCurrentSlice == m_iTextureDepth)
		{
			m_iCurrentSlice = 0;
			m_bRenderToFlatTexture = false;
			m_bRenderFlatTo3DTexture = true;
		}

		return false;
	}
	else if(m_bRenderFlatTo3DTexture)
	{
		//Set 3D Textures as RenderTargets
		ID3D11RenderTargetView* destTex3DRTVs[2];
		destTex3DRTVs[0] = m_pDestColorTex3DRTV;
		destTex3DRTVs[1] = m_pDestDistTex3DRTV;
		m_pd3dImmediateContext->OMSetRenderTargets(2, destTex3DRTVs, NULL);
	
		//Set Flat Textures as Variables in Voronoi Shader
		m_pFlatColorTex2DSRVar->SetResource(m_pFlatColorTexSRV);
		m_pFlatDistTex2DSRVar->SetResource(m_pFlatDistTexSRV);

		// Render Flat Textures to 3D Textures - slice by slice
		m_pFlatTo3DTexTechnique->GetPassByIndex(0)->Apply(0, m_pd3dImmediateContext);
			
		// Set viewport and scissor to match the size of a single slice 
		D3D11_VIEWPORT viewport2 = { 0, 0, float(m_iTextureWidth), float(m_iTextureHeight), 0.0f, 1.0f };
	    m_pd3dImmediateContext->RSSetViewports(1, &viewport2);
		D3D11_RECT scissorRect2 = { 0, 0, m_iTextureWidth, m_iTextureHeight};
		m_pd3dImmediateContext->RSSetScissorRects(1, &scissorRect2);
		DrawCurrentSlice();
		m_iCurrentSlice++;

		m_pFlatColorTex2DSRVar->SetResource(NULL);
		m_pFlatDistTex2DSRVar->SetResource(NULL);
		m_pFlatTo3DTexTechnique->GetPassByIndex(0)->Apply(0, m_pd3dImmediateContext);

		if(m_iCurrentSlice == m_iTextureDepth)
		{
			m_iCurrentSlice = 0;
			m_bRenderToFlatTexture = false;
			m_bRenderFlatTo3DTexture = false;
		}
		else
		{
			//restore old render targets
			m_pd3dImmediateContext->OMSetRenderTargets( 1,  &pOldRTV,  pOldDSV );
			m_pd3dImmediateContext->RSSetViewports( NumViewports, &pViewports[0]);
			return false;
		}
	}
	
	//restore old render targets
	m_pd3dImmediateContext->OMSetRenderTargets( 1,  &pOldRTV,  pOldDSV );
	m_pd3dImmediateContext->RSSetViewports( NumViewports, &pViewports[0]);

	return true;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::RenderToFlatTexture(D3DXMATRIX mModel1Orth, D3DXMATRIX mModel2Orth, D3DXMATRIX mNormalMatrix1, D3DXMATRIX mNormalMatrix2, int iSliceIndex)
{
	HRESULT hr;

	// compute x and y coordinates for the TOP-LEFT corner of the slice in the flat 3D texture
	int x = (iSliceIndex % m_cols) * m_iTextureWidth;
	int y = (iSliceIndex / m_cols) * m_iTextureHeight;

	// set viewport and scissor to match the size of single slice
	D3D11_VIEWPORT viewport = { float(x), float(y), float(m_iTextureWidth), float(m_iTextureHeight), 0.0f, 1.0f };
	m_pd3dImmediateContext->RSSetViewports(1, &viewport);
	D3D11_RECT scissorRect = { x, y, x+m_iTextureWidth, y+m_iTextureHeight};
	m_pd3dImmediateContext->RSSetScissorRects(1, &scissorRect);
	
	// Set Flat Textures and depthstencilview as rendertargets
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	V_RETURN(m_pSliceIndexVar->SetInt(iSliceIndex));

	// Render to flat textures
	m_pModelViewProjectionVar->SetMatrix(mModel1Orth);
	m_pNormalMatrixVar->SetMatrix(mNormalMatrix1);
	m_pIsoSurfaceVar->SetFloat(m_pSurface1->GetIsoColor());
	m_pSurface1->RenderVoronoi(m_pVoronoiDiagramTechnique, m_pSurfaceTextureVar);

	m_pModelViewProjectionVar->SetMatrix(mModel2Orth);
	m_pNormalMatrixVar->SetMatrix(mNormalMatrix2);
	m_pIsoSurfaceVar->SetFloat(m_pSurface2->GetIsoColor());
	m_pSurface2->RenderVoronoi(m_pVoronoiDiagramTechnique, m_pSurfaceTextureVar);
	
	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
void Voronoi::DrawSlices()
{
	assert(m_pSlicesLayout);
	assert(m_pSlicesVB);

	UINT strides = sizeof(SLICE_SCREENQUAD_VERTEX);
	UINT offsets = 0;

	m_pd3dImmediateContext->IASetInputLayout(m_pSlicesLayout);
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pSlicesVB, &strides, &offsets);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_pd3dImmediateContext->Draw(SLICEQUAD_VERTEX_COUNT*m_iTextureDepth, 0);
}

/****************************************************************************
 ****************************************************************************/
void Voronoi::DrawCurrentSlice()
{
	assert(m_pSlicesLayout);
	assert(m_pSlicesVB);

	UINT strides = sizeof(SLICE_SCREENQUAD_VERTEX);
	UINT offsets = 0;

	m_pd3dImmediateContext->IASetInputLayout(m_pSlicesLayout);
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pSlicesVB, &strides, &offsets);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_pd3dImmediateContext->Draw(SLICEQUAD_VERTEX_COUNT, SLICEQUAD_VERTEX_COUNT*m_iCurrentSlice);
}