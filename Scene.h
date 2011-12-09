#pragma once

#include "d3dx11effect.h"

class Surface;
class BoundingBox;


class Scene {
public:
	 enum RENDER_TARGET
    {
        RENDER_TARGET_DIFFUSE0,
        RENDER_TARGET_DIFFUSE1,
        NUM_RENDER_TARGETS
    };

	Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	~Scene();

	HRESULT InitShaders();
	HRESULT InitRenderTargets(int iWidth, int iHeight, int iDepth);
	HRESULT InitRasterizerStates();
	HRESULT InitSurfaces();

	void Render(D3DXMATRIX mViewProjection);

	void ChangeControlledSurface();
	void Translate(float fX, float fY, float fZ);
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void Scale(float fFactor);

	

protected:
	// Device
	ID3D11Device*					m_pd3dDevice;
	ID3D11DeviceContext*			m_pd3dImmediateContext;

	// Render targets (3d textures)
	/*	Textures from part 1
	ID3D10Texture2D *m_diffuseTexture[2];     // two textures used interleavedly for diffusion
	ID3D10Texture2D *m_distDirTexture;    // two textures used interleavedly for diffusion (blurr texture)
	ID3D10Texture2D *m_pDepthStencil;         // for z culling
	ID3D10Texture2D *m_otherTexture;		// texture that keeps the color on the other side of a curve
	*/
	ID3D11Texture3D*						m_pRenderTargets3D[NUM_RENDER_TARGETS]; 
	ID3D11ShaderResourceView*				m_pRenderTargetShaderViews[NUM_RENDER_TARGETS];
	ID3D11RenderTargetView*					m_pRenderTargetViews[NUM_RENDER_TARGETS];
	ID3DX11EffectShaderResourceVariable*	m_pShaderResourceVariables[NUM_RENDER_TARGETS];

	// Effects and techniques
	ID3DX11Effect*					m_pEffect;
	ID3DX11EffectTechnique*			Technique1;

	// Effect variables
	ID3DX11EffectMatrixVariable*	MVPMatrixShaderVariable;
	ID3DX11EffectScalarVariable*	TextureWidthShaderVariable;
    ID3DX11EffectScalarVariable*	TextureHeightShaderVariable;
    ID3DX11EffectScalarVariable*	TextureDepthShaderVariable;

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
	


	// Helper Functions
	HRESULT CreateRenderTarget(int rtIndex, D3D11_TEXTURE3D_DESC desc);
	HRESULT CreateRTTextureAsShaderResource(RENDER_TARGET rtIndex, LPCSTR shaderTextureName, ID3DX11Effect* pEffect, D3D11_SHADER_RESOURCE_VIEW_DESC *SRVDesc );
	HRESULT CreateEffect(WCHAR* name, ID3DX11Effect **ppEffect);
	HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );
};