#include "Globals.h"

#include "Scene.h"

#include "Surface.h"
#include "VolumeRenderer.h"
#include "Voronoi.h"


Scene::Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;

	m_pVolumeRenderEffect = NULL;
	m_pSurfaceEffect = NULL;
	m_pVoronoi3D1 = NULL;
	m_pVoronoi3D2 = NULL;
	m_pVoronoi3D1SRV = NULL;
	m_pVoronoi3D2SRV = NULL;
	m_pBBVertices = NULL;

}

Scene::~Scene()
{
	SAFE_RELEASE(m_pd3dDevice);
	SAFE_RELEASE(m_pd3dImmediateContext);

	SAFE_RELEASE(m_pVolumeRenderEffect);
	SAFE_RELEASE(m_pSurfaceEffect);
	SAFE_RELEASE(m_pVoronoiEffect);
	SAFE_DELETE(m_pVoronoi);
	SAFE_DELETE(m_pVolumeRenderer);

	SAFE_DELETE(m_pBBVertices);
	
	SAFE_DELETE(m_pSurface1);
	SAFE_DELETE(m_pSurface2);
	
	SAFE_RELEASE(m_pVoronoi3D1);
	SAFE_RELEASE(m_pVoronoi3D2);
	SAFE_RELEASE(m_pVoronoi3D1SRV);
	SAFE_RELEASE(m_pVoronoi3D2SRV);
}


HRESULT Scene::Initialize(int iTexWidth, int iTexHeight, int iTexDepth)
{
	HRESULT hr;

	// Initialize Shaders
    WCHAR str[MAX_PATH];

	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"Surface.fx"));
    V_RETURN(CreateEffect(str, &m_pSurfaceEffect));

	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"VolumeRenderer.fx"));
	V_RETURN(CreateEffect(str, &m_pVolumeRenderEffect));

	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"Voronoi.fx"));
	V_RETURN(CreateEffect(str, &m_pVoronoiEffect));

	iTextureWidth = iTexWidth;
	iTextureHeight = iTexHeight;
	iTextureDepth = iTexDepth;


	// Initialize Voronoi Diagram Renderer
	m_pVoronoi = new Voronoi(m_pd3dDevice, m_pd3dImmediateContext, m_pVoronoiEffect);

	// Initialize VolumeRenderer
	m_pVolumeRenderer = new VolumeRenderer(m_pd3dDevice, m_pd3dImmediateContext, m_pVolumeRenderEffect);

	V_RETURN(InitSurfaces());
	V_RETURN(UpdateBoundingBox());


	return S_OK;
}

HRESULT Scene::InitSurfaces()
{
	HRESULT hr;

	// Create surface1 and its buffers
	m_pSurface1 = new Surface(m_pd3dDevice, m_pd3dImmediateContext, m_pSurfaceEffect);
	V_RETURN(m_pSurface1->Initialize("Media\\surface1.xml"));
    m_pSurface1->SetColor(0.0, 0.0, 1.0);

	// Create surface2 and its buffers
	m_pSurface2 = new Surface(m_pd3dDevice, m_pd3dImmediateContext, m_pSurfaceEffect);
	V_RETURN(m_pSurface2->Initialize("Media\\surface1.xml"));
	m_pSurface2->SetColor(0.0, 1.0, 0.0);
	m_pSurface2->Scale(0.5);

	m_pControlledSurface = m_pSurface1;

	return S_OK;
}

