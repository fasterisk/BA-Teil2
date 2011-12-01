#pragma once

#include "d3dx11effect.h"

class Surface;
class BoundingBox;


class Scene {
public:
	Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	~Scene();

	HRESULT InitShaders();
	HRESULT SetupTextures(int iSizeX, int iSizeY, int iSizeZ);
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
	// Device
	ID3D11Device*					m_pd3dDevice;
	ID3D11DeviceContext*			m_pd3dImmediateContext;

	// Render targets (3d textures)
	ID3D11Texture3D					*m_pRenderTarget3D;

	// Effects and techniques
	ID3DX11Effect*					m_pEffect;
	ID3DX11EffectTechnique*			m_pMainTechnique;

	// Effect variables
	ID3DX11EffectMatrixVariable*	m_pMVPVariable;

	// Rasterizer states
	ID3D11RasterizerState*			m_pRasterizerStateSolid;
	ID3D11RasterizerState*			m_pRasterizerStateWireframe;
	
	// Inputlayout and shaders
	ID3D11InputLayout*				m_pInputLayout;
	ID3D11VertexShader*				m_pVertexShader;
	ID3D11PixelShader*				m_pPixelShader;

	// Surfaces
	Surface*						m_pSurface1;
	Surface*						m_pSurface2;
	Surface*						m_pControlledSurface;
	bool							m_bSurface1IsControlled;
	
	// Bounding Box
	BoundingBox*					m_pBoundingBox;
	
	
	

	

};