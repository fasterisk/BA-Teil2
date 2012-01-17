class Surface;
class Voxelizer;
class VolumeRenderer;
class TextureGrid;

class Scene {
public:

	Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	~Scene();

	HRESULT Initialize(int iTexWidth, int iTexHeight, int iTexDepth);
	HRESULT SetScreenSize(int iWidth, int iHeight);

	void Render(ID3D11RenderTargetView* pRTV, ID3D11RenderTargetView* pSceneDepthRT, ID3D11DepthStencilView* pDSV, D3DXMATRIX mViewProjection);

	void ChangeControlledSurface();
	void Translate(float fX, float fY, float fZ);
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void Scale(float fFactor);

	HRESULT InitRenderTargets(int iWidth, int iHeight, int iDepth);

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

	D3DXVECTOR4 m_vMin;
	D3DXVECTOR4 m_vMax;
	D3DXMATRIX m_mBBInv;

	// Voxelizer
	Voxelizer*				m_pVoxelizer;

	// VolumeRenderer
	VolumeRenderer*			m_pVolumeRenderer;

	// Effects and Techniques
	ID3DX11Effect*					m_pVolumeRenderEffect;
	ID3DX11Effect*					m_pVoxelizerEffect;
	ID3DX11Effect*					m_pSurfaceEffect;

	// TEST
	ID3D11Texture3D*						m_pSurface1Texture3D;
	ID3D11ShaderResourceView*				m_pSurface1SRV;
	D3D11_SHADER_RESOURCE_VIEW_DESC			SRVDesc;


	VERTEX* m_pBBVertices;

	// Helper Functions

	HRESULT CreateRenderTarget(int rtIndex, D3D11_TEXTURE3D_DESC desc);

	// Shader and effect creation
	HRESULT CreateEffect(WCHAR* name, ID3DX11Effect **ppEffect);
	HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

};