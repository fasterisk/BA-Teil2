class VolumeRenderer
{
public:
	/*
	 *  Constructor
	 */
	VolumeRenderer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pEffect);

	/*
	 *  Destructor
	 */
	~VolumeRenderer();

	/*
	 *  Initialize volumerenderer
	 *		- shader variables
	 *		- index buffer and layout of the bounding box
	 */
	HRESULT Initialize();

	/*
	 *  Update stepsize and iterations according to the texture size
	 */
	HRESULT Update(int iWidth, int iHeight, int iDepth);

	/*
	 *  Update screen size
	 */
	HRESULT SetScreenSize(int iWidth, int iHeight);

	/*
	 *  Switch between nearest neighbour and linear sampling
	 */
	void ChangeSampling();

	/*
	 *  Switch between settings for "normal" volume rendering and isosurface rendering
	 */
	void ShowIsoSurface(bool bShow);

	/*
	 *  Render a given 3D Texture into the bounding box
	 */
	void Render(SURFACE_VERTEX* pBBVertices, D3DXVECTOR3 vBBMin, D3DXVECTOR3 vBBMax, D3DXMATRIX mWorldViewProjection, ID3D11ShaderResourceView* p3DTextureSRV);

private:
	//true, if linear sampling, false if nearest neighbor
	bool m_bLinearSampling;

	//controls the setting when isosurface is rendered
	bool m_bShowIsoSurface;

	// Device
	ID3D11Device*			m_pd3dDevice;
	ID3D11DeviceContext*	m_pd3dImmediateContext;
	
	// Shader effect and variables
	ID3DX11Effect*							m_pEffect;
	ID3DX11EffectTechnique*					m_pVolumeRenderTechnique;
	
	ID3DX11EffectMatrixVariable*			m_pWorldViewProjectionVar;
	
	ID3DX11EffectShaderResourceVariable*	m_pFrontTextureVar;
	ID3DX11EffectShaderResourceVariable*	m_pBackTextureVar;
	ID3DX11EffectShaderResourceVariable*	m_pVolumeTextureVar;

	ID3DX11EffectVectorVariable*			m_pStepSizeVar;
	ID3DX11EffectVectorVariable*			m_pBBMinVar;
	ID3DX11EffectVectorVariable*			m_pBBMaxVar;

	ID3DX11EffectScalarVariable*			m_pIterationsVar;
	ID3DX11EffectScalarVariable*			m_fAlphaVar;
	ID3DX11EffectScalarVariable*			m_pSamplingVar;
	ID3DX11EffectScalarVariable*			m_pShowIsoSurfaceVar;

	//Screen size
	int m_iWidth;
	int m_iHeight;

	//Bounding Box
	ID3D11Buffer*			m_pBBVertexBuffer;
	ID3D11Buffer*			m_pBBIndexBuffer;
	ID3D11InputLayout*		m_pBBInputLayout;

	//front and back textures of the bounding box
	ID3D11Texture2D*			m_pFrontTexture2D;
    ID3D11RenderTargetView*		m_pFrontRTV;
    ID3D11ShaderResourceView*	m_pFrontSRV;
    ID3D11Texture2D*			m_pBackTexture2D;
    ID3D11RenderTargetView*		m_pBackRTV;
    ID3D11ShaderResourceView*	m_pBackSRV;

	//Screen Quad
	ID3D11Buffer*			m_pSQVertexBuffer;
	ID3D11InputLayout*		m_pSQInputLayout;

	/*
	 *  Initializing functions
	 */
	HRESULT InitShader();
	HRESULT InitBoundingIndicesAndLayout();
	HRESULT UpdateBoundingVertices(SURFACE_VERTEX* BBVertices);

	/*
	 * Gets called when:
	 *  1. Draw front face of bounding box
	 *  2. Draw back face of bounding box
	 *  3. Raycast
	 *  4. Draw wireframe bounding box
	 */
	void DrawBoundingBox();

};