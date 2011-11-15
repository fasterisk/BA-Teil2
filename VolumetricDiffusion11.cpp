//--------------------------------------------------------------------------------------
// File: VolumetricDiffusion11.cpp 
// derived from SimpleBezier11.cpp
//
// This sample shows an simple implementation of the DirectX 11 Hardware Tessellator
// for rendering a Bezier Patch.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "resource.h"
#include "Surface.h"

const DWORD MIN_DIVS = 4;
const DWORD MAX_DIVS = 16; // Min and Max divisions of the patch per side for the slider control

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager          g_DialogResourceManager; // manager for shared resources of dialogs
CModelViewerCamera                  g_Camera;                // A model viewing camera
CD3DSettingsDlg                     g_D3DSettingsDlg;        // Device settings dialog
CDXUTDialog                         g_HUD;                   // manages the 3D   
CDXUTDialog                         g_SampleUI;              // dialog for sample specific controls

// Surfaces
Surface*							g_surface1;
Surface*							g_surface2;
Surface*							g_controlledSurface;
bool								g_surface1IsControlled = true;
int									g_mouseX = 0;
int									g_mouseY = 0;
bool								g_bRotatesWithMouse = true;

// Resources
CDXUTTextHelper*                    g_pTxtHelper = NULL;

ID3D11InputLayout*                  g_pPatchLayout = NULL;

ID3D11VertexShader*                 g_pVertexShader = NULL;
ID3D11HullShader*                   g_pHullShaderInteger = NULL;
ID3D11DomainShader*                 g_pDomainShader = NULL;
ID3D11PixelShader*                  g_pPixelShader = NULL;
ID3D11PixelShader*                  g_pSolidColorPS = NULL;

ID3D11Buffer*   g_pControlPointVB;                           // Control points for mesh

UINT                                g_iBindPerFrame = 0;

ID3D11RasterizerState*              g_pRasterizerStateSolid = NULL;
ID3D11RasterizerState*              g_pRasterizerStateWireframe = NULL;

// Control variables
float                               g_fSubdivs = 8;          // Startup subdivisions per side
bool                                g_bDrawWires = false;    // Draw the mesh with wireframe overlay

float								g_fElapsedTime = 0;

enum E_PARTITION_MODE
{
   PARTITION_INTEGER,
   PARTITION_FRACTIONAL_EVEN,
   PARTITION_FRACTIONAL_ODD
};

E_PARTITION_MODE                    g_iPartitionMode = PARTITION_INTEGER;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN      1
#define IDC_TOGGLEREF             3
#define IDC_CHANGEDEVICE          4

#define IDC_PATCH_SUBDIVS         5
#define IDC_PATCH_SUBDIVS_STATIC  6
#define IDC_TOGGLE_LINES          7

#define IDC_CHANGE_CONTROL		  8
#define IDC_ROTATE_OR_MOVE		  9
#define IDC_ROTATE				 10
#define IDC_MOVE				 11

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool	CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void	CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT	CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void    CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void	CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
bool	CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT	CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void	CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void	CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void	CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext );
void	CALLBACK OnMouseEvent( bool bLeftDown, bool bRightDown, bool bMiddleDown, bool bSide1Down, bool bSide2Down, int iWheelDelta, int iX, int iY, void* pUserContext);

void InitApp();
void RenderText();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D10 or D3D11) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
	DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
	bool includeMouseMoveEvents = true;
	DXUTSetCallbackMouse(OnMouseEvent, includeMouseMoveEvents);

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    InitApp();
    DXUTInit( true, true ); // Parse the command line, show msgboxes on error, and an extra cmd line param to force REF for now
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"SimpleBezier11" );
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0,  true, 800, 600 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

    g_HUD.SetCallback( OnGUIEvent ); int iY = 20;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 30 );
    g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += 26, 170, 30, VK_F3 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += 26, 170, 30, VK_F2 );
	g_HUD.AddButton( IDC_CHANGE_CONTROL, L"Change contr. surface", 0, iY += 52, 170, 30);
	g_HUD.AddRadioButton( IDC_ROTATE, IDC_ROTATE_OR_MOVE, L"Rotate", 0, iY += 26, 170, 30);
	g_HUD.AddRadioButton( IDC_MOVE, IDC_ROTATE_OR_MOVE, L"Move", 0, iY += 26, 170, 30);
	g_HUD.GetRadioButton( IDC_ROTATE )->SetChecked(true);

    g_SampleUI.SetCallback( OnGUIEvent ); iY = 10;

    WCHAR sz[100];
    iY += 24;
    swprintf_s( sz, L"Patch Divisions: %2.1f", g_fSubdivs );
    g_SampleUI.AddStatic( IDC_PATCH_SUBDIVS_STATIC, sz, 10, iY += 26, 150, 22 );
    g_SampleUI.AddSlider( IDC_PATCH_SUBDIVS, 10, iY += 24, 150, 22, 10 * MIN_DIVS, 10 * MAX_DIVS, (int)(g_fSubdivs * 10) );

    iY += 24;
    g_SampleUI.AddCheckBox( IDC_TOGGLE_LINES, L"Toggle Wires", 20, iY += 26, 150, 22, g_bDrawWires );

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye( 1.0f, 1.5f, -3.5f );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );

}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
    //g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	static int cVertex = 0;
	
	if( bKeyDown )
	{
		switch( nChar )
		{
		
		}
    }
}

