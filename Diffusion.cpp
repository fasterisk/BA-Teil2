#include "Globals.h"
#include "Diffusion.h"

Diffusion::Diffusion(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pDiffusionEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pDiffusionEffect = pDiffusionEffect;

	m_pInputLayout = NULL;
	m_pSlicesVB = NULL;

	m_pColor3DTextures[0] = NULL;
	m_pColor3DTextures[1] = NULL;
	m_pColor3DTexturesRTV[0] = NULL;
	m_pColor3DTexturesRTV[1] = NULL;
	m_pColor3DTexturesSRV[0] = NULL;
	m_pColor3DTexturesSRV[1] = NULL;

	m_pOneSliceTexture = NULL;
	m_pOneSliceTextureRTV = NULL;
	m_pOneSliceTextureSRV = NULL;

	m_pIsoSurfaceTexture = NULL;
	m_pIsoSurfaceTextureRTV = NULL;
	m_pIsoSurfaceTextureSRV = NULL;

	m_iTextureWidth = 0;
	m_iTextureHeight = 0;
	m_iTextureDepth = 0;

	m_fIsoValue = 0.5f;
	m_iDiffTex = 0;

}

Diffusion::~Diffusion()
{
	Cleanup();
}

void Diffusion::Cleanup()
{
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pSlicesVB);

	SAFE_RELEASE(m_pColor3DTexturesRTV[0]);
	SAFE_RELEASE(m_pColor3DTexturesRTV[1]);

	SAFE_RELEASE(m_pOneSliceTexture);
	SAFE_RELEASE(m_pOneSliceTextureRTV);
	SAFE_RELEASE(m_pOneSliceTextureSRV);

	SAFE_RELEASE(m_pIsoSurfaceTexture);
	SAFE_RELEASE(m_pIsoSurfaceTextureRTV);
	SAFE_RELEASE(m_pIsoSurfaceTextureSRV);
}

HRESULT Diffusion::Initialize(ID3D11Texture3D *pColorTex3D1,
							  ID3D11Texture3D *pColorTex3D2,
							  ID3D11ShaderResourceView* pColor3DTex1SRV, 
							  ID3D11ShaderResourceView* pColor3DTex2SRV,
							  int iTextureWidth, int iTextureHeight, int iTextureDepth, float fIsoValue)
{
	HRESULT hr;

	m_iTextureWidth = iTextureWidth;
	m_iTextureHeight = iTextureHeight;
	m_iTextureDepth = iTextureDepth;
	m_fIsoValue = fIsoValue;

	assert(m_pd3dDevice);
	assert(m_pd3dImmediateContext);
	assert(pColorTex3D1);
	assert(pColorTex3D2);
	assert(pColor3DTex1SRV);
	assert(pColor3DTex2SRV);

	m_pColor3DTextures[0] = pColorTex3D1;
	m_pColor3DTextures[1] = pColorTex3D2;
	m_pColor3DTexturesSRV[0] = pColor3DTex1SRV;
	m_pColor3DTexturesSRV[1] = pColor3DTex2SRV;

	assert(m_pColor3DTextures[0]);
	assert(m_pColor3DTextures[1]);
	
	//Initialize Techniques and Shadervariables
	V_RETURN(InitShaders());

	//Initialize RTVs
	V_RETURN(Init3DRTVs());

	//Initialize Slices
	V_RETURN(InitSlices());

}

HRESULT Diffusion::Init3DRTVs()
{
	HRESULT hr;

	assert(m_pColor3DTextures[0]);
	assert(m_pColor3DTextures[1]);

	SAFE_RELEASE(m_pColor3DTexturesRTV[0]);
	SAFE_RELEASE(m_pColor3DTexturesRTV[1]);

	//create RTVs
	D3D11_TEXTURE3D_DESC descColorTex3D1;
	m_pColor3DTextures[0]->GetDesc(&descColorTex3D1);
	D3D11_RENDER_TARGET_VIEW_DESC descCT3D1RTV;
	descCT3D1RTV.Format = descColorTex3D1.Format;
	descCT3D1RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descCT3D1RTV.Texture3D.MipSlice = 0;
	descCT3D1RTV.Texture3D.FirstWSlice = 0;
	descCT3D1RTV.Texture3D.WSize = descColorTex3D1.Depth;
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pColor3DTextures[0], &descCT3D1RTV, &m_pColor3DTexturesRTV[0]));

	D3D11_TEXTURE3D_DESC descColorTex3D2;
	m_pColor3DTextures[1]->GetDesc(&descColorTex3D2);
	D3D11_RENDER_TARGET_VIEW_DESC descCT3D2RTV;
	descCT3D2RTV.Format = descColorTex3D2.Format;
	descCT3D2RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descCT3D2RTV.Texture3D.MipSlice = 0;
	descCT3D2RTV.Texture3D.FirstWSlice = 0;
	descCT3D2RTV.Texture3D.WSize = descColorTex3D2.Depth;
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pColor3DTextures[1], &descCT3D2RTV, &m_pColor3DTexturesRTV[1]));

	return S_OK;
}

