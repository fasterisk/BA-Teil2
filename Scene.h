#ifndef _SCENE_H_
#define _SCENE_H_

class Surface;
class Voxelizer;
class VolumeRenderer;
class TextureGrid;
class Voronoi;
class Diffusion;

#include "Globals.h"

class Scene
{
public:
	static Scene* GetInstance();
	static void DeleteInstance();

	void SetDevice(ID3D11Device* pd3dDevice) 
		{ m_pd3dDevice = pd3dDevice; }

	void SetContext(ID3D11DeviceContext* pd3dImmediateContext) 
		{ m_pd3dImmediateContext = pd3dImmediateContext; }
	
	ID3D11Device* GetDevice() const 
		{ return m_pd3dDevice; }

	ID3D11DeviceContext * GetContext() const 
		{ return m_pd3dImmediateContext; }

	Surface* GetSurface1() const { return m_pSurface1; }
	Surface* GetSurface2() const { return m_pSurface2; }


	/*
	 *	Initialize the scene:
	 *		- Load all shaders
	 *		- Initialize the surfaces
	 *		- Initialize Voronoi, Diffusion and Volumerenderer
	 *		- Update Bounding Box
	 */
	HRESULT Initialize(int iTexWidth, int iTexHeight, int iTexDepth);

	/*
	 *	This function is called when the screen size changes
	 */
	HRESULT SetScreenSize(int iWidth, int iHeight);

	/*
	 *	Updates the texture resolution
	 *		Input value is the slider value of the UI, updates the three sizes according to this value
	 */
	void UpdateTextureResolution(int iMaxRes);

	/*
	 *	Render the scene, gets called in a loop
	 */
	void Render(D3DXMATRIX mViewProjection, bool bShowSurfaces);

	/*
	 *	Changes the isovalue of the isosurface
	 */
	void ChangeIsoValue(float fIsoValue);

	/*
	 *	Changes the diffusion steps of the diffusion algorithm
	 */
	void ChangeDiffusionSteps(int iDiffusionSteps);

	/*
	 *  Changes the sampling type of the volumerenderer (Nearest neighbor or linear sampling)
	 */
	void ChangeSampling();

	/*
	 *	Input true, if isosurface should be shown, else false
	 */
	void ShowIsoSurface(bool bShow);

	/*
	 *	Input true, if isosurface should show the color of the diffusion, else false (isosurface is white)
	 */
	void ShowIsoColor(bool bShow);
	
	/*
	 *	Change the scene so only one slice of the 3D texture is shown
	 */
	HRESULT ChangeRenderingToOneSlice(int iSliceIndex);

	/*
	 *	Change the scene so that all slices of the textures are shown
	 */
	HRESULT ChangeRenderingToAllSlices();

	/*
	 *	Change the visibility of the volume - if it is shown or not
	 */
	void ChangeVolumeVisibility(bool bVisible);
	
	/*
	 *	Change the visibility of the bounding box - if it is shown or not
	 */
	void ChangeBoundingBoxVisibility(bool bVisible);

	/*
	 *	Translation, Rotation and Scale functions of surface 1 and 2
	 */
	void TranslateSurface1(float fX, float fY, float fZ);
	void RotateSurface1(D3DXVECTOR3 axis, float fFactor);
	void RotateXSurface1(float fFactor);
	void RotateYSurface1(float fFactor);
	void ScaleSurface1(float fFactor);
	void TranslateSurface2(float fX, float fY, float fZ);
	void RotateSurface2(D3DXVECTOR3 axis, float fFactor);
	void RotateXSurface2(float fFactor);
	void RotateYSurface2(float fFactor);
	void ScaleSurface2(float fFactor);

	/*
	 *	Replaces current surface 1 or 2 with a new surface
	 */
	HRESULT LoadSurface1(std::string strMeshName);
	HRESULT LoadSurface2(std::string strMeshName);

	/*
	 *	returns the texture sizes
	 */
	int GetTextureWidth()	{return m_iTextureWidth;}
	int GetTextureHeight()	{return m_iTextureHeight;}
	int GetTextureDepth()	{return m_iTextureDepth;}

	/*
	 *	This function is called, when in the next pass of the render loop the voronoi
	 *  diagram should be generated
	 */
	void GenerateVoronoi();

	/*
	 *	true, if the 3D texture should be visible, else false
	 */
	void Render3DTexture(bool bRender);

	/*
	 *	Updates the bounding box
	 */
	HRESULT UpdateBoundingBox();

	/*
	 *  Returns a LPCWSTR which shows the current render progress
	 */
	LPCWSTR GetProgress();

	/*
	 *  Saves the current volume texture to a .dds file
	 */
	HRESULT SaveCurrentVolume(LPCTSTR sDestination);

protected:

	/*
	 *	Constructor
	 */
	Scene();

	/*
	 *	Destructor
	 *	Release all textures
	 */
	~Scene();

	/*
	 *	Initializes the surfaces (is only called when the application starts
	 */
	HRESULT ItlInitSurfaces();


	static Scene* s_pInstance;

	/*
	 *	variables that control the behaviour of the render loop
	 */
	bool m_bUpdate3DTextures;
	bool m_bGenerateVoronoi;
	bool m_bRender3DTexture;
	bool m_bRenderIsoSurface;
	bool m_bGenerateDiffusion;
	bool m_bIsoValueChanged;
	bool m_bGenerateOneSliceTexture;
	bool	m_bDrawAllSlices;
	bool	m_bShowVolume;
	int		m_iCurrentSlice;
	int		m_iDiffusionSteps;
	float	m_fIsoValue;

	// Device
	ID3D11Device*			m_pd3dDevice;
	ID3D11DeviceContext*	m_pd3dImmediateContext;

	// Surfaces
	Surface*	m_pSurface1;
	Surface*	m_pSurface2;
	Surface*	m_pControlledSurface;
	bool		m_bSurface1IsControlled;

	//Texture size
	int m_iTextureWidth;
	int m_iTextureHeight;
	int m_iTextureDepth;

	//bounding box
	D3DXVECTOR3 m_vMin;
	D3DXVECTOR3 m_vMax;

	// Voronoi Diagram Renderer
	Voronoi*				m_pVoronoi;

	// Diffusion Renderer
	Diffusion*				m_pDiffusion;

	// VolumeRenderer
	VolumeRenderer*			m_pVolumeRenderer;

	// Effects and Techniques
	ID3DX11Effect*					m_pVolumeRenderEffect;
	ID3DX11Effect*					m_pSurfaceEffect;
	ID3DX11Effect*					m_pDiffusionEffect;
	ID3DX11Effect*					m_pVoronoiEffect;

	//Textures
	unsigned int m_nIsoSurfaceTexture;
	unsigned int m_nOneSliceTexture;

	//Bounding Box vertices
	SURFACE_VERTEX* m_pBBVertices;

	//WString that stores current render progress
	std::wstring m_wsRenderProgress;

	// Shader and effect creation
	HRESULT CreateEffect(WCHAR* name, ID3DX11Effect **ppEffect);
	HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

	// variables for measuring the time it took to render
	SYSTEMTIME	m_tStartRenderingVoronoi;
	SYSTEMTIME	m_tEndRenderingVoronoi;
	SYSTEMTIME	m_tStartRenderingDiffusion;
	SYSTEMTIME	m_tEndRenderingDiffusion;
	SYSTEMTIME m_tStartRenderingIsoSurface;
	SYSTEMTIME	m_tEndRenderingIsoSurface;

};

#endif //_SCENE_H_