//--------------------------------------------------------------------------------------
// Handle mouse
//--------------------------------------------------------------------------------------
void CALLBACK OnMouseEvent( bool bLeftDown, bool bRightDown, bool bMiddleDown, bool bSide1Down, bool bSide2Down, int iWheelDelta, int iX, int iY, void* pUserContext)
{
	if(g_mouseX == 0 && g_mouseY == 0)
	{
		g_mouseX = iX;
		g_mouseY = iY;
	}  

	if(bLeftDown)
	{
		if(g_bRotatesWithMouse)
		{
			g_controlledSurface->RotateX((g_mouseY-iY)*g_fElapsedTime*10);
			g_controlledSurface->RotateY((g_mouseX-iX)*g_fElapsedTime*10);
		}
		else
		{
			g_controlledSurface->Translate((iX-g_mouseX)*g_fElapsedTime*10, (g_mouseY-iY)*g_fElapsedTime*10, 0);
		}
	}
	g_mouseX = iX;
	g_mouseY = iY;
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
            // Standard DXUT controls
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;

            // Custom app controls
		case IDC_CHANGE_CONTROL:
			if(g_surface1IsControlled)
				g_controlledSurface = g_surface2;
			else
				g_controlledSurface = g_surface1;
			g_surface1IsControlled = !g_surface1IsControlled;
			break;
		case IDC_ROTATE:
			g_bRotatesWithMouse = true;
			break;
		case IDC_MOVE:
			g_bRotatesWithMouse = false;
			break;
        case IDC_PATCH_SUBDIVS:
        {
            g_fSubdivs = g_SampleUI.GetSlider( IDC_PATCH_SUBDIVS )->GetValue() / 10.0f;

            WCHAR sz[100];
            swprintf_s( sz, L"Patch Divisions: %2.1f", g_fSubdivs );
            g_SampleUI.GetStatic( IDC_PATCH_SUBDIVS_STATIC )->SetText( sz );
        }
            break;
        case IDC_TOGGLE_LINES:
            g_bDrawWires = g_SampleUI.GetCheckBox( IDC_TOGGLE_LINES )->GetChecked();
            break;
    }
}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}

