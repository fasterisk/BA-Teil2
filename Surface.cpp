#include "Globals.h"

#include "Surface.h"


Surface::Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pSurfaceEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pSurfaceEffect = pSurfaceEffect;

	m_pVertexBuffer = NULL;
	m_pIndexBuffer = NULL;

	D3DXMatrixIdentity(&m_mModel);
	D3DXMatrixIdentity(&m_mRot);
	D3DXMatrixIdentity(&m_mTrans);
	D3DXMatrixIdentity(&m_mTransInv);

	m_translation = D3DXVECTOR3(0.0, 0.0, 0.0);
}


Surface::~Surface()
{
	SAFE_DELETE(m_pVertices);
	SAFE_DELETE(m_pIndices);

	SAFE_RELEASE(m_pInputLayout);

	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);

	
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

void Surface::SetColor(float fR, float fG, float fB)
{
	for(int i = 0; i < m_vNum; i++)
	{
		m_pVertices[i].color = D3DXCOLOR(fR, fG, fB, 1.0);
	}

	InitBuffers();
}

HRESULT Surface::Initialize(char* s)
{
	HRESULT hr;

	ReadVectorFile(s);

	InitBuffers();

	m_pTechnique = m_pSurfaceEffect->GetTechniqueByName("RenderColorAndDepth");
	m_pModelViewProjectionVar = m_pSurfaceEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();

	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

	V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &m_pInputLayout));
	

	return S_OK;
}

void Surface::Render(D3DXMATRIX mViewProjection)
{
	D3DXMATRIX mModelViewProjection = m_mModel * mViewProjection;
	
	m_pModelViewProjectionVar->SetMatrix(reinterpret_cast<float*>(&mModelViewProjection));

	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout);
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	
	D3DX11_TECHNIQUE_DESC techDesc;
	m_pTechnique->GetDesc(&techDesc);

	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//apply technique
		m_pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);
				
		//draw
		m_pd3dImmediateContext->DrawIndexed(36, 0, 0);
	}
}

void Surface::Render(ID3DX11EffectTechnique* pTechnique)
{
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DX11_TECHNIQUE_DESC techDesc;
	pTechnique->GetDesc(&techDesc);

	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//apply technique
		pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);

		//draw
		m_pd3dImmediateContext->DrawIndexed( 36, 0, 0 );
	}
}

bool stringStartsWith(const char *s, const char *val)
{
        return !strncmp(s, val, strlen(val));
}

HRESULT Surface::InitBuffers()
{
	HRESULT hr;

	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);

	//Create Vertex buffer
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(VERTEX) * m_vNum;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = m_pVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	V_RETURN(m_pd3dDevice->CreateBuffer(&vbd, &vertexData, &m_pVertexBuffer));

	//Create Index buffer
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_DYNAMIC;
	ibd.ByteWidth = sizeof(unsigned int) * m_iNum;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = m_pIndices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;
	V_RETURN(m_pd3dDevice->CreateBuffer(&ibd, &indexData, &m_pIndexBuffer));


	return S_OK;
}

void Surface::ReadVectorFile(char *s)
{
	char buff[256];
	char *token;

	FILE *F = fopen(s, "rb");

	FILE *F_out = fopen("Media\\test_ReadVectorFile.txt", "w");
	char c[256];

	while (fgets(buff, 255, F))
		if (stringStartsWith(buff, "<!DOCTYPE SurfaceXML"))
		{
			fputs ("(INFO) : This seems to be a diffusion surface file.\n", F_out);//test
			break;
		}
	fgets(buff, 255, F);
	token = strtok(buff, " \"\t");
	while (!stringStartsWith(token, "nb_vertices="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_vNum = int(atof(token));
	sprintf(c, "m_vNum: %d \n", m_vNum);//test
	fputs(c, F_out);//test
	while (!stringStartsWith(token, "nb_indices="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_iNum = int(atof(token));
	sprintf(c, "m_iNum: %d \n", m_iNum);//test
	fputs(c, F_out);//test
	
	m_pVertices = new VERTEX[m_vNum];
	for(int i=0; i < m_vNum; i++)
	{
		while(!stringStartsWith(buff, "  <vertex "))
			fgets(buff, 255, F);
		token = strtok(buff, " \"\t");
		while (!stringStartsWith(token, "x="))
				token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].position.x = float(atof(token));
		while (!stringStartsWith(token, "y="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].position.y = float(atof(token));
		while (!stringStartsWith(token, "z="))
				token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].position.z = float(atof(token));
		fgets(buff, 255, F);
		
		
	}

	m_pIndices = new unsigned int[m_iNum];
	for(int i=0; i < m_iNum; i++)
	{
		while(!stringStartsWith(buff, "  <index "))
			fgets(buff, 255, F);
		token = strtok(buff, " \"\t");
		while (!stringStartsWith(token, "value="))
				token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pIndices[i] = unsigned int(atof(token));
		
		sprintf(c, "index[%d]=%d \n", i, m_pIndices[i]);
		fputs(c, F_out);
	}

	D3DXCOLOR color;
	while(!stringStartsWith(buff, " <color"))
		fgets(buff, 255, F);
	token = strtok(buff, " \"\t");
	while (!stringStartsWith(token, "r="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	color.r = float(atof(token));
	while (!stringStartsWith(token, "g="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	color.g = float(atof(token));
	while (!stringStartsWith(token, "b="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	color.b = float(atof(token));
	fgets(buff, 255, F);
	
	sprintf(c, "color=(%g,%g,%g) \n", color.r, color.g, color.b);
	fputs(c, F_out);

	for(int i=0; i < m_vNum; i++)
	{
		//m_pVertices[i].color = color;

	//	sprintf(c, "vertex[%d]=(%g,%g,%g) color=(%g,%g,%g) \n", i, m_pVertices[i].position.x, m_pVertices[i].position.y, m_pVertices[i].position.z, m_pVertices[i].color.r, m_pVertices[i].color.g, m_pVertices[i].color.b);
		fputs(c, F_out);
	}
	
	fclose(F);
	fclose(F_out);
}

