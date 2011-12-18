#include "Globals.h"

#include "Surface.h"
#include "TextureGrid.h"
#include "Voxelizer.h"
#include "VolumeRenderer.h"
#include "BoundingBox.h"


BoundingBox::BoundingBox(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pEffect, VolumeRenderer* pVolumeRenderer)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pEffect = pEffect;
	m_pVolumeRenderer = pVolumeRenderer;

	memset(m_pRenderTargets3D, 0, sizeof(m_pRenderTargets3D));
    memset(m_pShaderResourceVariables, 0, sizeof(m_pShaderResourceVariables));
    memset(m_pRenderTargetShaderViews, 0, sizeof(m_pRenderTargetShaderViews));
    memset(m_pRenderTargetViews, 0, sizeof(m_pRenderTargetViews));
}

BoundingBox::~BoundingBox()
{
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);

	SAFE_RELEASE(m_pEffect);
	SAFE_DELETE(m_pTextureGrid);
	SAFE_DELETE(m_pVoxelizer);

	SAFE_DELETE(m_pVertices);
	
	SAFE_RELEASE(m_pRasterizerStateSolid);
	SAFE_RELEASE(m_pRasterizerStateWireframe);
	
	SAFE_DELETE(m_pSurface1);
	SAFE_DELETE(m_pSurface2);

	for(int i=0;i<NUM_RENDER_TARGETS;i++)
    {
        SAFE_RELEASE(m_pRenderTargets3D[i]);
        SAFE_RELEASE(m_pRenderTargetShaderViews[i]);
        SAFE_RELEASE(m_pRenderTargetViews[i]);
    }
}

HRESULT BoundingBox::Initialize(int iWidth, int iHeight, int iDepth)
{
	HRESULT hr;

	V_RETURN(InitSurfaces());
	V_RETURN(InitBuffers());
	V_RETURN(InitRasterizerStates());
	V_RETURN(InitTechniques());
	V_RETURN(InitRenderTargets(iWidth, iHeight, iDepth));

	// Initialize TextureGrid
	m_pTextureGrid = new TextureGrid(m_pd3dDevice, m_pd3dImmediateContext);
	V_RETURN(m_pTextureGrid->Initialize(iWidth, iHeight, iDepth, m_pEffect->GetTechniqueByName("Grid")));

	// Initialize Voxelizer
	m_pVoxelizer = new Voxelizer();
	V_RETURN(m_pVoxelizer->SetDestination(m_pd3dDevice, m_pd3dImmediateContext, m_pSurface1Texture3D));

	return S_OK;
}

HRESULT BoundingBox::InitSurfaces()
{
	HRESULT hr;

	// Create surface1 and its buffers
	m_pSurface1 = new Surface(m_pd3dDevice, m_pd3dImmediateContext);
	m_pSurface1->ReadVectorFile("Media\\surface1.xml");
	V_RETURN(m_pSurface1->InitBuffers());
    
	// Create surface2 and its buffers
	m_pSurface2 = new Surface(m_pd3dDevice, m_pd3dImmediateContext);
	m_pSurface2->ReadVectorFile("Media\\surface1.xml");
	m_pSurface2->SetColor(1.0, 1.0, 1.0);
	V_RETURN(m_pSurface2->InitBuffers());
	m_pSurface2->Scale(0.5);

	m_pControlledSurface = m_pSurface1;

	ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    SRVDesc.Texture3D.MipLevels = 1;
    SRVDesc.Texture3D.MostDetailedMip = 0;
	SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	return S_OK;
}

HRESULT BoundingBox::InitRasterizerStates()
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

