#include "Globals.h"
#include "Voronoi.h"

Voronoi::Voronoi(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pVoronoiEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pVoronoiEffect = pVoronoiEffect;

	m_pDestColorTex3D = NULL;
	m_pDestDistTex3D = NULL;
	m_pDestColorTex3DRTV = NULL;
	m_pDestDistTex3DRTV = NULL;
	m_iTextureWidth = 0;
	m_iTextureHeight = 0;
	m_iTextureDepth = 0;
	m_pSurface1 = NULL;
	m_pSurface2 = NULL;
}

Voronoi::~Voronoi()
{
}

void Voronoi::Cleanup()
{
	SAFE_RELEASE(m_pDestColorTex3DRTV);
	SAFE_RELEASE(m_pDestDistTex3DRTV);
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

	//Initialize Rendertargets for the 3D Textures
	hr = InitRendertargets();
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

HRESULT Voronoi::InitRendertargets()
{
	HRESULT hr;

	assert(m_pDestColorTex3D != NULL);
	assert(m_pDestDistTex3D != NULL);

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

	assert(m_pVoronoiDiagramTechnique);
	assert(m_pModelViewProjectionVar);
	assert(m_pSliceIndexVar);

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

	// generate orth. matrix with bounding parameters
	//for(int sliceIndex = 0; sliceIndex < m_iTextureDepth; sliceIndex++)
	//{
		//m_pSliceIndexVar->SetInt(sliceIndex);
		//m_pModelViewProjectionVar->SetMatrix(MODELMATRIXOFSURFACE1*ORTHOMATRIX);
		//pSurface1->Render(m_pVoronoiDiagramTechnique);
		//m_pModelViewProjectionVar->SetMatrix(MODELMATRIXOFSURFACE2*ORTHOMATRIX);
		//pSurface2->Render(m_pVoronoiDiagramTechnique);
	//}
	return S_OK;
}

