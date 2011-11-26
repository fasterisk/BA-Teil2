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
#include "BoundingBox.h"


BoundingBox::BoundingBox(Surface* pSurface1, Surface* pSurface2)
{
	m_pSurface1 = pSurface1;
	m_pSurface2 = pSurface2;
	m_iBindPerFrame = 0;
}

BoundingBox::~BoundingBox()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pcbPerFrame);
}


HRESULT BoundingBox::InitBuffers(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext)
{
	HRESULT hr;

	FILE *F_out = fopen("Media\\test_CreateBoundingBox.txt", "w");
	char c[256];

	m_pVertices = new VERTEX[8];
	VERTEX min, max;
	for(int i = 0; i < m_pSurface1->m_vNum; i++)
	{
		VERTEX temp = m_pSurface1->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface1->m_mModel);
		temp.x = mul.x;
		temp.y = mul.y;
		temp.z = mul.z;
		if(i == 0)
		{
			min = temp;
			max = temp;
		}

		if(temp.x < min.x)
			min.x = temp.x;
		if(temp.y < min.y)
			min.y = temp.y;
		if(temp.z < min.z)
			min.z = temp.z;
		if(temp.x > max.x)
			max.x = temp.x;
		if(temp.y > max.y)
			max.y = temp.y;
		if(temp.z > max.z)
			max.z = temp.z;
	}

	for(int i = 0; i < m_pSurface2->m_vNum; i++)
	{
		VERTEX temp = m_pSurface2->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface2->m_mModel);
		temp.x = mul.x;
		temp.y = mul.y;
		temp.z = mul.z;

		if(temp.x < min.x)
			min.x = temp.x;
		if(temp.y < min.y)
			min.y = temp.y;
		if(temp.z < min.z)
			min.z = temp.z;
		if(temp.x > max.x)
			max.x = temp.x;
		if(temp.y > max.y)
			max.y = temp.y;
		if(temp.z > max.z)
			max.z = temp.z;
	}

	m_pVertices[0].x = min.x;
	m_pVertices[0].y = min.y;
	m_pVertices[0].z = min.z;
	m_pVertices[0].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[1].x = max.x;
	m_pVertices[1].y = min.y;
	m_pVertices[1].z = min.z;
	m_pVertices[1].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[2].x = max.x;
	m_pVertices[2].y = max.y;
	m_pVertices[2].z = min.z;
	m_pVertices[2].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[3].x = min.x;
	m_pVertices[3].y = max.y;
	m_pVertices[3].z = min.z;
	m_pVertices[3].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[4].x = max.x;
	m_pVertices[4].y = min.y;
	m_pVertices[4].z = max.z;
	m_pVertices[4].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[5].x = min.x;
	m_pVertices[5].y = min.y;
	m_pVertices[5].z = max.z;
	m_pVertices[5].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[6].x = max.x;
	m_pVertices[6].y = max.y;
	m_pVertices[6].z = max.z;
	m_pVertices[6].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);
	m_pVertices[7].x = min.x;
	m_pVertices[7].y = max.y;
	m_pVertices[7].z = max.z;
	m_pVertices[7].color = D3DXCOLOR(1.0, 0.0, 0.0, 1.0);

	for(int i = 0; i < 8; i++)
	{
		sprintf(c, "vertex[%d]=(%g,%g,%g) color=(%g,%g,%g) \n", i, m_pVertices[i].x, m_pVertices[i].y, m_pVertices[i].z, m_pVertices[i].color.r, m_pVertices[i].color.g, m_pVertices[i].color.b);
		fputs(c, F_out);
	}

	//Create Vertex buffer
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(VERTEX) * 8;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = m_pVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	V_RETURN(pd3dDevice->CreateBuffer(&vbd, &vertexData, &m_pVertexBuffer));

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

	fclose(F_out);

	return S_OK;
}

HRESULT BoundingBox::UpdateVertexBuffer(ID3D11Device* pd3dDevice)
{
	HRESULT hr;

	m_pVertexBuffer->Release();
	
	VERTEX min, max;
	for(int i = 0; i < m_pSurface1->m_vNum; i++)
	{
		VERTEX temp = m_pSurface1->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface1->m_mModel);
		temp.x = mul.x;
		temp.y = mul.y;
		temp.z = mul.z;
		if(i == 0)
		{
			min = temp;
			max = temp;
		}

		if(temp.x < min.x)
			min.x = temp.x;
		if(temp.y < min.y)
			min.y = temp.y;
		if(temp.z < min.z)
			min.z = temp.z;
		if(temp.x > max.x)
			max.x = temp.x;
		if(temp.y > max.y)
			max.y = temp.y;
		if(temp.z > max.z)
			max.z = temp.z;
	}

	for(int i = 0; i < m_pSurface2->m_vNum; i++)
	{
		VERTEX temp = m_pSurface2->m_pVertices[i];
		D3DXVECTOR4 mul;
		D3DXVec4Transform(&mul, &D3DXVECTOR4(temp.x, temp.y, temp.z, 1.0), &m_pSurface2->m_mModel);
		temp.x = mul.x;
		temp.y = mul.y;
		temp.z = mul.z;
		if(temp.x < min.x)
			min.x = temp.x;
		if(temp.y < min.y)
			min.y = temp.y;
		if(temp.z < min.z)
			min.z = temp.z;
		if(temp.x > max.x)
			max.x = temp.x;
		if(temp.y > max.y)
			max.y = temp.y;
		if(temp.z > max.z)
			max.z = temp.z;
	}

	m_pVertices[0].x = min.x;
	m_pVertices[0].y = min.y;
	m_pVertices[0].z = min.z;
	m_pVertices[1].x = max.x;
	m_pVertices[1].y = min.y;
	m_pVertices[1].z = min.z;
	m_pVertices[2].x = max.x;
	m_pVertices[2].y = max.y;
	m_pVertices[2].z = min.z;
	m_pVertices[3].x = min.x;
	m_pVertices[3].y = max.y;
	m_pVertices[3].z = min.z;
	m_pVertices[4].x = max.x;
	m_pVertices[4].y = min.y;
	m_pVertices[4].z = max.z;
	m_pVertices[5].x = min.x;
	m_pVertices[5].y = min.y;
	m_pVertices[5].z = max.z;
	m_pVertices[6].x = max.x;
	m_pVertices[6].y = max.y;
	m_pVertices[6].z = max.z;
	m_pVertices[7].x = min.x;
	m_pVertices[7].y = max.y;
	m_pVertices[7].z = max.z;

	//Create Vertex buffer
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(VERTEX) * 8;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = m_pVertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	V_RETURN(pd3dDevice->CreateBuffer(&vbd, &vertexData, &m_pVertexBuffer));

	return S_OK;
}

void BoundingBox::Render(ID3D11DeviceContext* pd3dImmediateContext, D3DXMATRIX mViewProjection)
{
	// Update per-frame variables
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map( m_pcbPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
    CB_PER_FRAME_CONSTANTS* pData = ( CB_PER_FRAME_CONSTANTS* )MappedResource.pData;

    D3DXMatrixTranspose( &pData->mModelViewProjection, &mViewProjection );

    pd3dImmediateContext->Unmap( m_pcbPerFrame, 0 );

	pd3dImmediateContext->VSSetConstantBuffers(m_iBindPerFrame, 1, &m_pcbPerFrame);

	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pd3dImmediateContext->DrawIndexed(36, 0, 0);
}