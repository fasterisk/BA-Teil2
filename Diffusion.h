#ifndef _DIFFUSION_H_
#define _DIFFUSION_H_

#include "Surface.h"


class Diffusion
{
public:
	Diffusion(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pDiffusionEffect);
	virtual ~Diffusion();

	HRESULT Initialize();
	
	HRESULT SetDestinationTextures(ID3D11Texture3D *pDestColorTex3D1, ID3D11Texture3D *pDestColorTex3D2);
	void ChangeIsoValue(float fIsoValue);

	HRESULT RenderDiffusion(int iDiffusionSteps);

private:
	//Methods
	HRESULT Update();
	HRESULT InitRendertargets3D();
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
	
	int							m_iTextureWidth;
	int							m_iTextureHeight;
	int							m_iTextureDepth;

	float						m_fIsoValue;


};

#endif