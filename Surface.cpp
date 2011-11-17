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

	// Create constant buffers
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;

    Desc.ByteWidth = sizeof( CB_VS_PER_OBJECT );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &m_pcbVSPerObject) );
    DXUT_SetDebugName( m_pcbVSPerObject, "CB_VS_PER_OBJECT" );

	/*Desc.ByteWidth = sizeof( CB_PS_PER_OBJECT );
    V_RETURN( pd3dDevice->CreateBuffer( &Desc, NULL, &m_pcbPSPerObject) );
    DXUT_SetDebugName( m_pcbPSPerObject, "CB_PS_PER_OBJECT" );
	
	D3D11_BUFFER_DESC vbDesc1;
    ZeroMemory( &vbDesc1, sizeof(D3D11_BUFFER_DESC) );
	vbDesc1.ByteWidth = sizeof(VERTEX) * m_vNum;
    vbDesc1.Usage = D3D11_USAGE_DEFAULT;
    vbDesc1.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbInitData1;
    ZeroMemory( &vbInitData1, sizeof(vbInitData1) );
    vbInitData1.pSysMem = m_pVertices;
    V_RETURN( pd3dDevice->CreateBuffer( &vbDesc1, &vbInitData1, &m_vertexbuffer ) );
    DXUT_SetDebugName( m_vertexbuffer, "Vertexbuffer" );

	D3D11_BUFFER_DESC vbDesc2;
    ZeroMemory( &vbDesc2, sizeof(D3D11_BUFFER_DESC) );
	vbDesc1.ByteWidth = sizeof(int) * m_iNum;
    vbDesc1.Usage = D3D11_USAGE_DEFAULT;
    vbDesc1.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbInitData2;
    ZeroMemory( &vbInitData2, sizeof(vbInitData2) );
    vbInitData2.pSysMem = m_pIndices;
    V_RETURN( pd3dDevice->CreateBuffer( &vbDesc2, &vbInitData2, &m_indexbuffer) );
	DXUT_SetDebugName( m_indexbuffer, "Indexbuffer" );
	*/
	V_RETURN( m_Mesh11.Create( pd3dDevice, L"Media\\Teapot.sdkmesh", true ) );


}

void Surface::Render(ID3D11DeviceContext* pd3dImmediateContext, D3DXMATRIX mViewProjection, D3DXVECTOR3 vCamEye, float fSubdivs)
{
	D3DXMATRIX mModelViewProjection = m_mModel * mViewProjection;

	
	
	/*
	// Render the meshes
    // Bind all of the CBs
    pd3dImmediateContext->VSSetConstantBuffers( m_iCBVSPerObjectBind, 1, &m_pcbVSPerObject);


	UINT Stride = sizeof( VERTEX );
    UINT Offset = 0;

	// Draw
	pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_vertexbuffer, &Stride, &Offset );
	pd3dImmediateContext->IASetIndexBuffer(m_indexbuffer, DXGI_FORMAT_D16_UNORM, Offset);
	pd3dImmediateContext->Draw( m_vNum, 0 );
	*/

	//Get the mesh
    //IA setup
    UINT Strides[1];
    UINT Offsets[1];
    ID3D11Buffer* pVB[1];
    pVB[0] = m_Mesh11.GetVB11( 0, 0 );
    Strides[0] = ( UINT )m_Mesh11.GetVertexStride( 0, 0 );
    Offsets[0] = 0;
    pd3dImmediateContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
    pd3dImmediateContext->IASetIndexBuffer( m_Mesh11.GetIB11( 0 ), m_Mesh11.GetIBFormat11( 0 ), 0 );

    // Update per-object variables
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    pd3dImmediateContext->Map( m_pcbVSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
    CB_VS_PER_OBJECT* pData = ( CB_VS_PER_OBJECT* )MappedResource.pData;

    D3DXMatrixTranspose( &pData->m_mModelViewProj, &mModelViewProjection );
	
    pd3dImmediateContext->Unmap( m_pcbVSPerObject, 0 );


    pd3dImmediateContext->VSSetConstantBuffers( m_iCBVSPerObjectBind, 1, &m_pcbVSPerObject );

    //Render
    SDKMESH_SUBSET* pSubset = NULL;
    D3D11_PRIMITIVE_TOPOLOGY PrimType;

    for( UINT subset = 0; subset < m_Mesh11.GetNumSubsets( 0 ); ++subset )
    {
        // Get the subset
        pSubset = m_Mesh11.GetSubset( 0, subset );

        PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
        pd3dImmediateContext->IASetPrimitiveTopology( PrimType );

        // TODO: D3D11 - material loading
        ID3D11ShaderResourceView* pDiffuseRV = m_Mesh11.GetMaterial( pSubset->MaterialID )->pDiffuseRV11;
        pd3dImmediateContext->PSSetShaderResources( 0, 1, &pDiffuseRV );

        pd3dImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, 0, ( UINT )pSubset->VertexStart );
    }
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
	D3DXVECTOR3 maxBound = D3DXVECTOR3(-1000000,-1000000,-1000000);
	D3DXVECTOR3 minBound = D3DXVECTOR3(1000000,1000000,1000000);
	
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
	
	m_pIndices = new int[m_iNum];
	for (int i = 0; i < m_iNum; i++)
	{
		while (!stringStartsWith(buff, "  <index "))
			fgets(buff, 255, F);
		token = strtok(buff, " \"\t");
		while (!stringStartsWith(token, "value="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_pIndices[i] = atof(token);

		sprintf(c, "index[%d]=%d \n", i, m_pIndices[i]);
		fputs(c, F_out);
	}
	
	fclose(F);
	fclose(F_out);
}

