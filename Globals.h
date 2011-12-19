#ifndef INC_GLOBALS_H
#define INC_GLOBALS_H


// Include the OS headers
//-----------------------
#include <windows.h>
#include <atlbase.h>
#pragma warning( disable: 4996 )
#include <strsafe.h>
#pragma warning( default: 4996 )

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

extern bool			g_useFire;
extern int			g_Width;
extern int			g_Height;
extern float		g_zNear;
extern float		g_zFar;
extern bool			g_renderGlow;
extern float		g_glowContribution;
extern float		g_finalIntensityScale; 
extern float		g_finalAlphaScale;
extern float		g_smokeColorMultiplier;   
extern float		g_smokeAlphaMultiplier; 
extern int			g_RednessFactor; 
extern float		g_xyVelocityScale;
extern float		g_zVelocityScale;
extern D3DXMATRIX	g_View;
extern D3DXMATRIX	g_Projection;
extern float        g_Fovy;

extern ID3D11ShaderResourceView*	g_pSceneDepthSRV;

extern D3DXMATRIX                   g_gridWorld;

struct VERTEX
{
	float x, y, z;
	D3DXCOLOR color;
};

struct CB_PER_FRAME_CONSTANTS
{
    D3DXMATRIX mModelViewProjection;
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