HRESULT Diffusion::InitShaders()
{
	HRESULT hr;

	assert(m_pDiffusionEffect);
	SAFE_RELEASE(m_pInputLayout);

	// Get Technique and variables
	m_pDiffusionTechnique	= m_pDiffusionEffect->GetTechniqueByName("Diffusion");

	m_pColor3DTexSRVar		= m_pDiffusionEffect->GetVariableByName("ColorTexture")->AsShaderResource();
	m_pDist3DTexSRVar		= m_pDiffusionEffect->GetVariableByName("DistTexture")->AsShaderResource();
	m_pIsoValueVar			= m_pDiffusionEffect->GetVariableByName("fIsoValue")->AsScalar();
	m_pTextureSizeVar		= m_pDiffusionEffect->GetVariableByName("vTextureSize")->AsVector();
	m_pPolySizeVar			= m_pDiffusionEffect->GetVariableByName("fPolySize")->AsScalar();
	m_pSliceIndexVar		= m_pDiffusionEffect->GetVariableByName("iSliceIndex")->AsScalar();

	assert(m_pDiffusionTechnique);
	assert(m_pColor3DTexSRVar);
	assert(m_pDist3DTexSRVar);
	assert(m_pTextureSizeVar);
	assert(m_pIsoValueVar);

	return S_OK;
}

HRESULT Diffusion::InitSlices()
{
	HRESULT hr;

	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pSlicesVB);

	//Create full-screen quad input layout
	const D3D11_INPUT_ELEMENT_DESC inputLayout[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pDiffusionTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;
	V_RETURN(m_pd3dDevice->CreateInputLayout(inputLayout, _countof(inputLayout), vsCodePtr, vsCodeLen, &m_pInputLayout));


#define SLICEQUAD_VERTEX_COUNT 6
	// Create a vertex buffers of quads, one per slice, with texcoords to lookup from a flat 3D texture
    // and with homogenous coordinates to cover a fullscreen quad
	SLICE_SCREENQUAD_VERTEX* sliceVertices = new SLICE_SCREENQUAD_VERTEX[SLICEQUAD_VERTEX_COUNT*m_iTextureDepth];
	SLICE_SCREENQUAD_VERTEX sliceVerticesTemp[4];
	int vertexIndex = 0;

	for(int z = 0; z < m_iTextureDepth; z++)
	{
		vertexIndex = z * SLICEQUAD_VERTEX_COUNT;

		sliceVerticesTemp[0].pos = D3DXVECTOR3(-1.0f, 1.0f, 0.5f);
		sliceVerticesTemp[0].tex = D3DXVECTOR3(0.0f, 0.0f, float(z));

		sliceVerticesTemp[1].pos = D3DXVECTOR3(-1.0f, -1.0f, 0.5f);
		sliceVerticesTemp[1].tex = D3DXVECTOR3(0.0f, 1.0f, float(z));
        
        sliceVerticesTemp[2].pos = D3DXVECTOR3(1.0f, -1.0f, 0.5f);
		sliceVerticesTemp[2].tex = D3DXVECTOR3(1.0f, 1.0f, float(z));
        
        sliceVerticesTemp[3].pos = D3DXVECTOR3(1.0f, 1.0f, 0.5f);
		sliceVerticesTemp[3].tex = D3DXVECTOR3(1.0f, 0.0f, float(z));

		sliceVertices[vertexIndex+0] = sliceVerticesTemp[0];
		sliceVertices[vertexIndex+1] = sliceVerticesTemp[1];
        sliceVertices[vertexIndex+2] = sliceVerticesTemp[2];
        sliceVertices[vertexIndex+3] = sliceVerticesTemp[0];
        sliceVertices[vertexIndex+4] = sliceVerticesTemp[2];
        sliceVertices[vertexIndex+5] = sliceVerticesTemp[3];

	}

	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = SLICEQUAD_VERTEX_COUNT*m_iTextureDepth*sizeof(SLICE_SCREENQUAD_VERTEX);
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initialData;
	initialData.pSysMem = sliceVertices;
	initialData.SysMemPitch = 0;
	initialData.SysMemSlicePitch = 0;
	V_RETURN(m_pd3dDevice->CreateBuffer(&vbDesc, &initialData, &m_pSlicesVB));

	delete[] sliceVertices;
	
	return S_OK;
}

