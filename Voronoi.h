#ifndef _VORONOI_H_
#define _VORONOI_H_

#include "Surface.h"


class Voronoi
{
public:
	/*
	 *  Constructor
	 */
	Voronoi(ID3DX11Effect *pVoronoiEffect);

	/*
	 *  Destructor
	 */
	virtual ~Voronoi();

	HRESULT Initialize(const int iWidth,
					   const int iHeight, 
					   const int iDepth);

	HRESULT Update(const int iWidth,
				   const int iHeight,
				   const int iDepth);

	unsigned int GetColor3DTexture() const { return m_nColorTex3D; }
	unsigned int GetDistance3DTexture() const { return m_nDistTex3D; }
	

	/*
	 *  Renders the voronoi diagram and the distance diagram into the 3D texture
	 */
	bool RenderVoronoi(D3DXVECTOR3 vBBMin, D3DXVECTOR3 vBBMax);

	/*
	 * Returns the current voronoi rendering progress
	 */
	std::wstring GetRenderProgress();

private:
	/*
	 *  Update and initialization
	 */
	HRESULT ItlInitSliceBuffer();
	HRESULT ItlInitShaders();
	HRESULT ItlInitTextures();
	HRESULT ItlUpdateTextures();

	/*
	 * Drawing
	 */
	void ItlDrawCurrentSlice();

	//Inputlayout and slice vertex buffer
	ID3D11InputLayout			*m_pInputLayout;
	ID3D11Buffer                *m_pSlicesVB;

	int							m_iCurrentSlice;

	bool						m_bRendering;

	ID3DX11Effect				*m_pVoronoiEffect;
	ID3DX11EffectTechnique		*m_pVoronoiDiagramTechnique;
	ID3DX11EffectTechnique		*m_p2Dto3DTechnique;
	
	//Shader variables
	ID3DX11EffectMatrixVariable				*m_pModelViewProjectionVar;
	ID3DX11EffectMatrixVariable				*m_pNormalMatrixVar;

	ID3DX11EffectScalarVariable				*m_pSliceIndexVar;
	ID3DX11EffectScalarVariable				*m_pIsoSurfaceVar;

	ID3DX11EffectVectorVariable				*m_pTextureSizeVar;
	ID3DX11EffectVectorVariable				*m_pBBMinVar;
	ID3DX11EffectVectorVariable				*m_pBBMaxVar;

	ID3DX11EffectShaderResourceVariable		*m_pSurfaceTextureVar;
	ID3DX11EffectShaderResourceVariable		*m_pColorSliceTex2DVar;
	ID3DX11EffectShaderResourceVariable		*m_pDistSliceTex2DVar;

	//Textures
	unsigned int		m_nColorTex3D;
	unsigned int		m_nDistTex3D;
	unsigned int		m_nColorSliceTex2D;
	unsigned int		m_nDistSliceTex2D;
	unsigned int		m_nDepthBufferTex2D;
	
	//3d texture size
	int							m_iTextureWidth;
	int							m_iTextureHeight;
	int							m_iTextureDepth;
};

#endif