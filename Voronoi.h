#ifndef _VORONOI_H_
#define _VORONOI_H_

#include "Surface.h"


class Voronoi
{
public:
	Voronoi(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pVoronoiEffect);
	virtual ~Voronoi();
	
	HRESULT SetDestination(ID3D11Texture3D *pDestColorTex3D, ID3D11Texture3D *pDestDistTex3D);

	HRESULT RenderVoronoi(Surface *pSurface1, Surface *pSurface2, D3DXVECTOR3 vBBMin, D3DXVECTOR3 vBBMax);

private:
	//Methods
	HRESULT Initialize();
	HRESULT InitDepthStencil();
	HRESULT InitRendertargets3D();
	HRESULT InitShaders();

	void Cleanup();

	//State
	ID3D11Device				*m_pd3dDevice;
	ID3D11DeviceContext			*m_pd3dImmediateContext;

	ID3DX11Effect				*m_pVoronoiEffect;
	ID3DX11EffectTechnique		*m_pVoronoiDiagramTechnique;

	ID3D11InputLayout			*m_pInputLayout;

	ID3DX11EffectMatrixVariable	*m_pModelViewProjectionVar;

	ID3DX11EffectScalarVariable *m_pSliceIndexVar;
	ID3DX11EffectScalarVariable *m_pTextureDepthVar;

	ID3DX11EffectVectorVariable *m_pBBMinVar;
	ID3DX11EffectVectorVariable *m_pBBMaxVar;

	ID3D11Texture3D				*m_pDestColorTex3D;
	ID3D11Texture3D				*m_pDestDistTex3D;
	ID3D11Texture2D				*m_pDepthStencil;
	ID3D11RenderTargetView		*m_pDestColorTex3DRTV;
	ID3D11RenderTargetView		*m_pDestDistTex3DRTV;
	ID3D11DepthStencilView		*m_pDepthStencilView;
	
	int							m_iTextureWidth;
	int							m_iTextureHeight;
	int							m_iTextureDepth;


};

#endif