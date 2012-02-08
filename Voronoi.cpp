#include "Globals.h"
#include "Voronoi.h"

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
}

Voronoi::~Voronoi()
{
}

void Voronoi::Cleanup()
{
	SAFE_RELEASE(m_pDestColorTex3DRTV);
	SAFE_RELEASE(m_pDestDistTex3DRTV);
	SAFE_RELEASE(m_pDepthStencil);
	SAFE_RELEASE(m_pDepthStencilView);
}

HRESULT Voronoi::SetDestination(ID3D11Texture3D *pDestColorTex3D, ID3D11Texture3D *pDestDistTex3D)
{
	m_pDestColorTex3D = pDestColorTex3D;
	m_pDestDistTex3D = pDestDistTex3D;

	return Initialize();
}

HRESULT Voronoi::Initialize()
{
	HRESULT hr(S_OK);

	assert(m_pd3dDevice);
	assert(m_pd3dImmediateContext);



	//Initialize Rendertargets for the 3D Textures -- needs to happen before depth stencil initialization
	// because m_iTextureWidth etc. are initialized in InitRendertargets3D
	hr = InitRendertargets3D();
	if(FAILED(hr))
	{
		Cleanup();
		return hr;
	}

	//Initialize DepthStencil Texture and -view
	hr = InitDepthStencil();
	if(FAILED(hr))
	{
		Cleanup();
		return hr;
	}

	//Initialize Techniques and Shadervariables
	hr = InitShaders();
	if(FAILED(hr))
	{
		Cleanup();
		return hr;
	}

	
}

HRESULT Voronoi::InitDepthStencil()
{
	HRESULT hr;
	SAFE_RELEASE(m_pDepthStencil);
	SAFE_RELEASE(m_pDepthStencilView);

	D3D11_TEXTURE2D_DESC dsTexDesc;
	dsTexDesc.Width = m_iTextureWidth;
	dsTexDesc.Height = m_iTextureHeight;
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

HRESULT Voronoi::InitRendertargets3D()
{
	HRESULT hr;

	assert(m_pDestColorTex3D != NULL);
	assert(m_pDestDistTex3D != NULL);

	SAFE_RELEASE(m_pDestColorTex3DRTV);
	SAFE_RELEASE(m_pDestDistTex3DRTV);

	D3D11_TEXTURE3D_DESC descColorTex3D;
	m_pDestColorTex3D->GetDesc(&descColorTex3D);
	D3D11_RENDER_TARGET_VIEW_DESC descCT3DRTV;
	descCT3DRTV.Format = descColorTex3D.Format;
	descCT3DRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descCT3DRTV.Texture3D.MipSlice = 0;
	descCT3DRTV.Texture3D.FirstWSlice = 0;
	descCT3DRTV.Texture3D.WSize = descColorTex3D.Depth;
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pDestColorTex3D, &descCT3DRTV, &m_pDestColorTex3DRTV));

	D3D11_TEXTURE3D_DESC descDistTex3D;
	m_pDestDistTex3D->GetDesc(&descDistTex3D);
	D3D11_RENDER_TARGET_VIEW_DESC descDT3DRTV;
	descDT3DRTV.Format = descDistTex3D.Format;
	descDT3DRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descDT3DRTV.Texture3D.MipSlice = 0;
	descDT3DRTV.Texture3D.FirstWSlice = 0;
	descDT3DRTV.Texture3D.WSize = descDistTex3D.Depth;
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pDestDistTex3D, &descCT3DRTV, &m_pDestDistTex3DRTV));

	m_iTextureWidth = descColorTex3D.Width;
	m_iTextureHeight = descColorTex3D.Height;
	m_iTextureDepth = descColorTex3D.Depth;

	return S_OK;
}

HRESULT Voronoi::InitShaders()
{
	HRESULT hr;

	assert(m_pVoronoiEffect);

	// Get Technique and variables
	m_pVoronoiDiagramTechnique	= m_pVoronoiEffect->GetTechniqueByName("GenerateVoronoiDiagram");

	m_pModelViewProjectionVar	= m_pVoronoiEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();
	m_pSliceIndexVar			= m_pVoronoiEffect->GetVariableByName("iSliceIndex")->AsScalar();
	m_pTextureDepthVar			= m_pVoronoiEffect->GetVariableByName("iTextureDepth")->AsScalar();
	m_pBBMinVar					= m_pVoronoiEffect->GetVariableByName("vBBMin")->AsVector();
	m_pBBMaxVar					= m_pVoronoiEffect->GetVariableByName("vBBMax")->AsVector();

	assert(m_pVoronoiDiagramTechnique);
	assert(m_pModelViewProjectionVar);
	assert(m_pSliceIndexVar);
	assert(m_pTextureDepthVar);
	assert(m_pBBMinVar);
	assert(m_pBBMaxVar);

	//Create InputLayout
	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pVoronoiDiagramTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));

	return S_OK;
}

HRESULT Voronoi::RenderVoronoi(Surface *pSurface1, Surface *pSurface2, D3DXVECTOR3 vBBMin, D3DXVECTOR3 vBBMax)
{
	//TODO
	HRESULT hr;
	D3DXMATRIX orth, model1Orth, model2Orth;

	// generate orth. matrix with bounding parameters
	D3DXMatrixOrthoOffCenterLH(&orth, vBBMin.x, vBBMax.x, vBBMin.y, vBBMax.y, vBBMin.z, vBBMax.z);
	D3DXMatrixMultiply(&model1Orth, &pSurface1->m_mModel, &orth);
	D3DXMatrixMultiply(&model2Orth, &pSurface2->m_mModel, &orth);

	//set bounding box parameters
	D3DXVECTOR4 vBBMinOrth, vBBMaxOrth, vBBMinMaxDistOrth;
	D3DXVec3Transform(&vBBMinOrth, &vBBMin, &orth);
	D3DXVec3Transform(&vBBMaxOrth, &vBBMax, &orth);

	assert(vBBMinOrth);
	assert(vBBMaxOrth);

	m_pBBMinVar->SetFloatVector(vBBMinOrth);
	m_pBBMaxVar->SetFloatVector(vBBMaxOrth);
	m_pTextureDepthVar->SetFloat(m_iTextureDepth);

	//Create render target array
	ID3D11RenderTargetView* destTex3DRTVs[2];
	destTex3DRTVs[0] = m_pDestColorTex3DRTV;
	destTex3DRTVs[1] = m_pDestDistTex3DRTV;
	m_pd3dImmediateContext->OMSetRenderTargets(2, destTex3DRTVs, NULL);

	// set viewport to the size of a single slice
//	D3D11_VIEWPORT viewport = { 0, 0, m_iTextureWidth, m_iTextureHeight, 0.0f, 1.0f };
  //  m_pd3dImmediateContext->RSSetViewports(1, &viewport);

	for(int sliceIndex = 0; sliceIndex < m_iTextureDepth; sliceIndex++)
	{
		m_pSliceIndexVar->SetInt(sliceIndex);
		m_pModelViewProjectionVar->SetMatrix(model1Orth);
		pSurface1->Render(m_pVoronoiDiagramTechnique);
		m_pModelViewProjectionVar->SetMatrix(model2Orth);
		pSurface2->Render(m_pVoronoiDiagramTechnique);
	}

	//Restore Rendertarget- and Depthstencilview
	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	m_pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, pDSV);

	return S_OK;
}

