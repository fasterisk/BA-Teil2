#ifndef SURFACE_H
#define SURFACE_H

class Surface
{
public:
	
	Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pSurfaceEffect);
	~Surface();
	
	int m_iNumVertices;
	VERTEX *m_pVertices;
	

	D3DXMATRIX m_mModel;

	void Translate(float fX, float fY, float fZ);
	
	void Rotate(D3DXVECTOR3 axis, float fFactor);
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void RotateZ(float fFactor);

	void Scale(float fFactor);

	void SetColor(float fR, float fG, float fB);//after that you need to init buffers again

	HRESULT Initialize(char *s);
	void Render(D3DXMATRIX mViewProjection);
	void Render(ID3DX11EffectTechnique* pTechnique);

protected:

	ID3D11Device*					m_pd3dDevice;
	ID3D11DeviceContext*			m_pd3dImmediateContext;
	ID3DX11Effect*					m_pSurfaceEffect;
	ID3DX11EffectTechnique*			m_pTechnique;
	ID3D11InputLayout*				m_pInputLayout;

	ID3DX11EffectMatrixVariable		*m_pModelViewProjectionVar;

	ID3D11Buffer* m_pVertexBuffer;

	D3DXMATRIX m_mRot;
	D3DXMATRIX m_mTrans;
	D3DXMATRIX m_mTransInv;
	D3DXVECTOR3 m_translation;

	HRESULT InitBuffers();
	void ReadVectorFile(char *s);
};

#endif