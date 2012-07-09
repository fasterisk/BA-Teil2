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
	 *	Input is the d3dDevice, d3dImmediateContext and the shader.
	 */
	Diffusion(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pDiffusionEffect);

	//Destructor
	virtual ~Diffusion();

	/*
	 *  Initialize the variables
	 *  Input:
	 *		pColorTex3D1	Destination Texture 1
	 *		pColorTex3D2	Destination Texture 2
	 *		pColor3DTex1SRV Shader Resource View for color texture 1
	 *		pColor3DTex2SRV Shader Resource View for color texture 2
	 *		iTextureWidth, iTextureHeight, iTextureDepth	Texturesize
	 *		fIsoValue		Current Isovalue for getting the Isosurface
	 *
	 *	Diffusion is rendered using ping-pong rendering - therefore we need 2 color textures
	 */
	HRESULT Initialize(ID3D11Texture3D *pColorTex3D1,
					   ID3D11Texture3D *pColorTex3D2,
					   ID3D11ShaderResourceView* pColor3DTex1SRV, 
					   ID3D11ShaderResourceView* pColor3DTex2SRV,
					   int iTextureWidth, int iTextureHeight, int iTextureDepth, float fIsoValue);
	
	/*
	 *  Changes the Isovalue 
	 */
	void ChangeIsoValue(float fIsoValue);

	/*
	 *  if true		shows the color of the Isosurface at this point of the diffusion texture
	 *  if false	shows the isosurface as white surface
	 */
	void ShowIsoColor(bool bShow);

	/*
	 *  Applies the Diffusion Algorithm using the two SRVs and the Diffusion steps
	 *	returns a SRV that can be rendered using the Volume Renderer
	 */
	ID3D11ShaderResourceView* RenderDiffusion(ID3D11ShaderResourceView* pVoronoi3DTextureSRV, 
										      ID3D11ShaderResourceView* pDist3DTextureSRV, 
											  int iDiffusionSteps);

	/*
	 *  Returns a SRV of a 3D Texture containing only one slice of the given SRV of the 3D texture 
	 */
	ID3D11ShaderResourceView* GetOneDiffusionSlice(int iSliceIndex, ID3D11ShaderResourceView* pCurrentDiffusionSRV);

	/*
	 *  Returns a SRV of a Isosurface 3D texture
	 *  Parameter has to be a Diffusion 3D texture
	 */
	ID3D11ShaderResourceView* RenderIsoSurface(ID3D11ShaderResourceView* pCurrentDiffusionSRV);

private:
	
	//Initializes the Render Target Views of the Color Textures
	HRESULT Init3DRTVs();

	//Initializes the shader variables
	HRESULT InitShaders();

	//Initializes Slice Vertices, Vertexbuffer and the inputlayout for rendering into the 3D textures
	HRESULT InitSlices();

	//Draws the slices into the current bound 3D texture
	void DrawSlices();

	void Cleanup();

	//Device and ImmediateContext
	ID3D11Device				*m_pd3dDevice;
	ID3D11DeviceContext			*m_pd3dImmediateContext;

	//Shader
	ID3DX11Effect				*m_pDiffusionEffect;
	ID3DX11EffectTechnique		*m_pDiffusionTechnique;

	//Inputlayout and slice vertex buffer
	ID3D11InputLayout			*m_pInputLayout;
	ID3D11Buffer                *m_pSlicesVB;

	//Textures, RTVs and SRV
	ID3D11Texture3D				*m_pColor3DTextures[2];
	ID3D11RenderTargetView		*m_pColor3DTexturesRTV[2];
	ID3D11ShaderResourceView	*m_pColor3DTexturesSRV[2];

	//One Slice Texture, RTV and SRV
	ID3D11Texture3D				*m_pOneSliceTexture;
	ID3D11RenderTargetView		*m_pOneSliceTextureRTV;
	ID3D11ShaderResourceView	*m_pOneSliceTextureSRV;

	//IsoSurface Texture, RTV and SRV
	ID3D11Texture3D				*m_pIsoSurfaceTexture;
	ID3D11RenderTargetView		*m_pIsoSurfaceTextureRTV;
	ID3D11ShaderResourceView	*m_pIsoSurfaceTextureSRV;

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