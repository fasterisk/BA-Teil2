#include "Globals.h"
#include "VolumeRenderer.h"

VolumeRenderer::VolumeRenderer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pEffect = pEffect;

	m_pVolumeRenderTechnique = NULL;
	
	m_pBBVertexBuffer = NULL;
	m_pBBIndexBuffer = NULL;
	m_pBBInputLayout = NULL;

	m_pFrontTexture2D = NULL;
    m_pFrontRTV = NULL;
    m_pFrontSRV = NULL;
    m_pBackTexture2D = NULL;
    m_pBackRTV = NULL;
    m_pBackSRV = NULL;

	m_pSQInputLayout = NULL;
	m_pSQVertexBuffer = NULL;

	m_bAllSlices = true;
}

VolumeRenderer::~VolumeRenderer()
{
	SAFE_RELEASE(m_pBBVertexBuffer);
	SAFE_RELEASE(m_pBBIndexBuffer);
	SAFE_RELEASE(m_pBBInputLayout);

	SAFE_RELEASE(m_pFrontTexture2D);
    SAFE_RELEASE(m_pFrontRTV);
    SAFE_RELEASE(m_pFrontSRV);
    SAFE_RELEASE(m_pBackTexture2D);
    SAFE_RELEASE(m_pBackRTV);
    SAFE_RELEASE(m_pBackSRV);

	SAFE_RELEASE(m_pSQInputLayout);
	SAFE_RELEASE(m_pSQVertexBuffer);
}

HRESULT VolumeRenderer::Initialize()
{
	HRESULT hr;
	V_RETURN(InitShader());
	V_RETURN(InitBoundingIndicesAndLayout());
	V_RETURN(CreateScreenQuad());
	return S_OK;
}

HRESULT VolumeRenderer::Update(int iWidth, int iHeight, int iDepth)
{
	float maxSize = (float)max(iWidth, max(iHeight, iDepth));
	D3DXVECTOR3 vStepSize = D3DXVECTOR3(1.0f / (iWidth * (maxSize/iWidth)),
										1.0f / (iHeight * (maxSize / iHeight)),
										1.0f / (iDepth * (maxSize / iDepth)));
	//if(m_bAllSlices)
		m_pStepSizeVar->SetFloatVector(vStepSize * (1/maxSize+3));
	//else
	//	m_pStepSizeVar->SetFloatVector(vStepSize);
	int iIterations = (int)maxSize;// * 2.0f;
	m_pIterationsVar->SetInt(iIterations);
	
	return S_OK;
}

HRESULT VolumeRenderer::SetScreenSize(int iWidth, int iHeight)
{
	HRESULT hr;

	//clean up all resources
	SAFE_RELEASE(m_pFrontTexture2D);
    SAFE_RELEASE(m_pFrontRTV);
    SAFE_RELEASE(m_pFrontSRV);
    SAFE_RELEASE(m_pBackTexture2D);
    SAFE_RELEASE(m_pBackRTV);
    SAFE_RELEASE(m_pBackSRV);

	//Set members
	m_iWidth = iWidth;
	m_iHeight = iHeight;

	//create 2D texture for front- and backface rendering
	D3D11_TEXTURE2D_DESC desc;
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = iWidth;
	desc.Height = iHeight;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	V_RETURN(m_pd3dDevice->CreateTexture2D(&desc, NULL, &m_pFrontTexture2D));
	V_RETURN(m_pd3dDevice->CreateTexture2D(&desc, NULL, &m_pBackTexture2D));

	//create the render target views
	D3D11_RENDER_TARGET_VIEW_DESC descRT;
	descRT.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	descRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	descRT.Texture2D.MipSlice = 0;
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pFrontTexture2D, &descRT, &m_pFrontRTV));
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pBackTexture2D, &descRT, &m_pBackRTV));

	//create the shader resource views
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	//ZeroMemory( &descSRV, sizeof(descSRV) ); -> used in nvidia sample; don't know if it is needed
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	descSRV.Texture2D.MostDetailedMip = 0;
	descSRV.Texture2D.MipLevels = 1;
	descSRV.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pFrontTexture2D, &descSRV, &m_pFrontSRV));
	V_RETURN(m_pd3dDevice->CreateShaderResourceView(m_pBackTexture2D, &descSRV, &m_pBackSRV));

	return S_OK;
}

