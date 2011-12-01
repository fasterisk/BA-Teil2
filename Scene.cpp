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

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"

#include "Surface.h"
#include "BoundingBox.h"
#include "Scene.h"

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

Scene::Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
	: m_pd3dDevice(pd3dDevice), m_pd3dImmediateContext(pd3dImmediateContext)
{
}

Scene::~Scene()
{
	SAFE_RELEASE(m_pEffect);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pRasterizerStateSolid);
	SAFE_RELEASE(m_pRasterizerStateWireframe);
	
	SAFE_DELETE(m_pSurface1);
	SAFE_DELETE(m_pSurface2);
	SAFE_DELETE(m_pBoundingBox);
}


HRESULT Scene::InitShaders()
{
	HRESULT hr;
	// Read the D3DX effect file
    WCHAR str[MAX_PATH];
	ID3D10Blob *effectBlob = 0, *errorsBlob = 0;
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"DiffusionShader11.fx" ) );
    hr = D3DX11CompileFromFile( str, NULL, NULL, NULL, "fx_5_0", NULL, NULL, NULL, &effectBlob, &errorsBlob, NULL );
	if(FAILED ( hr ))
	{
		std::string errStr((LPCSTR)errorsBlob->GetBufferPointer(), errorsBlob->GetBufferSize());
		WCHAR err[256];
		MultiByteToWideChar(CP_ACP, 0, errStr.c_str(), (int)errStr.size(), err, errStr.size());
		MessageBox( NULL, (LPCWSTR)err, L"Error", MB_OK );
		return hr;
	}

	V_RETURN(D3DX11CreateEffectFromMemory(effectBlob->GetBufferPointer(), effectBlob->GetBufferSize(), 0, m_pd3dDevice, &m_pEffect));


	m_pMainTechnique = m_pEffect->GetTechniqueByName("Main");
	assert(m_pMainTechnique && m_pMainTechnique->IsValid());
	
	m_pMVPVariable = m_pEffect->GetVariableByName("g_mModelViewProjection")->AsMatrix();

	D3DX11_PASS_SHADER_DESC effectVsDesc;
	m_pMainTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&effectVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc2;
	effectVsDesc.pShaderVariable->GetShaderDesc(effectVsDesc.ShaderIndex, &effectVsDesc2);
	const void *vsCodePtr = effectVsDesc2.pBytecode;
	unsigned vsCodeLen = effectVsDesc2.BytecodeLength;

	// Create our vertex input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
	UINT numElements = 2;

	V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));
	m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout);

	return S_OK;
}

HRESULT Scene::SetupTextures(int iSizeX, int iSizeY, int iSizeZ)
{
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
	m_pSurface1 = new Surface(m_pd3dDevice, m_pd3dImmediateContext, m_pMainTechnique, m_pMVPVariable);
	m_pSurface1->ReadVectorFile("Media\\surface1.xml");
	V_RETURN(m_pSurface1->InitBuffers());
    
	// Create surface2 and its buffers
	m_pSurface2 = new Surface(m_pd3dDevice, m_pd3dImmediateContext, m_pMainTechnique, m_pMVPVariable);
	m_pSurface2->ReadVectorFile("Media\\surface1.xml");
	m_pSurface2->SetColor(1.0, 1.0, 1.0);
	V_RETURN(m_pSurface2->InitBuffers());
	m_pSurface2->Scale(0.5);

	// Create bounding box
	m_pBoundingBox = new BoundingBox(m_pd3dDevice, m_pd3dImmediateContext, m_pMainTechnique, m_pMVPVariable, m_pSurface1, m_pSurface2);
	V_RETURN(m_pBoundingBox->InitBuffers());

	m_pControlledSurface = m_pSurface1;

	return S_OK;
}

void Scene::Render(D3DXMATRIX mViewProjection)
{
	m_pBoundingBox->UpdateVertexBuffer();
		
	m_pSurface1->Render(mViewProjection);
	m_pSurface2->Render(mViewProjection);

	m_pd3dImmediateContext->RSSetState(m_pRasterizerStateWireframe);
	m_pBoundingBox->Render(mViewProjection);
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