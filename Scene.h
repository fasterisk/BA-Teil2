#pragma once

class Surface;
class BoundingBox;

class Scene {
public:
	Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	~Scene();

	HRESULT InitShaders();
	HRESULT InitRasterizerStates();
	HRESULT InitSurfaces();

	void Render(D3DXMATRIX mViewProjection);

	void ChangeControlledSurface();
	void Translate(float fX, float fY, float fZ);
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void Scale(float fFactor);

	HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

protected:
	ID3D11Device*				m_pd3dDevice;
	ID3D11DeviceContext*		m_pd3dImmediateContext;

	// Surfaces
	Surface*					m_pSurface1;
	Surface*					m_pSurface2;
	Surface*					m_pControlledSurface;
	bool						m_bSurface1IsControlled;
	
	// Bounding Box
	BoundingBox*				m_pBoundingBox;
	
	
	// Rasterizer states
	ID3D11RasterizerState*		m_pRasterizerStateSolid;
	ID3D11RasterizerState*		m_pRasterizerStateWireframe;

	
	// Inputlayout and shaders
	ID3D11InputLayout*          m_pVertexLayout;
	ID3D11VertexShader*         m_pVertexShader;
	ID3D11PixelShader*          m_pPixelShader;

	

};