HRESULT VolumeRenderer::ChangeSliceRenderingParameters(float fAlpha, bool bAllSlices)
{
	HRESULT hr;
	V_RETURN(m_fAlphaVar->SetFloat(fAlpha));
	m_bAllSlices = bAllSlices;
	return S_OK;
}

void VolumeRenderer::Render(VERTEX* pBBVertices, D3DXVECTOR3 vBBMin, D3DXVECTOR3 vBBMax, D3DXMATRIX mWorldViewProjection, ID3D11ShaderResourceView* p3DTextureSRV)
{
	m_pBBMinVar->SetFloatVector(vBBMin);
	m_pBBMaxVar->SetFloatVector(vBBMax);
	
	//Update vertex buffer for boundingbox
	UpdateBoundingVertices(pBBVertices);

	float color[4] = {0, 0, 0, 0 };

	//Update shader variables
	m_pWorldViewProjectionVar->SetMatrix(mWorldViewProjection);

	//Set rendertarget-viewport
	D3D11_VIEWPORT rtViewport;
	rtViewport.TopLeftX = 0;
	rtViewport.TopLeftY = 0;
	rtViewport.MinDepth = 0;
	rtViewport.MaxDepth = 1;
	rtViewport.Width = float(m_iWidth);
	rtViewport.Height = float(m_iHeight);
	m_pd3dImmediateContext->RSSetViewports(1, &rtViewport);

	

	//Render frontfaces of boundingbox
	m_pd3dImmediateContext->ClearRenderTargetView(m_pFrontRTV, color);
	m_pd3dImmediateContext->OMSetRenderTargets(1, &m_pFrontRTV, NULL);
	m_pVolumeRenderTechnique->GetPassByName("BoundingBoxFront")->Apply(0, m_pd3dImmediateContext);
	DrawBoundingBox();

	//Render backfaces of boundingbox
	m_pd3dImmediateContext->ClearRenderTargetView(m_pBackRTV, color);
	m_pd3dImmediateContext->OMSetRenderTargets(1, &m_pBackRTV, NULL);
	m_pVolumeRenderTechnique->GetPassByName("BoundingBoxBack")->Apply(0, m_pd3dImmediateContext);
	DrawBoundingBox();

	

	//Restore Rendertarget- and Depthstencilview
	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	m_pd3dImmediateContext->OMSetRenderTargets(1, &pRTV, pDSV);

	m_pFrontTextureVar->SetResource(m_pFrontSRV);
	m_pBackTextureVar->SetResource(m_pBackSRV);
	m_pVolumeTextureVar->SetResource(p3DTextureSRV);

	m_pVolumeRenderTechnique->GetPassByName("RayCast")->Apply(0, m_pd3dImmediateContext);
	
	DrawScreenQuad();

	//Draw wireframe boundingbox
	m_pVolumeRenderTechnique->GetPassByName("Wireframe")->Apply(0, m_pd3dImmediateContext);
	DrawBoundingBox();

	m_pFrontTextureVar->SetResource(0);
	m_pBackTextureVar->SetResource(0);
	m_pVolumeTextureVar->SetResource(0);//doesnt work.. reason unknown
}

