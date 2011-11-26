#pragma once



class BoundingBox {
public:
	Surface* m_pSurface1;
	Surface* m_pSurface2;

	VERTEX *m_pVertices;

	ID3D11Buffer *m_pVertexBuffer;
	ID3D11Buffer *m_pIndexBuffer;
	ID3D11Buffer *m_pcbPerFrame;
	UINT m_iBindPerFrame;

	BoundingBox(Surface* pSurface1, Surface* pSurface2);
	~BoundingBox();

	HRESULT InitBuffers(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	HRESULT UpdateVertexBuffer(ID3D11Device* pd3dDevice);
	void Render(ID3D11DeviceContext* pd3dImmediateContext, D3DXMATRIX mViewProjection);
};