#ifndef SURFACE_H
#define SURFACE_H

class Surface
{
public:
	
	Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pSurfaceEffect);
	~Surface();
	
	int m_iNumTriangleVertices;
	int m_iNumEdgeVertices;
	VERTEX *m_pTriangleVertices;
	VERTEX *m_pEdgeVertices;
	

	D3DXMATRIX m_mModel;

	void Translate(float fX, float fY, float fZ);
	
	void Rotate(D3DXVECTOR3 axis, float fFactor);
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void RotateZ(float fFactor);

	void Scale(float fFactor);

	void SetColor(float fR, float fG, float fB);//after that you need to init buffers again

	HRESULT LoadMesh(LPWSTR lsFileName);

	HRESULT Initialize(char *s);
	void Render(D3DXMATRIX mViewProjection);
	void Render(ID3DX11EffectTechnique* pTechnique);
	void RenderVoronoi(ID3DX11EffectTechnique* pTechnique);

protected:

	ID3D11Device*					m_pd3dDevice;
	ID3D11DeviceContext*			m_pd3dImmediateContext;
	ID3DX11Effect*					m_pSurfaceEffect;
	ID3DX11EffectTechnique*			m_pTechnique;
	ID3D11InputLayout*				m_pInputLayout;

	ID3DX11EffectMatrixVariable		*m_pModelViewProjectionVar;
	ID3DX11EffectMatrixVariable		*m_pNormalMatrixVar;

	ID3DX11EffectShaderResourceVariable *m_pSurfaceTextureVar;

	ID3D11Buffer* m_pTriangleVertexBuffer;
	ID3D11Buffer* m_pEdgeVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;

	D3DXMATRIX m_mRot;
	D3DXMATRIX m_mTrans;
	D3DXMATRIX m_mTransInv;
	D3DXVECTOR3 m_translation;

	CDXUTSDKMesh	m_pSurfaceMesh;


	//TEST TEST TEST TEST
	CDXUTSDKMesh                g_Mesh11;

ID3D11InputLayout*          g_pVertexLayout11;
ID3D11Buffer*               g_pVertexBuffer;
ID3D11Buffer*               g_pIndexBuffer;
ID3D11VertexShader*         g_pVertexShader;
ID3D11PixelShader*          g_pPixelShader;
ID3D11SamplerState*         g_pSamLinear;


UINT                        g_iCBVSPerObjectBind;


UINT                        g_iCBPSPerObjectBind;


ID3D11Buffer*               g_pcbVSPerObject;
ID3D11Buffer*               g_pcbPSPerObject;
};

#endif