HRESULT VolumeRenderer::InitShader()
{
	m_pVolumeRenderTechnique = m_pEffect->GetTechniqueByName("VolumeRendering");
	m_pWorldViewProjectionVar = m_pEffect->GetVariableByName("WorldViewProjection")->AsMatrix();
	m_pFrontTextureVar = m_pEffect->GetVariableByName("FrontTexture")->AsShaderResource();
	m_pBackTextureVar = m_pEffect->GetVariableByName("BackTexture")->AsShaderResource();
	m_pVolumeTextureVar = m_pEffect->GetVariableByName("VolumeTexture")->AsShaderResource();
	m_pStepSizeVar = m_pEffect->GetVariableByName("vStepSize")->AsVector();
	m_pIterationsVar = m_pEffect->GetVariableByName("iIterations")->AsScalar();
	m_pBBMinVar = m_pEffect->GetVariableByName("vBBMin")->AsVector();
	m_pBBMaxVar = m_pEffect->GetVariableByName("vBBMax")->AsVector();
	m_fAlphaVar = m_pEffect->GetVariableByName("fAlpha")->AsScalar();

	return S_OK;
}

HRESULT VolumeRenderer::InitBoundingIndicesAndLayout()
{
	HRESULT hr;

	SAFE_RELEASE(m_pBBIndexBuffer);
	SAFE_RELEASE(m_pBBInputLayout);

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
	V_RETURN(m_pd3dDevice->CreateBuffer(&ibd, &indexData, &m_pBBIndexBuffer));

	
	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pVolumeRenderTechnique->GetPassByName("BoundingBoxFront")->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

	V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pBBInputLayout));

	return S_OK;
}

HRESULT VolumeRenderer::CreateScreenQuad()
{
	HRESULT hr;

	SAFE_RELEASE(m_pSQInputLayout);
	SAFE_RELEASE(m_pSQVertexBuffer);

	//Create input layout
	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pVolumeRenderTechnique->GetPassByName("RayCast")->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

	V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pSQInputLayout));

	// Create screenquad vertices
	SCREENQUAD_VERTEX vSQ[4];
	vSQ[0].pos =  D3DXVECTOR3(-1.0f,  1.0f, 0.0f);
	vSQ[0].tex = D3DXVECTOR2( 0.0f,  0.0f);
	vSQ[1].pos =  D3DXVECTOR3( 1.0f,  1.0f, 0.0f);
	vSQ[1].tex = D3DXVECTOR2( 1.0f,  0.0f);
	vSQ[2].pos =  D3DXVECTOR3(-1.0f, -1.0f, 0.0f);
	vSQ[2].tex = D3DXVECTOR2( 0.0f,  1.0f);
	vSQ[3].pos =  D3DXVECTOR3( 1.0f, -1.0f, 0.0f);
	vSQ[3].tex = D3DXVECTOR2( 1.0f,  1.0f);


	//Create Vertex buffer
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(vSQ);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vSQ;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	
	V_RETURN(m_pd3dDevice->CreateBuffer(&vbd, &vertexData, &m_pSQVertexBuffer));

	return S_OK;
}

HRESULT VolumeRenderer::UpdateBoundingVertices(VERTEX* BBVertices)
{
	HRESULT hr;

	SAFE_RELEASE(m_pBBVertexBuffer);
	
	//Create Vertex buffer
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(VERTEX) * 8;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = BBVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	
	V_RETURN(m_pd3dDevice->CreateBuffer(&vbd, &vertexData, &m_pBBVertexBuffer));

	return S_OK;
}

void VolumeRenderer::DrawBoundingBox()
{
	UINT strides = sizeof(VERTEX);
    UINT offsets = 0;
	m_pd3dImmediateContext->IASetInputLayout(m_pBBInputLayout);
	m_pd3dImmediateContext->IASetIndexBuffer(m_pBBIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pBBVertexBuffer, &strides, &offsets);
    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pd3dImmediateContext->DrawIndexed(36, 0, 0);
}

void VolumeRenderer::DrawScreenQuad()
{
	UINT strides = sizeof(SCREENQUAD_VERTEX);
    UINT offsets = 0;
	m_pd3dImmediateContext->IASetInputLayout(m_pSQInputLayout);
    m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pSQVertexBuffer, &strides, &offsets);
    m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pd3dImmediateContext->Draw(4, 0);
}