HRESULT BoundingBox::InitBuffers()
{
	HRESULT hr;

	FILE *F_out = fopen("Media\\test_CreateBoundingBox.txt", "w");
	char c[256];

	m_pVertices = new VERTEX[8];
	VERTEX min, max;
	for(int i = 0; i < m_pSurface1->m_vNum; i++)
	{
		VERTEX temp = m_pSurface1->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface1->m_mModel);
		temp.x = mul.x;
		temp.y = mul.y;
		temp.z = mul.z;
		if(i == 0)
		{
			min = temp;
			max = temp;
		}

		if(temp.x < min.x)
			min.x = temp.x;
		if(temp.y < min.y)
			min.y = temp.y;
		if(temp.z < min.z)
			min.z = temp.z;
		if(temp.x > max.x)
			max.x = temp.x;
		if(temp.y > max.y)
			max.y = temp.y;
		if(temp.z > max.z)
			max.z = temp.z;
	}

	for(int i = 0; i < m_pSurface2->m_vNum; i++)
	{
		VERTEX temp = m_pSurface2->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface2->m_mModel);
		temp.x = mul.x;
		temp.y = mul.y;
		temp.z = mul.z;

		if(temp.x < min.x)
			min.x = temp.x;
		if(temp.y < min.y)
			min.y = temp.y;
		if(temp.z < min.z)
			min.z = temp.z;
		if(temp.x > max.x)
			max.x = temp.x;
		if(temp.y > max.y)
			max.y = temp.y;
		if(temp.z > max.z)
			max.z = temp.z;
	}

	m_pVertices[0].x = min.x;
	m_pVertices[0].y = min.y;
	m_pVertices[0].z = min.z;
	m_pVertices[0].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[1].x = max.x;
	m_pVertices[1].y = min.y;
	m_pVertices[1].z = min.z;
	m_pVertices[1].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[2].x = max.x;
	m_pVertices[2].y = max.y;
	m_pVertices[2].z = min.z;
	m_pVertices[2].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[3].x = min.x;
	m_pVertices[3].y = max.y;
	m_pVertices[3].z = min.z;
	m_pVertices[3].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[4].x = max.x;
	m_pVertices[4].y = min.y;
	m_pVertices[4].z = max.z;
	m_pVertices[4].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[5].x = min.x;
	m_pVertices[5].y = min.y;
	m_pVertices[5].z = max.z;
	m_pVertices[5].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[6].x = max.x;
	m_pVertices[6].y = max.y;
	m_pVertices[6].z = max.z;
	m_pVertices[6].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[7].x = min.x;
	m_pVertices[7].y = max.y;
	m_pVertices[7].z = max.z;
	m_pVertices[7].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);

	for(int i = 0; i < 8; i++)
	{
		sprintf(c, "vertex[%d]=(%g,%g,%g) color=(%g,%g,%g) \n", i, m_pVertices[i].x, m_pVertices[i].y, m_pVertices[i].z, m_pVertices[i].color.r, m_pVertices[i].color.g, m_pVertices[i].color.b);
		fputs(c, F_out);
	}

	//Create Vertex buffer
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(VERTEX) * 8;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = m_pVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	V_RETURN(m_pd3dDevice->CreateBuffer(&vbd, &vertexData, &m_pVertexBuffer));

	//Create Index buffer
	unsigned int indices[36] = {0,2,1,0,3,2,0,1,4,0,4,5,1,2,6,1,6,4,2,3,7,2,7,6,3,0,5,3,5,7,5,4,6,5,6,7};

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_DYNAMIC;
	ibd.ByteWidth = sizeof(unsigned int) * 36;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;
	V_RETURN(m_pd3dDevice->CreateBuffer(&ibd, &indexData, &m_pIndexBuffer));

	fclose(F_out);

	return S_OK;
}

void BoundingBox::Render(D3DXMATRIX mViewProjection)
{

	// Compute mesh-to-grid xform
    D3DXMATRIX gridWorldInv;
    D3DXMatrixInverse(&gridWorldInv, NULL, &g_gridWorld);
        
	m_pVoxelizer->Voxelize(gridWorldInv, m_pSurface1);
	
	
	D3D11_TEXTURE3D_DESC desc;
	m_pSurface1Texture3D->GetDesc(&desc);

	m_pd3dDevice->CreateShaderResourceView( m_pSurface1Texture3D, &SRVDesc, &m_pSurface1SRV);



	m_pVolumeRenderer->Draw(m_pSurface1SRV);

	SAFE_RELEASE(m_pSurface1SRV);




	/*m_pd3dImmediateContext->RSSetState(m_pRasterizerStateWireframe);

	MVPMatrixShaderVariable->SetMatrix(mViewProjection);

	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DX11_TECHNIQUE_DESC techDesc;
	Technique->GetDesc(&techDesc);

	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//apply technique
		Technique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext );
				
		//draw
		m_pd3dImmediateContext->DrawIndexed( 36, 0, 0 );
	}

	m_pd3dImmediateContext->RSSetState(m_pRasterizerStateSolid); 
	*/
}


