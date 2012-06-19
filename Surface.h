#ifndef SURFACE_H
#define SURFACE_H

class Surface
{
public:
	
	Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pSurfaceEffect);
	~Surface();
	
	D3DXMATRIX m_mModel;

	void Translate(float fX, float fY, float fZ);
	
	void Rotate(D3DXVECTOR3 axis, float fFactor);
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void RotateZ(float fFactor);

	void Scale(float fFactor);

	void SetColor(D3DXVECTOR3 vColor);
	void SetColor(float fR, float fG, float fB);

	HRESULT LoadMesh(LPWSTR lsFileName);

	HRESULT Initialize(LPWSTR lsFileName);
	void Render(D3DXMATRIX mViewProjection);
	void RenderVoronoi(ID3DX11EffectTechnique* pTechnique, ID3DX11EffectShaderResourceVariable *pSurfaceTextureVar);

	//GETTER
	D3DXVECTOR3 GetBoundingBoxCenter();
	D3DXVECTOR3 GetBoundingBoxExtents();
	D3DXVECTOR3 GetColor();

protected:

	HRESULT InitializeShader();

	ID3D11Device*					m_pd3dDevice;
	ID3D11DeviceContext*			m_pd3dImmediateContext;
	ID3DX11Effect*					m_pSurfaceEffect;
	ID3DX11EffectTechnique*			m_pTechnique;
	ID3D11InputLayout*				m_pInputLayout;

	ID3DX11EffectMatrixVariable		*m_pModelViewProjectionVar;

	ID3DX11EffectShaderResourceVariable *m_pSurfaceTextureVar;

	ID3D11Buffer*	m_pVertexBuffer;
	ID3D11Buffer*	m_pIndexBuffer;
	unsigned int m_mNumVertices;
	unsigned int m_mNumIndices;

	ID3D11Texture2D	*m_pDiffuseTexture;
	ID3D11ShaderResourceView	*m_pDiffuseTextureSRV;

	D3DXMATRIX m_mRot;
	D3DXMATRIX m_mTrans;
	D3DXMATRIX m_mTransInv;
	D3DXVECTOR3 m_translation;

	D3DXVECTOR3 m_vColor;

	CDXUTSDKMesh	m_pSurfaceMesh;


};

#endif