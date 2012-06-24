#include "Globals.h"

#include "Scene.h"

#include "Surface.h"
#include "VolumeRenderer.h"
#include "Voronoi.h"
#include "Diffusion.h"


Scene::Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;

	m_pVolumeRenderEffect = NULL;
	m_pDiffusionEffect = NULL;
	m_pVoronoiEffect = NULL;
	m_pSurfaceEffect = NULL;
	m_pVoronoi3DTex = NULL;
	m_pColor3DTex1 = NULL;
	m_pColor3DTex2 = NULL;
	m_pDist3DTex = NULL;
	m_pVoronoi3DTexSRV = NULL;
	m_pColor3DTex1SRV = NULL;
	m_pColor3DTex2SRV = NULL;
	m_pDist3DTexSRV = NULL;
	m_pBBVertices = new VERTEX[8];
	initialized = false;
	m_bRender3DTexture = false;
	m_bGenerateVoronoi = false;
	m_bRenderIsoSurface = false;
	m_bDiffusionStepsChanged = true;

	m_iTextureWidth = 128;
	m_iTextureHeight = 128;
	m_iTextureDepth = 128;

	m_pVoronoi = NULL;
	m_pDiffusion = NULL;
	m_pVolumeRenderer = NULL;
	
	m_bDrawAllSlices = true;
	m_iCurrentSlice = 64;
	m_iDiffusionSteps = 8;
	m_fIsoValue = 0.5f;
}

Scene::~Scene()
{
	SAFE_RELEASE(m_pd3dDevice);
	SAFE_RELEASE(m_pd3dImmediateContext);

	SAFE_RELEASE(m_pVolumeRenderEffect);
	SAFE_RELEASE(m_pSurfaceEffect);
	SAFE_RELEASE(m_pVoronoiEffect);
	SAFE_RELEASE(m_pDiffusionEffect);
	SAFE_DELETE(m_pVoronoi);
	SAFE_DELETE(m_pDiffusion);
	SAFE_DELETE(m_pVolumeRenderer);

	SAFE_DELETE(m_pBBVertices);
	
	SAFE_DELETE(m_pSurface1);
	SAFE_DELETE(m_pSurface2);
	
	SAFE_RELEASE(m_pVoronoi3DTex);
	SAFE_RELEASE(m_pColor3DTex1);
	SAFE_RELEASE(m_pColor3DTex2);
	SAFE_RELEASE(m_pDist3DTex);
	SAFE_RELEASE(m_pVoronoi3DTexSRV);
	SAFE_RELEASE(m_pColor3DTex1SRV);
	SAFE_RELEASE(m_pColor3DTex2SRV);
	SAFE_RELEASE(m_pDist3DTexSRV);
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

	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"Diffusion.fx"));
	V_RETURN(CreateEffect(str, &m_pDiffusionEffect));

	m_iTextureWidth = iTexWidth;
	m_iTextureHeight = iTexHeight;
	m_iTextureDepth = iTexDepth;

	// Initialize Surfaces
	V_RETURN(InitSurfaces());

	// Initialize Voronoi Diagram Renderer
	m_pVoronoi = new Voronoi(m_pd3dDevice, m_pd3dImmediateContext, m_pVoronoiEffect);
	V_RETURN(m_pVoronoi->Initialize());
	m_pVoronoi->SetSurfaces(m_pSurface1, m_pSurface2);

	// Initialize Diffusion Renderer
	m_pDiffusion = new Diffusion(m_pd3dDevice, m_pd3dImmediateContext, m_pDiffusionEffect);

	// Initialize VolumeRenderer
	m_pVolumeRenderer = new VolumeRenderer(m_pd3dDevice, m_pd3dImmediateContext, m_pVolumeRenderEffect);
	V_RETURN(m_pVolumeRenderer->Initialize());

	
	V_RETURN(UpdateBoundingBox());

	return S_OK;
}

HRESULT Scene::InitSurfaces()
{
	HRESULT hr;

	// Create surface1 and its buffers
	m_pSurface1 = new Surface(m_pd3dDevice, m_pd3dImmediateContext, m_pSurfaceEffect);
	V_RETURN(m_pSurface1->Initialize("Media\\meshes\\Cube\\cube.obj", "Media\\meshes\\Textures\\blue.JPG"));
	m_pSurface1->Translate(0.0f, -0.25f, 0.0f);
	m_pSurface1->Scale(3.0f);
	
	// Create surface2 and its buffers
	m_pSurface2 = new Surface(m_pd3dDevice, m_pd3dImmediateContext, m_pSurfaceEffect);
	V_RETURN(m_pSurface2->Initialize("Media\\meshes\\Cube\\cube.obj", "Media\\meshes\\Textures\\red.JPG"));
	m_pSurface2->Translate(0.0f, -0.25f, 0.0f);

	m_pControlledSurface = m_pSurface1;

	return S_OK;
}

