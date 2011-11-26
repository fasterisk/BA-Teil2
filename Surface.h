#pragma once

struct VERTEX
{
	float x, y, z;
	D3DXCOLOR color;
};

struct CB_PER_FRAME_CONSTANTS
{
    D3DXMATRIX mModelViewProjection;
};


class Surface
{
public:
	int m_vNum;
	VERTEX *m_pVertices;

	int m_iNum;
	unsigned int *m_pIndices;

	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;
	ID3D11Buffer* m_pcbPerFrame;
	UINT m_iBindPerFrame;

	D3DXMATRIX m_mModel;
	D3DXMATRIX m_mRot;
	D3DXMATRIX m_mTrans;
	D3DXMATRIX m_mTransInv;
	

	Surface();
	~Surface();

	D3DXVECTOR3 m_translation;

	const D3DXVECTOR3* m_xAxis;
	const D3DXVECTOR3* m_yAxis;
	const D3DXVECTOR3* m_zAxis;

	void Translate(float fX, float fY, float fZ);
	
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void RotateZ(float fFactor);

	void Scale(float fFactor);

	void SetColor(float fR, float fG, float fB);//has to be called before initbuffers or you have to repeat initbuffers

	HRESULT InitBuffers(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	void Render(ID3D11DeviceContext* pd3dImmediateContext, D3DXMATRIX mViewProjection);

	void ReadVectorFile(char *s);
};