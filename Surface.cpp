// Include the OS headers
//-----------------------
#include <windows.h>
#include <atlbase.h>
#pragma warning( disable: 4996 )
#include <strsafe.h>
#pragma warning( default: 4996 )
// Include the D3D11 headers
//--------------------------
#include <d3d11.h>
#include <d3dx11.h>

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "Surface.h"


Surface::Surface()
{
	D3DXMatrixIdentity(&m_mModel);
	D3DXMatrixIdentity(&m_mRot);
	D3DXMatrixIdentity(&m_mTrans);
	D3DXMatrixIdentity(&m_mTransInv);
	m_iBindPerFrame = 0;

	m_translation = D3DXVECTOR3(0.0, 0.0, 0.0);

	m_xAxis = new D3DXVECTOR3(1.0, 0.0, 0.0);
	m_yAxis = new D3DXVECTOR3(0.0, 1.0, 0.0);
	m_zAxis = new D3DXVECTOR3(0.0, 0.0, 1.0);
}


Surface::~Surface()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pcbPerFrame);
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
	D3DXMatrixRotationAxis(&mRot, m_xAxis, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}
void Surface::RotateY(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationAxis(&mRot, m_yAxis, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}
void Surface::RotateZ(float fFactor)
{
	D3DXMATRIX mRot;
	D3DXMatrixRotationAxis(&mRot, m_zAxis, fFactor);

	m_mModel *= m_mTransInv * mRot * m_mTrans;
}

void Surface::Scale(float fFactor)
{
	D3DXMATRIX mScale;
	D3DXMatrixScaling(&mScale, fFactor, fFactor, fFactor);

	m_mModel *= mScale;
}

HRESULT Surface::InitBuffers(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	HRESULT hr;

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
	V_RETURN(pd3dDevice->CreateBuffer(&vbd, &vertexData, &m_pVertexBuffer));

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
	V_RETURN(pd3dDevice->CreateBuffer(&ibd, &indexData, &m_pIndexBuffer));


	// Create constant buffers
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;

    Desc.ByteWidth = sizeof( CB_PER_FRAME_CONSTANTS );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &m_pcbPerFrame ) );
    DXUT_SetDebugName( m_pcbPerFrame, "CB_PER_FRAME_CONSTANTS" );

	return S_OK;

}

void Surface::Render(ID3D11DeviceContext* pd3dImmediateContext, D3DXMATRIX mViewProjection)
{
	D3DXMATRIX mModelViewProjection = m_mModel * mViewProjection;
	
	// Update per-frame variables
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map( m_pcbPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
    CB_PER_FRAME_CONSTANTS* pData = ( CB_PER_FRAME_CONSTANTS* )MappedResource.pData;

    D3DXMatrixTranspose( &pData->mModelViewProjection, &mModelViewProjection );

    pd3dImmediateContext->Unmap( m_pcbPerFrame, 0 );

	pd3dImmediateContext->VSSetConstantBuffers(m_iBindPerFrame, 1, &m_pcbPerFrame);

	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pd3dImmediateContext->DrawIndexed(m_iNum, 0, 0);
}

bool stringStartsWith(const char *s, const char *val)
{
        return !strncmp(s, val, strlen(val));
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
	m_vNum = atof(token);
	sprintf(c, "m_vNum: %d \n", m_vNum);//test
	fputs(c, F_out);//test
	while (!stringStartsWith(token, "nb_indices="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_iNum = atof(token);
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
		m_pVertices[i].x = atof(token);
		while (!stringStartsWith(token, "y="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].y = atof(token);
		while (!stringStartsWith(token, "z="))
				token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pVertices[i].z = atof(token);
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
		m_pIndices[i] = atof(token);
		
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
	color.r = atof(token);
	while (!stringStartsWith(token, "g="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	color.g = atof(token);
	while (!stringStartsWith(token, "b="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	color.b = atof(token);
	fgets(buff, 255, F);
	
	sprintf(c, "color=(%g,%g,%g) \n", color.r, color.g, color.b);
	fputs(c, F_out);

	for(int i=0; i < m_vNum; i++)
	{
		m_pVertices[i].color = color;

		sprintf(c, "vertex[%d]=(%g,%g,%g) color=(%g,%g,%g) \n", i, m_pVertices[i].x, m_pVertices[i].y, m_pVertices[i].z, m_pVertices[i].color.r, m_pVertices[i].color.g, m_pVertices[i].color.b);
		fputs(c, F_out);
	}
	
	fclose(F);
	fclose(F_out);
}