void Diffusion::ChangeIsoValue(float fIsoValue)
{
	m_fIsoValue = fIsoValue;
}

ID3D11ShaderResourceView* Diffusion::RenderDiffusion(ID3D11ShaderResourceView* pVoronoi3DTextureSRV, 
								   ID3D11ShaderResourceView* pDist3DTextureSRV, 
								   int iDiffusionSteps)
{
	HRESULT hr(S_OK);

	hr = m_pTextureSizeVar->SetFloatVector(D3DXVECTOR3((float)m_iTextureWidth, (float)m_iTextureHeight, (float)m_iTextureDepth));
	assert(hr == S_OK);

	hr = m_pDist3DTexSRVar->SetResource(pDist3DTextureSRV);
	assert(hr == S_OK);
	
	for(int i = 0; i < iDiffusionSteps; i++)
	{
		hr = m_pPolySizeVar->SetFloat(1.0 - (float)(i)/(float)iDiffusionSteps);
		assert(hr == S_OK);

		if(i == 0)
		{
			m_pd3dImmediateContext->OMSetRenderTargets(1, &m_pColor3DTexturesRTV[m_iDiffTex], NULL);
			hr = m_pColor3DTexSRVar->SetResource(pVoronoi3DTextureSRV);
			assert(hr == S_OK);
		}
		else
		{
			m_pd3dImmediateContext->OMSetRenderTargets(1, &m_pColor3DTexturesRTV[m_iDiffTex], NULL);
			hr = m_pColor3DTexSRVar->SetResource(m_pColor3DTexturesSRV[1-m_iDiffTex]);
			assert(hr == S_OK);
		}

		m_iDiffTex = 1-m_iDiffTex;

		hr = m_pDiffusionTechnique->GetPassByName("DiffuseTexture")->Apply(0, m_pd3dImmediateContext);
		assert(hr == S_OK);
		
		DrawSlices();

		//unbind textures and apply pass again to confirm this
		hr = m_pColor3DTexSRVar->SetResource(NULL);
		assert(hr == S_OK);
		hr = m_pDiffusionTechnique->GetPassByName("DiffuseTexture")->Apply(0, m_pd3dImmediateContext);
		assert(hr == S_OK);
	}

	return m_pColor3DTexturesSRV[1-m_iDiffTex];
}

ID3D11ShaderResourceView* Diffusion::GetOneDiffusionSlice(int iSliceIndex, ID3D11ShaderResourceView* pCurrentDiffusionSRV)
{
	HRESULT hr(S_OK);

	SAFE_RELEASE(m_pOneSliceTexture);
	SAFE_RELEASE(m_pOneSliceTextureRTV);
	SAFE_RELEASE(m_pOneSliceTextureSRV);

	//create empty 3D Texture
	D3D11_TEXTURE3D_DESC desc;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = m_iTextureWidth;
	desc.Height = m_iTextureHeight;
	desc.Depth = m_iTextureDepth;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	hr = m_pd3dDevice->CreateTexture3D(&desc, NULL, &m_pOneSliceTexture);
	assert(hr == S_OK);
	
	//create RTV
	D3D11_RENDER_TARGET_VIEW_DESC descRTV;
	descRTV.Format = desc.Format;
	descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descRTV.Texture3D.MipSlice = 0;
	descRTV.Texture3D.FirstWSlice = 0;
	descRTV.Texture3D.WSize = desc.Depth;
	hr = m_pd3dDevice->CreateRenderTargetView(m_pOneSliceTexture, &descRTV, &m_pOneSliceTextureRTV);
	assert(hr == S_OK);

	//create the shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	descSRV.Texture3D.MostDetailedMip = 0;
	descSRV.Texture3D.MipLevels = 1;
	descSRV.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	hr = m_pd3dDevice->CreateShaderResourceView(m_pOneSliceTexture, &descSRV, &m_pOneSliceTextureSRV);
	assert(hr == S_OK);

	m_pd3dImmediateContext->OMSetRenderTargets(1, &m_pOneSliceTextureRTV, NULL);
	hr = m_pColor3DTexSRVar->SetResource(pCurrentDiffusionSRV);
	assert(hr == S_OK);
	
	hr = m_pSliceIndexVar->SetFloat(iSliceIndex);
	assert(hr == S_OK);

	hr = m_pDiffusionTechnique->GetPassByName("RenderOneSlice")->Apply(0, m_pd3dImmediateContext);
	assert(hr == S_OK);	
		
	DrawSlices();

	hr = m_pColor3DTexSRVar->SetResource(NULL);
	assert(hr == S_OK);
	
	return m_pOneSliceTextureSRV;
}

