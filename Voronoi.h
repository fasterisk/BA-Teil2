#ifndef _VORONOI_H_
#define _VORONOI_H_

#include "Surface.h"


class Voronoi
{
public:
	Voronoi(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pVoronoiEffect);
	virtual ~Voronoi();

	HRESULT Initialize();
	
	HRESULT SetDestination(ID3D11Texture3D *pDestColorTex3D, ID3D11Texture3D *pDestDistTex3D);
	void SetSurfaces(Surface *pSurface1, Surface *pSurface2);
	void ChangeRenderingToOneSlice(int iSliceIndex);
	void ChangeRenderingToAllSlices();

	HRESULT RenderVoronoi(D3DXVECTOR3 vBBMin, D3DXVECTOR3 vBBMax);

private:
	//Methods
	HRESULT Update();
	HRESULT InitFlatTextures();
	HRESULT InitRendertargets3D();
	HRESULT InitShaders();
	HRESULT InitSlices();
	void DrawSlices();

	HRESULT RenderToFlatTexture(D3DXMATRIX mModel1Orth, D3DXMATRIX mModel2Orth, D3DXMATRIX mNormalMatrix1, D3DXMATRIX mNormalMatrix2, int iSliceIndex);

	void Cleanup();

	//State
	Surface						*m_pSurface1;
	Surface						*m_pSurface2;

	ID3D11Device				*m_pd3dDevice;
	ID3D11DeviceContext			*m_pd3dImmediateContext;

	ID3DX11Effect				*m_pVoronoiEffect;
	ID3DX11EffectTechnique		*m_pVoronoiDiagramTechnique;
	ID3DX11EffectTechnique		*m_pFlatTo3DTexTechnique;

	ID3D11InputLayout			*m_pInputLayout;

	// Slices state
	ID3D11InputLayout			*m_pSlicesLayout;
	ID3D11Buffer                *m_pSlicesVB;

	//  for flat 3D texture
    int                         m_cols;
    int                         m_rows;

	bool						m_bDrawAllSlices;
	int							m_iCurrentSlice;

	//Shader variables
	ID3DX11EffectMatrixVariable				*m_pModelViewProjectionVar;
	ID3DX11EffectMatrixVariable				*m_pNormalMatrixVar;

	ID3DX11EffectScalarVariable				*m_pSliceIndexVar;
	ID3DX11EffectScalarVariable				*m_pTextureDepthVar;

	ID3DX11EffectVectorVariable				*m_pBBMinVar;
	ID3DX11EffectVectorVariable				*m_pBBMaxVar;

	ID3DX11EffectShaderResourceVariable		*m_pFlatColorTex2DSRVar;
	ID3DX11EffectShaderResourceVariable		*m_pFlatDistTex2DSRVar;

	//Textures, RTVs and DSV
	ID3D11Texture3D				*m_pDestColorTex3D;
	ID3D11Texture3D				*m_pDestDistTex3D;
	ID3D11Texture2D				*m_pFlatColorTex;
	ID3D11Texture2D				*m_pFlatDistTex;
	ID3D11Texture2D				*m_pDepthStencil;
	ID3D11RenderTargetView		*m_pDestColorTex3DRTV;
	ID3D11RenderTargetView		*m_pDestDistTex3DRTV;
	ID3D11RenderTargetView		*m_pFlatColorTexRTV;
	ID3D11RenderTargetView		*m_pFlatDistTexRTV;
	ID3D11ShaderResourceView	*m_pFlatColorTexSRV;
	ID3D11ShaderResourceView	*m_pFlatDistTexSRV;
	ID3D11DepthStencilView		*m_pDepthStencilView;
	
	int							m_iTextureWidth;
	int							m_iTextureHeight;
	int							m_iTextureDepth;


};

#endif