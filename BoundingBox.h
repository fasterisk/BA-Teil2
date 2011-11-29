#pragma once



class BoundingBox {
public:
	Surface* m_pSurface1;
	Surface* m_pSurface2;

	VERTEX *m_pVertices;

	ID3D11Buffer *m_pVertexBuffer;
	ID3D11Buffer *m_pIndexBuffer;

	ID3D11Device*					m_pd3dDevice;
	ID3D11DeviceContext*			m_pd3dImmediateContext;
	ID3DX11EffectTechnique*			m_pMainTechnique;
	ID3DX11EffectMatrixVariable*	m_pMVPVariable;

	BoundingBox(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11EffectTechnique* pMainTechnique, ID3DX11EffectMatrixVariable* pMVPVariable, Surface* pSurface1, Surface* pSurface2);
	~BoundingBox();

	HRESULT InitBuffers();
	HRESULT UpdateVertexBuffer();
	void Render(D3DXMATRIX mViewProjection);
};