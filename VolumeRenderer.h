class VolumeRenderer
{
public:
	VolumeRenderer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pEffect);
	~VolumeRenderer();

	HRESULT Initialize();
	HRESULT SetScreenSize(int iWidth, int iHeight);

	void Render(VERTEX* pBBVertices, ID3D11Texture3D* p3DTexture);

private:
	// Device
	ID3D11Device*			m_pd3dDevice;
	ID3D11DeviceContext*	m_pd3dImmediateContext;
	ID3DX11Effect*			m_pEffect;
	ID3DX11EffectTechnique* m_pVolumeRenderTechnique;


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


	HRESULT InitShader();
	HRESULT InitBoundingIndicesAndLayout();
	HRESULT UpdateBoundingVertices(VERTEX* BBVertices);
	

};