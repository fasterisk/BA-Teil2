#include "Globals.h"
#include "Surface.h"
#include "SDKMesh.h"
#include <assimp.hpp>
#include <aiScene.h>
#include <aiPostProcess.h>


Surface::Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pSurfaceEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pSurfaceEffect = pSurfaceEffect;

	m_pVertexBuffer = NULL;
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
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
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



	//convert LPCWSTR to std::string
	std::string strFileName = ConvertWideCharToChar(lsFileName);

	//V_RETURN(m_pSurfaceMesh.Create(m_pd3dDevice, lsFileName, true));
	D3DXMatrixIdentity(&m_mModel);

	Assimp::Importer Importer;

	const aiScene* pScene = Importer.ReadFile(strFileName.c_str(), aiProcess_Triangulate |
															aiProcess_GenSmoothNormals | 
															aiProcess_FlipUVs);

	assert(pScene);

	//std::string errorstring = Importer.GetErrorString();

	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);

	// Get vertex and index count of the whole mesh
	unsigned int mNumVertices = 0;
	unsigned int mNumIndices = 0;
	for(unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		mNumVertices += pScene->mMeshes[i]->mNumVertices;
		mNumIndices += pScene->mMeshes[i]->mNumFaces*3;
	}

	// Create vertex and index array
	VERTEX* pVertices = new VERTEX[mNumVertices];
	unsigned int* pIndices = new unsigned int[mNumIndices];
	unsigned int mCurrentVertex = 0;
	unsigned int mCurrentIndex = 0;

	for(unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		const aiMesh* paiMesh = pScene->mMeshes[i];
		for(unsigned int j = 0; j < paiMesh->mNumVertices; j++)
		{
			const aiVector3D* pPos = &(paiMesh->mVertices[i]);
			const aiVector3D* pNormal = &(paiMesh->mNormals[i]);
			const aiVector3D* pTexcoord = &(paiMesh->mTextureCoords[0][i]);

			VERTEX vertex;
			vertex.pos = D3DXVECTOR3(pPos->x, pPos->y, pPos->z);
			vertex.normal = D3DXVECTOR3(pNormal->x, pNormal->y, pNormal->z);
			vertex.texcoord = D3DXVECTOR2(pTexcoord->x, pTexcoord->y);
			pVertices[mCurrentVertex] = vertex;
			mCurrentVertex++;
		}

		for(unsigned int j = 0; j < paiMesh->mNumFaces; j++)
		{
			const aiFace& face = paiMesh->mFaces[j];
			assert(face.mNumIndices == 3);
			pIndices[mCurrentIndex] = face.mIndices[0];
			pIndices[mCurrentIndex+1] = face.mIndices[1];
			pIndices[mCurrentIndex+2] = face.mIndices[2];
			mCurrentIndex += 3;
		}
	}

	//Create vertex buffer
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = mNumVertices*sizeof(VERTEX);
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vbInitialData;
	vbInitialData.pSysMem = pVertices;
	vbInitialData.SysMemPitch = 0;
	vbInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&vbDesc, &vbInitialData, &m_pVertexBuffer);

	//Create index buffer
	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = mNumIndices*sizeof(unsigned int);
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA ibInitialData;
	ibInitialData.pSysMem = pIndices;
	ibInitialData.SysMemPitch = 0;
	ibInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&ibDesc, &ibInitialData, &m_pIndexBuffer);





	return hr;
}


HRESULT Surface::Initialize(LPWSTR lsFileName)
{
	HRESULT hr(S_OK);

	V_RETURN(InitializeShader());

	V_RETURN(LoadMesh(lsFileName));

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

		for( UINT subset = 0; subset < m_pSurfaceMesh.GetNumSubsets( 0 ); ++subset )
		{
		    // Get the subset
		    pSubset = m_pSurfaceMesh.GetSubset( 0, subset );

	        PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
	        m_pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

	        ID3D11ShaderResourceView* pDiffuseRV = m_pSurfaceMesh.GetMaterial( pSubset->MaterialID )->pDiffuseRV11;
			m_pSurfaceTextureVar->SetResource(pDiffuseRV);

			//apply pass
			m_pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);
	
	        m_pd3dImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
	    }
	}
}

void Surface::RenderVoronoi(ID3DX11EffectTechnique* pVoronoiTechnique, ID3DX11EffectShaderResourceVariable *pSurfaceTextureVar)
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

	//Get technique descriptor for passes
	D3DX11_TECHNIQUE_DESC techDesc;
	pVoronoiTechnique->GetDesc(&techDesc);

	//draw all passes of the technique
	
	//TRIANGLE

	for( UINT subset = 0; subset < m_pSurfaceMesh.GetNumSubsets( 0 ); ++subset )
	{
	    // Get the subset
	    pSubset = m_pSurfaceMesh.GetSubset( 0, subset );
	
		PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
	    m_pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

	    ID3D11ShaderResourceView* pDiffuseRV = m_pSurfaceMesh.GetMaterial( pSubset->MaterialID )->pDiffuseRV11;
		pSurfaceTextureVar->SetResource(pDiffuseRV);

		//apply triangle pass
		pVoronoiTechnique->GetPassByName("Triangle")->Apply( 0, m_pd3dImmediateContext);
	
	    m_pd3dImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
	}

	//EDGE

	for( UINT subset = 0; subset < m_pSurfaceMesh.GetNumSubsets( 0 ); ++subset )
	{
	    // Get the subset
	    pSubset = m_pSurfaceMesh.GetSubset( 0, subset );
	
		PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
	    m_pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

	    ID3D11ShaderResourceView* pDiffuseRV = m_pSurfaceMesh.GetMaterial( pSubset->MaterialID )->pDiffuseRV11;
		pSurfaceTextureVar->SetResource(pDiffuseRV);

		//apply edge pass
		pVoronoiTechnique->GetPassByName("Edge")->Apply( 0, m_pd3dImmediateContext);
	
		m_pd3dImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
	}

	
	//POINT

	for( UINT subset = 0; subset < m_pSurfaceMesh.GetNumSubsets( 0 ); ++subset )
	{
	    // Get the subset
	    pSubset = m_pSurfaceMesh.GetSubset( 0, subset );
	
		PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
	    m_pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

	    ID3D11ShaderResourceView* pDiffuseRV = m_pSurfaceMesh.GetMaterial( pSubset->MaterialID )->pDiffuseRV11;
		pSurfaceTextureVar->SetResource(pDiffuseRV);

		//apply point pass
		pVoronoiTechnique->GetPassByName("Point")->Apply( 0, m_pd3dImmediateContext);
	
		m_pd3dImmediateContext->Draw( ( UINT )pSubset->VertexCount, ( UINT )pSubset->VertexStart );
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

