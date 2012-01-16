#ifndef _VOXELIZER_H_
#define _VOXELIZER_H_

#include "Surface.h"


class Voxelizer
{
public:
    Voxelizer(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pVoxelizerEffect);
    virtual ~Voxelizer();
   
    HRESULT SetDestination(ID3D11Texture3D *pDstInOutTexture3D);
    
    HRESULT Voxelize(Surface *pSurface1, Surface *pSurface2);

private:
    HRESULT Initialize();
    HRESULT InitTextures();
    HRESULT InitShaders();
    HRESULT InitSlices();
    void Cleanup();

    void DrawSlices();

    HRESULT DoVoxelization();
    HRESULT StencilClipVolume();
    HRESULT RenderClippedMesh(float zNear, float zFar, ID3DX11EffectTechnique *pTechnique);

    // INTERNAL STATE
    
    // Destination state
    ID3D11Device                *m_pd3dDevice;
	ID3D11DeviceContext			*m_pd3dImmediateContext;
    ID3D11Texture3D             *m_pDstInOutTexture3D;
    ID3D11RenderTargetView      *m_pDstInOutTexRTView;

    UINT                        m_width;
    UINT                        m_height;
    UINT                        m_depth;

    //  for flat 3D texture
    int                         m_cols;
    int                         m_rows;
    
    // Source state
    Surface						*m_pSurface1;
	Surface						*m_pSurface2;

    // Other state
    bool m_initialized;

    // The depth-stencil buffer
    ID3D11Texture2D             *m_pDSTex2D;
    ID3D11DepthStencilView      *m_pDSTex2DDSView;
    ID3D11ShaderResourceView    *m_pDSTex2DSRView;

    // Effect/shader state
    ID3DX11Effect                *m_pVoxelizerEffect;
    ID3DX11EffectTechnique       *m_pNZTech;
    ID3DX11EffectTechnique       *m_pResolveWithPSTech;

    ID3D11InputLayout           *m_pInputLayout;
    ID3DX11EffectMatrixVariable  *m_pWorldViewProjectionVar;
    ID3DX11EffectShaderResourceVariable *m_pDSTex2DSRVar;

    // Slices state
    ID3D11InputLayout           *m_pSlicesLayout;
    ID3D11Buffer                *m_pSlicesVB;


	//TEST
	bool b;
};

#endif // _VOXELIZER_H_