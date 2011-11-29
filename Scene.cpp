// Include the OS headers
//-----------------------
#include <windows.h>
#include <atlbase.h>
#pragma warning( disable: 4996 )
#include <strsafe.h>
#pragma warning( default: 4996 )
// Include the D3D11 headers
//--------------------------
#include <d3d11.h>
#include <d3dx11.h>

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "Surface.h"
#include "BoundingBox.h"
#include "Scene.h"

Scene::Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
	: m_pd3dDevice(pd3dDevice), m_pd3dImmediateContext(pd3dImmediateContext)
{
}

Scene::~Scene()
{
	SAFE_RELEASE(m_pVertexLayout);
    SAFE_RELEASE(m_pVertexShader);
    SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pRasterizerStateSolid);
	SAFE_RELEASE(m_pRasterizerStateWireframe);
	
	SAFE_DELETE(m_pSurface1);
	SAFE_DELETE(m_pSurface2);
	SAFE_DELETE(m_pBoundingBox);
}


HRESULT Scene::InitShaders()
{
	HRESULT hr;

    	// Compile the shaders using the lowest possible profile for broadest feature level support
    ID3DBlob* pVertexShaderBuffer = NULL;
    V_RETURN( CompileShaderFromFile( L"DiffusionShader11.hlsl", "VSMain", "vs_5_0", &pVertexShaderBuffer ) );

    ID3DBlob* pPixelShaderBuffer = NULL;
    V_RETURN( CompileShaderFromFile( L"DiffusionShader11.hlsl", "PSMain", "ps_5_0", &pPixelShaderBuffer ) );

    // Create the shaders
    V_RETURN( m_pd3dDevice->CreateVertexShader( pVertexShaderBuffer->GetBufferPointer(), pVertexShaderBuffer->GetBufferSize(), NULL, &m_pVertexShader ) );
    DXUT_SetDebugName( m_pVertexShader, "VSMain" );
    V_RETURN( m_pd3dDevice->CreatePixelShader( pPixelShaderBuffer->GetBufferPointer(), pPixelShaderBuffer->GetBufferSize(), NULL, &m_pPixelShader ) );
    DXUT_SetDebugName( m_pPixelShader, "PSMain" );

	m_pd3dImmediateContext->VSSetShader(m_pVertexShader, 0, 0);//check if needed
	m_pd3dImmediateContext->PSSetShader(m_pPixelShader, 0, 0);

	// Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",	   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

	V_RETURN( m_pd3dDevice->CreateInputLayout( layout, ARRAYSIZE( layout ), pVertexShaderBuffer->GetBufferPointer(), pVertexShaderBuffer->GetBufferSize(), &m_pVertexLayout ) );
    DXUT_SetDebugName( m_pVertexLayout, "Primary" );
	
	m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout);

    SAFE_RELEASE( pVertexShaderBuffer );
    SAFE_RELEASE( pPixelShaderBuffer );

	return S_OK;
}


HRESULT Scene::InitRasterizerStates()
{
	HRESULT hr;

	// Create solid and wireframe rasterizer state objects
    D3D11_RASTERIZER_DESC RasterDesc;
    ZeroMemory( &RasterDesc, sizeof(D3D11_RASTERIZER_DESC) );
    RasterDesc.FillMode = D3D11_FILL_SOLID;
    RasterDesc.CullMode = D3D11_CULL_NONE;
    RasterDesc.DepthClipEnable = TRUE;
    V_RETURN( m_pd3dDevice->CreateRasterizerState( &RasterDesc, &m_pRasterizerStateSolid ) );
    DXUT_SetDebugName( m_pRasterizerStateSolid, "Solid" );

    RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    V_RETURN( m_pd3dDevice->CreateRasterizerState( &RasterDesc, &m_pRasterizerStateWireframe ) );
    DXUT_SetDebugName( m_pRasterizerStateWireframe, "Wireframe" );

	return S_OK;
}

HRESULT Scene::InitSurfaces()
{
	HRESULT hr;

	// Create surface1 and its buffers
	m_pSurface1 = new Surface();
	m_pSurface1->ReadVectorFile("Media\\surface1.xml");
	V_RETURN(m_pSurface1->InitBuffers(m_pd3dDevice, m_pd3dImmediateContext));
    
	// Create surface2 and its buffers
	m_pSurface2 = new Surface();
	m_pSurface2->ReadVectorFile("Media\\surface1.xml");
	m_pSurface2->SetColor(1.0, 1.0, 1.0);
	V_RETURN(m_pSurface2->InitBuffers(m_pd3dDevice, m_pd3dImmediateContext));
	m_pSurface2->Scale(0.5);

	// Create bounding box
	m_pBoundingBox = new BoundingBox(m_pSurface1, m_pSurface2);
	V_RETURN(m_pBoundingBox->InitBuffers(m_pd3dDevice, m_pd3dImmediateContext));

	m_pControlledSurface = m_pSurface1;

	return S_OK;
}

void Scene::Render(D3DXMATRIX mViewProjection)
{
	m_pBoundingBox->UpdateVertexBuffer(m_pd3dDevice);

	// Set the shaders
    m_pd3dImmediateContext->VSSetShader( m_pVertexShader, NULL, 0 );
    m_pd3dImmediateContext->PSSetShader( m_pPixelShader, NULL, 0 );
	
	
	m_pSurface1->Render(m_pd3dImmediateContext, mViewProjection);
	m_pSurface2->Render(m_pd3dImmediateContext, mViewProjection);

	m_pd3dImmediateContext->RSSetState(m_pRasterizerStateWireframe);
	m_pBoundingBox->Render(m_pd3dImmediateContext, mViewProjection);
	m_pd3dImmediateContext->RSSetState(m_pRasterizerStateSolid); 
}

void Scene::ChangeControlledSurface()
{
	if(m_bSurface1IsControlled)
		m_pControlledSurface = m_pSurface2;
	else
		m_pControlledSurface = m_pSurface1;

	m_bSurface1IsControlled = !m_bSurface1IsControlled;
}

void Scene::Translate(float fX, float fY, float fZ)
{
	m_pControlledSurface->Translate(fX, fY, fZ);
}

void Scene::RotateX(float fFactor)
{
	m_pControlledSurface->RotateX(fFactor);
}

void Scene::RotateY(float fFactor)
{
	m_pControlledSurface->RotateY(fFactor);
}

void Scene::Scale(float fFactor)
{
	m_pControlledSurface->Scale(fFactor);
}

//--------------------------------------------------------------------------------------
// Find and compile the specified shader
//--------------------------------------------------------------------------------------
HRESULT Scene::CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( str, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return S_OK;
}