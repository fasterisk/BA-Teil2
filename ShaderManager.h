#ifndef _SHADERMANAGER_H_
#define _SHADERMANAGER_H_

#include "Globals.h"

class ShaderManager
{
public:
	static ShaderManager* GetInstance();

	void Render3DSliceTo2DTexture(const unsigned int n3DTexID, 
								  const unsigned int n2DTexID,
								  const int iSliceIndex);


protected:
	ShaderManager();
	~ShaderManager();

	HRESULT ItlInitialize();

	static ShaderManager* s_pInstance;
	
	ID3DX11Effect			*m_pHelperEffect;
	ID3DX11Effect			*m_pVolumeRenderEffect;
	ID3DX11Effect			*m_pSurfaceEffect;
	ID3DX11Effect			*m_pDiffusionEffect;
	ID3DX11Effect			*m_pVoronoiEffect;

	ID3DX11EffectTechnique	*m_p3DSliceTo2DTextureTechnique;

	

	HRESULT ItlCreateEffect(WCHAR* name, ID3DX11Effect **ppEffect);
};

#endif //_SHADERMANAGER_H_