HRESULT BoundingBox::InitRenderTargets(int iWidth, int iHeight, int iDepth)
{
	HRESULT hr;

	D3D11_TEXTURE3D_DESC desc;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = iWidth;
	desc.Height = iHeight;
	desc.Depth = iDepth;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	for(int i = 0; i < NUM_RENDER_TARGETS; i++)
	{
		V_RETURN(CreateRenderTarget(i, desc));
	}

	//TEST
	V_RETURN( m_pd3dDevice->CreateTexture3D(&desc,NULL,&m_pSurface1Texture3D));

	V_RETURN(TextureWidthShaderVariable->SetFloat(float(iWidth)));
    V_RETURN(TextureHeightShaderVariable->SetFloat(float(iHeight)));
    V_RETURN(TextureDepthShaderVariable->SetFloat(float(iDepth)));

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
    SRVDesc.Texture3D.MipLevels = 1;
    SRVDesc.Texture3D.MostDetailedMip = 0;
	SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	V_RETURN(CreateRTTextureAsShaderResource(RENDER_TARGET_DIFFUSE0,"Texture_diffuse0",m_pEffect,&SRVDesc));
	V_RETURN(CreateRTTextureAsShaderResource(RENDER_TARGET_DIFFUSE1,"Texture_diffuse1",m_pEffect,&SRVDesc));

	return S_OK;
}

HRESULT BoundingBox::InitTechniques()
{
	HRESULT hr;
	
	m_TechniqueRenderSurfacesToTexture = m_pEffect->GetTechniqueByName("RenderSurfacesToTexture");

	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_TechniqueRenderSurfacesToTexture->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	// Create our vertex input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

	V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));

	MVPMatrixShaderVariable = m_pEffect->GetVariableByName("g_mModelViewProjection")->AsMatrix();
	TextureWidthShaderVariable = m_pEffect->GetVariableByName( "textureWidth")->AsScalar();
    TextureHeightShaderVariable = m_pEffect->GetVariableByName( "textureHeight")->AsScalar();
    TextureDepthShaderVariable = m_pEffect->GetVariableByName( "textureDepth")->AsScalar();

	return S_OK;
}

void BoundingBox::ChangeControlledSurface()
{
	if(m_bSurface1IsControlled)
		m_pControlledSurface = m_pSurface2;
	else
		m_pControlledSurface = m_pSurface1;

	m_bSurface1IsControlled = !m_bSurface1IsControlled;
}

void BoundingBox::CSTranslate(float fX, float fY, float fZ)
{
	m_pControlledSurface->Translate(fX, fY, fZ);
}

void BoundingBox::CSRotateX(float fFactor)
{
	m_pControlledSurface->RotateX(fFactor);
}

void BoundingBox::CSRotateY(float fFactor)
{
	m_pControlledSurface->RotateY(fFactor);
}

void BoundingBox::CSScale(float fFactor)
{
	m_pControlledSurface->Scale(fFactor);
}


HRESULT BoundingBox::CreateRenderTarget(int rtIndex, D3D11_TEXTURE3D_DESC desc)
{
	 HRESULT hr;

    // Release resources in case they exist
	SAFE_RELEASE( m_pRenderTargets3D[rtIndex] ); //  exception raises; dunno why
    SAFE_RELEASE( m_pRenderTargetViews[rtIndex] );

    // Create the texture
    V_RETURN( m_pd3dDevice->CreateTexture3D(&desc,NULL,&m_pRenderTargets3D[rtIndex]));
    // Create the render target view
    D3D11_RENDER_TARGET_VIEW_DESC DescRT;
    DescRT.Format = desc.Format;
    DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
    DescRT.Texture3D.FirstWSlice = 0;
    DescRT.Texture3D.MipSlice = 0;
    DescRT.Texture3D.WSize = desc.Depth;

    V_RETURN( m_pd3dDevice->CreateRenderTargetView( m_pRenderTargets3D[rtIndex], &DescRT, &m_pRenderTargetViews[rtIndex]) );

    return S_OK;
}

