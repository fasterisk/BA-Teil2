#include "Globals.h"
#include "Surface.h"
#include "SDKMesh.h"
#include <assimp.hpp>
#include <aiScene.h>
#include <aiPostProcess.h>
#include <FreeImage.h>

Surface::Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pSurfaceEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pSurfaceEffect = pSurfaceEffect;

	m_pTriangleVertexBuffer = NULL;
	m_pTriangleIndexBuffer = NULL;
	m_pEdgeIndexBuffer = NULL;
	m_pVertices = NULL;

	m_pDiffuseTexture = NULL;
	m_pDiffuseTextureSRV = NULL;

	D3DXMatrixIdentity(&m_mModel);
	D3DXMatrixIdentity(&m_mRot);
	D3DXMatrixIdentity(&m_mTrans);
	D3DXMatrixIdentity(&m_mTransInv);

	m_translation = D3DXVECTOR3(0.0, 0.0, 0.0);

	m_vColor = D3DXVECTOR3(0.0, 0.0, 0.0);

	m_fIsoColor = 1.0f;

	FreeImage_Initialise();
}


Surface::~Surface()
{
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pTriangleVertexBuffer);
	SAFE_RELEASE(m_pTriangleIndexBuffer);
	SAFE_RELEASE(m_pEdgeIndexBuffer);

	SAFE_RELEASE(m_pDiffuseTexture);
	SAFE_RELEASE(m_pDiffuseTextureSRV);
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

void Surface::SetIsoColor(float fIsoColor)
{
	m_fIsoColor = fIsoColor;
}

float Surface::GetIsoColor()
{
	return m_fIsoColor;
}