//--------------------------------------------------------------------------------------
// Find and compile the specified shader
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( WCHAR* szFileName, D3D_SHADER_MACRO* pDefines, LPCSTR szEntryPoint, 
                               LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( str, pDefines, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
            OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    SAFE_RELEASE( pErrorBlob );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

    // Compile shaders
    ID3DBlob* pBlobVS = NULL;
    ID3DBlob* pBlobHSInt = NULL;
    ID3DBlob* pBlobDS = NULL;
    ID3DBlob* pBlobPS = NULL;
    ID3DBlob* pBlobPSSolid = NULL;

    V_RETURN( CompileShaderFromFile( L"DiffusionShader11.hlsl", NULL, "BezierVS", "vs_5_0",  &pBlobVS ) );
    V_RETURN( CompileShaderFromFile( L"DiffusionShader11.hlsl", NULL, "BezierHS", "hs_5_0", &pBlobHSInt ) );
    V_RETURN( CompileShaderFromFile( L"DiffusionShader11.hlsl", NULL, "BezierDS", "ds_5_0", &pBlobDS ) );
    V_RETURN( CompileShaderFromFile( L"DiffusionShader11.hlsl", NULL, "BezierPS", "ps_5_0", &pBlobPS ) );
    V_RETURN( CompileShaderFromFile( L"DiffusionShader11.hlsl", NULL, "SolidColorPS", "ps_5_0", &pBlobPSSolid ) );

    // Create shaders
    V_RETURN( pd3dDevice->CreateVertexShader( pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), NULL, &g_pVertexShader ) );
    DXUT_SetDebugName( g_pVertexShader, "BezierVS" );

    V_RETURN( pd3dDevice->CreateHullShader( pBlobHSInt->GetBufferPointer(), pBlobHSInt->GetBufferSize(), NULL, &g_pHullShaderInteger ) );
    DXUT_SetDebugName( g_pHullShaderInteger, "BezierHS" );

    V_RETURN( pd3dDevice->CreateDomainShader( pBlobDS->GetBufferPointer(), pBlobDS->GetBufferSize(), NULL, &g_pDomainShader ) );
    DXUT_SetDebugName( g_pDomainShader, "BezierDS" );

    V_RETURN( pd3dDevice->CreatePixelShader( pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), NULL, &g_pPixelShader ) );
    DXUT_SetDebugName( g_pPixelShader, "BezierPS" );

    V_RETURN( pd3dDevice->CreatePixelShader( pBlobPSSolid->GetBufferPointer(), pBlobPSSolid->GetBufferSize(), NULL, &g_pSolidColorPS ) );
    DXUT_SetDebugName( g_pSolidColorPS, "SolidColorPS" );

    // Create our vertex input layout - this matches the BEZIER_CONTROL_POINT structure
    const D3D11_INPUT_ELEMENT_DESC patchlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    V_RETURN( pd3dDevice->CreateInputLayout( patchlayout, ARRAYSIZE( patchlayout ), pBlobVS->GetBufferPointer(),
                                             pBlobVS->GetBufferSize(), &g_pPatchLayout ) );
    DXUT_SetDebugName( g_pPatchLayout, "Primary" );

    SAFE_RELEASE( pBlobVS );
    SAFE_RELEASE( pBlobHSInt );
    SAFE_RELEASE( pBlobDS );
    SAFE_RELEASE( pBlobPS );
    SAFE_RELEASE( pBlobPSSolid );

    // Create solid and wireframe rasterizer state objects
    D3D11_RASTERIZER_DESC RasterDesc;
    ZeroMemory( &RasterDesc, sizeof(D3D11_RASTERIZER_DESC) );
    RasterDesc.FillMode = D3D11_FILL_SOLID;
    RasterDesc.CullMode = D3D11_CULL_NONE;
    RasterDesc.DepthClipEnable = TRUE;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterDesc, &g_pRasterizerStateSolid ) );
    DXUT_SetDebugName( g_pRasterizerStateSolid, "Solid" );

    RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterDesc, &g_pRasterizerStateWireframe ) );
    DXUT_SetDebugName( g_pRasterizerStateWireframe, "Wireframe" );
	
	// Create surface1 and its vertex buffer
	g_surface1 = new Surface();
	g_surface1->ReadVectorFile("Media\\surface1.xml");
	g_surface1->InitBuffers(pd3dDevice);
    
	// Create surface2 and its vertex buffer
	g_surface2 = new Surface();
	g_surface2->ReadVectorFile("Media\\surface2.xml");
	g_surface2->InitBuffers(pd3dDevice);

	g_controlledSurface = g_surface1;
	
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 0.1f, 20.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );
	
    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300 );
    g_SampleUI.SetSize( 170, 300 );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext )
{
	g_fElapsedTime = fElapsedTime;

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
	}
	
	// Clear the render target and depth stencil
    float ClearColor[4] = { 0.05f, 0.05f, 0.05f, 0.0f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

	// Set state for solid rendering
    pd3dImmediateContext->RSSetState( g_pRasterizerStateSolid );

	// Optionally draw the wireframe
    if( g_bDrawWires )
    {
        pd3dImmediateContext->PSSetShader( g_pSolidColorPS, NULL, 0 );
        pd3dImmediateContext->RSSetState( g_pRasterizerStateWireframe ); 
    }

	// Set the shaders
    pd3dImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
    pd3dImmediateContext->HSSetShader( g_pHullShaderInteger, NULL, 0 );
    pd3dImmediateContext->DSSetShader( g_pDomainShader, NULL, 0 );
    pd3dImmediateContext->GSSetShader( NULL, NULL, 0 );
    pd3dImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );

	// Set the input assembler
    // This sample uses patches with 16 control points each
    // Although the Mobius strip only needs to use a vertex buffer,
    // you can use an index buffer as well by calling IASetIndexBuffer().
    pd3dImmediateContext->IASetInputLayout( g_pPatchLayout );
    

    // WVP
    D3DXMATRIX mViewProjection;
    D3DXMATRIX mProj = *g_Camera.GetProjMatrix();
    D3DXMATRIX mView = *g_Camera.GetViewMatrix();

    mViewProjection = mView * mProj;
	    
	// Draw surfaces   
	g_surface1->Render(pd3dImmediateContext, g_iBindPerFrame, mViewProjection, *(g_Camera.GetEyePt()), g_fSubdivs);
	g_surface2->Render(pd3dImmediateContext, g_iBindPerFrame, mViewProjection, *(g_Camera.GetEyePt()), g_fSubdivs);
	
	
    pd3dImmediateContext->RSSetState( g_pRasterizerStateSolid );

    // Render the HUD
    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_HUD.OnRender( fElapsedTime );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );
    SAFE_RELEASE( g_pPatchLayout );

    SAFE_RELEASE( g_pVertexShader );
    SAFE_RELEASE( g_pHullShaderInteger );
    SAFE_RELEASE( g_pDomainShader );
    SAFE_RELEASE( g_pPixelShader );
    SAFE_RELEASE( g_pSolidColorPS );

    SAFE_RELEASE( g_pRasterizerStateSolid );
    SAFE_RELEASE( g_pRasterizerStateWireframe );

	SAFE_DELETE(g_surface1);
	SAFE_DELETE(g_surface2);
}
