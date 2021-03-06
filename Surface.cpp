#include "Globals.h"
#include "Surface.h"
#include "SDKMesh.h"
#include <assimp.hpp>
#include <aiScene.h>
#include <aiPostProcess.h>
#include <FreeImage.h>


/****************************************************************************
 ****************************************************************************/
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

	m_vTranslation = D3DXVECTOR3(0.0, 0.0, 0.0);

	m_fIsoColor = 1.0f;

	m_bIsTextured = false;
	m_bHasTextureCoords = false;

	FreeImage_Initialise();
}

/****************************************************************************
 ****************************************************************************/
Surface::~Surface()
{
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pTriangleVertexBuffer);
	SAFE_RELEASE(m_pTriangleIndexBuffer);
	SAFE_RELEASE(m_pEdgeIndexBuffer);

	SAFE_RELEASE(m_pDiffuseTexture);
	SAFE_RELEASE(m_pDiffuseTextureSRV);
}

/****************************************************************************
 ****************************************************************************/
void Surface::Translate(float fX, float fY, float fZ)
{
	m_vTranslation.x += fX;
	m_vTranslation.y += fY;
	m_vTranslation.z += fZ;

	D3DXMATRIX mTrans;
	D3DXMatrixTranslation(&mTrans, fX, fY, fZ);
	D3DXMatrixTranslation(&m_mTrans, m_vTranslation.x, m_vTranslation.y, m_vTranslation.z);
	D3DXMatrixTranslation(&m_mTransInv, -m_vTranslation.x, -m_vTranslation.y, -m_vTranslation.z);

	m_mModel *= mTrans;
}

/****************************************************************************
 ****************************************************************************/