HRESULT Surface::LoadMesh(std::string strMeshName)
{
	HRESULT hr(S_OK);
	
	Assimp::Importer Importer;

	const aiScene* pScene = Importer.ReadFile(strMeshName.c_str(), aiProcess_Triangulate |
																	aiProcess_JoinIdenticalVertices |
																	aiProcess_GenSmoothNormals
																	);

	//assert(pScene);

	if(pScene == NULL)
	{
		std::string errorstring = Importer.GetErrorString();

		MessageBox ( NULL , L"Mesh type is not supported!", ConvertMultibyteToWideChar(strMeshName).c_str(), MB_OK);
		return S_OK;
	}

	D3DXMatrixIdentity(&m_mModel);

	SAFE_RELEASE(m_pTriangleVertexBuffer);
	SAFE_RELEASE(m_pTriangleIndexBuffer);
	SAFE_RELEASE(m_pEdgeIndexBuffer);
	SAFE_DELETE(m_pVertices);

	// Get vertex and index count of the whole mesh
	unsigned int mNumVertices = 0;
	unsigned int mNumIndices = 0;
	for(unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		mNumVertices += pScene->mMeshes[i]->mNumVertices;
		mNumIndices += pScene->mMeshes[i]->mNumFaces*3;
	}

	// Create vertex and index array
	m_pVertices = new VERTEX[mNumVertices];
	unsigned int* pTriangleIndices = new unsigned int[mNumIndices];
	unsigned int* pEdgeIndices = new unsigned int[mNumIndices*2];
	unsigned int mCurrentVertex = 0;
	unsigned int mCurrentIndex = 0;
	float fMaxVertexValue = 0;

	bool bLoadTexture = false;

	for(unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		const aiMesh* paiMesh = pScene->mMeshes[i];
		for(unsigned int j = 0; j < paiMesh->mNumVertices; j++)
		{
			const aiVector3D* pPos = &(paiMesh->mVertices[j]);
			const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
			const aiVector3D* pTexcoord = &(paiMesh->mTextureCoords[0][j]);
			

			VERTEX vertex;
			vertex.pos = D3DXVECTOR3(pPos->x, pPos->y, pPos->z);
			vertex.normal = D3DXVECTOR3(pNormal->x, pNormal->y, pNormal->z);
			vertex.texcoord = D3DXVECTOR2(pTexcoord->x, pTexcoord->y);

			//get maximum vertex value to scale the model inside the window
			if(abs(vertex.pos.x) > fMaxVertexValue)
				fMaxVertexValue = abs(vertex.pos.x);
			if(abs(vertex.pos.y) > fMaxVertexValue)
				fMaxVertexValue = abs(vertex.pos.y);
			if(abs(vertex.pos.z) > fMaxVertexValue)
				fMaxVertexValue = abs(vertex.pos.z);
			
			m_pVertices[mCurrentVertex] = vertex;
			mCurrentVertex++;
		}

		for(unsigned int j = 0; j < paiMesh->mNumFaces; j++)
		{
			const aiFace& face = paiMesh->mFaces[j];
			assert(face.mNumIndices == 3);
			pTriangleIndices[mCurrentIndex] = face.mIndices[0];
			pTriangleIndices[mCurrentIndex+1] = face.mIndices[1];
			pTriangleIndices[mCurrentIndex+2] = face.mIndices[2];

			pEdgeIndices[mCurrentIndex*2] = face.mIndices[0];
			pEdgeIndices[mCurrentIndex*2+1] = face.mIndices[1];
			pEdgeIndices[mCurrentIndex*2+2] = face.mIndices[1];
			pEdgeIndices[mCurrentIndex*2+3] = face.mIndices[2];
			pEdgeIndices[mCurrentIndex*2+4] = face.mIndices[2];
			pEdgeIndices[mCurrentIndex*2+5] = face.mIndices[0];

			mCurrentIndex += 3;
		}
	}

	//Scale the model
	Scale(1/fMaxVertexValue * 0.5f);

	m_mNumVertices = mNumVertices;
	m_mNumIndices = mNumIndices;

	//Create vertex buffer
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = mNumVertices*sizeof(VERTEX);
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vbInitialData;
	vbInitialData.pSysMem = m_pVertices;
	vbInitialData.SysMemPitch = 0;
	vbInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&vbDesc, &vbInitialData, &m_pTriangleVertexBuffer);

	//Create triangle index buffer
	D3D11_BUFFER_DESC ibtDesc;
	ibtDesc.ByteWidth = mNumIndices*sizeof(unsigned int);
	ibtDesc.Usage = D3D11_USAGE_DEFAULT;
	ibtDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibtDesc.CPUAccessFlags = 0;
	ibtDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA ibtInitialData;
	ibtInitialData.pSysMem = pTriangleIndices;
	ibtInitialData.SysMemPitch = 0;
	ibtInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&ibtDesc, &ibtInitialData, &m_pTriangleIndexBuffer);

	//Create edge index buffer
	D3D11_BUFFER_DESC ibeDesc;
	ibeDesc.ByteWidth = mNumIndices*2*sizeof(unsigned int);
	ibeDesc.Usage = D3D11_USAGE_DEFAULT;
	ibeDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibeDesc.CPUAccessFlags = 0;
	ibeDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA ibeInitialData;
	ibeInitialData.pSysMem = pEdgeIndices;
	ibeInitialData.SysMemPitch = 0;
	ibeInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&ibeDesc, &ibeInitialData, &m_pEdgeIndexBuffer);

	MessageBox ( NULL , L"Mesh successfully loaded. Please choose a texture file in the next dialog!", ConvertMultibyteToWideChar(strMeshName).c_str(), MB_OK);

	OPENFILENAME ofnTexture;
	std::string strTextureName;
	WCHAR sz[300];

	//Open file dialog
	ZeroMemory(&ofnTexture, sizeof(ofnTexture));
	ofnTexture.lStructSize = sizeof (ofnTexture);
	ofnTexture.hwndOwner = NULL;
	ofnTexture.lpstrFile = sz;
	ofnTexture.lpstrFile[0] = '\0';
	ofnTexture.nMaxFile = sizeof(sz);
	ofnTexture.lpstrFilter = L"All\0*.*\0";
	ofnTexture.nFilterIndex =1;
	ofnTexture.lpstrFileTitle = NULL ;
	ofnTexture.nMaxFileTitle = 0 ;
	ofnTexture.lpstrInitialDir=NULL ;
	ofnTexture.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
	GetOpenFileName( &ofnTexture );
	strTextureName = ConvertWideCharToChar(ofnTexture.lpstrFile);
					
	//Get the image file type
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(strTextureName.c_str());

	//load the image file
	FIBITMAP *texture = FreeImage_Load(fif, strTextureName.c_str());
	
	if(texture != NULL)
	{
		unsigned char* bits = (unsigned char*)FreeImage_GetBits(texture);
		
		unsigned int nBPP = FreeImage_GetBPP(texture);
		unsigned int nWidth = FreeImage_GetWidth(texture);
		unsigned int nHeight = FreeImage_GetHeight(texture);
		unsigned int nPitch = FreeImage_GetPitch(texture);
	
		unsigned char* texturedata = new unsigned char[nWidth*nHeight*4];
	
		int offset = 0;
		int offset_img = 0;

		for(unsigned int y = 0; y < nHeight; y++)
		{
			for(unsigned int x = 0; x < nWidth; x++)
			{
				texturedata[offset+2] = ((unsigned char*)bits)[offset_img+0];
				texturedata[offset+1] = ((unsigned char*)bits)[offset_img+1];
				texturedata[offset+0] = ((unsigned char*)bits)[offset_img+2];
				texturedata[offset+3] = ((unsigned char*)bits)[offset_img+3];
				offset += 4;
				offset_img += 3;
			}
			offset_img = y * nPitch;
		}
	
		SAFE_RELEASE(m_pDiffuseTexture);
		SAFE_RELEASE(m_pDiffuseTextureSRV);

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MipLevels = 1;
		texDesc.MiscFlags = 0;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.Width = FreeImage_GetWidth(texture);
		texDesc.Height = FreeImage_GetHeight(texture);
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.ArraySize = 1;
		D3D11_SUBRESOURCE_DATA texData;
		texData.pSysMem = texturedata;
		texData.SysMemPitch = nWidth*4;
		texData.SysMemSlicePitch = 0;
		V_RETURN(m_pd3dDevice->CreateTexture2D(&texDesc, &texData, &m_pDiffuseTexture));
		DXUT_SetDebugName( m_pDiffuseTexture, strTextureName.c_str());
	
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pDiffuseTexture, &srvDesc, &m_pDiffuseTextureSRV));	
	}

	FreeImage_Unload(texture);

	return hr;
}

