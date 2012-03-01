#ifndef INC_GLOBALS_H
#define INC_GLOBALS_H


// Include the OS headers
//-----------------------
#include <windows.h>
#include <atlbase.h>
#include <strsafe.h>

#pragma warning(disable: 4995)
#pragma warning(disable: 4996)
#pragma warning(disable: 4238)

// Include the D3D11 and effects headers
//--------------------------
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

struct SCREENQUAD_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR2 tex;
};

struct SLICE_SCREENQUAD_VERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 tex;
};

struct VERTEX
{
	D3DXVECTOR3 pos;
	D3DXCOLOR color;
};


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



#endif
