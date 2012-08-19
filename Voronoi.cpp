#include "Globals.h"
#include "Voronoi.h"
#include "TextureManager.h"
#include "Scene.h"

/****************************************************************************
 ****************************************************************************/
Voronoi::Voronoi(ID3DX11Effect *pVoronoiEffect)
{
	m_pVoronoiEffect = pVoronoiEffect;

	m_iTextureWidth = 0;
	m_iTextureHeight = 0;
	m_iTextureDepth = 0;

	m_pInputLayout = NULL;
	m_pSlicesVB = NULL;

	m_iCurrentSlice = 0;

	m_bRendering = false;
}

/****************************************************************************
 ****************************************************************************/
Voronoi::~Voronoi()
{
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::Initialize(const int iWidth,
						const int iHeight,
						const int iDepth)
{
	HRESULT hr;

	m_iTextureWidth = iWidth;
	m_iTextureHeight = iHeight;
	m_iTextureDepth = iDepth;

	//Initialize Techniques and Shadervariables
	V_RETURN(ItlInitShaders());

	V_RETURN(ItlInitTextures());

	V_RETURN(ItlInitSliceBuffer());

	//Re-initialize the behaviour variables for incremental voronoi generation
	m_iCurrentSlice = 0;
	m_bRendering = false;

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::Update(const int iWidth,
						const int iHeight,
						const int iDepth)
{
	HRESULT hr;

	m_iTextureWidth = iWidth;
	m_iTextureHeight = iHeight;
	m_iTextureDepth = iDepth;

	V_RETURN(ItlUpdateTextures());

	//Re-initialize the behaviour variables for incremental voronoi generation
	m_iCurrentSlice = 0;
	m_bRendering = false;

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT	Voronoi::ItlInitTextures()
{
	HRESULT hr(S_OK);

	m_nColorTex3D = TextureManager::GetInstance()->Create3DTexture("Voronoi 3D Texture", m_iTextureWidth, m_iTextureHeight, m_iTextureDepth);
	m_nDistTex3D = TextureManager::GetInstance()->Create3DTexture("Distance 3D Texture", m_iTextureWidth, m_iTextureHeight, m_iTextureDepth);

	m_nColorSliceTex2D = TextureManager::GetInstance()->Create2DTexture("Color 2D Slice", m_iTextureWidth, m_iTextureHeight);
	m_nDistSliceTex2D = TextureManager::GetInstance()->Create2DTexture("Distance 2D Slice", m_iTextureWidth, m_iTextureHeight);

	m_nDepthBufferTex2D = TextureManager::GetInstance()->Create2DDepthBuffer("Depthbuffer 2D Slice", m_iTextureWidth, m_iTextureHeight);

	return hr;
}

/****************************************************************************
 ****************************************************************************/
HRESULT	Voronoi::ItlUpdateTextures()
{
	HRESULT hr(S_OK);

	TextureManager::GetInstance()->Update3DTexture(m_nColorTex3D, m_iTextureWidth, m_iTextureHeight, m_iTextureDepth);
	TextureManager::GetInstance()->Update3DTexture(m_nDistTex3D, m_iTextureWidth, m_iTextureHeight, m_iTextureDepth);

	TextureManager::GetInstance()->Update2DTexture(m_nColorSliceTex2D, m_iTextureWidth, m_iTextureHeight);
	TextureManager::GetInstance()->Update2DTexture(m_nDistSliceTex2D, m_iTextureWidth, m_iTextureHeight);

	TextureManager::GetInstance()->Update2DDepthBuffer(m_nDepthBufferTex2D, m_iTextureWidth, m_iTextureHeight);

	return hr;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::ItlInitSliceBuffer()
{
	HRESULT hr;

	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pSlicesVB);

	const D3D11_INPUT_ELEMENT_DESC inputlayout[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_p2Dto3DTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned int vsCodeLen = effectVsDesc.BytecodeLength;
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateInputLayout(inputlayout, _countof(inputlayout), vsCodePtr, vsCodeLen, &m_pInputLayout));

	SCREENQUAD_VERTEX sliceVertices[6];
	sliceVertices[0].pos = D3DXVECTOR3(-1.0f, 1.0f, 0.5f);
	sliceVertices[0].tex = D3DXVECTOR2(0.0f, 0.0f);
	sliceVertices[1].pos = D3DXVECTOR3(-1.0f, -1.0f, 0.5f);
	sliceVertices[1].tex = D3DXVECTOR2(0.0f, 1.0f);
	sliceVertices[2].pos = D3DXVECTOR3(1.0f, -1.0f, 0.5f);
	sliceVertices[2].tex = D3DXVECTOR2(1.0f, 1.0f);
	sliceVertices[3].pos = sliceVertices[0].pos;
	sliceVertices[3].tex = sliceVertices[0].tex;
	sliceVertices[4].pos = sliceVertices[2].pos;
	sliceVertices[4].tex = sliceVertices[2].tex;
	sliceVertices[5].pos = D3DXVECTOR3(1.0f, 1.0f, 0.5f);
	sliceVertices[5].tex = D3DXVECTOR2(1.0f, 0.0f);

	//create the vertex buffer
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = 6*sizeof(SCREENQUAD_VERTEX);
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initialData;
	initialData.pSysMem = &sliceVertices;
	initialData.SysMemPitch = 0;
	initialData.SysMemSlicePitch = 0;
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateBuffer(&vbDesc, &initialData, &m_pSlicesVB));

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Voronoi::ItlInitShaders()
{
	HRESULT hr;

	assert(m_pVoronoiEffect);

	// Get Technique and variables
	m_pVoronoiDiagramTechnique	= m_pVoronoiEffect->GetTechniqueByName("GenerateVoronoiDiagram");
	m_p2Dto3DTechnique			= m_pVoronoiEffect->GetTechniqueByName("Technique2Dto3D");

	m_pModelViewProjectionVar	= m_pVoronoiEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();
	m_pNormalMatrixVar			= m_pVoronoiEffect->GetVariableByName("NormalMatrix")->AsMatrix();
	m_pSliceIndexVar			= m_pVoronoiEffect->GetVariableByName("iSliceIndex")->AsScalar();
	m_pTextureSizeVar			= m_pVoronoiEffect->GetVariableByName("vTextureSize")->AsVector();
	m_pBBMinVar					= m_pVoronoiEffect->GetVariableByName("vBBMin")->AsVector();
	m_pBBMaxVar					= m_pVoronoiEffect->GetVariableByName("vBBMax")->AsVector();
	m_pSurfaceTextureVar		= m_pVoronoiEffect->GetVariableByName("SurfaceTexture")->AsShaderResource();
	m_pIsoSurfaceVar			= m_pVoronoiEffect->GetVariableByName("fIsoSurfaceVal")->AsScalar();
	m_pColorSliceTex2DVar		= m_pVoronoiEffect->GetVariableByName("ColorSliceTex2D")->AsShaderResource();
	m_pDistSliceTex2DVar		= m_pVoronoiEffect->GetVariableByName("DistSliceTex2D")->AsShaderResource();

	assert(m_pVoronoiDiagramTechnique);
	assert(m_pModelViewProjectionVar);
	assert(m_pSliceIndexVar);
	assert(m_pTextureSizeVar);
	assert(m_pBBMinVar);
	assert(m_pBBMaxVar);

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
bool Voronoi::RenderVoronoi(D3DXVECTOR3 vBBMin, D3DXVECTOR3 vBBMax)
{
	m_bRendering = true;

	//store the old render targets and viewports
    ID3D11RenderTargetView* pOldRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pOldDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	Scene::GetInstance()->GetContext()->RSGetViewports( &NumViewports, &pViewports[0]);

	// generate orth. matrices with bounding parameters
	D3DXMATRIX mOrth, mModel1Orth, mModel2Orth;
	D3DXMatrixOrthoOffCenterLH(&mOrth, vBBMin.x, vBBMax.x, vBBMin.y, vBBMax.y, vBBMin.z, vBBMax.z);
	D3DXMatrixMultiply(&mModel1Orth, &Scene::GetInstance()->GetSurface1()->m_mModel, &mOrth);
	D3DXMatrixMultiply(&mModel2Orth, &Scene::GetInstance()->GetSurface2()->m_mModel, &mOrth);

	//set bounding box parameters
	D3DXVECTOR4 vBBMinOrth, vBBMaxOrth;
	D3DXVec3Transform(&vBBMinOrth, &vBBMin, &mOrth);
	D3DXVec3Transform(&vBBMaxOrth, &vBBMax, &mOrth);

	assert(vBBMinOrth);
	assert(vBBMaxOrth);

	//Compute NormalMatrix for both surfaces
	D3DXMATRIX mModel1_3x3 = D3DXMATRIX(Scene::GetInstance()->GetSurface1()->m_mModel._11, Scene::GetInstance()->GetSurface1()->m_mModel._12, Scene::GetInstance()->GetSurface1()->m_mModel._13, 0.0f, 
									  Scene::GetInstance()->GetSurface1()->m_mModel._21, Scene::GetInstance()->GetSurface1()->m_mModel._22, Scene::GetInstance()->GetSurface1()->m_mModel._23, 0.0f, 
									  Scene::GetInstance()->GetSurface1()->m_mModel._31, Scene::GetInstance()->GetSurface1()->m_mModel._32, Scene::GetInstance()->GetSurface1()->m_mModel._33, 0.0f, 
									  0.0f, 0.0f, 0.0f, 1.0f);
	D3DXMATRIX mModel1_3x3Inv, mNormalMatrix1;
	D3DXMatrixInverse(&mModel1_3x3Inv, NULL, &mModel1_3x3);
	D3DXMatrixTranspose(&mNormalMatrix1, &mModel1_3x3Inv);

	D3DXMATRIX mModel2_3x3 = D3DXMATRIX(Scene::GetInstance()->GetSurface2()->m_mModel._11, Scene::GetInstance()->GetSurface2()->m_mModel._12, Scene::GetInstance()->GetSurface2()->m_mModel._13, 0.0f, 
									  Scene::GetInstance()->GetSurface2()->m_mModel._21, Scene::GetInstance()->GetSurface2()->m_mModel._22, Scene::GetInstance()->GetSurface2()->m_mModel._23, 0.0f, 
									  Scene::GetInstance()->GetSurface2()->m_mModel._31, Scene::GetInstance()->GetSurface2()->m_mModel._32, Scene::GetInstance()->GetSurface2()->m_mModel._33, 0.0f, 
									  0.0f, 0.0f, 0.0f, 1.0f);
	D3DXMATRIX mModel2_3x3Inv, mNormalMatrix2;
	D3DXMatrixInverse(&mModel2_3x3Inv, NULL, &mModel2_3x3);
	D3DXMatrixTranspose(&mNormalMatrix2, &mModel2_3x3Inv);
	
	// Set Variables needed for Voronoi Diagram Computation
	m_pBBMinVar->SetFloatVector(vBBMinOrth);
	m_pBBMaxVar->SetFloatVector(vBBMaxOrth);
	m_pTextureSizeVar->SetFloatVector(D3DXVECTOR3((float)m_iTextureWidth, (float)m_iTextureHeight, (float)m_iTextureDepth));
	
	// clear depthstencilview
	TextureManager::GetInstance()->Clear2DDepthBuffer(m_nDepthBufferTex2D);

	//Set 2D slice Textures and depthstencil view as RenderTargets
	TextureManager::GetInstance()->BindTextureAsRTV(m_nColorSliceTex2D, m_nDistSliceTex2D, m_nDepthBufferTex2D);

	// Set viewport and scissor to match the size of a single slice 
	D3D11_VIEWPORT viewport = { 0, 0, float(m_iTextureWidth), float(m_iTextureHeight), 0.0f, 1.0f };
	Scene::GetInstance()->GetContext()->RSSetViewports(1, &viewport);
	D3D11_RECT scissorRect = { 0, 0, m_iTextureWidth, m_iTextureHeight};
	Scene::GetInstance()->GetContext()->RSSetScissorRects(1, &scissorRect);

	// Draw the current slice
	m_pSliceIndexVar->SetInt(m_iCurrentSlice);

	m_pVoronoiDiagramTechnique->GetPassByIndex(0)->Apply(0, Scene::GetInstance()->GetContext());

	// Render the surfaces
	m_pModelViewProjectionVar->SetMatrix(mModel1Orth);
	m_pNormalMatrixVar->SetMatrix(mNormalMatrix1);
	m_pIsoSurfaceVar->SetFloat(Scene::GetInstance()->GetSurface1()->GetIsoColor());
	Scene::GetInstance()->GetSurface1()->RenderVoronoi(m_pVoronoiDiagramTechnique, m_pSurfaceTextureVar);

	m_pModelViewProjectionVar->SetMatrix(mModel2Orth);
	m_pNormalMatrixVar->SetMatrix(mNormalMatrix2);
	m_pIsoSurfaceVar->SetFloat(Scene::GetInstance()->GetSurface2()->GetIsoColor());
	Scene::GetInstance()->GetSurface2()->RenderVoronoi(m_pVoronoiDiagramTechnique, m_pSurfaceTextureVar);

	// render the 2D texture slices into the 3D Textures

	/*HRESULT hr;
	SAFE_RELEASE(m_pDestColorTex3DRTV);
	SAFE_RELEASE(m_pDestDistTex3DRTV);
	
	//create color texture RTV
	D3D11_TEXTURE3D_DESC descColorTex3D;
	m_pDestColorTex3D->GetDesc(&descColorTex3D);
	D3D11_RENDER_TARGET_VIEW_DESC descCT3DRTV;
	descCT3DRTV.Format = descColorTex3D.Format;
	descCT3DRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descCT3DRTV.Texture3D.MipSlice = 0;
	descCT3DRTV.Texture3D.FirstWSlice = m_iCurrentSlice;
	descCT3DRTV.Texture3D.WSize = 1;
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateRenderTargetView(m_pDestColorTex3D, &descCT3DRTV, &m_pDestColorTex3DRTV));

	//create dist texture RTV
	D3D11_TEXTURE3D_DESC descDistTex3D;
	m_pDestDistTex3D->GetDesc(&descDistTex3D);
	D3D11_RENDER_TARGET_VIEW_DESC descDT3DRTV;
	descDT3DRTV.Format = descDistTex3D.Format;
	descDT3DRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descDT3DRTV.Texture3D.MipSlice = 0;
	descDT3DRTV.Texture3D.FirstWSlice = m_iCurrentSlice;
	descDT3DRTV.Texture3D.WSize = 1;
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateRenderTargetView(m_pDestDistTex3D, &descDT3DRTV, &m_pDestDistTex3DRTV));
	*/

	//set the 3d textures as rendertargets and the 2D textures as resources
	/*ID3D11RenderTargetView* destTex3DRTVs[2];
	destTex3DRTVs[0] = m_pDestColorTex3DRTV;
	destTex3DRTVs[1] = m_pDestDistTex3DRTV;
	Scene::GetInstance()->GetContext()->OMSetRenderTargets(2, destTex3DRTVs, NULL);

	m_pVoronoiDiagramTechnique->GetPassByIndex(0)->Apply(0, Scene::GetInstance()->GetContext());

	m_pColorSliceTex2DVar->SetResource(m_pColorSliceSRV);
	m_pDistSliceTex2DVar->SetResource(m_pDistSliceSRV);

	m_p2Dto3DTechnique->GetPassByIndex(0)->Apply(0, Scene::GetInstance()->GetContext());

	ItlDrawCurrentSlice();

	m_pColorSliceTex2DVar->SetResource(NULL);
	m_pDistSliceTex2DVar->SetResource(NULL);

	m_p2Dto3DTechnique->GetPassByIndex(0)->Apply(0, Scene::GetInstance()->GetContext());*/

	TextureManager::GetInstance()->Render2DTextureInto3DSlice(m_nColorSliceTex2D, m_nColorTex3D, m_iCurrentSlice);
	TextureManager::GetInstance()->Render2DTextureInto3DSlice(m_nDistSliceTex2D, m_nDistTex3D, m_iCurrentSlice);

	m_iCurrentSlice++;
	
	//restore old render targets
	TextureManager::GetInstance()->UnBindRTVs();

	//restore old render targets
	Scene::GetInstance()->GetContext()->OMSetRenderTargets( 1,  &pOldRTV,  pOldDSV );
	Scene::GetInstance()->GetContext()->RSSetViewports( NumViewports, &pViewports[0]);

	if(m_iCurrentSlice == m_iTextureDepth)
	{
		m_iCurrentSlice = 0;
		m_bRendering = false;
		return true;
	}
	else
	{
		return false;
	}
}

/****************************************************************************
 ****************************************************************************/
void Voronoi::ItlDrawCurrentSlice()
{
	assert(m_pInputLayout);
	assert(m_pSlicesVB);
	
	UINT strides = sizeof(SCREENQUAD_VERTEX);
	UINT offsets = 0;

	Scene::GetInstance()->GetContext()->IASetInputLayout(m_pInputLayout);
	Scene::GetInstance()->GetContext()->IASetVertexBuffers(0, 1, &m_pSlicesVB, &strides, &offsets);
	Scene::GetInstance()->GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	Scene::GetInstance()->GetContext()->Draw(6, 0);
}

/****************************************************************************
 ****************************************************************************/
std::wstring Voronoi::GetRenderProgress()
{
	if(m_bRendering)
	{
		int iProgress = int((m_iCurrentSlice * 100)/m_iTextureDepth + 0.5);
		std::wstringstream sstm;
		sstm << "Generating Voronoi Diagram: " << iProgress << " %";
		return sstm.str();
	}

	return L"Generation of Voronoi Diagram completed!";
}