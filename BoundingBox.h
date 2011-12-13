#pragma once



class BoundingBox {
public:
	enum RENDER_TARGET
    {
        RENDER_TARGET_DIFFUSE0,
        RENDER_TARGET_DIFFUSE1,
        NUM_RENDER_TARGETS
    };
	
	BoundingBox(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pEffect);
	~BoundingBox();
	
	HRESULT InitSurfaces();
	HRESULT InitBuffers();
	HRESULT InitRasterizerStates();
	HRESULT InitTechniques();
	HRESULT InitRenderTargets(int iWidth, int iHeight, int iDepth);
	
	HRESULT UpdateVertexBuffer();
	
	void Render(D3DXMATRIX mViewProjection);

	void ChangeControlledSurface();
	void CSTranslate(float fX, float fY, float fZ);
	void CSRotateX(float fFactor);
	void CSRotateY(float fFactor);
	void CSScale(float fFactor);

protected:
	Surface*	m_pSurface1;
	Surface*	m_pSurface2;
	Surface*	m_pControlledSurface;
	bool		m_bSurface1IsControlled;

	VERTEX *m_pVertices;

	// Rasterizer states
	ID3D11RasterizerState*			m_pRasterizerStateSolid;
	ID3D11RasterizerState*			m_pRasterizerStateWireframe;

	ID3D11Buffer *m_pVertexBuffer;
	ID3D11Buffer *m_pIndexBuffer;

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

	ID3D11Device*					m_pd3dDevice;
	ID3D11DeviceContext*			m_pd3dImmediateContext;

	ID3DX11Effect*					m_pEffect;
	ID3DX11EffectTechnique*			Technique;
	ID3D11InputLayout*				m_pInputLayout;

	// Effect variables
	ID3DX11EffectMatrixVariable*	MVPMatrixShaderVariable;
	ID3DX11EffectScalarVariable*	TextureWidthShaderVariable;
    ID3DX11EffectScalarVariable*	TextureHeightShaderVariable;
    ID3DX11EffectScalarVariable*	TextureDepthShaderVariable;

	// Helper Functions

	HRESULT CreateRenderTarget(int rtIndex, D3D11_TEXTURE3D_DESC desc);
	HRESULT CreateRTTextureAsShaderResource(RENDER_TARGET rtIndex, LPCSTR shaderTextureName, ID3DX11Effect* pEffect, D3D11_SHADER_RESOURCE_VIEW_DESC *SRVDesc );
};