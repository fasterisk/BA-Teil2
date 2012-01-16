#include "Globals.h"

#include "Scene.h"

#include "Surface.h"
#include "VolumeRenderer.h"
#include "Voxelizer.h"



Scene::Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;

}

Scene::~Scene()
{
	SAFE_RELEASE(m_pVolumeRenderEffect);
	SAFE_RELEASE(m_pVoxelizerEffect);
	SAFE_RELEASE(m_pSurfaceEffect);
	SAFE_DELETE(m_pVoxelizer);
	SAFE_DELETE(m_pVolumeRenderer);

	SAFE_DELETE(m_pBBVertices);
	
	SAFE_DELETE(m_pSurface1);
	SAFE_DELETE(m_pSurface2);
	
	SAFE_RELEASE(m_pSurface1Texture3D);
	SAFE_RELEASE(m_pSurface1SRV);
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

	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"Voxelizer.fx"));
	V_RETURN(CreateEffect(str, &m_pVoxelizerEffect));

	V_RETURN(InitSurfaces());
	V_RETURN(InitBoundingBox());
	V_RETURN(InitTechniques());
	V_RETURN(InitRenderTargets(iTexWidth, iTexHeight, iTexDepth));
	

	// Initialize Voxelizer
	m_pVoxelizer = new Voxelizer(m_pd3dDevice, m_pd3dImmediateContext, m_pVoxelizerEffect);
	V_RETURN(m_pVoxelizer->SetDestination(m_pSurface1Texture3D));

	// Initialize VolumeRenderer
	m_pVolumeRenderer = new VolumeRenderer(m_pd3dDevice, m_pd3dImmediateContext, m_pVolumeRenderEffect);
	V_RETURN(m_pVolumeRenderer->Initialize());
	//V_RETURN(m_pVolumeRenderer->Initialize(iTexWidth, iTexHeight, iTexDepth, m_vMin, m_vMax));


	return S_OK;
}

HRESULT Scene::InitSurfaces()
{
	HRESULT hr;

	// Create surface1 and its buffers
	m_pSurface1 = new Surface(m_pd3dDevice, m_pd3dImmediateContext, m_pSurfaceEffect);
	V_RETURN(m_pSurface1->Initialize("Media\\surface1.xml"));
    
	// Create surface2 and its buffers
	m_pSurface2 = new Surface(m_pd3dDevice, m_pd3dImmediateContext, m_pSurfaceEffect);
	m_pSurface2->SetColor(1.0, 1.0, 1.0);
	V_RETURN(m_pSurface2->Initialize("Media\\surface1.xml"));
	m_pSurface2->SetColor(1.0, 1.0, 1.0);
	m_pSurface2->Scale(0.5);

	m_pControlledSurface = m_pSurface1;

	return S_OK;
}

HRESULT Scene::InitBoundingBox()
{
	HRESULT hr;

	m_pBBVertices = new VERTEX[8];
	VERTEX min, max;
	for(int i = 0; i < m_pSurface1->m_vNum; i++)
	{
		VERTEX temp = m_pSurface1->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface1->m_mModel);
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

	/*for(int i = 0; i < m_pSurface2->m_vNum; i++)
	{
		VERTEX temp = m_pSurface2->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface2->m_mModel);
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
	}*/

	m_vMin = min;
	m_vMax = max;


	m_pBBVertices[0].x = min.x;
	m_pBBVertices[0].y = min.y;
	m_pBBVertices[0].z = min.z;
	m_pBBVertices[1].x = max.x;
	m_pBBVertices[1].y = min.y;
	m_pBBVertices[1].z = min.z;
	m_pBBVertices[2].x = max.x;
	m_pBBVertices[2].y = max.y;
	m_pBBVertices[2].z = min.z;
	m_pBBVertices[3].x = min.x;
	m_pBBVertices[3].y = max.y;
	m_pBBVertices[3].z = min.z;
	m_pBBVertices[4].x = max.x;
	m_pBBVertices[4].y = min.y;
	m_pBBVertices[4].z = max.z;
	m_pBBVertices[5].x = min.x;
	m_pBBVertices[5].y = min.y;
	m_pBBVertices[5].z = max.z;
	m_pBBVertices[6].x = max.x;
	m_pBBVertices[6].y = max.y;
	m_pBBVertices[6].z = max.z;
	m_pBBVertices[7].x = min.x;
	m_pBBVertices[7].y = max.y;
	m_pBBVertices[7].z = max.z;

	return S_OK;
}

HRESULT Scene::SetScreenSize(int iWidth, int iHeight)
{
    return m_pVolumeRenderer->SetScreenSize(iWidth, iHeight);
}

void Scene::Render(ID3D11RenderTargetView* pRTV, ID3D11RenderTargetView* pSceneDepthRTV, ID3D11DepthStencilView* pDSV, D3DXMATRIX mViewProjection)
{
	m_pVoxelizer->Voxelize(m_pSurface1, m_pSurface2, m_vMin, m_vMax);

	UpdateBoundingBox();
	
	m_pVolumeRenderer->Render(m_pBBVertices, mViewProjection, m_pSurface1Texture3D);
}	


HRESULT Scene::InitRenderTargets(int iWidth, int iHeight, int iDepth)
{
	HRESULT hr;

	D3D11_TEXTURE3D_DESC desc;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = iWidth;
	desc.Height = iHeight;
	desc.Depth = iDepth;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	V_RETURN( m_pd3dDevice->CreateTexture3D(&desc,NULL, &m_pSurface1Texture3D));

	return S_OK;
}

HRESULT Scene::InitTechniques()
{
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


HRESULT Scene::UpdateBoundingBox()
{
	HRESULT hr;

	VERTEX min, max;
	for(int i = 0; i < m_pSurface1->m_vNum; i++)
	{
		VERTEX temp = m_pSurface1->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface1->m_mModel);
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

	/*for(int i = 0; i < m_pSurface2->m_vNum; i++)
	{
		VERTEX temp = m_pSurface2->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface2->m_mModel);
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
	}*/

	m_vMin = min;
	m_vMax = max;

	m_pBBVertices[0].x = min.x;
	m_pBBVertices[0].y = min.y;
	m_pBBVertices[0].z = min.z;
	m_pBBVertices[1].x = max.x;
	m_pBBVertices[1].y = min.y;
	m_pBBVertices[1].z = min.z;
	m_pBBVertices[2].x = max.x;
	m_pBBVertices[2].y = max.y;
	m_pBBVertices[2].z = min.z;
	m_pBBVertices[3].x = min.x;
	m_pBBVertices[3].y = max.y;
	m_pBBVertices[3].z = min.z;
	m_pBBVertices[4].x = max.x;
	m_pBBVertices[4].y = min.y;
	m_pBBVertices[4].z = max.z;
	m_pBBVertices[5].x = min.x;
	m_pBBVertices[5].y = min.y;
	m_pBBVertices[5].z = max.z;
	m_pBBVertices[6].x = max.x;
	m_pBBVertices[6].y = max.y;
	m_pBBVertices[6].z = max.z;
	m_pBBVertices[7].x = min.x;
	m_pBBVertices[7].y = max.y;
	m_pBBVertices[7].z = max.z;

	return S_OK;
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