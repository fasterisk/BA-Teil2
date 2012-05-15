#include "Globals.h"
#include "Surface.h"
#include "SDKMesh.h"


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

	m_vColor = D3DXVECTOR3(0.0, 0.0, 0.0);
}


Surface::~Surface()
{
	SAFE_RELEASE(m_pInputLayout);
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

void Surface::SetColor(D3DXVECTOR3 vColor)
{
	m_vColor = vColor;
}

void Surface::SetColor(float fR, float fG, float fB)
{
	m_vColor = D3DXVECTOR3(fR, fG, fB);
}

D3DXVECTOR3 Surface::GetColor()
{
	return m_vColor;
}

HRESULT Surface::LoadMesh(LPWSTR lsFileName)
{
	HRESULT hr(S_OK);
	V_RETURN(m_pSurfaceMesh.Create(m_pd3dDevice, lsFileName, true));
	return hr;
}


HRESULT Surface::Initialize(LPWSTR lsFileName)
{
	HRESULT hr(S_OK);

	V_RETURN(InitializeShader());

	V_RETURN(InitializeMesh(lsFileName));

	return hr;
}

void Surface::Render(D3DXMATRIX mViewProjection)
{
	//Set up global shader variables
    D3DXMATRIX mModelViewProjection = m_mModel * mViewProjection;
	m_pModelViewProjectionVar->SetMatrix(reinterpret_cast<float*>(&mModelViewProjection));

	//Set up vertex and index buffer
    UINT Strides[1];
    UINT Offsets[1];
    ID3D11Buffer* pVB[1];
    pVB[0] = m_pSurfaceMesh.GetVB11( 0, 0 );
    Strides[0] = ( UINT )m_pSurfaceMesh.GetVertexStride( 0, 0 );
    Offsets[0] = 0;
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    m_pd3dImmediateContext->IASetIndexBuffer( m_pSurfaceMesh.GetIB11( 0 ), m_pSurfaceMesh.GetIBFormat11( 0 ), 0 );

    
    //Render
    SDKMESH_SUBSET* pSubset = NULL;
    D3D11_PRIMITIVE_TOPOLOGY PrimType;

	//Set input layout
	m_pd3dImmediateContext->IASetInputLayout( m_pInputLayout );

	//Get technique descriptor for passes
	D3DX11_TECHNIQUE_DESC techDesc;
	m_pTechnique->GetDesc(&techDesc);

	//draw all passes of the technique
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//apply pass
		m_pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);

		for( UINT subset = 0; subset < m_pSurfaceMesh.GetNumSubsets( 0 ); ++subset )
		{
		    // Get the subset
		    pSubset = m_pSurfaceMesh.GetSubset( 0, subset );

	        PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
	        m_pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

	        ID3D11ShaderResourceView* pDiffuseRV = m_pSurfaceMesh.GetMaterial( pSubset->MaterialID )->pDiffuseRV11;
	        m_pd3dImmediateContext->PSSetShaderResources( 0, 1, &pDiffuseRV );
	
	        m_pd3dImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
	    }
	}
}

void Surface::Render(ID3DX11EffectTechnique* pTechnique)
{
	//Set up vertex and index buffer
    UINT Strides[1];
    UINT Offsets[1];
    ID3D11Buffer* pVB[1];
    pVB[0] = m_pSurfaceMesh.GetVB11( 0, 0 );
    Strides[0] = ( UINT )m_pSurfaceMesh.GetVertexStride( 0, 0 );
    Offsets[0] = 0;
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    m_pd3dImmediateContext->IASetIndexBuffer( m_pSurfaceMesh.GetIB11( 0 ), m_pSurfaceMesh.GetIBFormat11( 0 ), 0 );

	 //Render
    SDKMESH_SUBSET* pSubset = NULL;
    D3D11_PRIMITIVE_TOPOLOGY PrimType;

	//Set input layout
	//m_pd3dImmediateContext->IASetInputLayout( m_pInputLayout );

	//Get technique descriptor for passes
	D3DX11_TECHNIQUE_DESC techDesc;
	pTechnique->GetDesc(&techDesc);

	//draw all passes of the technique
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//apply pass
		pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);

		for( UINT subset = 0; subset < m_pSurfaceMesh.GetNumSubsets( 0 ); ++subset )
		{
		    // Get the subset
		    pSubset = m_pSurfaceMesh.GetSubset( 0, subset );

	        PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
	        m_pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

	        //ID3D11ShaderResourceView* pDiffuseRV = m_pSurfaceMesh.GetMaterial( pSubset->MaterialID )->pDiffuseRV11;
	        //m_pd3dImmediateContext->PSSetShaderResources( 0, 1, &pDiffuseRV );
	
	        m_pd3dImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
	    }
	}
}

D3DXVECTOR3 Surface::GetBoundingBoxCenter()
{
	return m_pSurfaceMesh.GetMeshBBoxCenter(0);
}

D3DXVECTOR3 Surface::GetBoundingBoxExtents()
{
	return m_pSurfaceMesh.GetMeshBBoxExtents(0);
}

HRESULT Surface::InitializeShader()
{
	HRESULT hr(S_OK);

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

   return hr;
}

HRESULT Surface::InitializeMesh(LPWSTR lsFileName)
{
	HRESULT hr(S_OK);

	// Load the mesh
    V_RETURN( m_pSurfaceMesh.Create( m_pd3dDevice, lsFileName, true ) );

	return hr;
}

