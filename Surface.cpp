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
	m_pcbPSPerObject = NULL;
	m_pcbVSPerObject = NULL;
	m_iCBVSPerObjectBind = 0;
	m_iCBPSPerObjectBind = 0;
	D3DXMatrixIdentity(&m_mModel);
	D3DXMatrixIdentity(&m_mRot);
	D3DXMatrixIdentity(&m_mTrans);
	D3DXMatrixIdentity(&m_mTransInv);

	m_translation = D3DXVECTOR3(0.0, 0.0, 0.0);

	m_xAxis = new D3DXVECTOR3(1.0, 0.0, 0.0);
	m_yAxis = new D3DXVECTOR3(0.0, 1.0, 0.0);
	m_zAxis = new D3DXVECTOR3(0.0, 0.0, 1.0);
}


Surface::~Surface()
{
	SAFE_RELEASE(m_vertexbuffer);
	SAFE_RELEASE(m_pcbVSPerObject);
	SAFE_RELEASE(m_pcbPSPerObject);
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

HRESULT Surface::InitBuffers(ID3D11Device* pd3dDevice)
{
	HRESULT hr;

	return S_OK;

}

void Surface::Render(ID3D11DeviceContext* pd3dImmediateContext, D3DXMATRIX mViewProjection, D3DXVECTOR3 vCamEye, float fSubdivs)
{
	
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
		
		sprintf(c, "vertex[%d]=(%g,%g,%g) \n", i, m_pVertices[i].x, m_pVertices[i].y, m_pVertices[i].z);
		fputs(c, F_out);

	}
	
	fclose(F);
	fclose(F_out);
}