HRESULT Surface::LoadMesh(std::string strMeshName, std::string strTextureName)
{
	HRESULT hr(S_OK);
	
	//V_RETURN(m_pSurfaceMesh.Create(m_pd3dDevice, lsFileName, true));
	D3DXMatrixIdentity(&m_mModel);

	Assimp::Importer Importer;

	const aiScene* pScene = Importer.ReadFile(strMeshName.c_str(), aiProcess_Triangulate |
																	aiProcess_JoinIdenticalVertices |
																	aiProcess_GenSmoothNormals
																	);

	assert(pScene);

	//std::string errorstring = Importer.GetErrorString();

	SAFE_RELEASE(m_pTriangleVertexBuffer);
	SAFE_RELEASE(m_pTriangleIndexBuffer);
	SAFE_DELETE(m_pVertices);

	// Get vertex and index count of the whole mesh
	unsigned int mNumVertices = 0;
	unsigned int mNumIndices = 0;
	for(unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		mNumVertices += pScene->mMeshes[i]->mNumVertices;
		mNumIndices += pScene->mMeshes[i]->mNumFaces*3;
	}

	// Create vertex and index array
	m_pVertices = new VERTEX[mNumVertices];
	unsigned int* pTriangleIndices = new unsigned int[mNumIndices];
	unsigned int* pEdgeIndices = new unsigned int[mNumIndices*2];
	unsigned int mCurrentVertex = 0;
	unsigned int mCurrentIndex = 0;
	float fMaxVertexValue = 0;

	bool bLoadTexture = false;

	for(unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		const aiMesh* paiMesh = pScene->mMeshes[i];
		for(unsigned int j = 0; j < paiMesh->mNumVertices; j++)
		{
			const aiVector3D* pPos = &(paiMesh->mVertices[j]);
			const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
			const aiVector3D* pTexcoord = &(paiMesh->mTextureCoords[0][j]);
			

			VERTEX vertex;
			vertex.pos = D3DXVECTOR3(pPos->x, pPos->y, pPos->z);
			vertex.normal = D3DXVECTOR3(pNormal->x, pNormal->y, pNormal->z);
			vertex.texcoord = D3DXVECTOR2(pTexcoord->x, pTexcoord->y);

			//get maximum vertex value to scale the model inside the window
			if(abs(vertex.pos.x) > fMaxVertexValue)
				fMaxVertexValue = abs(vertex.pos.x);
			if(abs(vertex.pos.y) > fMaxVertexValue)
				fMaxVertexValue = abs(vertex.pos.y);
			if(abs(vertex.pos.z) > fMaxVertexValue)
				fMaxVertexValue = abs(vertex.pos.z);
			
			m_pVertices[mCurrentVertex] = vertex;
			mCurrentVertex++;
		}

		for(unsigned int j = 0; j < paiMesh->mNumFaces; j++)
		{
			const aiFace& face = paiMesh->mFaces[j];
			assert(face.mNumIndices == 3);
			pTriangleIndices[mCurrentIndex] = face.mIndices[0];
			pTriangleIndices[mCurrentIndex+1] = face.mIndices[1];
			pTriangleIndices[mCurrentIndex+2] = face.mIndices[2];

			pEdgeIndices[mCurrentIndex*2] = face.mIndices[0];
			pEdgeIndices[mCurrentIndex*2+1] = face.mIndices[1];
			pEdgeIndices[mCurrentIndex*2+2] = face.mIndices[1];
			pEdgeIndices[mCurrentIndex*2+3] = face.mIndices[2];
			pEdgeIndices[mCurrentIndex*2+4] = face.mIndices[2];
			pEdgeIndices[mCurrentIndex*2+5] = face.mIndices[0];

			mCurrentIndex += 3;
		}
	}

	//Scale the model
	Scale(1/fMaxVertexValue * 0.5f);

	m_mNumVertices = mNumVertices;
	m_mNumIndices = mNumIndices;

	//Create vertex buffer
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = mNumVertices*sizeof(VERTEX);
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vbInitialData;
	vbInitialData.pSysMem = m_pVertices;
	vbInitialData.SysMemPitch = 0;
	vbInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&vbDesc, &vbInitialData, &m_pTriangleVertexBuffer);

	//Create triangle index buffer
	D3D11_BUFFER_DESC ibtDesc;
	ibtDesc.ByteWidth = mNumIndices*sizeof(unsigned int);
	ibtDesc.Usage = D3D11_USAGE_DEFAULT;
	ibtDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibtDesc.CPUAccessFlags = 0;
	ibtDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA ibtInitialData;
	ibtInitialData.pSysMem = pTriangleIndices;
	ibtInitialData.SysMemPitch = 0;
	ibtInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&ibtDesc, &ibtInitialData, &m_pTriangleIndexBuffer);

	//Create edge index buffer
	D3D11_BUFFER_DESC ibeDesc;
	ibeDesc.ByteWidth = mNumIndices*2*sizeof(unsigned int);
	ibeDesc.Usage = D3D11_USAGE_DEFAULT;
	ibeDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibeDesc.CPUAccessFlags = 0;
	ibeDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA ibeInitialData;
	ibeInitialData.pSysMem = pEdgeIndices;
	ibeInitialData.SysMemPitch = 0;
	ibeInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&ibeDesc, &ibeInitialData, &m_pEdgeIndexBuffer);


	//Get the image file type
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(strTextureName.c_str());
	
	//load the image file
	FIBITMAP *texture = FreeImage_Load(fif, strTextureName.c_str());
	
	if(texture != NULL)
	{
		unsigned char* bits = (unsigned char*)FreeImage_GetBits(texture);
		unsigned int nBPP = FreeImage_GetBPP(texture);
		unsigned int nWidth = FreeImage_GetWidth(texture);
		unsigned int nHeight = FreeImage_GetHeight(texture);
		unsigned int nPitch = FreeImage_GetPitch(texture);
	
		unsigned char* texturedata = new unsigned char[nWidth*nHeight*4];
	
		int offset = 0;
		int offset_img = 0;
		
		for(unsigned int y = 0; y < nHeight; y++)
		{
			for(unsigned int x = 0; x < nWidth; x++)
			{
				texturedata[offset+2] = ((unsigned char*)bits)[offset_img+0];
				texturedata[offset+1] = ((unsigned char*)bits)[offset_img+1];
				texturedata[offset+0] = ((unsigned char*)bits)[offset_img+2];
				texturedata[offset+3] = ((unsigned char*)bits)[offset_img+3];
				offset += 4;
				offset_img += 3;
			}
			offset_img = y * nPitch;
		}
	
		SAFE_RELEASE(m_pDiffuseTexture);
		SAFE_RELEASE(m_pDiffuseTextureSRV);

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MipLevels = 1;
		texDesc.MiscFlags = 0;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.Width = FreeImage_GetWidth(texture);
		texDesc.Height = FreeImage_GetHeight(texture);
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.ArraySize = 1;
		D3D11_SUBRESOURCE_DATA texData;
		texData.pSysMem = texturedata;
		texData.SysMemPitch = nWidth*4;
		texData.SysMemSlicePitch = 0;
		V_RETURN(m_pd3dDevice->CreateTexture2D(&texDesc, &texData, &m_pDiffuseTexture));
		DXUT_SetDebugName( m_pDiffuseTexture, strTextureName.c_str());
	
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pDiffuseTexture, &srvDesc, &m_pDiffuseTextureSRV));
	}

	FreeImage_Unload(texture);

	return hr;
}