HRESULT BoundingBox::CreateRTTextureAsShaderResource(RENDER_TARGET rtIndex, LPCSTR shaderTextureName,
                                            ID3DX11Effect* pEffect, D3D11_SHADER_RESOURCE_VIEW_DESC *SRVDesc )
{
    HRESULT hr;

    // Create the "shader resource view" and "shader resource variable" for the given texture 
    SAFE_RELEASE(m_pRenderTargetShaderViews[rtIndex]);
    V_RETURN(m_pd3dDevice->CreateShaderResourceView( m_pRenderTargets3D[rtIndex], 
        SRVDesc, &m_pRenderTargetShaderViews[rtIndex]));
    m_pShaderResourceVariables[rtIndex] = m_pEffect->GetVariableByName(shaderTextureName)->AsShaderResource();

    // Then we bind the texture SRView to the SRVar
    V_RETURN(m_pShaderResourceVariables[rtIndex]->SetResource(m_pRenderTargetShaderViews[rtIndex] ));
    
    return S_OK;
}

HRESULT BoundingBox::UpdateVertexBuffer()
{
	HRESULT hr;

	m_pVertexBuffer->Release();
	
	VERTEX min, max;
	for(int i = 0; i < m_pSurface1->m_vNum; i++)
	{
		VERTEX temp = m_pSurface1->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface1->m_mModel);
		temp.x = mul.x;
		temp.y = mul.y;
		temp.z = mul.z;
		if(i == 0)
		{
			min = temp;
			max = temp;
		}

		if(temp.x < min.x)
			min.x = temp.x;
		if(temp.y < min.y)
			min.y = temp.y;
		if(temp.z < min.z)
			min.z = temp.z;
		if(temp.x > max.x)
			max.x = temp.x;
		if(temp.y > max.y)
			max.y = temp.y;
		if(temp.z > max.z)
			max.z = temp.z;
	}

	for(int i = 0; i < m_pSurface2->m_vNum; i++)
	{
		VERTEX temp = m_pSurface2->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface2->m_mModel);
		temp.x = mul.x;
		temp.y = mul.y;
		temp.z = mul.z;
		if(temp.x < min.x)
			min.x = temp.x;
		if(temp.y < min.y)
			min.y = temp.y;
		if(temp.z < min.z)
			min.z = temp.z;
		if(temp.x > max.x)
			max.x = temp.x;
		if(temp.y > max.y)
			max.y = temp.y;
		if(temp.z > max.z)
			max.z = temp.z;
	}

	m_pVertices[0].x = min.x;
	m_pVertices[0].y = min.y;
	m_pVertices[0].z = min.z;
	m_pVertices[1].x = max.x;
	m_pVertices[1].y = min.y;
	m_pVertices[1].z = min.z;
	m_pVertices[2].x = max.x;
	m_pVertices[2].y = max.y;
	m_pVertices[2].z = min.z;
	m_pVertices[3].x = min.x;
	m_pVertices[3].y = max.y;
	m_pVertices[3].z = min.z;
	m_pVertices[4].x = max.x;
	m_pVertices[4].y = min.y;
	m_pVertices[4].z = max.z;
	m_pVertices[5].x = min.x;
	m_pVertices[5].y = min.y;
	m_pVertices[5].z = max.z;
	m_pVertices[6].x = max.x;
	m_pVertices[6].y = max.y;
	m_pVertices[6].z = max.z;
	m_pVertices[7].x = min.x;
	m_pVertices[7].y = max.y;
	m_pVertices[7].z = max.z;

	//Create Vertex buffer
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(VERTEX) * 8;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = m_pVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	V_RETURN(m_pd3dDevice->CreateBuffer(&vbd, &vertexData, &m_pVertexBuffer));

	return S_OK;
}