HRESULT Scene::UpdateBoundingBox()
{
	HRESULT hr;

	BOUNDINGBOX bbSurface1 = m_pSurface1->GetBoundingBox();
	BOUNDINGBOX bbSurface2 = m_pSurface2->GetBoundingBox();

	BOUNDINGBOX bbFinal = bbSurface1;

	if(bbSurface2.vMin.x < bbFinal.vMin.x)
		bbFinal.vMin.x = bbSurface2.vMin.x;
	if(bbSurface2.vMin.y < bbFinal.vMin.y)
		bbFinal.vMin.y = bbSurface2.vMin.y;
	if(bbSurface2.vMin.z < bbFinal.vMin.z)
		bbFinal.vMin.z = bbSurface2.vMin.z;
	if(bbSurface2.vMax.x > bbFinal.vMax.x)
		bbFinal.vMax.x = bbSurface2.vMax.x;
	if(bbSurface2.vMax.y > bbFinal.vMax.y)
		bbFinal.vMax.y = bbSurface2.vMax.y;
	if(bbSurface2.vMax.z > bbFinal.vMax.z)
		bbFinal.vMax.z = bbSurface2.vMax.z;

	if(initialized
		&& bbFinal.vMin.x == m_vMin.x
		&& bbFinal.vMin.y == m_vMin.y
		&& bbFinal.vMin.z == m_vMin.z
		&& bbFinal.vMax.x == m_vMax.x
		&& bbFinal.vMax.y == m_vMax.y
		&& bbFinal.vMax.z == m_vMax.z)
		return S_OK;
		
	m_vMin = D3DXVECTOR3(bbFinal.vMin.x, bbFinal.vMin.y, bbFinal.vMin.z);
	m_vMax = D3DXVECTOR3(bbFinal.vMax.x, bbFinal.vMax.y, bbFinal.vMax.z);
	

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

	int previousMax = max(m_iTextureWidth, max(m_iTextureHeight, m_iTextureDepth));
	m_iTextureWidth = int(vDiff.x * previousMax + 0.5);
	m_iTextureHeight = int(vDiff.y * previousMax + 0.5);
	m_iTextureDepth = int(vDiff.z * previousMax + 0.5);
	
	V_RETURN(Init3DTextures());
	V_RETURN(m_pVoronoi->SetDestination(m_pVoronoi3DTex, m_pDist3DTex));
	V_RETURN(m_pVolumeRenderer->Update(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth));

	V_RETURN(m_pDiffusion->Initialize(m_pColor3DTex1, 
									  m_pColor3DTex2, 
									  m_pColor3DTex1SRV, 
									  m_pColor3DTex2SRV, 
									  m_iTextureWidth, 
									  m_iTextureHeight, 
									  m_iTextureDepth, 
									  m_fIsoValue));

	initialized = true;
	
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

	m_iTextureWidth = int(vDiff.x * iMaxRes + 0.5);
	m_iTextureHeight = int(vDiff.y * iMaxRes + 0.5);
	m_iTextureDepth = int(vDiff.z * iMaxRes + 0.5);

	initialized = false;
}

void Scene::Render(D3DXMATRIX mViewProjection, bool bShowSurfaces)
{
	UpdateBoundingBox();

	if(m_bGenerateVoronoi)
	{
		bool b = m_pVoronoi->RenderVoronoi(m_vMin, m_vMax, m_bRenderIsoSurface);
		if(b)
		{
			m_bGenerateVoronoi = false;
			m_bRender3DTexture = true;
			m_bDiffusionStepsChanged = true;
		}
		else
		{
			m_bRender3DTexture = false;
		}
	}

	if(m_bRender3DTexture)
	{
		if(m_bDiffusionStepsChanged)
		{
			m_pCurrentDiffusionSRV = m_pDiffusion->RenderDiffusion(m_pVoronoi3DTexSRV, m_pDist3DTexSRV, m_iDiffusionSteps);
			m_bDiffusionStepsChanged = false;
		}

		if(m_bRenderIsoSurface)
		{
			m_pIsoSurfaceSRV = m_pDiffusion->RenderIsoSurface(m_pCurrentDiffusionSRV);
		}
		
		if(m_bDrawAllSlices == false)
		{
			
			if(m_bRenderIsoSurface)
			{
				m_pOneSliceDiffusionSRV = m_pDiffusion->GetOneDiffusionSlice(m_iCurrentSlice, m_pIsoSurfaceSRV);
				//m_pOneSliceDiffusionSRV = m_pDiffusion->GetOneDiffusionSlice(m_iCurrentSlice, m_pVoronoi3DTexSRV);//m_pIsoSurfaceSRV);
			}
			else
			{
				m_pOneSliceDiffusionSRV = m_pDiffusion->GetOneDiffusionSlice(m_iCurrentSlice, m_pCurrentDiffusionSRV);
				//m_pOneSliceDiffusionSRV = m_pDiffusion->GetOneDiffusionSlice(m_iCurrentSlice, m_pVoronoi3DTexSRV);//m_pCurrentDiffusionSRV);
			}	
			
			m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_pOneSliceDiffusionSRV);
		}
		else
		{
			if(m_bRenderIsoSurface)
			{
				m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_pIsoSurfaceSRV);
				//m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_pVoronoi3DTexSRV);//m_pIsoSurfaceSRV);
			}
			else
			{
				m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_pCurrentDiffusionSRV);
				//m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_pVoronoi3DTexSRV);//m_pCurrentDiffusionSRV);
			}

		}
	}
	
	
	if(bShowSurfaces)
	{
		m_pSurface1->Render(mViewProjection);
		m_pSurface2->Render(mViewProjection);
	}
	
}	


