#include "Scene.h"
#include "Surface.h"
#include "VolumeRenderer.h"
#include "Voronoi.h"
#include "Diffusion.h"
#include "TextureManager.h"

Scene* Scene::s_pInstance = NULL;

Scene *		Scene::GetInstance()
{
	if(s_pInstance == NULL)
		s_pInstance = new Scene();
	return s_pInstance;
}

void	Scene::DeleteInstance()
{
	SAFE_DELETE(s_pInstance);
}

/****************************************************************************
 ****************************************************************************/
Scene::Scene()
{
	m_pVolumeRenderEffect = NULL;
	m_pDiffusionEffect = NULL;
	m_pVoronoiEffect = NULL;
	m_pSurfaceEffect = NULL;
	m_pBBVertices = new SURFACE_VERTEX[8];
	m_bUpdate3DTextures = false;
	m_bRender3DTexture = false;
	m_bGenerateVoronoi = false;
	m_bRenderIsoSurface = false;
	m_bGenerateDiffusion = false;
	m_bIsoValueChanged = true;
	m_bGenerateOneSliceTexture = true;

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

	m_wsRenderProgress = L"Surfaces are displayed";
}

/****************************************************************************
 ****************************************************************************/
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
}

/****************************************************************************
 ****************************************************************************/
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
	V_RETURN(ItlInitSurfaces());

	// Initialize Voronoi Diagram Renderer
	m_pVoronoi = new Voronoi(m_pVoronoiEffect);
	V_RETURN(m_pVoronoi->Initialize(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth));

	// Initialize Diffusion Renderer
	m_pDiffusion = new Diffusion(m_pDiffusionEffect);
	V_RETURN(m_pDiffusion->Initialize(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth));

	// Initialize VolumeRenderer
	m_pVolumeRenderer = new VolumeRenderer(m_pVolumeRenderEffect);
	V_RETURN(m_pVolumeRenderer->Initialize());
	
	// Update bounding box
	V_RETURN(UpdateBoundingBox());

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Scene::ItlInitSurfaces()
{
	HRESULT hr;

	// Create surface1 and its buffers
	m_pSurface1 = new Surface(m_pd3dDevice, m_pd3dImmediateContext, m_pSurfaceEffect);
	V_RETURN(m_pSurface1->Initialize("Media\\meshes\\Cube\\cube.obj", "Media\\meshes\\Textures\\red.JPG"));
	m_pSurface1->Scale(2.4f);
	//m_pSurface1->Translate(0.0f, -0.6f, -0.5f);
	m_pSurface1->SetIsoColor(0.0f);
	
	// Create surface2 and its buffers
	m_pSurface2 = new Surface(m_pd3dDevice, m_pd3dImmediateContext, m_pSurfaceEffect);
	V_RETURN(m_pSurface2->Initialize("Media\\meshes\\Cone\\cone.obj", "Media\\meshes\\Textures\\yellow.JPG"));
	m_pSurface2->SetIsoColor(1.0f);
	m_pSurface2->Scale(1.9f);
	//m_pSurface2->RotateX(3*PI/2);

	m_pControlledSurface = m_pSurface1;

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Scene::UpdateBoundingBox()
{
	HRESULT hr;

	//Get the bounding boxes of the surfaces
	BOUNDINGBOX bbSurface1 = m_pSurface1->GetBoundingBox();
	BOUNDINGBOX bbSurface2 = m_pSurface2->GetBoundingBox();


	//check which surface is the inner surface
	bool bSurface1IsInner = false;
	D3DXVECTOR3* bbVerticesSurface1 = new D3DXVECTOR3[8];
	bbVerticesSurface1[0].x = bbSurface1.vMin.x;
	bbVerticesSurface1[0].y = bbSurface1.vMin.y;
	bbVerticesSurface1[0].z = bbSurface1.vMin.z;
	bbVerticesSurface1[1].x = bbSurface1.vMax.x;
	bbVerticesSurface1[1].y = bbSurface1.vMin.y;
	bbVerticesSurface1[1].z = bbSurface1.vMin.z;
	bbVerticesSurface1[2].x = bbSurface1.vMax.x;
	bbVerticesSurface1[2].y = bbSurface1.vMax.y;
	bbVerticesSurface1[2].z = bbSurface1.vMin.z;
	bbVerticesSurface1[3].x = bbSurface1.vMin.x;
	bbVerticesSurface1[3].y = bbSurface1.vMax.y;
	bbVerticesSurface1[3].z = bbSurface1.vMin.z;
	bbVerticesSurface1[4].x = bbSurface1.vMax.x;
	bbVerticesSurface1[4].y = bbSurface1.vMin.y;
	bbVerticesSurface1[4].z = bbSurface1.vMax.z;
	bbVerticesSurface1[5].x = bbSurface1.vMin.x;
	bbVerticesSurface1[5].y = bbSurface1.vMin.y;
	bbVerticesSurface1[5].z = bbSurface1.vMax.z;
	bbVerticesSurface1[6].x = bbSurface1.vMax.x;
	bbVerticesSurface1[6].y = bbSurface1.vMax.y;
	bbVerticesSurface1[6].z = bbSurface1.vMax.z;
	bbVerticesSurface1[7].x = bbSurface1.vMin.x;
	bbVerticesSurface1[7].y = bbSurface1.vMax.y;
	bbVerticesSurface1[7].z = bbSurface1.vMax.z;

	for(int i = 0; i < 8; i++)
	{
		bSurface1IsInner = CheckIfPointIsInBoundingBox(bbSurface2, bbVerticesSurface1[i]);
		if(bSurface1IsInner == true)
			break;
	}

	SAFE_DELETE_ARRAY(bbVerticesSurface1);

	if(bSurface1IsInner)
	{
		m_pSurface1->SetIsoColor(1.0f);
		m_pSurface2->SetIsoColor(0.0f);
	}
	else
	{
		m_pSurface1->SetIsoColor(0.0f);
		m_pSurface2->SetIsoColor(1.0f);
	}

	//get overall bounding box
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

	//early break, when bounding box was not changed
	if(m_bUpdate3DTextures
		&& bbFinal.vMin.x == m_vMin.x
		&& bbFinal.vMin.y == m_vMin.y
		&& bbFinal.vMin.z == m_vMin.z
		&& bbFinal.vMax.x == m_vMax.x
		&& bbFinal.vMax.y == m_vMax.y
		&& bbFinal.vMax.z == m_vMax.z)
		return S_OK;
		
	m_vMin = D3DXVECTOR3(bbFinal.vMin.x, bbFinal.vMin.y, bbFinal.vMin.z);
	m_vMax = D3DXVECTOR3(bbFinal.vMax.x, bbFinal.vMax.y, bbFinal.vMax.z);
	
	//update bounding box vertices according to the new bounding box
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
	
	//Initialize the textures, voronoi, volumerenderer and diffusion
	V_RETURN(m_pVoronoi->Update(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth));
	V_RETURN(m_pVolumeRenderer->Update(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth));

	V_RETURN(m_pDiffusion->Update(m_iTextureWidth, 
								  m_iTextureHeight, 
								  m_iTextureDepth, 
								  m_fIsoValue));

	m_bUpdate3DTextures = true;
	
	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Scene::SetScreenSize(int iWidth, int iHeight)
{
    return m_pVolumeRenderer->SetScreenSize(iWidth, iHeight);
}

/****************************************************************************
 ****************************************************************************/
void Scene::UpdateTextureResolution(int iMaxRes)
{
	D3DXVECTOR3 vDiff = m_vMax - m_vMin;
	float fMaxDiff = max(vDiff.x, max(vDiff.y, vDiff.z));
	vDiff /= fMaxDiff;

	m_iTextureWidth = int(vDiff.x * iMaxRes + 0.5);
	m_iTextureHeight = int(vDiff.y * iMaxRes + 0.5);
	m_iTextureDepth = int(vDiff.z * iMaxRes + 0.5);

	m_bUpdate3DTextures = false;
}

/****************************************************************************
 ****************************************************************************/
void Scene::Render(D3DXMATRIX mViewProjection, bool bShowSurfaces)
{
	bool bContinue = true;

	if(m_bGenerateVoronoi) //if voronoi has to be generated
	{
		UpdateBoundingBox();
		/*
		 *	Voronoi diagram is generated in more steps. this is done because if it would be generated all at once,
		 *  the graphics driver would crash due to a timeout
		 */
		bContinue = m_pVoronoi->RenderVoronoi(m_vMin, m_vMax);
		m_wsRenderProgress = m_pVoronoi->GetRenderProgress();
		if(bContinue)
		{
			m_bGenerateVoronoi = false;
			m_bGenerateDiffusion = true;
			m_bRender3DTexture = true;
			m_wsRenderProgress = L"Generate Diffusion...";
		}
	}

	if(bContinue && m_bRender3DTexture && m_bShowVolume)//if 3d texture should be rendered by the volumerenderer
	{
		
		if(m_bGenerateDiffusion)//generate the diffusion
		{
			bContinue = m_pDiffusion->RenderDiffusion(m_pVoronoi->GetColor3DTexture(),
													  m_pVoronoi->GetDistance3DTexture(), 
													  m_iDiffusionSteps);
			m_wsRenderProgress = m_pDiffusion->GetRenderProgress();

			if(bContinue)
			{
				m_bGenerateDiffusion = false;
			}
		}

		if(bContinue && m_bRenderIsoSurface && m_bIsoValueChanged)//generate iso surface
		{
			m_nIsoSurfaceTexture = m_pDiffusion->RenderIsoSurface(m_pDiffusion->GetDiffusionTexture());
			m_bIsoValueChanged = false;
		}
		
		if(bContinue)
		{
			if(m_bDrawAllSlices == false)//draw only one slice
			{
				if(m_bGenerateOneSliceTexture)
				{			
					if(m_bRenderIsoSurface)
					{
						m_wsRenderProgress = L"Rendering one slice of the Isosurface 3D Texture";
						m_nOneSliceTexture = m_pDiffusion->RenderOneDiffusionSlice(m_iCurrentSlice, m_nIsoSurfaceTexture);
						//m_nOneSliceTexture = m_pDiffusion->RenderOneDiffusionSlice(m_iCurrentSlice, m_pVoronoi->GetColor3DTexture());
					}
					else
					{
						m_wsRenderProgress = L"Rendering one slice of the Diffusion 3D Texture";
						m_nOneSliceTexture = m_pDiffusion->RenderOneDiffusionSlice(m_iCurrentSlice, m_pDiffusion->GetDiffusionTexture());
						//m_nOneSliceTexture = m_pDiffusion->RenderOneDiffusionSlice(m_iCurrentSlice, m_pVoronoi->GetColor3DTexture());
					}
					m_bGenerateOneSliceTexture = false;
				}
				
				m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_nOneSliceTexture);
			}
			else//draw all slices
			{
				if(m_bRenderIsoSurface)
				{
					m_wsRenderProgress = L"Rendering the Isosurface 3D Texture";
					m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_nIsoSurfaceTexture);
					//m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_pVoronoi->GetColor3DTexture());
				}
				else
				{
					m_wsRenderProgress = L"Rendering the Diffusion 3D Texture";
					m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_pDiffusion->GetDiffusionTexture());
					//m_pVolumeRenderer->Render(m_pBBVertices, m_vMin, m_vMax, mViewProjection, m_pVoronoi->GetColor3DTexture());
				}
			}

		}
	}

	//show surfaces
	if(bShowSurfaces)
	{
		m_pSurface1->Render(mViewProjection);
		m_pSurface2->Render(mViewProjection);
	}
}	