HRESULT Scene::UpdateBoundingBox()
{
	HRESULT hr;

	SAFE_DELETE(m_pBBVertices);

	m_pBBVertices = new VERTEX[8];
	D3DXVECTOR4 min, max;
	for(int i = 0; i < m_pSurface1->m_iNumVertices; i++)
	{
		D3DXVECTOR4 temp = D3DXVECTOR4(m_pSurface1->m_pVertices[i].pos.x, 
									   m_pSurface1->m_pVertices[i].pos.y, 
									   m_pSurface1->m_pVertices[i].pos.z, 
									   1.0);
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &temp, &m_pSurface1->m_mModel);
		temp.x = mul.x;
		temp.y = mul.y;
		temp.z = mul.z;
		if(i == 0)
		{
			min = temp;
			max = temp;
		}

		if(temp.x < min.x)
			min.x = temp.x;
		if(temp.y < min.y)
			min.y = temp.y;
		if(temp.z < min.z)
			min.z = temp.z;
		if(temp.x > max.x)
			max.x = temp.x;
		if(temp.y > max.y)
			max.y = temp.y;
		if(temp.z > max.z)
			max.z = temp.z;
	}

	for(int i = 0; i < m_pSurface2->m_iNumVertices; i++)
	{
		D3DXVECTOR4 temp = D3DXVECTOR4(m_pSurface2->m_pVertices[i].pos.x,
									   m_pSurface2->m_pVertices[i].pos.y, 
									   m_pSurface2->m_pVertices[i].pos.z,
									   1.0f);
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &temp, &m_pSurface2->m_mModel);
		temp.x = mul.x;
		temp.y = mul.y;
		temp.z = mul.z;

		if(temp.x < min.x)
			min.x = temp.x;
		if(temp.y < min.y)
			min.y = temp.y;
		if(temp.z < min.z)
			min.z = temp.z;
		if(temp.x > max.x)
			max.x = temp.x;
		if(temp.y > max.y)
			max.y = temp.y;
		if(temp.z > max.z)
			max.z = temp.z;
	}

	m_vMin = D3DXVECTOR3(min.x, min.y, min.z);
	m_vMax = D3DXVECTOR3(max.x, max.y, max.z);
	
	m_pBBVertices[0].pos.x = m_vMin.x;
	m_pBBVertices[0].pos.y = m_vMin.y;
	m_pBBVertices[0].pos.z = m_vMin.z;
	m_pBBVertices[1].pos.x = m_vMax.x;
	m_pBBVertices[1].pos.y = m_vMin.y;
	m_pBBVertices[1].pos.z = m_vMin.z;
	m_pBBVertices[2].pos.x = m_vMax.x;
	m_pBBVertices[2].pos.y = m_vMax.y;
	m_pBBVertices[2].pos.z = m_vMin.z;
	m_pBBVertices[3].pos.x = m_vMin.x;
	m_pBBVertices[3].pos.y = m_vMax.y;
	m_pBBVertices[3].pos.z = m_vMin.z;
	m_pBBVertices[4].pos.x = m_vMax.x;
	m_pBBVertices[4].pos.y = m_vMin.y;
	m_pBBVertices[4].pos.z = m_vMax.z;
	m_pBBVertices[5].pos.x = m_vMin.x;
	m_pBBVertices[5].pos.y = m_vMin.y;
	m_pBBVertices[5].pos.z = m_vMax.z;
	m_pBBVertices[6].pos.x = m_vMax.x;
	m_pBBVertices[6].pos.y = m_vMax.y;
	m_pBBVertices[6].pos.z = m_vMax.z;
	m_pBBVertices[7].pos.x = m_vMin.x;
	m_pBBVertices[7].pos.y = m_vMax.y;
	m_pBBVertices[7].pos.z = m_vMax.z;


	// Change texture size corresponding to the ratio between x y and z of BB
	D3DXVECTOR3 vDiff = m_vMax - m_vMin;
	float fMaxDiff = max(vDiff.x, max(vDiff.y, vDiff.z));
	vDiff /= fMaxDiff;

	int previousMax = max(iTextureWidth, max(iTextureHeight, iTextureDepth));
	iTextureWidth = int(vDiff.x * previousMax + 0.5);
	iTextureHeight = int(vDiff.y * previousMax + 0.5);
	iTextureDepth = int(vDiff.z * previousMax + 0.5);
	
	V_RETURN(Init3DTextures());
	V_RETURN(m_pVoronoi->SetDestination(m_pVoronoi3D1, m_pVoronoi3D2));
	V_RETURN(m_pVolumeRenderer->Initialize(iTextureWidth, iTextureHeight, iTextureDepth));
	return S_OK;
}

HRESULT Scene::SetScreenSize(int iWidth, int iHeight)
{
    return m_pVolumeRenderer->SetScreenSize(iWidth, iHeight);
}

