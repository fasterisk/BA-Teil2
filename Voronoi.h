#ifndef _VORONOI_H_
#define _VORONOI_H_

#include "Surface.h"


class Voronoi
{
public:
	/*
	 *  Constructor
	 */
	Voronoi(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pVoronoiEffect);

	/*
	 *  Destructor
	 */
	virtual ~Voronoi();

	/*
	 *  Initialize
	 */
	HRESULT Initialize();
	
	/*
	 *  Set the destination 3D textures of the voronoi diagram
	 */
	HRESULT SetDestination(ID3D11Texture3D *pDestColorTex3D, ID3D11Texture3D *pDestDistTex3D);

	/*
	 *  Set the 2 surfaces of which the voronoi diagram is generated
	 */
	void SetSurfaces(Surface *pSurface1, Surface *pSurface2);

	/*
	 *  Renders the voronoi diagram and the distance diagram into the 3D texture
	 */
	bool RenderVoronoi(D3DXVECTOR3 vBBMin, D3DXVECTOR3 vBBMax, bool bRenderIsoSurface);

	/*
	 * Returns the current voronoi rendering progress
	 */
	std::wstring GetRenderProgress();

private:
	/*
	 *  Update Rendertargets, flat textures and slices
	 */
	HRESULT Update();
	HRESULT InitFlatTextures();
	HRESULT InitRendertargets3D();
	HRESULT InitShaders();
	HRESULT InitSlices();

	/*
	 *  Draw the slices of the flat textures to the 3D texture
	 */
	void DrawSlices();

	/*
	 * Draw the current slice to the 3D texture
	 */
	void DrawCurrentSlice();

	/*
	 *  Render the Voronoi Diagram to a flat texture
	 */
	HRESULT RenderToFlatTexture(D3DXMATRIX mModel1Orth, D3DXMATRIX mModel2Orth, D3DXMATRIX mNormalMatrix1, D3DXMATRIX mNormalMatrix2, int iSliceIndex);

	/*
	 *  Cleanup of the resources
	 */
	void Cleanup();

	//State
	Surface						*m_pSurface1;
	Surface						*m_pSurface2;

	bool						m_bRenderIsoSurface;
	int							m_iCurrentSlice;

	bool						m_bRenderToFlatTexture;
	bool						m_bRenderFlatTo3DTexture;

	ID3D11Device				*m_pd3dDevice;
	ID3D11DeviceContext			*m_pd3dImmediateContext;

	ID3DX11Effect				*m_pVoronoiEffect;
	ID3DX11EffectTechnique		*m_pVoronoiDiagramTechnique;
	ID3DX11EffectTechnique		*m_pFlatTo3DTexTechnique;
	
	// Slices state
	ID3D11InputLayout			*m_pSlicesLayout;
	ID3D11Buffer                *m_pSlicesVB;

	//  for flat 3D texture
    int                         m_cols;
    int                         m_rows;

	//Shader variables
	ID3DX11EffectMatrixVariable				*m_pModelViewProjectionVar;
	ID3DX11EffectMatrixVariable				*m_pNormalMatrixVar;

	ID3DX11EffectScalarVariable				*m_pSliceIndexVar;
	ID3DX11EffectScalarVariable				*m_pIsoSurfaceVar;

	ID3DX11EffectVectorVariable				*m_pTextureSizeVar;
	ID3DX11EffectVectorVariable				*m_pBBMinVar;
	ID3DX11EffectVectorVariable				*m_pBBMaxVar;

	ID3DX11EffectShaderResourceVariable		*m_pFlatColorTex2DSRVar;
	ID3DX11EffectShaderResourceVariable		*m_pFlatDistTex2DSRVar;
	ID3DX11EffectShaderResourceVariable		*m_pSurfaceTextureVar;

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
	
	//3d texture size
	int							m_iTextureWidth;
	int							m_iTextureHeight;
	int							m_iTextureDepth;
};

#endif