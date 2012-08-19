#ifndef _DIFFUSION_H_
#define _DIFFUSION_H_

#include "Surface.h"

/*
 *	Generates a 3D Diffusion Texture by using a Voronoi and Distance Texture.
 *  
 *  Implements the Diffusion Algorithm from the first part of this BA, extending it to the 3rd dimension.
 *  Implements the Algorithm for creating an Isosurface 3D Texture from the Diffusion texture
 */
class Diffusion
{
public:
	/*
	 *  Constructor
	 */
	Diffusion(ID3DX11Effect *pDiffusionEffect);

	//Destructor
	virtual ~Diffusion();

	HRESULT Initialize(const int iTextureWidth, 
					   const int iTextureHeight, 
					   const int iTextureDepth);

	/*
	 *  Update the variables
	 *
	 *	Diffusion is rendered using ping-pong rendering - therefore we need 2 color textures
	 */
	HRESULT Update(const int iTextureWidth, 
				   const int iTextureHeight, 
				   const int iTextureDepth, 
				   const float fIsoValue);
	
	/*
	 *  Changes the Isovalue 
	 */
	void ChangeIsoValue(float fIsoValue);

	/*
	 *  if true		shows the color of the Isosurface at this point of the diffusion texture
	 *  if false	shows the isosurface as white surface
	 */
	void ShowIsoColor(bool bShow);

	unsigned int RenderDiffusion(const unsigned int nVoronoiTex3D,
								 const unsigned int nDistanceTex3D, 
								 const int iDiffusionSteps);

	
	unsigned int RenderOneDiffusionSlice(const int iSliceIndex, 
										 const unsigned int nCurrentDiffusionTexture);

	
	unsigned int RenderIsoSurface(const unsigned int nCurrentDiffusionTexture);

private:
	
	//Initializes the Render Target Views of the Color Textures
	HRESULT Init3DRTVs();

	//Initializes the shader variables
	HRESULT InitShaders();

	//Initializes Slice Vertices, Vertexbuffer and the inputlayout for rendering into the 3D textures
	HRESULT InitSlices();

	//Draws the slices into the current bound 3D texture
	void DrawSlices();

	void DrawSlice(int iSlice);

	void Cleanup();

	//Shader
	ID3DX11Effect				*m_pDiffusionEffect;
	ID3DX11EffectTechnique		*m_pDiffusionTechnique;

	//Inputlayout and slice vertex buffer
	ID3D11InputLayout			*m_pInputLayout;
	ID3D11Buffer                *m_pSlicesVB;

	//Textures, RTVs and SRV
	unsigned int				m_nDiffuseTex3D[2];

	//One Slice Texture, RTV and SRV
	unsigned int				m_nOneSliceTex3D;

	//IsoSurface Texture, RTV and SRV
	unsigned int				m_nIsoSurfaceTex3D;

	//Shader variables
	ID3DX11EffectShaderResourceVariable		*m_pColor3DTexSRVar;
	ID3DX11EffectShaderResourceVariable		*m_pDist3DTexSRVar;
	ID3DX11EffectScalarVariable				*m_pIsoValueVar;
	ID3DX11EffectScalarVariable				*m_pPolySizeVar;
	ID3DX11EffectScalarVariable				*m_pSliceIndexVar;
	ID3DX11EffectScalarVariable				*m_pShowIsoColorVar;
	ID3DX11EffectVectorVariable				*m_pTextureSizeVar;

	//3D texture size
	int							m_iTextureWidth;
	int							m_iTextureHeight;
	int							m_iTextureDepth;

	//Isovalue
	float						m_fIsoValue;

	//Index to determine which color texture has to be used for rendering
	int							m_iDiffTex;

	//Determines if isosurfaces is white or gets the color of the diffusion texture
	bool						m_bShowIsoColor;
};

#endif