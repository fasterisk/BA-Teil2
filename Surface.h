#pragma once

#include "d3dx11effect.h"

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

	D3DXMATRIX m_mModel;
	D3DXMATRIX m_mRot;
	D3DXMATRIX m_mTrans;
	D3DXMATRIX m_mTransInv;
	

	Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11EffectTechnique* pTechnique, ID3DX11EffectMatrixVariable* pMVPMatrixShaderVariable);
	~Surface();


	ID3D11Device*					m_pd3dDevice;
	ID3D11DeviceContext*			m_pd3dImmediateContext;
	ID3DX11EffectTechnique*			Technique;
	ID3DX11EffectMatrixVariable*	MVPMatrixShaderVariable;


	D3DXVECTOR3 m_translation;

	void Translate(float fX, float fY, float fZ);
	
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void RotateZ(float fFactor);

	void Scale(float fFactor);

	void SetColor(float fR, float fG, float fB);//has to be called before initbuffers or you have to repeat initbuffers

	HRESULT InitBuffers();
	void Render(D3DXMATRIX mViewProjection);

	void ReadVectorFile(char *s);
};