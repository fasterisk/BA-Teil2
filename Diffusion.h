#ifndef _DIFFUSION_H_
#define _DIFFUSION_H_

#include "Surface.h"


class Diffusion
{
public:
	Diffusion(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pDiffusionEffect);
	virtual ~Diffusion();

	HRESULT Initialize(ID3D11Texture3D *pDestColorTex3D, ID3D11Texture3D *pDestDistTex3D);
	
	void ChangeIsoValue(float fIsoValue);

	HRESULT Update(int iTextureWidth, int iTextureHeight, int iTextureDepth, float fIsoValue);
	HRESULT RenderDiffusion(int iDiffusionSteps);

private:
	//Methods
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
	ID3D11Texture3D				*m_pColorTex3D1;
	ID3D11Texture3D				*m_pColorTex3D2;

	ID3D11RenderTargetView		*m_pColorTex3D1RTV;
	ID3D11RenderTargetView		*m_pColorTex3D2RTV;
	
	int							m_iTextureWidth;
	int							m_iTextureHeight;
	int							m_iTextureDepth;

	float						m_fIsoValue;


};

#endif