void Scene::UpdateTextureResolution(int iMaxRes)
{
	D3DXVECTOR3 vDiff = m_vMax - m_vMin;
	float fMaxDiff = max(vDiff.x, max(vDiff.y, vDiff.z));
	vDiff /= fMaxDiff;

	iTextureWidth = int(vDiff.x * iMaxRes + 0.5);
	iTextureHeight = int(vDiff.y * iMaxRes + 0.5);
	iTextureDepth = int(vDiff.z * iMaxRes + 0.5);
}

void Scene::Render(ID3D11RenderTargetView* pRTV, ID3D11RenderTargetView* pSceneDepthRTV, ID3D11DepthStencilView* pDSV, D3DXMATRIX mViewProjection)
{
	UpdateBoundingBox();

	m_pVoronoi->RenderVoronoi(m_pSurface1, m_pSurface2, m_vMin, m_vMax);

	m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_pVoronoi3D1SRV);

	//m_pSurface1->Render(mViewProjection);
	//m_pSurface2->Render(mViewProjection);
}	


HRESULT Scene::Init3DTextures()
{
	HRESULT hr;

	SAFE_RELEASE(m_pVoronoi3D1);
	SAFE_RELEASE(m_pVoronoi3D2);
	SAFE_RELEASE(m_pVoronoi3D1SRV);
	SAFE_RELEASE(m_pVoronoi3D2SRV);

	D3D11_TEXTURE3D_DESC desc;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = iTextureWidth;
	desc.Height = iTextureHeight;
	desc.Depth = iTextureDepth;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	V_RETURN(m_pd3dDevice->CreateTexture3D(&desc, NULL, &m_pVoronoi3D1));
	V_RETURN(m_pd3dDevice->CreateTexture3D(&desc, NULL, &m_pVoronoi3D2));

	//create the shader resource views
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	descSRV.Texture3D.MostDetailedMip = 0;
	descSRV.Texture3D.MipLevels = 1;
	descSRV.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pVoronoi3D1, &descSRV, &m_pVoronoi3D1SRV));
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pVoronoi3D2, &descSRV, &m_pVoronoi3D2SRV));

	return S_OK;
}

void Scene::ChangeControlledSurface()
{
	if(m_bSurface1IsControlled)
		m_pControlledSurface = m_pSurface2;
	else
		m_pControlledSurface = m_pSurface1;

	m_bSurface1IsControlled = !m_bSurface1IsControlled;
}

void Scene::Translate(float fX, float fY, float fZ)
{
	m_pControlledSurface->Translate(fX, fY, fZ);
}

void Scene::RotateX(float fFactor)
{
	m_pControlledSurface->RotateX(fFactor);
}

void Scene::RotateY(float fFactor)
{
	m_pControlledSurface->RotateY(fFactor);
}

void Scene::Scale(float fFactor)
{
	m_pControlledSurface->Scale(fFactor);
}


HRESULT Scene::CreateEffect(WCHAR* name, ID3DX11Effect **ppEffect)
{
	HRESULT hr;
	ID3D10Blob *effectBlob = 0, *errorsBlob = 0;
	hr = D3DX11CompileFromFile( name, NULL, NULL, NULL, "fx_5_0", NULL, NULL, NULL, &effectBlob, &errorsBlob, NULL );
	if(FAILED ( hr ))
	{
		std::string errStr((LPCSTR)errorsBlob->GetBufferPointer(), errorsBlob->GetBufferSize());
		WCHAR err[256];
		MultiByteToWideChar(CP_ACP, 0, errStr.c_str(), (int)errStr.size(), err, errStr.size());
		MessageBox( NULL, (LPCWSTR)err, L"Error", MB_OK );
		return hr;
	}
	
	V_RETURN(D3DX11CreateEffectFromMemory(effectBlob->GetBufferPointer(), effectBlob->GetBufferSize(), 0, m_pd3dDevice, ppEffect));
	return S_OK;
}

//--------------------------------------------------------------------------------------
// Find and compile the specified shader
//--------------------------------------------------------------------------------------
HRESULT Scene::CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( str, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return S_OK;
}