ID3D11ShaderResourceView* Diffusion::RenderIsoSurface(ID3D11ShaderResourceView* pCurrentDiffusionSRV)
{
	HRESULT hr(S_OK);

	SAFE_RELEASE(m_pIsoSurfaceTexture);
	SAFE_RELEASE(m_pIsoSurfaceTextureRTV);
	SAFE_RELEASE(m_pIsoSurfaceTextureSRV);

	//create empty 3D Texture
	D3D11_TEXTURE3D_DESC desc;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = m_iTextureWidth;
	desc.Height = m_iTextureHeight;
	desc.Depth = m_iTextureDepth;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	hr = m_pd3dDevice->CreateTexture3D(&desc, NULL, &m_pIsoSurfaceTexture);
	assert(hr == S_OK);
	
	//create RTV
	D3D11_RENDER_TARGET_VIEW_DESC descRTV;
	descRTV.Format = desc.Format;
	descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descRTV.Texture3D.MipSlice = 0;
	descRTV.Texture3D.FirstWSlice = 0;
	descRTV.Texture3D.WSize = desc.Depth;
	hr = m_pd3dDevice->CreateRenderTargetView(m_pIsoSurfaceTexture, &descRTV, &m_pIsoSurfaceTextureRTV);
	assert(hr == S_OK);

	//create the shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
	descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	descSRV.Texture3D.MostDetailedMip = 0;
	descSRV.Texture3D.MipLevels = 1;
	descSRV.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	hr = m_pd3dDevice->CreateShaderResourceView(m_pIsoSurfaceTexture, &descSRV, &m_pIsoSurfaceTextureSRV);
	assert(hr == S_OK);

	m_pd3dImmediateContext->OMSetRenderTargets(1, &m_pIsoSurfaceTextureRTV, NULL);
	hr = m_pColor3DTexSRVar->SetResource(pCurrentDiffusionSRV);
	assert(hr == S_OK);
	
	hr = m_pIsoValueVar->SetFloat(m_fIsoValue);
	assert(hr == S_OK);

	hr = m_pDiffusionTechnique->GetPassByName("RenderIsoSurface")->Apply(0, m_pd3dImmediateContext);
	assert(hr == S_OK);	
	
	DrawSlices();

	hr = m_pColor3DTexSRVar->SetResource(NULL);
	assert(hr == S_OK);
	
	return m_pIsoSurfaceTextureSRV;
}

void Diffusion::DrawSlices()
{
	assert(m_pInputLayout);
	assert(m_pSlicesVB);

	//store the old render targets and viewports
    ID3D11RenderTargetView* pOldRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pOldDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	m_pd3dImmediateContext->RSGetViewports( &NumViewports, &pViewports[0]);

	// Set viewport and scissor to match the size of a single slice 
	D3D11_VIEWPORT viewport = { 0, 0, float(m_iTextureWidth), float(m_iTextureHeight), 0.0f, 1.0f };
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);
	D3D11_RECT scissorRect = { 0, 0, float(m_iTextureWidth), float(m_iTextureHeight)};
	m_pd3dImmediateContext->RSSetScissorRects(1, &scissorRect);

	UINT strides = sizeof(SLICE_SCREENQUAD_VERTEX);
	UINT offsets = 0;

	m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout);
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pSlicesVB, &strides, &offsets);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for(int i = 0; i < m_iTextureDepth; i++)
	{
		m_pSliceIndexVar->SetInt(i);
		m_pd3dImmediateContext->Draw(SLICEQUAD_VERTEX_COUNT, SLICEQUAD_VERTEX_COUNT*i);
	}

	//restore old render targets
	m_pd3dImmediateContext->OMSetRenderTargets( 1,  &pOldRTV,  pOldDSV );
	m_pd3dImmediateContext->RSSetViewports( NumViewports, &pViewports[0]);
}