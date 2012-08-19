#include "ShaderManager.h"
#include "Scene.h"
#include "TextureManager.h"

ShaderManager* ShaderManager::s_pInstance = NULL;


/****************************************************************************
 ****************************************************************************/
ShaderManager* ShaderManager::GetInstance()
{
	if(s_pInstance == NULL)
		s_pInstance = new ShaderManager();
	return s_pInstance;
}

/****************************************************************************
 ****************************************************************************/
ShaderManager::ShaderManager()
{
	ItlInitialize();
}

/****************************************************************************
 ****************************************************************************/
ShaderManager::~ShaderManager()
{}

/****************************************************************************
 ****************************************************************************/
void	ShaderManager::Render3DSliceTo2DTexture(const unsigned int n3DTexID,
												const unsigned int n2DTexID,
												const int iSliceIndex)
{

}

/****************************************************************************
 ****************************************************************************/
HRESULT ShaderManager::ItlInitialize()
{
	HRESULT hr(S_OK);

	WCHAR str[MAX_PATH];
	
	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"Helper.fx"));
    V_RETURN(ItlCreateEffect(str, &m_pHelperEffect));

	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"Surface.fx"));
    V_RETURN(ItlCreateEffect(str, &m_pSurfaceEffect));

	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"VolumeRenderer.fx"));
	V_RETURN(ItlCreateEffect(str, &m_pVolumeRenderEffect));

	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"Voronoi.fx"));
	V_RETURN(ItlCreateEffect(str, &m_pVoronoiEffect));

	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"Diffusion.fx"));
	V_RETURN(ItlCreateEffect(str, &m_pDiffusionEffect));

	m_p3DSliceTo2DTextureTechnique = m_pHelperEffect->GetTechniqueByName("Render3DSliceTo2DTexture");

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT ShaderManager::ItlCreateEffect(WCHAR* name, ID3DX11Effect **ppEffect)
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
	
	V_RETURN(D3DX11CreateEffectFromMemory(effectBlob->GetBufferPointer(), effectBlob->GetBufferSize(), 0, Scene::GetInstance()->GetDevice(), ppEffect));
	return S_OK;
}
