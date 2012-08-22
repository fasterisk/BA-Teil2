#include "Globals.h"
#include "Diffusion.h"
#include "TextureManager.h"
#include "Scene.h"

/****************************************************************************
 ****************************************************************************/
Diffusion::Diffusion(ID3DX11Effect *pDiffusionEffect)
{
	m_pDiffusionEffect = pDiffusionEffect;

	m_pInputLayout = NULL;
	m_pSlicesVB = NULL;

	m_nDiffuseTex3D[0] = 0;
	m_nDiffuseTex3D[1] = 0;

	m_nDiffuseSliceTex2D[0] = 0;
	m_nDiffuseSliceTex2D[1] = 0;

	m_nOneSliceTex3D = 0;
	m_nOneSliceSliceTex2D = 0;

	m_nIsoSurfaceTex3D = 0;
	m_nIsoSurfaceSliceTex2D = 0;

	m_iTextureWidth = 0;
	m_iTextureHeight = 0;
	m_iTextureDepth = 0;

	m_fIsoValue = 0.5f;
	m_iDiffTex = 0;

	m_bShowIsoColor = false;
}

/****************************************************************************
 ****************************************************************************/
Diffusion::~Diffusion()
{
	Cleanup();
}

void Diffusion::Cleanup()
{
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pSlicesVB);
}

/****************************************************************************
 ****************************************************************************/
