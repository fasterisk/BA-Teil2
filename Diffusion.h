#ifndef _DIFFUSION_H_
#define _DIFFUSION_H_

#include "Surface.h"


class Diffusion
{
public:
	Diffusion(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pDiffusionEffect);
	virtual ~Diffusion();

	HRESULT Initialize(ID3D11Texture3D *pColorTex3D1,
					   ID3D11Texture3D *pColorTex3D2,
					   ID3D11ShaderResourceView* pColor3DTex1SRV, 
					   ID3D11ShaderResourceView* pColor3DTex2SRV,
					   int iTextureWidth, int iTextureHeight, int iTextureDepth, float fIsoValue);
	
	void ChangeIsoValue(float fIsoValue);

	ID3D11ShaderResourceView* RenderDiffusion(ID3D11ShaderResourceView* pVoronoi3DTextureSRV, 
										      ID3D11ShaderResourceView* pDist3DTextureSRV, 
											  int iDiffusionSteps);

private:
	//Methods
	HRESULT Init3DRTVs();
	HRESULT InitShaders();
	HRESULT InitSlices();
	void DrawSlices();

	void Cleanup();

	ID3D11Device				*m_pd3dDevice;
	ID3D11DeviceContext			*m_pd3dImmediateContext;

	ID3DX11Effect				*m_pDiffusionEffect;
	ID3DX11EffectTechnique		*m_pDiffusionTechnique;

	ID3D11InputLayout			*m_pInputLayout;
	ID3D11Buffer                *m_pSlicesVB;

	//Textures, RTVs and DSV
	ID3D11Texture3D				*m_pColor3DTextures[2];
	ID3D11RenderTargetView		*m_pColor3DTexturesRTV[2];
	ID3D11ShaderResourceView	*m_pColor3DTexturesSRV[2];

	ID3DX11EffectShaderResourceVariable		*m_pColor3DTexSRVar;
	ID3DX11EffectShaderResourceVariable		*m_pDist3DTexSRVar;

	ID3DX11EffectScalarVariable				*m_pIsoValueVar;
	ID3DX11EffectScalarVariable				*m_pPolySizeVar;

	ID3DX11EffectVectorVariable				*m_pTextureSizeVar;
	
	int							m_iTextureWidth;
	int							m_iTextureHeight;
	int							m_iTextureDepth;

	float						m_fIsoValue;
	int							m_iDiffTex;

};

#endif