#include "Globals.h"

#include "Surface.h"


Surface::Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pSurfaceEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pSurfaceEffect = pSurfaceEffect;

	m_pVertexBuffer = NULL;

	D3DXMatrixIdentity(&m_mModel);
	D3DXMatrixIdentity(&m_mRot);
	D3DXMatrixIdentity(&m_mTrans);
	D3DXMatrixIdentity(&m_mTransInv);

	m_translation = D3DXVECTOR3(0.0, 0.0, 0.0);
}


Surface::~Surface()
{
	SAFE_DELETE(m_pVertices);

	SAFE_RELEASE(m_pInputLayout);

	SAFE_RELEASE(m_pVertexBuffer);
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

void Surface::Rotate(D3DXVECTOR3 axis, float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationAxis(&mRot, &axis, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
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
	for(int i = 0; i < m_iNumVertices; i++)
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

	m_pTechnique = m_pSurfaceEffect->GetTechniqueByName("RenderColor");
	m_pNormalTechnique = m_pSurfaceEffect->GetTechniqueByName("RenderNormals");
	m_pModelViewProjectionVar = m_pSurfaceEffect->GetVariableByName("ModelViewProjectionMatrix")->AsMatrix();
	m_pNormalMatrixVar = m_pSurfaceEffect->GetVariableByName("NormalMatrix")->AsMatrix();

	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pTechnique->GetPassByIndex(0)->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
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
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	
	D3DX11_TECHNIQUE_DESC techDesc;
	m_pTechnique->GetDesc(&techDesc);

	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//apply technique
		m_pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);
				
		//draw
		m_pd3dImmediateContext->Draw(m_iNumVertices, 0);
	}
}

void Surface::Render(ID3DX11EffectTechnique* pTechnique)
{
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DX11_TECHNIQUE_DESC techDesc;
	pTechnique->GetDesc(&techDesc);

	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//apply technique
		pTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);

		//draw
		m_pd3dImmediateContext->Draw(m_iNumVertices, 0);
	}
}

void Surface::RenderVoronoi(ID3DX11EffectTechnique* pTechnique)
{
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//apply triangle technique & draw
	//pTechnique->GetPassByName("Triangle")->Apply( 0, m_pd3dImmediateContext);
	//m_pd3dImmediateContext->Draw(m_iNumVertices, 0);
	
	//apply line technique & draw
	pTechnique->GetPassByName("Edge")->Apply( 0, m_pd3dImmediateContext);
	m_pd3dImmediateContext->Draw(m_iNumVertices, 0);

	//apply point technique & draw
	//pTechnique->GetPassByName("Point")->Apply( 0, m_pd3dImmediateContext);
	//m_pd3dImmediateContext->Draw(m_iNumVertices, 0);
}

void Surface::RenderNormals(D3DXMATRIX mViewProjection)
{
	D3DXMATRIX mModelViewProjection = m_mModel * mViewProjection;
	D3DXMATRIX mModel3x3 = D3DXMATRIX(m_mModel._11, m_mModel._12, m_mModel._13, 0.0f, m_mModel._21, m_mModel._22, m_mModel._23, 0.0f, m_mModel._31, m_mModel._32, m_mModel._33, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	D3DXMATRIX mModel3x3Inv, mNormalMatrix;
	D3DXMatrixInverse(&mModel3x3Inv, NULL, &mModel3x3);
	D3DXMatrixTranspose(&mNormalMatrix, &mModel3x3Inv);

	m_pModelViewProjectionVar->SetMatrix(reinterpret_cast<float*>(&mModelViewProjection));
	m_pNormalMatrixVar->SetMatrix(reinterpret_cast<float*>(&mNormalMatrix));

	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetInputLayout(m_pInputLayout);
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	
	
	D3DX11_TECHNIQUE_DESC techDesc;
	m_pNormalTechnique->GetDesc(&techDesc);

	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		//apply technique
		m_pNormalTechnique->GetPassByIndex( p )->Apply( 0, m_pd3dImmediateContext);
				
		//draw
		m_pd3dImmediateContext->Draw(m_iNumVertices, 0);
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

	//Create Vertex buffer
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(VERTEX) * m_iNumVertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = m_pVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	V_RETURN(m_pd3dDevice->CreateBuffer(&vbd, &vertexData, &m_pVertexBuffer));

	return S_OK;
}

void Surface::ReadVectorFile(char *s)
{
	char buff[256];
	char *token;

	FILE *F = fopen(s, "rb");

	while (fgets(buff, 255, F))
		if (stringStartsWith(buff, "<!DOCTYPE SurfaceXML"))
		{
			break;
		}
	fgets(buff, 255, F);
	token = strtok(buff, " \"\t");
	while (!stringStartsWith(token, "nb_vertices="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_iNumVertices = int(atof(token));
	
	m_pVertices = new VERTEX[m_iNumVertices];
	for(int i=0; i < m_iNumVertices; i++)
	{
		while(!stringStartsWith(buff, "  <vertex "))
			fgets(buff, 255, F);
		token = strtok(buff, " \"\t");
		while (!stringStartsWith(token, "x="))
				token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].pos.x = float(atof(token));
		while (!stringStartsWith(token, "y="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].pos.y = float(atof(token));
		while (!stringStartsWith(token, "z="))
				token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].pos.z = float(atof(token));
		fgets(buff, 255, F);
	}

	for(int i=0; i < m_iNumVertices; i++)
	{
		while(!stringStartsWith(buff, "  <normal "))
			fgets(buff, 255, F);
		token = strtok(buff, " \"\t");
		while (!stringStartsWith(token, "x="))
				token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].normal.x = float(atof(token));
		while (!stringStartsWith(token, "y="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].normal.y = float(atof(token));
		while (!stringStartsWith(token, "z="))
				token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].normal.z = float(atof(token));
		fgets(buff, 255, F);
		
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
	
	for(int i=0; i < m_iNumVertices; i++)
	{
		m_pVertices[i].color = color;
	}
	
	fclose(F);
}

