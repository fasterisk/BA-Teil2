#include "Globals.h"
#include "Diffusion.h"

Diffusion::Diffusion(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dImmediateContext, ID3DX11Effect *pDiffusionEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pDiffusionEffect = pDiffusionEffect;

	m_pInputLayout = NULL;
	m_pSlicesVB = NULL;

	m_pColorTex3D1 = NULL;
	m_pColorTex3D2 = NULL;
	m_pColorTex3D1RTV = NULL;
	m_pColorTex3D2RTV = NULL;

	m_iTextureWidth = 0;
	m_iTextureHeight = 0;
	m_iTextureDepth = 0;

	m_fIsoValue = 0.5f;
}

Diffusion::~Diffusion()
{
	Cleanup();
}

void Diffusion::Cleanup()
{
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pSlicesVB);
}

HRESULT Diffusion::Initialize(ID3D11Texture3D *pColorTex3D1, ID3D11Texture3D *pColorTex3D2)
{
	HRESULT hr;

	assert(m_pd3dDevice);
	assert(m_pd3dImmediateContext);
	assert(pColorTex3D1);
	assert(pColorTex3D2);

	m_pColorTex3D1 = pColorTex3D1;
	m_pColorTex3D2 = pColorTex3D2;

	V_RETURN(InitRendertargets3D());

	//Initialize Slices
	V_RETURN(InitSlices());

	//Initialize Techniques and Shadervariables
	V_RETURN(InitShaders());
}


HRESULT Diffusion::Update(int iTextureWidth, int iTextureHeight, int iTextureDepth, float fIsoValue)
{
	HRESULT hr(S_OK);

	m_iTextureWidth = iTextureWidth;
	m_iTextureHeight = iTextureHeight;
	m_iTextureDepth = iTextureDepth;
	m_fIsoValue = fIsoValue;

	//Initialize Rendertargets for the 3D Textures -- needs to happen before depth stencil initialization
	// because m_iTextureWidth etc. are initialized in InitRendertargets3D
	V_RETURN(InitRendertargets3D());

	//Initialize Slices
	V_RETURN(InitSlices());
	
	return S_OK;
}

HRESULT Diffusion::InitRendertargets3D()
{
	HRESULT hr;

	assert(m_pColorTex3D1 != NULL);
	assert(m_pColorTex3D2 != NULL);

	SAFE_RELEASE(m_pColorTex3D1);
	SAFE_RELEASE(m_pColorTex3D2);

	D3D11_TEXTURE3D_DESC descColorTex3D1;
	m_pColorTex3D1->GetDesc(&descColorTex3D1);
	D3D11_RENDER_TARGET_VIEW_DESC descCT3D1RTV;
	descCT3D1RTV.Format = descColorTex3D1.Format;
	descCT3D1RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descCT3D1RTV.Texture3D.MipSlice = 0;
	descCT3D1RTV.Texture3D.FirstWSlice = 0;
	descCT3D1RTV.Texture3D.WSize = descColorTex3D1.Depth;
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pColorTex3D1, &descCT3D1RTV, &m_pColorTex3D1RTV));

	D3D11_TEXTURE3D_DESC descColorTex3D2;
	m_pColorTex3D2->GetDesc(&descColorTex3D2);
	D3D11_RENDER_TARGET_VIEW_DESC descCT3D2RTV;
	descCT3D2RTV.Format = descColorTex3D2.Format;
	descCT3D2RTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
	descCT3D2RTV.Texture3D.MipSlice = 0;
	descCT3D2RTV.Texture3D.FirstWSlice = 0;
	descCT3D2RTV.Texture3D.WSize = descColorTex3D2.Depth;
	V_RETURN(m_pd3dDevice->CreateRenderTargetView(m_pColorTex3D2, &descCT3D2RTV, &m_pColorTex3D2RTV));

	return S_OK;
}

