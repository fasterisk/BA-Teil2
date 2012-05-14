#include "Globals.h"
#include "Surface.h"
#include "SDKMesh.h"


//TEST
struct CB_VS_PER_OBJECT
{
    D3DXMATRIX m_WorldViewProj;
    D3DXMATRIX m_World;
};
struct CB_PS_PER_OBJECT
{
    D3DXVECTOR4 m_vObjectColor;
};
//TEST END

Surface::Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pSurfaceEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pSurfaceEffect = pSurfaceEffect;

	m_pTriangleVertexBuffer = NULL;
	m_pEdgeVertexBuffer = NULL;
	m_pIndexBuffer = NULL;

	D3DXMatrixIdentity(&m_mModel);
	D3DXMatrixIdentity(&m_mRot);
	D3DXMatrixIdentity(&m_mTrans);
	D3DXMatrixIdentity(&m_mTransInv);

	m_translation = D3DXVECTOR3(0.0, 0.0, 0.0);

	//TEST
	g_pVertexLayout11 = NULL;
	g_pVertexBuffer = NULL;
	g_pIndexBuffer = NULL;
	g_pVertexShader = NULL;
	g_pPixelShader = NULL;
	g_pSamLinear = NULL;
	g_iCBVSPerObjectBind = 0;
	g_iCBPSPerObjectBind = 0;
	g_pcbVSPerObject = NULL;
	g_pcbPSPerObject = NULL;
}


Surface::~Surface()
{
	SAFE_DELETE(m_pTriangleVertices);
	SAFE_DELETE(m_pEdgeVertices);

	SAFE_RELEASE(m_pInputLayout);

	SAFE_RELEASE(m_pTriangleVertexBuffer);
	SAFE_RELEASE(m_pEdgeVertexBuffer);
}

void Surface::Translate(float fX, float fY, float fZ)
{
	m_translation.x += fX;
	m_translation.y += fY;
	m_translation.z += fZ;

	D3DXMATRIX mTrans;
	D3DXMatrixTranslation(&mTrans, fX, fY, fZ);
	D3DXMatrixTranslation(&m_mTrans, m_translation.x, m_translation.y, m_translation.z);
	D3DXMatrixTranslation(&m_mTransInv, -m_translation.x, -m_translation.y, -m_translation.z);

	m_mModel *= mTrans;
}