HRESULT Surface::Initialize(std::string strMeshName, std::string strTextureName)
{
	HRESULT hr(S_OK);

	V_RETURN(InitializeShader());

	V_RETURN(LoadMesh(strMeshName, strTextureName));

	return hr;
}

void Surface::Render(D3DXMATRIX mViewProjection)
{
	//Set up global shader variables
    D3DXMATRIX mModelViewProjection = m_mModel * mViewProjection;
	m_pModelViewProjectionVar->SetMatrix(reinterpret_cast<float*>(&mModelViewProjection));

	//Set up vertex and index buffer
	UINT strides = sizeof(VERTEX);
	UINT offsets = 0;
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pTriangleVertexBuffer, &strides, &offsets );
	m_pd3dImmediateContext->IASetIndexBuffer( m_pTriangleIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );
    
	//Set input layout
	m_pd3dImmediateContext->IASetInputLayout( m_pInputLayout );

	//Get technique descriptor for passes
	D3DX11_TECHNIQUE_DESC techDesc;
	m_pTechnique->GetDesc(&techDesc);

	//draw all passes of the technique
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_pSurfaceTextureVar->SetResource(m_pDiffuseTextureSRV);

		m_pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);

		m_pd3dImmediateContext->DrawIndexed(m_mNumIndices, 0, 0);
	}
}