HRESULT Diffusion::Initialize(const int iTextureWidth, 
							  const int iTextureHeight, 
							  const int iTextureDepth)
{
	HRESULT hr(S_OK);

	m_iTextureWidth = iTextureWidth;
	m_iTextureHeight = iTextureHeight;
	m_iTextureDepth = iTextureDepth;
	
	//Initialize Techniques and Shadervariables
	V_RETURN(InitShaders());

	//Initialize Slices
	V_RETURN(InitSlices());

	m_nDiffuseTex3D[0] = TextureManager::GetInstance()->Create3DTexture("Diffusion 3D Tex1", iTextureWidth, iTextureHeight, iTextureDepth);
	m_nDiffuseTex3D[1] = TextureManager::GetInstance()->Create3DTexture("Diffusion 3D Tex2", iTextureWidth, iTextureHeight, iTextureDepth);

	m_nDiffuseSliceTex2D[0] = TextureManager::GetInstance()->Create2DTexture("Diffusion Slice 2D Tex1", iTextureWidth, iTextureHeight);
	m_nDiffuseSliceTex2D[1] = TextureManager::GetInstance()->Create2DTexture("Diffusion Slice 2D Tex2", iTextureWidth, iTextureHeight);

	m_nOneSliceTex3D = TextureManager::GetInstance()->Create3DTexture("One Slice 3D Tex", iTextureWidth, iTextureHeight, iTextureDepth);
	m_nOneSliceSliceTex2D = TextureManager::GetInstance()->Create2DTexture("One Slice Slice 2D Tex", iTextureWidth, iTextureHeight);

	m_nIsoSurfaceTex3D = TextureManager::GetInstance()->Create3DTexture("Isosurface 3D Tex", iTextureWidth, iTextureHeight, iTextureDepth);
	m_nIsoSurfaceSliceTex2D = TextureManager::GetInstance()->Create2DTexture("Isosurface Slice 2D Tex", iTextureWidth, iTextureHeight);

	return hr;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Diffusion::Update(const int iTextureWidth, 
						  const int iTextureHeight, 
						  const int iTextureDepth, 
						  const float fIsoValue)
{
	HRESULT hr;

	m_iTextureWidth = iTextureWidth;
	m_iTextureHeight = iTextureHeight;
	m_iTextureDepth = iTextureDepth;
	m_fIsoValue = fIsoValue;

	V_RETURN(InitSlices());

	TextureManager::GetInstance()->Update3DTexture(m_nDiffuseTex3D[0], iTextureWidth, iTextureHeight, iTextureDepth);
	TextureManager::GetInstance()->Update3DTexture(m_nDiffuseTex3D[1], iTextureWidth, iTextureHeight, iTextureDepth);
	TextureManager::GetInstance()->Update2DTexture(m_nDiffuseSliceTex2D[0], iTextureWidth, iTextureHeight);
	TextureManager::GetInstance()->Update2DTexture(m_nDiffuseSliceTex2D[1], iTextureWidth, iTextureHeight);

	TextureManager::GetInstance()->Update3DTexture(m_nOneSliceTex3D, iTextureWidth, iTextureHeight, iTextureDepth);
	TextureManager::GetInstance()->Update2DTexture(m_nOneSliceSliceTex2D, iTextureWidth, iTextureHeight);

	TextureManager::GetInstance()->Update3DTexture(m_nIsoSurfaceTex3D, iTextureWidth, iTextureHeight, iTextureDepth);
	TextureManager::GetInstance()->Update2DTexture(m_nIsoSurfaceSliceTex2D, iTextureWidth, iTextureHeight);


	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Diffusion::InitShaders()
{
	assert(m_pDiffusionEffect);

	// Get Technique and variables
	m_pDiffusionTechnique	= m_pDiffusionEffect->GetTechniqueByName("Diffusion");

	m_pColor3DTexSRVar		= m_pDiffusionEffect->GetVariableByName("ColorTexture")->AsShaderResource();
	m_pDist3DTexSRVar		= m_pDiffusionEffect->GetVariableByName("DistTexture")->AsShaderResource();
	m_pIsoValueVar			= m_pDiffusionEffect->GetVariableByName("fIsoValue")->AsScalar();
	m_pTextureSizeVar		= m_pDiffusionEffect->GetVariableByName("vTextureSize")->AsVector();
	m_pPolySizeVar			= m_pDiffusionEffect->GetVariableByName("fPolySize")->AsScalar();
	m_pSliceIndexVar		= m_pDiffusionEffect->GetVariableByName("iSliceIndex")->AsScalar();
	m_pShowIsoColorVar		= m_pDiffusionEffect->GetVariableByName("bShowIsoColor")->AsScalar();

	assert(m_pDiffusionTechnique);
	assert(m_pColor3DTexSRVar);
	assert(m_pDist3DTexSRVar);
	assert(m_pIsoValueVar);
	assert(m_pTextureSizeVar);
	assert(m_pPolySizeVar);
	assert(m_pSliceIndexVar);
	assert(m_pShowIsoColorVar);

	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
HRESULT Diffusion::InitSlices()
{
	HRESULT hr;

	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pSlicesVB);

	//Create full-screen quad input layout
	const D3D11_INPUT_ELEMENT_DESC inputLayout[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"SLICEINDEX", 0, DXGI_FORMAT_R8_UINT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pDiffusionTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateInputLayout(inputLayout, _countof(inputLayout), vsCodePtr, vsCodeLen, &m_pInputLayout));


#define VERTEXCOUNT 6
	// Create a vertex buffers of quads, one per slice, with texcoords to lookup from a flat 3D texture
    // and with homogenous coordinates to cover a fullscreen quad
	SLICE_VERTEX* sliceVertices = new SLICE_VERTEX[VERTEXCOUNT*m_iTextureDepth];
	SLICE_VERTEX sliceVerticesTemp[4];
	int vertexIndex = 0;

	for(int z = 0; z < m_iTextureDepth; z++)
	{
		vertexIndex = z * VERTEXCOUNT;

		sliceVerticesTemp[0].pos = D3DXVECTOR3(-1.0f, 1.0f, 0.5f);
		sliceVerticesTemp[0].tex = D3DXVECTOR3(0.0f, 0.0f, float(z)/float(m_iTextureDepth-1));

		sliceVerticesTemp[1].pos = D3DXVECTOR3(-1.0f, -1.0f, 0.5f);
		sliceVerticesTemp[1].tex = D3DXVECTOR3(0.0f, 1.0f, float(z)/float(m_iTextureDepth-1));
		sliceVerticesTemp[1].sliceindex = z;

        sliceVerticesTemp[2].pos = D3DXVECTOR3(1.0f, -1.0f, 0.5f);
		sliceVerticesTemp[2].tex = D3DXVECTOR3(1.0f, 1.0f, float(z)/float(m_iTextureDepth-1));
		sliceVerticesTemp[2].sliceindex = z;
        
        sliceVerticesTemp[3].pos = D3DXVECTOR3(1.0f, 1.0f, 0.5f);
		sliceVerticesTemp[3].tex = D3DXVECTOR3(1.0f, 0.0f, float(z)/float(m_iTextureDepth-1));
		sliceVerticesTemp[3].sliceindex = z;

		sliceVertices[vertexIndex+0] = sliceVerticesTemp[0];
		sliceVertices[vertexIndex+1] = sliceVerticesTemp[1];
        sliceVertices[vertexIndex+2] = sliceVerticesTemp[2];
        sliceVertices[vertexIndex+3] = sliceVerticesTemp[0];
        sliceVertices[vertexIndex+4] = sliceVerticesTemp[2];
        sliceVertices[vertexIndex+5] = sliceVerticesTemp[3];

	}

	//create the vertex buffer
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = VERTEXCOUNT*m_iTextureDepth*sizeof(SLICE_VERTEX);
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initialData;
	initialData.pSysMem = sliceVertices;
	initialData.SysMemPitch = 0;
	initialData.SysMemSlicePitch = 0;
	V_RETURN(Scene::GetInstance()->GetDevice()->CreateBuffer(&vbDesc, &initialData, &m_pSlicesVB));

	delete[] sliceVertices;
	
	return S_OK;
}

/****************************************************************************
 ****************************************************************************/
void Diffusion::ChangeIsoValue(float fIsoValue)
{
	m_fIsoValue = fIsoValue;
}

/****************************************************************************
 ****************************************************************************/
void Diffusion::ShowIsoColor(bool bShow)
{
	m_bShowIsoColor = bShow;
}

/****************************************************************************
 ****************************************************************************/
unsigned int	Diffusion::RenderDiffusion(const unsigned int nVoronoiTex3D,
										   const unsigned int nDistanceTex3D,
										   int iDiffusionSteps)
{
	HRESULT hr(S_OK);

	//store the old render targets and viewports
    ID3D11RenderTargetView* pOldRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pOldDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	Scene::GetInstance()->GetContext()->RSGetViewports( &NumViewports, &pViewports[0]);

	hr = m_pTextureSizeVar->SetFloatVector(D3DXVECTOR3((float)m_iTextureWidth, (float)m_iTextureHeight, (float)m_iTextureDepth));
	assert(hr == S_OK);

	TextureManager::GetInstance()->BindTextureAsSRV(nDistanceTex3D, m_pDist3DTexSRVar);
	
	// Set viewport and scissor to match the size of a single slice 
	D3D11_VIEWPORT viewport = { 0, 0, float(m_iTextureWidth), float(m_iTextureHeight), 0.0f, 1.0f };
    Scene::GetInstance()->GetContext()->RSSetViewports(1, &viewport);
	D3D11_RECT scissorRect = { 0, 0, float(m_iTextureWidth), float(m_iTextureHeight)};
	Scene::GetInstance()->GetContext()->RSSetScissorRects(1, &scissorRect);

	assert(m_pInputLayout);
	assert(m_pSlicesVB);

	UINT strides = sizeof(SLICE_VERTEX);
	UINT offsets = 0;

	Scene::GetInstance()->GetContext()->IASetInputLayout(m_pInputLayout);
	Scene::GetInstance()->GetContext()->IASetVertexBuffers(0, 1, &m_pSlicesVB, &strides, &offsets);
	Scene::GetInstance()->GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	//ping pong rendering
	for(int i = 0; i < iDiffusionSteps; i++)
	{
		hr = m_pPolySizeVar->SetFloat(1.0 - (float)(i)/(float)iDiffusionSteps);
		assert(hr == S_OK);

		if(i == 0)
		{
			//As first resource texture you have to use the voronoi texture
			TextureManager::GetInstance()->BindTextureAsRTV(m_nDiffuseSliceTex2D[m_iDiffTex]);
			TextureManager::GetInstance()->BindTextureAsSRV(nVoronoiTex3D, m_pColor3DTexSRVar);
		}
		else
		{
			//after the first render pass, color textures are alternated
			TextureManager::GetInstance()->BindTextureAsRTV(m_nDiffuseSliceTex2D[m_iDiffTex]);
			TextureManager::GetInstance()->BindTextureAsSRV(m_nDiffuseTex3D[1-m_iDiffTex], m_pColor3DTexSRVar);
		}

		hr = m_pDiffusionTechnique->GetPassByName("DiffuseTexture")->Apply(0, Scene::GetInstance()->GetContext());
		assert(hr == S_OK);

		//RENDER
		for(int j = 0; j < m_iTextureDepth; j++)
		{
			Scene::GetInstance()->GetContext()->Draw(VERTEXCOUNT, VERTEXCOUNT*j);
			TextureManager::GetInstance()->Render2DTextureInto3DSlice(m_nDiffuseSliceTex2D[m_iDiffTex], m_nDiffuseTex3D[m_iDiffTex], j);
		}

		m_iDiffTex = 1-m_iDiffTex;

		//unbind textures and apply pass again to confirm this
		hr = m_pColor3DTexSRVar->SetResource(NULL);
		assert(hr == S_OK);
		hr = m_pDiffusionTechnique->GetPassByName("DiffuseTexture")->Apply(0, Scene::GetInstance()->GetContext());
		assert(hr == S_OK);
	}

	//restore old render targets
	Scene::GetInstance()->GetContext()->OMSetRenderTargets( 1,  &pOldRTV,  pOldDSV );
	Scene::GetInstance()->GetContext()->RSSetViewports( NumViewports, &pViewports[0]);

	return m_nDiffuseTex3D[1-m_iDiffTex];
}

/****************************************************************************
 ****************************************************************************/
unsigned int	Diffusion::RenderOneDiffusionSlice(const int iSliceIndex,
													const unsigned int nCurrentDiffusionTexture)
{
	HRESULT hr(S_OK);

	//store the old render targets and viewports
    ID3D11RenderTargetView* pOldRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pOldDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	Scene::GetInstance()->GetContext()->RSGetViewports( &NumViewports, &pViewports[0]);

	//set one slice 2D texture as render target
	TextureManager::GetInstance()->BindTextureAsRTV(m_nOneSliceSliceTex2D);
	TextureManager::GetInstance()->BindTextureAsSRV(nCurrentDiffusionTexture, m_pColor3DTexSRVar);

	// Set viewport and scissor to match the size of a single slice 
	D3D11_VIEWPORT viewport = { 0, 0, float(m_iTextureWidth), float(m_iTextureHeight), 0.0f, 1.0f };
    Scene::GetInstance()->GetContext()->RSSetViewports(1, &viewport);
	D3D11_RECT scissorRect = { 0, 0, float(m_iTextureWidth), float(m_iTextureHeight)};
	Scene::GetInstance()->GetContext()->RSSetScissorRects(1, &scissorRect);
	
	//set shader variables
	hr = m_pSliceIndexVar->SetFloat(iSliceIndex);
	assert(hr == S_OK);

	assert(m_pInputLayout);
	assert(m_pSlicesVB);

	UINT strides = sizeof(SLICE_VERTEX);
	UINT offsets = 0;

	Scene::GetInstance()->GetContext()->IASetInputLayout(m_pInputLayout);
	Scene::GetInstance()->GetContext()->IASetVertexBuffers(0, 1, &m_pSlicesVB, &strides, &offsets);
	Scene::GetInstance()->GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//apply pass
	hr = m_pDiffusionTechnique->GetPassByName("RenderOneBlackSlice")->Apply(0, Scene::GetInstance()->GetContext());
	assert(hr == S_OK);	
		
	//RENDER
	for(int j = 0; j < m_iTextureDepth; j++)
	{
		if(j == iSliceIndex)
		{
			hr = m_pDiffusionTechnique->GetPassByName("RenderOneColorSlice")->Apply(0, Scene::GetInstance()->GetContext());
			assert(hr == S_OK);
			Scene::GetInstance()->GetContext()->Draw(VERTEXCOUNT, VERTEXCOUNT*j);
			hr = m_pDiffusionTechnique->GetPassByName("RenderOneBlackSlice")->Apply(0, Scene::GetInstance()->GetContext());
			assert(hr == S_OK);
		}
		else
		{
			Scene::GetInstance()->GetContext()->Draw(VERTEXCOUNT, VERTEXCOUNT*j);
		}

		
		TextureManager::GetInstance()->Render2DTextureInto3DSlice(m_nOneSliceSliceTex2D, m_nOneSliceTex3D, j);
	}

	hr = m_pColor3DTexSRVar->SetResource(NULL);
	assert(hr == S_OK);

	//apply pass again to unbind the resource texture
	hr = m_pDiffusionTechnique->GetPassByName("RenderOneBlackSlice")->Apply(0, Scene::GetInstance()->GetContext());
	assert(hr == S_OK);	
	
	//restore old render targets
	Scene::GetInstance()->GetContext()->OMSetRenderTargets( 1,  &pOldRTV,  pOldDSV );
	Scene::GetInstance()->GetContext()->RSSetViewports( NumViewports, &pViewports[0]);

	return m_nOneSliceTex3D;
}

/****************************************************************************
 ****************************************************************************/
unsigned int	Diffusion::RenderIsoSurface(const unsigned int nCurrentDiffusionTexture)
{
	HRESULT hr(S_OK);

	//store the old render targets and viewports
    ID3D11RenderTargetView* pOldRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pOldDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	Scene::GetInstance()->GetContext()->RSGetViewports( &NumViewports, &pViewports[0]);

	// Set viewport and scissor to match the size of a single slice 
	D3D11_VIEWPORT viewport = { 0, 0, float(m_iTextureWidth), float(m_iTextureHeight), 0.0f, 1.0f };
    Scene::GetInstance()->GetContext()->RSSetViewports(1, &viewport);
	D3D11_RECT scissorRect = { 0, 0, float(m_iTextureWidth), float(m_iTextureHeight)};
	Scene::GetInstance()->GetContext()->RSSetScissorRects(1, &scissorRect);

	//set iso surface texture as render target
	TextureManager::GetInstance()->BindTextureAsRTV(m_nIsoSurfaceSliceTex2D);
	TextureManager::GetInstance()->BindTextureAsSRV(nCurrentDiffusionTexture, m_pColor3DTexSRVar);
	assert(hr == S_OK);
	
	//set shader variables
	hr = m_pIsoValueVar->SetFloat(m_fIsoValue);
	assert(hr == S_OK);

	hr = m_pShowIsoColorVar->SetBool(m_bShowIsoColor);
	assert(hr == S_OK);
	
	//set texture size
	m_pTextureSizeVar->SetFloatVector(D3DXVECTOR3((float)m_iTextureWidth, (float)m_iTextureHeight, (float)m_iTextureDepth));

	//apply pass
	hr = m_pDiffusionTechnique->GetPassByName("RenderIsoSurface")->Apply(0, Scene::GetInstance()->GetContext());
	assert(hr == S_OK);	

	assert(m_pInputLayout);
	assert(m_pSlicesVB);

	UINT strides = sizeof(SLICE_VERTEX);
	UINT offsets = 0;

	Scene::GetInstance()->GetContext()->IASetInputLayout(m_pInputLayout);
	Scene::GetInstance()->GetContext()->IASetVertexBuffers(0, 1, &m_pSlicesVB, &strides, &offsets);
	Scene::GetInstance()->GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	//RENDER
	for(int j = 0; j < m_iTextureDepth; j++)
	{
		Scene::GetInstance()->GetContext()->Draw(VERTEXCOUNT, VERTEXCOUNT*j);
		TextureManager::GetInstance()->Render2DTextureInto3DSlice(m_nIsoSurfaceSliceTex2D, m_nIsoSurfaceTex3D, j);
	}

	hr = m_pColor3DTexSRVar->SetResource(NULL);
	assert(hr == S_OK);

	//apply pass again to unbind the resource texture
	hr = m_pDiffusionTechnique->GetPassByName("RenderIsoSurface")->Apply(0, Scene::GetInstance()->GetContext());
	assert(hr == S_OK);	

	//restore old render targets
	Scene::GetInstance()->GetContext()->OMSetRenderTargets( 1,  &pOldRTV,  pOldDSV );
	Scene::GetInstance()->GetContext()->RSSetViewports( NumViewports, &pViewports[0]);
	
	return m_nIsoSurfaceTex3D;
}