void Surface::Rotate(D3DXVECTOR3 axis, float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationAxis(&mRot, &axis, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}

void Surface::RotateX(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationX(&mRot, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}
void Surface::RotateY(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationY(&mRot, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}
void Surface::RotateZ(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationZ(&mRot, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}

void Surface::Scale(float fFactor)
{
	D3DXMATRIX mScale;
	D3DXMatrixScaling(&mScale, fFactor, fFactor, fFactor);

	m_mModel *= mScale;
}

void Surface::SetColor(float fR, float fG, float fB)
{
	for(int i = 0; i < m_iNumTriangleVertices; i++)
	{
		m_pTriangleVertices[i].color = D3DXCOLOR(fR, fG, fB, 1.0);
	}
	for(int i=0; i < m_iNumEdgeVertices; i++)
	{
		m_pEdgeVertices[i].color = D3DXCOLOR(fR, fG, fB, 1.0);
	}

}

HRESULT Surface::LoadMesh(LPWSTR lsFileName)
{
	HRESULT hr(S_OK);
	V_RETURN(m_pSurfaceMesh.Create(m_pd3dDevice, lsFileName, true));
	return hr;
}


// TEST TEST 
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
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

HRESULT Surface::Initialize(char* s)
{
	/*HRESULT hr;

	//ReadVectorFile(s);

	V_RETURN(m_pSurfaceMesh.Create(m_pd3dDevice, L"Media\\meshes\\tiny.sdkmesh", true));

	m_pTriangleVertexBuffer = m_pSurfaceMesh.GetVB11(0,0);
	m_pIndexBuffer = m_pSurfaceMesh.GetIB11(0);

	m_pTechnique = m_pSurfaceEffect->GetTechniqueByName("RenderColor");
	m_pModelViewProjectionVar = m_pSurfaceEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();
	m_pNormalMatrixVar = m_pSurfaceEffect->GetVariableByName("NormalMatrix")->AsMatrix();
	m_pSurfaceTextureVar = m_pSurfaceEffect->GetVariableByName("SurfaceTexture")->AsShaderResource();

	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

	V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));
	

	return S_OK;*/

	 HRESULT hr;

	 m_pTechnique = m_pSurfaceEffect->GetTechniqueByName("RenderColor");
	m_pModelViewProjectionVar = m_pSurfaceEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();
	m_pNormalMatrixVar = m_pSurfaceEffect->GetVariableByName("NormalMatrix")->AsMatrix();
	m_pSurfaceTextureVar = m_pSurfaceEffect->GetVariableByName("SurfaceTexture")->AsShaderResource();

	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

   V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));


    // Load the mesh
    V_RETURN( g_Mesh11.Create( m_pd3dDevice, L"Media\\meshes\\tiny.sdkmesh", true ) );

}

void Surface::Render(D3DXMATRIX mViewProjection)
{
	/*D3DXMATRIX mModelViewProjection = m_mModel * mViewProjection;
	
	m_pModelViewProjectionVar->SetMatrix(reinterpret_cast<float*>(&mModelViewProjection));

	UINT stride = (UINT)m_pSurfaceMesh.GetVertexStride(0,0);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout);
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pTriangleVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer, m_pSurfaceMesh.GetIBFormat11(0), 0);
	
	SDKMESH_SUBSET* pSubset = NULL;
	D3D11_PRIMITIVE_TOPOLOGY primType;

	for(UINT subset = 0; subset < m_pSurfaceMesh.GetNumSubsets(0); ++subset)
	{
		// Get the subset
		pSubset = m_pSurfaceMesh.GetSubset(0, subset);

		primType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
		m_pd3dImmediateContext->IASetPrimitiveTopology( primType );

		ID3D11ShaderResourceView* pDiffuseRV = m_pSurfaceMesh.GetMaterial( pSubset->MaterialID )->pDiffuseRV11;
		m_pSurfaceTextureVar->SetResource(pDiffuseRV);

        m_pd3dImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
	}*/


    D3DXMATRIX mModelViewProjection = m_mModel * mViewProjection;
	m_pModelViewProjectionVar->SetMatrix(reinterpret_cast<float*>(&mModelViewProjection));

    //Get the mesh
    //IA setup
    
    UINT Strides[1];
    UINT Offsets[1];
    ID3D11Buffer* pVB[1];
    pVB[0] = g_Mesh11.GetVB11( 0, 0 );
    Strides[0] = ( UINT )g_Mesh11.GetVertexStride( 0, 0 );
    Offsets[0] = 0;
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    m_pd3dImmediateContext->IASetIndexBuffer( g_Mesh11.GetIB11( 0 ), g_Mesh11.GetIBFormat11( 0 ), 0 );

    
    //Render
    SDKMESH_SUBSET* pSubset = NULL;
    D3D11_PRIMITIVE_TOPOLOGY PrimType;

	m_pd3dImmediateContext->IASetInputLayout( m_pInputLayout );

	D3DX11_TECHNIQUE_DESC techDesc;
	m_pTechnique->GetDesc(&techDesc);

	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//apply technique
		m_pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);

		for( UINT subset = 0; subset < g_Mesh11.GetNumSubsets( 0 ); ++subset )
		{
		    // Get the subset
		    pSubset = g_Mesh11.GetSubset( 0, subset );

	        PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
	        m_pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

	        // TODO: D3D11 - material loading
	        ID3D11ShaderResourceView* pDiffuseRV = g_Mesh11.GetMaterial( pSubset->MaterialID )->pDiffuseRV11;
	        m_pd3dImmediateContext->PSSetShaderResources( 0, 1, &pDiffuseRV );
	
	        m_pd3dImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
	    }
	}

   
	
}

void Surface::Render(ID3DX11EffectTechnique* pTechnique)
{
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pTriangleVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DX11_TECHNIQUE_DESC techDesc;
	pTechnique->GetDesc(&techDesc);

	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//apply technique
		pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);

		//draw
		m_pd3dImmediateContext->Draw(m_iNumTriangleVertices, 0);
	}
}

void Surface::RenderVoronoi(ID3DX11EffectTechnique* pTechnique)
{
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;

	//apply triangle technique & draw
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pTriangleVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pTechnique->GetPassByName("Triangle")->Apply( 0, m_pd3dImmediateContext);
	m_pd3dImmediateContext->Draw(m_iNumTriangleVertices, 0);
	
	//apply edge technique & draw
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pEdgeVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	pTechnique->GetPassByName("Edge")->Apply( 0, m_pd3dImmediateContext);
	m_pd3dImmediateContext->Draw(m_iNumEdgeVertices, 0);
	
	//apply point technique & draw
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pTriangleVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	pTechnique->GetPassByName("Point")->Apply( 0, m_pd3dImmediateContext);
	m_pd3dImmediateContext->Draw(m_iNumTriangleVertices, 0);
}

