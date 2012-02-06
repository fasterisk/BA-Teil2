#include "Globals.h"
#include "Voronoi.h"

Voronoi::Voronoi(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pVoronoiEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pVoronoiEffect = pVoronoiEffect;
}

Voronoi::~Voronoi()
{
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
{}

HRESULT Voronoi::InitShaders()
{}

HRESULT Voronoi::RenderVoronoi(Surface *pSurface1, Surface *pSurface2, D3DXVECTOR3 vBBMin, D3DXVECTOR3 vBBMax)
{}

void Voronoi::Draw()
{}