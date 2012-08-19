#include "Globals.h"
#include "VolumeRenderer.h"
#include "Scene.h"
#include "TextureManager.h"

/****************************************************************************
 ****************************************************************************/
VolumeRenderer::VolumeRenderer(ID3DX11Effect* pEffect)
{
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

	m_bLinearSampling = true;
	m_bShowIsoSurface = false;
	m_bShowBoundingBox = true;
}

/****************************************************************************
 ****************************************************************************/
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

/****************************************************************************
 ****************************************************************************/
HRESULT VolumeRenderer::Initialize()
{
	HRESULT hr;
	V_RETURN(InitShader());
	V_RETURN(InitBoundingIndicesAndLayout());
	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT VolumeRenderer::Update(int iWidth, int iHeight, int iDepth)
{
	float maxSize = (float)max(iWidth, max(iHeight, iDepth));

	D3DXVECTOR3 vStepSize = D3DXVECTOR3(1.0f / (iWidth  * (maxSize / iWidth)),
										1.0f / (iHeight * (maxSize / iHeight)),
										1.0f / (iDepth  * (maxSize / iDepth)));
	m_pStepSizeVar->SetFloatVector(vStepSize);

	int iIterations = (int)maxSize * 2;
	m_pIterationsVar->SetInt(iIterations);
	
	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
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
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateTexture2D(&desc, NULL, &m_pFrontTexture2D));
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateTexture2D(&desc, NULL, &m_pBackTexture2D));

	//create the render target views
	D3D11_RENDER_TARGET_VIEW_DESC descRT;
	descRT.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	descRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	descRT.Texture2D.MipSlice = 0;
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateRenderTargetView(m_pFrontTexture2D, &descRT, &m_pFrontRTV));
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateRenderTargetView(m_pBackTexture2D, &descRT, &m_pBackRTV));

	//create the shader resource views
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	//ZeroMemory( &descSRV, sizeof(descSRV) ); -> used in nvidia sample; don't know if it is needed
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	descSRV.Texture2D.MostDetailedMip = 0;
	descSRV.Texture2D.MipLevels = 1;
	descSRV.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateShaderResourceView(m_pFrontTexture2D, &descSRV, &m_pFrontSRV));
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateShaderResourceView(m_pBackTexture2D, &descSRV, &m_pBackSRV));

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
void VolumeRenderer::ChangeSampling()
{
	m_bLinearSampling = !m_bLinearSampling;
}

/****************************************************************************
 ****************************************************************************/
void VolumeRenderer::ShowIsoSurface(bool bShow)
{
	m_bShowIsoSurface = bShow;
}

/****************************************************************************
 ****************************************************************************/
void VolumeRenderer::ShowBoundingBox(bool bShow)
{
	m_bShowBoundingBox = bShow;
}


/****************************************************************************
 ****************************************************************************/
void VolumeRenderer::Render(SURFACE_VERTEX* pBBVertices, 
							D3DXVECTOR3 vBBMin, 
							D3DXVECTOR3 vBBMax, 
							D3DXMATRIX mWorldViewProjection, 
							const unsigned int n3DTexture)
{
	m_pBBMinVar->SetFloatVector(vBBMin);
	m_pBBMaxVar->SetFloatVector(vBBMax);
	m_pSamplingVar->SetBool(m_bLinearSampling);
	m_pShowIsoSurfaceVar->SetBool(m_bShowIsoSurface);
	
	//Update vertex buffer for boundingbox
	UpdateBoundingVertices(pBBVertices);

	float clearColor[4] = {0, 0, 0, 1};

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
	Scene::GetInstance()->GetContext()->RSSetViewports(1, &rtViewport);

	//Render frontfaces of boundingbox
	Scene::GetInstance()->GetContext()->ClearRenderTargetView(m_pFrontRTV, clearColor);
	Scene::GetInstance()->GetContext()->OMSetRenderTargets(1, &m_pFrontRTV, NULL);
	m_pVolumeRenderTechnique->GetPassByName("BoundingBoxFront")->Apply(0, Scene::GetInstance()->GetContext());
	DrawBoundingBox();

	//Render backfaces of boundingbox
	Scene::GetInstance()->GetContext()->ClearRenderTargetView(m_pBackRTV, clearColor);
	Scene::GetInstance()->GetContext()->OMSetRenderTargets(1, &m_pBackRTV, NULL);
	m_pVolumeRenderTechnique->GetPassByName("BoundingBoxBack")->Apply(0, Scene::GetInstance()->GetContext());
	DrawBoundingBox();

	

	//Restore Rendertarget- and Depthstencilview
	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	Scene::GetInstance()->GetContext()->OMSetRenderTargets(1, &pRTV, pDSV);

	m_pFrontTextureVar->SetResource(m_pFrontSRV);
	m_pBackTextureVar->SetResource(m_pBackSRV);
	TextureManager::GetInstance()->BindTextureAsSRV(n3DTexture, m_pVolumeTextureVar);

	m_pVolumeRenderTechnique->GetPassByName("RayCast")->Apply(0, Scene::GetInstance()->GetContext());
	DrawBoundingBox();

	//unbind textures
	m_pFrontTextureVar->SetResource(NULL);
	m_pBackTextureVar->SetResource(NULL);
	m_pVolumeTextureVar->SetResource(NULL);
	m_pVolumeRenderTechnique->GetPassByName("RayCast")->Apply(0, Scene::GetInstance()->GetContext());
	

	//Draw wireframe boundingbox
	if(m_bShowBoundingBox)
	{
		m_pVolumeRenderTechnique->GetPassByName("Wireframe")->Apply(0, Scene::GetInstance()->GetContext());
		DrawBoundingBox();
	}

	
}

/****************************************************************************
 ****************************************************************************/
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
	m_pSamplingVar = m_pEffect->GetVariableByName("bLinearSampling")->AsScalar();
	m_pShowIsoSurfaceVar = m_pEffect->GetVariableByName("bShowIsoSurface")->AsScalar();

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
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
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateBuffer(&ibd, &indexData, &m_pBBIndexBuffer));

	
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

	V_RETURN(Scene::GetInstance()->GetDevice()->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pBBInputLayout));

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT VolumeRenderer::UpdateBoundingVertices(SURFACE_VERTEX* BBVertices)
{
	HRESULT hr;

	SAFE_RELEASE(m_pBBVertexBuffer);
	
	//Create Vertex buffer
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(SURFACE_VERTEX) * 8;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = BBVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateBuffer(&vbd, &vertexData, &m_pBBVertexBuffer));

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
void VolumeRenderer::DrawBoundingBox()
{
	UINT strides = sizeof(SURFACE_VERTEX);
    UINT offsets = 0;
	Scene::GetInstance()->GetContext()->IASetInputLayout(m_pBBInputLayout);
	Scene::GetInstance()->GetContext()->IASetIndexBuffer(m_pBBIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Scene::GetInstance()->GetContext()->IASetVertexBuffers(0, 1, &m_pBBVertexBuffer, &strides, &offsets);
    Scene::GetInstance()->GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Scene::GetInstance()->GetContext()->DrawIndexed(36, 0, 0);
}

