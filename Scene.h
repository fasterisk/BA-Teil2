class Surface;
class Voxelizer;
class VolumeRenderer;
class TextureGrid;
class Voronoi;
class Diffusion;

class Scene {
public:

	Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	~Scene();

	HRESULT Initialize(int iTexWidth, int iTexHeight, int iTexDepth);
	HRESULT SetScreenSize(int iWidth, int iHeight);
	void UpdateTextureResolution(int iMaxRes);

	void Render(D3DXMATRIX mViewProjection, bool bShowSurfaces);

	void ChangeIsoValue(float fIsoValue);

	void ChangeControlledSurface();
	HRESULT ChangeRenderingToOneSlice(int iSliceIndex);
	HRESULT ChangeRenderingToAllSlices();
	void TranslateCurrentSurface(float fX, float fY, float fZ);
	void RotateCurrentSurface(D3DXVECTOR3 axis, float fFactor);
	void RotateXCurrentSurface(float fFactor);
	void RotateYCurrentSurface(float fFactor);
	void ScaleCurrentSurface(float fFactor);

	int GetTextureWidth()	{return iTextureWidth;}
	int GetTextureHeight()	{return iTextureHeight;}
	int GetTextureDepth()	{return iTextureDepth;}

	void GenerateVoronoi();
	void Render3DTexture(bool bRender);

	HRESULT UpdateBoundingBox();

protected:
	HRESULT InitSurfaces();
	HRESULT Init3DTextures();

	bool initialized;
	bool m_bGenerateVoronoi;
	bool m_bRender3DTexture;

	// Device
	ID3D11Device*			m_pd3dDevice;
	ID3D11DeviceContext*	m_pd3dImmediateContext;

	// Surfaces
	Surface*	m_pSurface1;
	Surface*	m_pSurface2;
	Surface*	m_pControlledSurface;
	bool		m_bSurface1IsControlled;

	int iTextureWidth;
	int iTextureHeight;
	int iTextureDepth;

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

	ID3D11Texture3D*				m_pVoronoi3DTex;
	ID3D11Texture3D*				m_pColor3DTex1;
	ID3D11Texture3D*				m_pColor3DTex2;
	ID3D11Texture3D*				m_pDist3DTex;
	ID3D11ShaderResourceView*		m_pVoronoi3DTexSRV;
	ID3D11ShaderResourceView*		m_pColor3DTex1SRV;
	ID3D11ShaderResourceView*		m_pColor3DTex2SRV;
	ID3D11ShaderResourceView*		m_pDist3DTexSRV;

	ID3D11ShaderResourceView*		m_pCurrentDiffusionSRV;

	//Bounding Box vertices
	VERTEX* m_pBBVertices;

	// Shader and effect creation
	HRESULT CreateEffect(WCHAR* name, ID3DX11Effect **ppEffect);
	HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

};