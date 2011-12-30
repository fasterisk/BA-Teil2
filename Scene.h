class Surface;
class Voxelizer;
class VolumeRenderer;
class TextureGrid;

class Scene {
public:
	enum RENDER_TARGET
    {
        RENDER_TARGET_DIFFUSE0,
        RENDER_TARGET_DIFFUSE1,
        NUM_RENDER_TARGETS
    };

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

	// BoundingBox Wireframe Methods
	HRESULT UpdateVertexBuffer();
	void RenderWireframe(D3DXMATRIX mViewProjection);

protected:
	HRESULT InitSurfaces();
	HRESULT InitBuffers();
	HRESULT InitTechniques();

	// Device
	ID3D11Device*			m_pd3dDevice;
	ID3D11DeviceContext*	m_pd3dImmediateContext;

	// Surfaces
	Surface*	m_pSurface1;
	Surface*	m_pSurface2;
	Surface*	m_pControlledSurface;
	bool		m_bSurface1IsControlled;

	// Voxelizer
	Voxelizer*				m_pVoxelizer;

	// VolumeRenderer
	VolumeRenderer*			m_pVolumeRenderer;

	// Effects and Techniques
	ID3DX11Effect*					m_pDiffusionEffect;
	ID3DX11Effect*					m_pSurfaceEffect;
	ID3DX11Effect*					m_pVolumeRenderEffect;
	ID3DX11Effect*					m_pVoxelizerEffect;

	// Effect variables
	ID3DX11EffectMatrixVariable*	MVPMatrixShaderVariable;
	ID3DX11EffectScalarVariable*	TextureWidthShaderVariable;
    ID3DX11EffectScalarVariable*	TextureHeightShaderVariable;
    ID3DX11EffectScalarVariable*	TextureDepthShaderVariable;

	// Render targets (3d textures)
	/*	Textures from part 1
	ID3D10Texture2D *m_diffuseTexture[2];     // two textures used interleavedly for diffusion
	ID3D10Texture2D *m_distDirTexture;    // two textures used interleavedly for diffusion (blurr texture)
	ID3D10Texture2D *m_pDepthStencil;         // for z culling
	ID3D10Texture2D *m_otherTexture;		// texture that keeps the color on the other side of a curve
	*/
	ID3D11Texture3D*						m_pRenderTargets3D[NUM_RENDER_TARGETS]; 
	ID3D11ShaderResourceView*				m_pRenderTargetShaderViews[NUM_RENDER_TARGETS];
	ID3D11RenderTargetView*					m_pRenderTargetViews[NUM_RENDER_TARGETS];
	ID3DX11EffectShaderResourceVariable*	m_pShaderResourceVariables[NUM_RENDER_TARGETS];

	// TEST
	ID3D11Texture3D*						m_pSurface1Texture3D;
	ID3D11ShaderResourceView*				m_pSurface1SRV;
	D3D11_SHADER_RESOURCE_VIEW_DESC			SRVDesc;


	// BoundingBox Wireframe members
	VERTEX *m_pVertices;
	ID3D11Buffer *m_pVertexBuffer;
	ID3D11Buffer *m_pIndexBuffer;



	// Helper Functions

	HRESULT CreateRenderTarget(int rtIndex, D3D11_TEXTURE3D_DESC desc);
	HRESULT CreateRTTextureAsShaderResource(RENDER_TARGET rtIndex, LPCSTR shaderTextureName, ID3DX11Effect* pEffect, D3D11_SHADER_RESOURCE_VIEW_DESC *SRVDesc );

	// Shader and effect creation
	HRESULT CreateEffect(WCHAR* name, ID3DX11Effect **ppEffect);
	HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

};