/****************************************************************************
 ****************************************************************************/
void Scene::ChangeIsoValue(float fIsoValue)
{
	m_fIsoValue = fIsoValue;
	m_pDiffusion->ChangeIsoValue(fIsoValue);
	m_bIsoValueChanged = true;
	m_bGenerateOneSliceTexture = true;
}

/****************************************************************************
 ****************************************************************************/
void Scene::ShowIsoSurface(bool bShow)
{
	m_bRenderIsoSurface = bShow;
	m_pVolumeRenderer->ShowIsoSurface(bShow);
	m_bIsoValueChanged = true;
	m_bGenerateOneSliceTexture = true;
}

/****************************************************************************
 ****************************************************************************/
void Scene::ShowIsoColor(bool bShow)
{
	m_pDiffusion->ShowIsoColor(bShow);
	m_bIsoValueChanged = true;
	m_bGenerateOneSliceTexture = true;
}

/****************************************************************************
 ****************************************************************************/
void Scene::ChangeDiffusionSteps(int iDiffusionSteps)
{
	m_iDiffusionSteps = iDiffusionSteps;
	m_bGenerateDiffusion = true;
	m_bIsoValueChanged = true;
	m_bGenerateOneSliceTexture = true;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Scene::ChangeRenderingToOneSlice(int iSliceIndex)
{
	HRESULT hr;
	m_bDrawAllSlices = false;
	m_iCurrentSlice = iSliceIndex;
	m_bGenerateOneSliceTexture = true;
	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Scene::ChangeRenderingToAllSlices()
{
	HRESULT hr;
	m_bDrawAllSlices = true;
	m_bGenerateOneSliceTexture = true;
	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
void Scene::ChangeVolumeVisibility(bool bVisible)
{
	m_bShowVolume = bVisible;
}

/****************************************************************************
 ****************************************************************************/
void Scene::ChangeBoundingBoxVisibility(bool bVisible)
{
	m_pVolumeRenderer->ShowBoundingBox(bVisible);
}

/****************************************************************************
 ****************************************************************************/
void Scene::ChangeSampling()
{
	m_pVolumeRenderer->ChangeSampling();
}

/****************************************************************************
 ****************************************************************************/
void Scene::TranslateSurface1(float fX, float fY, float fZ)
{
	m_pSurface1->Translate(fX, fY, fZ);
}

void Scene::RotateSurface1(D3DXVECTOR3 axis, float fFactor)
{
	m_pSurface1->Rotate(axis, fFactor);
}

void Scene::RotateXSurface1(float fFactor)
{
	m_pSurface1->RotateX(fFactor);
}

void Scene::RotateYSurface1(float fFactor)
{
	m_pSurface1->RotateY(fFactor);
}

void Scene::ScaleSurface1(float fFactor)
{
	m_pSurface1->Scale(fFactor);
}

void Scene::TranslateSurface2(float fX, float fY, float fZ)
{
	m_pSurface2->Translate(fX, fY, fZ);
}

void Scene::RotateSurface2(D3DXVECTOR3 axis, float fFactor)
{
	m_pSurface2->Rotate(axis, fFactor);
}

void Scene::RotateXSurface2(float fFactor)
{
	m_pSurface2->RotateX(fFactor);
}

void Scene::RotateYSurface2(float fFactor)
{
	m_pSurface2->RotateY(fFactor);
}

void Scene::ScaleSurface2(float fFactor)
{
	m_pSurface2->Scale(fFactor);
}

/****************************************************************************
 ****************************************************************************/
HRESULT Scene::LoadSurface1(std::string strMeshName)
{
	HRESULT hr(S_OK);
	V_RETURN(m_pSurface1->LoadMesh(strMeshName, "notexture"));
	return hr;
}
HRESULT Scene::LoadSurface2(std::string strMeshName)
{
	HRESULT hr(S_OK);
	V_RETURN(m_pSurface2->LoadMesh(strMeshName, "notexture"));
	return hr;
}

/****************************************************************************
 ****************************************************************************/
void Scene::GenerateVoronoi()
{
	m_bGenerateVoronoi = true;
	m_bGenerateDiffusion = false;
	m_bIsoValueChanged = true;
	m_bRender3DTexture = false;
}

/****************************************************************************
 ****************************************************************************/
void Scene::Render3DTexture(bool bRender3DTexture)
{
	m_bRender3DTexture = bRender3DTexture;
}

/****************************************************************************
 ****************************************************************************/
LPCWSTR Scene::GetProgress()
{
	return m_wsRenderProgress.c_str();
}

/****************************************************************************
 ****************************************************************************/
HRESULT Scene::SaveCurrentVolume(LPCTSTR sDestination)
{
	if(m_bRenderIsoSurface)
	{
		return D3DX11SaveTextureToFile(m_pd3dImmediateContext, TextureManager::GetInstance()->GetTexture(m_pDiffusion->GetIsoSurfaceTexture()), D3DX11_IFF_DDS, sDestination);
	}
	else
	{
		return D3DX11SaveTextureToFile(m_pd3dImmediateContext, TextureManager::GetInstance()->GetTexture(m_pDiffusion->GetDiffusionTexture()), D3DX11_IFF_DDS, sDestination);
	}

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
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