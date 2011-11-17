#pragma once
#include "SDKMesh.h"

struct VERTEX
{
	float x;
	float y;
	float z;
};

struct CB_VS_PER_OBJECT
{
    D3DXMATRIX m_mModelViewProj;
};


struct CB_PS_PER_OBJECT
{
    D3DXVECTOR4 m_vObjectColor;
};


class Surface
{
public:
	int m_vNum;
	VERTEX *m_pVertices;
	int m_iNum;
	int *m_pIndices;

	ID3D11Buffer* m_pcbVSPerObject;
	ID3D11Buffer* m_pcbPSPerObject;
	ID3D11Buffer* m_vertexbuffer;
	ID3D11Buffer* m_indexbuffer;

	CDXUTSDKMesh m_Mesh11;
	UINT m_iCBVSPerObjectBind;
	UINT m_iCBPSPerObjectBind;

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

	HRESULT InitBuffers(ID3D11Device* pd3dDevice);
	void Render(ID3D11DeviceContext* pd3dImmediateContext, D3DXMATRIX mViewProjection, D3DXVECTOR3 vCamEye, float fSubdivs);

	void ReadVectorFile(char *s);
};