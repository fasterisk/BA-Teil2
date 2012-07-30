#ifndef INC_GLOBALS_H
#define INC_GLOBALS_H

// Include the OS headers
#include <windows.h>
#include <atlbase.h>
#include <strsafe.h>

#pragma warning(disable: 4995)
#pragma warning(disable: 4996)
#pragma warning(disable: 4238)

// Include the D3D11 and effects headers
#include <d3d11.h>
#include <d3dx11.h>
#include "d3dx11effect.h"

// Include the DXUT headers
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"

#include <string>
#include <stdlib.h>

#include <stdio.h>
#include <iostream>
#include <sstream>

//vertex structure used in volume renderer
struct SCREENQUAD_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR2 tex;
};

//vertex structure used in the voronoi algorithm
struct SLICE_SCREENQUAD_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 tex;
};

//vertex structure used for storing the vertices of the surfaces
struct SURFACE_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR2 texcoord;
};

//vertex structure used in the diffusion algorithm
struct DIFFUSION_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 tex;
	int sliceindex;
};

//structure for the bounding box
struct BOUNDINGBOX
{
	D3DXVECTOR4 vMin;
	D3DXVECTOR4 vMax;
};

#define PI 3.1415926535897932384626433832795028841971693993751058

#define TEXTURE_FORMAT DXGI_FORMAT_R32G32B32A32_FLOAT

#define WIDEN( w ) WIDEN2( w )
#define WIDEN2( w )	L ##w

#define INFO_OUT( text ) OutputDebugString( L"(INFO) : " WIDEN( __FUNCTION__ ) L"() - " text L"\n" )
#define ERR_OUT( text ) OutputDebugString( L"(ERROR) : " WIDEN( __FUNCTION__ ) L"() - " text L"\n" )
#define WARN_OUT( text ) OutputDebugString( L"(WARNING) : " WIDEN( __FUNCTION__ ) L"() - " text L"\n" )

#ifndef SAFE_ACQUIRE
#define SAFE_ACQUIRE(dst, p)      { if(dst) { SAFE_RELEASE(dst); } if (p) { (p)->AddRef(); } dst = (p); }
#endif
#ifndef SAFE_RELEASE
	#define SAFE_RELEASE( p ) {if(p){(p)->Release();(p)=NULL;}}
#endif
#ifndef SAFE_DELETE
	#define SAFE_DELETE( p ) {if(p){delete(p);(p)=NULL;}}
#endif
#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY( p ) {if(p){delete[](p);(p)=NULL;}}
#endif

void ComputeSizeAsString( WCHAR *wc, UINT wcLen, SIZE_T bytes );

/*
 *	Computes the rows and cols for the flat 3D texture in the voronoi algorithm
 */
inline void ComputeRowColsForFlat3DTexture( int depth, int *outCols, int *outRows )
{
    // Compute # of rows and cols for a "flat 3D-texture" configuration
    // (in this configuration all the slices in the volume are spread in a single 2D texture)
    int rows =(int)floorf(sqrtf((float)depth));
    int cols = rows;
    while( rows * cols < depth ) {
        cols++;
    }
    assert( rows*cols >= depth );
    
    *outCols = cols;
    *outRows = rows;
}

/*
 *	converts std::string into a std::wstring
 */
inline std::wstring ConvertMultibyteToWideChar(std::string str)
{
	int size_needed = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)strlen(str.c_str()), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)strlen(str.c_str()), &wstrTo[0], 0);
	return wstrTo;
}

/*
 *	converts LPWSTR into a std::string
 */
inline std::string ConvertWideCharToChar(LPWSTR lpwstr)
{
	int size_needed = WideCharToMultiByte(CP_ACP, 0, lpwstr, (int)wcslen(lpwstr), NULL, 0, NULL, NULL);
    std::string strTo( size_needed, 0 );
    WideCharToMultiByte(CP_ACP, 0, lpwstr, (int)wcslen(lpwstr), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

/*
 *	checks if point lies in the bounding box
 */
inline bool CheckIfPointIsInBoundingBox(BOUNDINGBOX bb, D3DXVECTOR3 point)
{
	if(point.x > bb.vMin.x && point.x < bb.vMax.x &&
		point.y > bb.vMin.y && point.y < bb.vMax.y &&
		point.z > bb.vMin.z && point.z < bb.vMax.z)
		return true;
	else
		return false;
}


#endif
