#include "Globals.h"
#include "VolumeRenderer.h"

struct VsInput
{
    D3DXVECTOR3 pos;  
};

VolumeRenderer::VolumeRenderer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pEffect = pEffect;

}

VolumeRenderer::~VolumeRenderer()
{
}

HRESULT VolumeRenderer::Initialize()
{
	HRESULT hr;
	V_RETURN(InitShader());
	

	V_RETURN(InitBoundingIndicesAndLayout());

	return S_OK;
}

void VolumeRenderer::Render(VERTEX* pBBVertices, ID3D11Texture3D* p3DTexture)
{
	UpdateBoundingVertices(pBBVertices);
}

HRESULT VolumeRenderer::InitShader()
{
	HRESULT hr;

	m_pVolumeRenderTechnique = m_pEffect->GetTechniqueByName("VolumeRendering");

	return S_OK;
}

HRESULT VolumeRenderer::InitBoundingIndicesAndLayout()
{
	HRESULT hr;

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

HRESULT VolumeRenderer::UpdateBoundingVertices(VERTEX* BBVertices)
{
	HRESULT hr;
	
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