void Surface::RenderVoronoi(ID3DX11EffectTechnique* pVoronoiTechnique, ID3DX11EffectShaderResourceVariable *pSurfaceTextureVar)
{
	//Set up vertex and index buffer
	UINT strides = sizeof(VERTEX);
	UINT offsets = 0;
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pTriangleVertexBuffer, &strides, &offsets );
	m_pd3dImmediateContext->IASetIndexBuffer( m_pTriangleIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

	//Set input layout
	m_pd3dImmediateContext->IASetInputLayout( m_pInputLayout );

	//draw all passes of the technique
	
	
	pSurfaceTextureVar->SetResource(m_pDiffuseTextureSRV);

	//TRIANGLE
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pd3dImmediateContext->IASetIndexBuffer( m_pTriangleIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

	//apply triangle pass
	pVoronoiTechnique->GetPassByName("Triangle")->Apply( 0, m_pd3dImmediateContext);
	
	m_pd3dImmediateContext->DrawIndexed(m_mNumIndices, 0, 0);

	//EDGE
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	m_pd3dImmediateContext->IASetIndexBuffer( m_pEdgeIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

	//apply edge pass
	pVoronoiTechnique->GetPassByName("Edge")->Apply( 0, m_pd3dImmediateContext);
	
	m_pd3dImmediateContext->DrawIndexed(m_mNumIndices*2, 0, 0);

	//POINT
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	//apply point pass
	pVoronoiTechnique->GetPassByName("Point")->Apply( 0, m_pd3dImmediateContext);
	
	m_pd3dImmediateContext->Draw(m_mNumVertices, 0);
}

BOUNDINGBOX Surface::GetBoundingBox()
{
	BOUNDINGBOX bbFinal;
	D3DXVECTOR4 vCurrent;
	
	D3DXVec3Transform(&bbFinal.vMin, &m_pVertices[0].pos, &m_mModel);
	D3DXVec3Transform(&bbFinal.vMax, &m_pVertices[0].pos, &m_mModel);

	for(int i = 1; i < m_mNumVertices; i++)
	{
		D3DXVec3Transform(&vCurrent, &m_pVertices[i].pos, &m_mModel);
		if(vCurrent.x < bbFinal.vMin.x)
			bbFinal.vMin.x = vCurrent.x;
		if(vCurrent.y < bbFinal.vMin.y)
			bbFinal.vMin.y = vCurrent.y;
		if(vCurrent.z < bbFinal.vMin.z)
			bbFinal.vMin.z = vCurrent.z;
		if(vCurrent.x > bbFinal.vMax.x)
			bbFinal.vMax.x = vCurrent.x;
		if(vCurrent.y > bbFinal.vMax.y)
			bbFinal.vMax.y = vCurrent.y;
		if(vCurrent.z > bbFinal.vMax.z)
			bbFinal.vMax.z = vCurrent.z;
	}
	return bbFinal;
}

HRESULT Surface::InitializeShader()
{
	HRESULT hr(S_OK);

	m_pTechnique = m_pSurfaceEffect->GetTechniqueByName("RenderColor");
	m_pModelViewProjectionVar = m_pSurfaceEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();
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
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

   V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));

   return hr;
}

