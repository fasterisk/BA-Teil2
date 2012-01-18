class VolumeRenderer
{
public:
	VolumeRenderer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pEffect);
	~VolumeRenderer();

	HRESULT Initialize(int iWidth, int iHeight, int iDepth);
	HRESULT SetScreenSize(int iWidth, int iHeight);

	void Render(VERTEX* pBBVertices, D3DXMATRIX mWorldViewProjection, ID3D11ShaderResourceView* p3DTextureSRV);

private:
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
    
	ID3DX11EffectScalarVariable*			m_pIterationsVar;

	//Screen size
	int m_iWidth;
	int m_iHeight;

	//Bounding Box
	ID3D11Buffer*			m_pBBVertexBuffer;
	ID3D11Buffer*			m_pBBIndexBuffer;
	ID3D11InputLayout*		m_pBBInputLayout;

	ID3D11Texture2D*			m_pFrontTexture2D;
    ID3D11RenderTargetView*		m_pFrontRTV;
    ID3D11ShaderResourceView*	m_pFrontSRV;
    ID3D11Texture2D*			m_pBackTexture2D;
    ID3D11RenderTargetView*		m_pBackRTV;
    ID3D11ShaderResourceView*	m_pBackSRV;

	//Screen Quad
	ID3D11Buffer*			m_pSQVertexBuffer;
	ID3D11InputLayout*		m_pSQInputLayout;


	HRESULT InitShader();
	HRESULT InitBoundingIndicesAndLayout();
	HRESULT CreateScreenQuad();
	HRESULT UpdateBoundingVertices(VERTEX* BBVertices);
	void InitTextureSize(int iWidth, int iHeight, int iDepth);
	void DrawBoundingBox();
	void DrawScreenQuad();
};