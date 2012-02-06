class Surface;
class Voxelizer;
class VolumeRenderer;
class TextureGrid;
class Voronoi;

class Scene {
public:

	Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	~Scene();

	HRESULT Initialize(int iTexWidth, int iTexHeight, int iTexDepth);
	HRESULT SetScreenSize(int iWidth, int iHeight);
	void UpdateTextureResolution(int iMaxRes);

	void Render(ID3D11RenderTargetView* pRTV, ID3D11RenderTargetView* pSceneDepthRT, ID3D11DepthStencilView* pDSV, D3DXMATRIX mViewProjection);


	void ChangeControlledSurface();
	void Translate(float fX, float fY, float fZ);
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void Scale(float fFactor);

	int GetTextureWidth()	{return iTextureWidth;}
	int GetTextureHeight()	{return iTextureHeight;}
	int GetTextureDepth()	{return iTextureDepth;}


	HRESULT UpdateBoundingBox();

protected:
	HRESULT InitSurfaces();

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

	// Voxelizer
	Voxelizer*				m_pVoxelizer;

	// VolumeRenderer
	VolumeRenderer*			m_pVolumeRenderer;

	// Effects and Techniques
	ID3DX11Effect*					m_pVolumeRenderEffect;
	ID3DX11Effect*					m_pVoxelizerEffect;
	ID3DX11Effect*					m_pSurfaceEffect;
	ID3DX11Effect*					m_pVoronoiEffect;

	// TEST
	ID3D11Texture3D*				m_pTexture3D;
	ID3D11ShaderResourceView*		m_pTexture3DSRV;


	VERTEX* m_pBBVertices;


	HRESULT Init3DTexture();

	// Shader and effect creation
	HRESULT CreateEffect(WCHAR* name, ID3DX11Effect **ppEffect);
	HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

};