HRESULT Diffusion::InitShaders()
{
	HRESULT hr;

	assert(m_pDiffusionEffect);
	SAFE_RELEASE(m_pInputLayout);

	// Get Technique and variables
	/*m_pVoronoiDiagramTechnique	= m_pDiffusionEffect->GetTechniqueByName("GenerateVoronoiDiagram");
	m_pFlatTo3DTexTechnique		= m_pDiffusionEffect->GetTechniqueByName("Flat2DTextureTo3D");

	m_pModelViewProjectionVar	= m_pDiffusionEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();
	m_pNormalMatrixVar			= m_pDiffusionEffect->GetVariableByName("NormalMatrix")->AsMatrix();
	m_pSliceIndexVar			= m_pDiffusionEffect->GetVariableByName("iSliceIndex")->AsScalar();
	m_pIsoValueVar				= m_pDiffusionEffect->GetVariableByName("fIsoValue")->AsScalar();
	m_pTextureSizeVar			= m_pDiffusionEffect->GetVariableByName("vTextureSize")->AsVector();
	m_pBBMinVar					= m_pDiffusionEffect->GetVariableByName("vBBMin")->AsVector();
	m_pBBMaxVar					= m_pDiffusionEffect->GetVariableByName("vBBMax")->AsVector();
	m_pFlatColorTex2DSRVar		= m_pDiffusionEffect->GetVariableByName("flatColorTexture")->AsShaderResource();
	m_pFlatDistTex2DSRVar		= m_pDiffusionEffect->GetVariableByName("flatDistTexture")->AsShaderResource();

	assert(m_pVoronoiDiagramTechnique);
	assert(m_pModelViewProjectionVar);
	assert(m_pSliceIndexVar);
	assert(m_pTextureSizeVar);
	assert(m_pBBMinVar);
	assert(m_pBBMaxVar);
	assert(m_pFlatColorTex2DSRVar);
	assert(m_pFlatDistTex2DSRVar);

	//Create InputLayout
	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pVoronoiDiagramTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));
	*/
	return S_OK;
}

HRESULT Diffusion::InitSlices()
{
	HRESULT hr;

	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pSlicesVB);

	//Create full-screen quad input layout
	/*const D3D11_INPUT_ELEMENT_DESC slicesLayout[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pFlatTo3DTexTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;
	V_RETURN(m_pd3dDevice->CreateInputLayout(slicesLayout, _countof(slicesLayout), vsCodePtr, vsCodeLen, &m_pSlicesLayout));


#define SLICEQUAD_VERTEX_COUNT 6
	// Create a vertex buffers of quads, one per slice, with texcoords to lookup from a flat 3D texture
    // and with homogenous coordinates to cover a fullscreen quad
	SLICE_SCREENQUAD_VERTEX* sliceVertices = new SLICE_SCREENQUAD_VERTEX[SLICEQUAD_VERTEX_COUNT*m_iTextureDepth];
	SLICE_SCREENQUAD_VERTEX sliceVerticesTemp[4];
	int row, col;
	float x1, y1, x2, y2;
	int vertexIndex = 0;

	for(int z = 0; z < m_iTextureDepth; z++)
	{
		row = z / m_cols;
		col = z % m_cols;
		x1 = float(col)/m_cols;
		y1 = float(row)/m_rows;
		x2 = float(col+1)/m_cols;
		y2 = float(row+1)/m_rows;
		
		vertexIndex = z * SLICEQUAD_VERTEX_COUNT;

		sliceVerticesTemp[0].pos = D3DXVECTOR3(-1.0f, 1.0f, 0.5f);
		sliceVerticesTemp[0].tex = D3DXVECTOR3(x1, y2, float(z));

		sliceVerticesTemp[1].pos = D3DXVECTOR3(-1.0f, -1.0f, 0.5f);
		sliceVerticesTemp[1].tex = D3DXVECTOR3(x1, y1, float(z));
        
        sliceVerticesTemp[2].pos = D3DXVECTOR3(1.0f, -1.0f, 0.5f);
		sliceVerticesTemp[2].tex = D3DXVECTOR3(x2, y1, float(z));
        
        sliceVerticesTemp[3].pos = D3DXVECTOR3(1.0f, 1.0f, 0.5f);
		sliceVerticesTemp[3].tex = D3DXVECTOR3(x2, y2, float(z));

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
	*/
	return S_OK;
}

HRESULT Diffusion::RenderDiffusion(int iDiffusionSteps)
{
	HRESULT hr(S_OK);

	//store the old render targets and viewports
    ID3D11RenderTargetView* pOldRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pOldDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	m_pd3dImmediateContext->RSGetViewports( &NumViewports, &pViewports[0]);

	//TODO DRAWING
	
	//restore old render targets
	m_pd3dImmediateContext->OMSetRenderTargets( 1,  &pOldRTV,  pOldDSV );
	m_pd3dImmediateContext->RSSetViewports( NumViewports, &pViewports[0]);

	return S_OK;
}


void Diffusion::DrawSlices()
{
	assert(m_pInputLayout);
	assert(m_pSlicesVB);

	/*UINT strides = sizeof(SLICE_SCREENQUAD_VERTEX);
	UINT offsets = 0;

	m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout);
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pSlicesVB, &strides, &offsets);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_pd3dImmediateContext->Draw(SLICEQUAD_VERTEX_COUNT*m_iTextureDepth, 0);*/
}