void Surface::Rotate(D3DXVECTOR3 axis, float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationAxis(&mRot, &axis, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}

/****************************************************************************
 ****************************************************************************/
void Surface::RotateX(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationX(&mRot, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}

/****************************************************************************
 ****************************************************************************/
void Surface::RotateY(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationY(&mRot, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}

/****************************************************************************
 ****************************************************************************/
void Surface::RotateZ(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationZ(&mRot, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}

/****************************************************************************
 ****************************************************************************/
void Surface::Scale(float fFactor)
{
	D3DXMATRIX mScale;
	D3DXMatrixScaling(&mScale, fFactor, fFactor, fFactor);

	m_mModel *= mScale;
}

/****************************************************************************
 ****************************************************************************/
void Surface::SetIsoColor(float fIsoColor)
{
	m_fIsoColor = fIsoColor;
}

float Surface::GetIsoColor()
{
	return m_fIsoColor;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Surface::LoadMesh(std::string strMeshName, std::string* pTextureName /* = NULL */, D3DXCOLOR* pColor /* = NULL */)
{
	HRESULT hr(S_OK);
	
	//load mesh with assimp
	Assimp::Importer Importer;

	const aiScene* pScene = Importer.ReadFile(strMeshName.c_str(), aiProcess_Triangulate |
																	aiProcess_GenNormals
																	);

	if(pScene == NULL)
	{
		std::string errorstring = Importer.GetErrorString();

		MessageBox ( NULL , L"Mesh type is not supported!", ConvertMultibyteToWideChar(strMeshName).c_str(), MB_OK);
		return S_OK;
	}

	//reset model matrix
	D3DXMatrixIdentity(&m_mModel);

	//release buffers
	SAFE_RELEASE(m_pTriangleVertexBuffer);
	SAFE_RELEASE(m_pTriangleIndexBuffer);
	SAFE_RELEASE(m_pEdgeIndexBuffer);
	SAFE_DELETE(m_pVertices);

	m_nNumVertices = 0;
	m_nNumIndices = 0;

	// Get vertex and index count of the whole mesh
	for(unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		m_nNumVertices += pScene->mMeshes[i]->mNumVertices;
		m_nNumIndices += pScene->mMeshes[i]->mNumFaces*3;
	}

	// Create vertex and index array
	m_pVertices = new SURFACE_VERTEX[m_nNumVertices];
	unsigned int* pTriangleIndices = new unsigned int[m_nNumIndices];
	unsigned int* pEdgeIndices = new unsigned int[m_nNumIndices*2];
	unsigned int mCurrentVertex = 0;
	unsigned int mCurrentIndex = 0;
	float fMaxVertexValue = 0;

	bool bLoadTexture = false;
	m_bHasTextureCoords = false;
	m_bIsTextured = false;

	//load vertices, normals and texcoords
	for(unsigned int i = 0; i < pScene->mNumMeshes; i++)
	{
		const aiMesh* paiMesh = pScene->mMeshes[i];

		if(paiMesh->HasTextureCoords(0))
			m_bHasTextureCoords = true;

		for(unsigned int j = 0; j < paiMesh->mNumVertices; j++)
		{
			const aiVector3D* pPos = &(paiMesh->mVertices[j]);
			const aiVector3D* pTexcoord = &(paiMesh->mTextureCoords[0][j]);
			const aiColor4D* pColor = &(paiMesh->mColors[0][j]);

			SURFACE_VERTEX vertex;
			vertex.pos = D3DXVECTOR3(pPos->x, pPos->y, pPos->z);
			if(paiMesh->HasTextureCoords(0) && pTexcoord != NULL)
				vertex.texcoord = D3DXVECTOR2(pTexcoord->x, pTexcoord->y);
			else
				vertex.texcoord = D3DXVECTOR2();

			if(paiMesh->HasVertexColors(0) && pColor != NULL)
				vertex.color = D3DXVECTOR4(pColor->r, pColor->g, pColor->b, pColor->a);
			else
				vertex.color = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);

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

	if(m_bHasTextureCoords && pColor == NULL)
	{
		std::string sTextureName;
		//create an openfile dialog if this function was called without a texture string
		if(pTextureName == NULL)
		{
			MessageBox ( NULL , L"Mesh successfully loaded. Please choose a texture file in the next dialog!", ConvertMultibyteToWideChar(strMeshName).c_str(), MB_OK);

			OPENFILENAME ofnTexture;
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
			sTextureName = ConvertWideCharToChar(ofnTexture.lpstrFile);
		}
		else
		{
			sTextureName = *pTextureName;
		}

		//load the texture with freeimage
		//Get the image file type
		FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(sTextureName.c_str());

		//load the image file
		FIBITMAP *texture = FreeImage_Load(fif, sTextureName.c_str());
	
		if(texture != NULL)
		{
			unsigned char* bits = (unsigned char*)FreeImage_GetBits(texture);
		
			unsigned int nBPP = FreeImage_GetBPP(texture);
			unsigned int nWidth = FreeImage_GetWidth(texture);
			unsigned int nHeight = FreeImage_GetHeight(texture);
			unsigned int nPitch = FreeImage_GetPitch(texture);
			unsigned int nLine = FreeImage_GetLine(texture);
	
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
	
			//release previous texture
			SAFE_RELEASE(m_pDiffuseTexture);
			SAFE_RELEASE(m_pDiffuseTextureSRV);

			//create new texture
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
			DXUT_SetDebugName( m_pDiffuseTexture, sTextureName.c_str());
	
			//create SRV
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(srvDesc));
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pDiffuseTexture, &srvDesc, &m_pDiffuseTextureSRV));	
		}

		FreeImage_Unload(texture);

		m_bIsTextured = true;
	}
	else
	{
		D3DXCOLOR cColor;

		if(pColor == NULL)
		{
			MessageBox ( NULL , L"Mesh successfully loaded. Please choose a color in the next dialog!", ConvertMultibyteToWideChar(strMeshName).c_str(), MB_OK);
			CHOOSECOLOR cc;                 // common dialog box structure 
			static COLORREF acrCustClr[16]; // array of custom colors 
			HBRUSH hbrush;                  // brush handle 
			static DWORD rgbCurrent;        // initial color selection 
  
			// Initialize CHOOSECOLOR 
			ZeroMemory(&cc, sizeof(cc)); 
			cc.lStructSize = sizeof(cc); 
			cc.lpCustColors = (LPDWORD) acrCustClr; 
			cc.rgbResult = rgbCurrent; 
			cc.Flags = CC_FULLOPEN | CC_RGBINIT; 
  	
			while(ChooseColor(&cc)==0)
			{}

			hbrush = CreateSolidBrush(cc.rgbResult); 
			D3DXCOLOR cTempColor = D3DXCOLOR(cc.rgbResult);
			cColor.r = cTempColor.b;
			cColor.g = cTempColor.g;
			cColor.b = cTempColor.r;
			cColor.a = 1.0f;
		}
		else
		{
			cColor.r = pColor->r;
			cColor.g = pColor->g;
			cColor.b = pColor->b;
			cColor.a = pColor->a;
		}
	
		for(unsigned int i = 0; i < m_nNumVertices; i++)
		{
			m_pVertices[i].color.x = cColor.r;
			m_pVertices[i].color.y = cColor.g;
			m_pVertices[i].color.z = cColor.b;
			m_pVertices[i].color.w = 1.0f;
		}

		m_bIsTextured = false;
	}


	//Scale the model
	Scale(1/fMaxVertexValue * 0.5f);

	//Create vertex buffer
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = m_nNumVertices*sizeof(SURFACE_VERTEX);
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
	ibtDesc.ByteWidth = m_nNumIndices*sizeof(unsigned int);
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
	ibeDesc.ByteWidth = m_nNumIndices*2*sizeof(unsigned int);
	ibeDesc.Usage = D3D11_USAGE_DEFAULT;
	ibeDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibeDesc.CPUAccessFlags = 0;
	ibeDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA ibeInitialData;
	ibeInitialData.pSysMem = pEdgeIndices;
	ibeInitialData.SysMemPitch = 0;
	ibeInitialData.SysMemSlicePitch = 0;
	m_pd3dDevice->CreateBuffer(&ibeDesc, &ibeInitialData, &m_pEdgeIndexBuffer);
	
	
	return hr;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Surface::Initialize(std::string strMeshName, std::string strTextureName)
{
	HRESULT hr(S_OK);

	V_RETURN(InitializeShader());


	V_RETURN(LoadMesh(strMeshName, &strTextureName));

	return hr;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Surface::Initialize(std::string strMeshName, D3DXCOLOR cColor)
{
	HRESULT hr(S_OK);

	V_RETURN(InitializeShader());

	V_RETURN(LoadMesh(strMeshName, NULL, &cColor));

	return hr;
}

/****************************************************************************
 ****************************************************************************/
void Surface::Render(D3DXMATRIX mViewProjection)
{
	//Set up global shader variables
    D3DXMATRIX mModelViewProjection = m_mModel * mViewProjection;
	m_pModelViewProjectionVar->SetMatrix(reinterpret_cast<float*>(&mModelViewProjection));
	m_pIsTexturedVar->SetBool(m_bIsTextured);

	//Set up vertex and index buffer
	UINT strides = sizeof(SURFACE_VERTEX);
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

		m_pd3dImmediateContext->DrawIndexed(m_nNumIndices, 0, 0);
	}
}

/****************************************************************************
 ****************************************************************************/
void Surface::RenderVoronoi(ID3DX11EffectTechnique* pVoronoiTechnique, ID3DX11EffectShaderResourceVariable *pSurfaceTextureVar)
{
	//Set up vertex and index buffer
	UINT strides = sizeof(SURFACE_VERTEX);
	UINT offsets = 0;
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pTriangleVertexBuffer, &strides, &offsets );
	m_pd3dImmediateContext->IASetIndexBuffer( m_pTriangleIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

	//Set input layout
	m_pd3dImmediateContext->IASetInputLayout( m_pInputLayout );

	//set texture
	pSurfaceTextureVar->SetResource(m_pDiffuseTextureSRV);

	//TRIANGLE
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pd3dImmediateContext->IASetIndexBuffer( m_pTriangleIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

	//apply triangle pass
	pVoronoiTechnique->GetPassByName("Triangle")->Apply( 0, m_pd3dImmediateContext);
	
	m_pd3dImmediateContext->DrawIndexed(m_nNumIndices, 0, 0);

	//EDGE
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	m_pd3dImmediateContext->IASetIndexBuffer( m_pEdgeIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

	//apply edge pass
	pVoronoiTechnique->GetPassByName("Edge")->Apply( 0, m_pd3dImmediateContext);
	
	m_pd3dImmediateContext->DrawIndexed(m_nNumIndices*2, 0, 0);

	//POINT
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	//apply point pass
	pVoronoiTechnique->GetPassByName("Point")->Apply( 0, m_pd3dImmediateContext);
	
	m_pd3dImmediateContext->Draw(m_nNumVertices, 0);
}

/****************************************************************************
 ****************************************************************************/
BOUNDINGBOX Surface::GetBoundingBox()
{
	BOUNDINGBOX bbFinal;
	D3DXVECTOR4 vCurrent;
	
	D3DXVec3Transform(&bbFinal.vMin, &m_pVertices[0].pos, &m_mModel);
	D3DXVec3Transform(&bbFinal.vMax, &m_pVertices[0].pos, &m_mModel);

	for(int i = 1; i < m_nNumVertices; i++)
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
	//increase the size of the bounding box a bit
	bbFinal.vMin.x -= 0.1;
	bbFinal.vMin.y -= 0.1;
	bbFinal.vMin.z -= 0.1;
	bbFinal.vMax.x += 0.1;
	bbFinal.vMax.y += 0.1;
	bbFinal.vMax.z += 0.1;
	
	return bbFinal;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Surface::InitializeShader()
{
	HRESULT hr(S_OK);

	m_pTechnique				= m_pSurfaceEffect->GetTechniqueByName("RenderColor");
	m_pModelViewProjectionVar	= m_pSurfaceEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();
	m_pSurfaceTextureVar		= m_pSurfaceEffect->GetVariableByName("SurfaceTexture")->AsShaderResource();
	m_pIsTexturedVar			= m_pSurfaceEffect->GetVariableByName("bIsTextured")->AsScalar();

	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

    // Create our vertex input layout
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",		0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

   V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));

   return hr;
}