HRESULT Scene::Init3DTextures()
{
	HRESULT hr;

	SAFE_RELEASE(m_pVoronoi3DTex);
	SAFE_RELEASE(m_pColor3DTex1);
	SAFE_RELEASE(m_pColor3DTex2);
	SAFE_RELEASE(m_pDist3DTex);
	SAFE_RELEASE(m_pVoronoi3DTexSRV);
	SAFE_RELEASE(m_pColor3DTex1SRV);
	SAFE_RELEASE(m_pColor3DTex2SRV);
	SAFE_RELEASE(m_pDist3DTexSRV);

	D3D11_TEXTURE3D_DESC desc;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = m_iTextureWidth;
	desc.Height = m_iTextureHeight;
	desc.Depth = m_iTextureDepth;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	V_RETURN(m_pd3dDevice->CreateTexture3D(&desc, NULL, &m_pVoronoi3DTex));
	V_RETURN(m_pd3dDevice->CreateTexture3D(&desc, NULL, &m_pColor3DTex1));
	V_RETURN(m_pd3dDevice->CreateTexture3D(&desc, NULL, &m_pColor3DTex2));
	V_RETURN(m_pd3dDevice->CreateTexture3D(&desc, NULL, &m_pDist3DTex));

	DXUT_SetDebugName( m_pVoronoi3DTex, "Voronoi Texture" );
	DXUT_SetDebugName( m_pColor3DTex1, "Diffusion Texture 1" );
	DXUT_SetDebugName( m_pColor3DTex2, "Diffusion Texture 2" );
	DXUT_SetDebugName( m_pDist3DTex, "Dist Texture" );

	//create the shader resource views
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	descSRV.Texture3D.MostDetailedMip = 0;
	descSRV.Texture3D.MipLevels = 1;
	descSRV.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pVoronoi3DTex, &descSRV, &m_pVoronoi3DTexSRV));
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pColor3DTex1, &descSRV, &m_pColor3DTex1SRV));
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pColor3DTex2, &descSRV, &m_pColor3DTex2SRV));
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pDist3DTex, &descSRV, &m_pDist3DTexSRV));

	return S_OK;
}

void Scene::ChangeIsoValue(float fIsoValue)
{
	m_fIsoValue = fIsoValue;
	m_pDiffusion->ChangeIsoValue(fIsoValue);
}

void Scene::ShowIsoSurface(bool bShow)
{
	m_bRenderIsoSurface = bShow;
}

void Scene::ShowIsoColor(bool bShow)
{
	m_pDiffusion->ShowIsoColor(bShow);
}

void Scene::ChangeDiffusionSteps(int iDiffusionSteps)
{
	m_iDiffusionSteps = iDiffusionSteps;
	m_bDiffusionStepsChanged = true;
}

void Scene::ChangeControlledSurface()
{
	if(m_bSurface1IsControlled)
		m_pControlledSurface = m_pSurface2;
	else
		m_pControlledSurface = m_pSurface1;

	m_bSurface1IsControlled = !m_bSurface1IsControlled;
}

HRESULT Scene::ChangeRenderingToOneSlice(int iSliceIndex)
{
	HRESULT hr;
	m_bDrawAllSlices = false;
	m_iCurrentSlice = iSliceIndex;
	return S_OK;
}

HRESULT Scene::ChangeRenderingToAllSlices()
{
	HRESULT hr;
	m_bDrawAllSlices = true;
	return S_OK;
}

void Scene::ChangeSampling()
{
	m_pVolumeRenderer->ChangeSampling();
}

void Scene::TranslateCurrentSurface(float fX, float fY, float fZ)
{
	m_pControlledSurface->Translate(fX, fY, fZ);
}

void Scene::RotateCurrentSurface(D3DXVECTOR3 axis, float fFactor)
{
	m_pControlledSurface->Rotate(axis, fFactor);
}

void Scene::RotateXCurrentSurface(float fFactor)
{
	m_pControlledSurface->RotateX(fFactor);
}

void Scene::RotateYCurrentSurface(float fFactor)
{
	m_pControlledSurface->RotateY(fFactor);
}

void Scene::ScaleCurrentSurface(float fFactor)
{
	m_pControlledSurface->Scale(fFactor);
}

HRESULT Scene::ChangeCurrentSurfaceMesh(std::string strMeshName)
{
	HRESULT hr(S_OK);
	V_RETURN(m_pControlledSurface->LoadMesh(strMeshName));
	return hr;
}

void Scene::GenerateVoronoi()
{
	m_bGenerateVoronoi = true;
}

void Scene::Render3DTexture(bool bRenderVoronoi)
{
	m_bRender3DTexture = bRenderVoronoi;
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