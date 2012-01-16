class VolumeRenderer
{
public:
	VolumeRenderer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pEffect);
	~VolumeRenderer();

	HRESULT Initialize();

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


	HRESULT InitShader();
	HRESULT InitBoundingIndicesAndLayout();
	HRESULT UpdateBoundingVertices(VERTEX* BBVertices);
	

};