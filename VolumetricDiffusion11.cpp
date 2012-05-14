#include "Globals.h"
#include "Scene.h"
#include "resource.h"
#include <string>

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CModelViewerCamera          g_Camera;               // A model viewing camera
CD3DSettingsDlg             g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls

// Scene
Scene*						g_pScene;

D3DXVECTOR3                 g_Eye = D3DXVECTOR3( 0.0f, 0.0f, -5.0f );
D3DXVECTOR3                 g_At = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
D3DXVECTOR3                 g_Up = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );

// Global variables
int							g_Width = 800;
int							g_Height = 600;
float						g_zNear = 0.1f;
float						g_zFar = 100.0f;
D3DXMATRIX					g_View;
D3DXMATRIX					g_Proj;

ID3D11Texture2D*            g_pSceneDepthTex2D      = NULL;
ID3D11Texture2D*            g_pSceneDepthTex2DNonMS = NULL;
ID3D11ShaderResourceView*   g_pSceneDepthSRV        = NULL;
ID3D11RenderTargetView*     g_pSceneDepthRTV        = NULL;

// Control parameters
int							g_mouseX = 0;
int							g_mouseY = 0;
int							g_mouseSpeed = 1;
bool						g_bRotatesWithMouse = true;
bool						g_bCameraActive = false;
float						g_fAspectRatio = 0;

float						g_fElapsedTime = 0;

int							g_iTextureWidth = 128;
int							g_iTextureHeight = 128;
int							g_iTextureDepth = 128;
int							g_iTextureMaximum = 128;
bool						g_bBlockMouseDragging = false;
int							g_iSliceIndex = 64;
bool						g_bShowSurfaces = true;
float						g_fIsoValue = 0.5f;
int							g_iDiffusionSteps = 8;

// Texthelper
CDXUTTextHelper*            g_pTxtHelper = NULL;

//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN		1
#define IDC_TOGGLEREF				2
#define IDC_CHANGEDEVICE			3

#define IDC_LOAD_SURFACE			4
#define IDC_CHANGE_CONTROL			5
#define IDC_ROTATE_MOVE_CAMERA		6
#define IDC_ROTATE					7
#define IDC_MOVE					8
#define IDC_CAMERA					9
#define IDC_TEXTRES_STATIC			10
#define IDC_TEXTRES_MAX_STATIC		11
#define IDC_TEXTRES_MAX_SLIDER		12
#define IDC_SLICES					13
#define IDC_ALL_SLICES				14
#define IDC_ONE_SLICE				15
#define IDC_SLICEINDEX_STATIC		16
#define IDC_SLICEINDEX_SLIDER		17
#define IDC_SHOW_SURFACES			18
#define IDC_DIFFUSION				19
#define IDC_ISO_SLIDER				20
#define IDC_ISO_SLIDER_STATIC		21
#define IDC_ISO_CHECK				22
#define IDC_DIFFSTEPS_STATIC		23
#define IDC_DIFFSTEPS_SLIDER		24

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnMouseEvent( bool bLeftDown, bool bRightDown, bool bMiddleDown, bool bSide1Down, bool bSide2Down, int iWheelDelta, int iX, int iY, void* pUserContext);

HRESULT ReinitWindowSizeDependentRenderTargets(ID3D11Device* pd3dDevice);


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

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
	DXUTSetCallbackMouse(OnMouseEvent, true);

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Volumetric Diffusion" );
	DXUTCreateDevice (D3D_FEATURE_LEVEL_10_0, true, g_Width, g_Height);
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
    g_SampleUI.Init( &g_DialogResourceManager );

	WCHAR sz[100];

    g_SampleUI.SetCallback( OnGUIEvent ); int iY = 10;
	g_SampleUI.AddButton( IDC_CHANGE_CONTROL, L"Change contr. surface", 0, iY, 170, 30);
	g_SampleUI.AddRadioButton( IDC_ROTATE, IDC_ROTATE_MOVE_CAMERA, L"Rotate & Scale", 0, iY += 40, 170, 22);
	g_SampleUI.AddRadioButton( IDC_MOVE, IDC_ROTATE_MOVE_CAMERA, L"Move", 0, iY += 26, 170, 22);
	g_SampleUI.AddRadioButton( IDC_CAMERA, IDC_ROTATE_MOVE_CAMERA, L"Camera", 0, iY += 26, 170, 22);
	g_SampleUI.GetRadioButton( IDC_ROTATE )->SetChecked(true);
	
	StringCchPrintf( sz, 100, L"Size: (%d,%d,%d)", g_iTextureWidth, g_iTextureHeight, g_iTextureDepth); 
	g_SampleUI.AddStatic( IDC_TEXTRES_STATIC, sz, 0, iY += 35, 100, 22 );
    StringCchPrintf( sz, 100, L"Max. Texture Res: %d", g_iTextureMaximum);
	g_SampleUI.AddStatic( IDC_TEXTRES_MAX_STATIC, sz, 0, iY += 24, 100, 22 );
	g_SampleUI.AddSlider( IDC_TEXTRES_MAX_SLIDER, 0, iY += 20, 130, 22, 64, 256, 128);

	g_SampleUI.AddCheckBox(IDC_SHOW_SURFACES, L"Show Surfaces", 0, iY+=36, 170, 22);
	g_SampleUI.GetCheckBox(IDC_SHOW_SURFACES)->SetChecked(true);

	g_SampleUI.AddButton(IDC_DIFFUSION, L"Diffuse!", 0, iY+=36, 170, 30);

	StringCchPrintf( sz, 100, L"Steps: %d", g_iDiffusionSteps);
	g_SampleUI.AddStatic(IDC_DIFFSTEPS_STATIC, sz, 0, iY += 36, 100, 22);
	g_SampleUI.AddSlider(IDC_DIFFSTEPS_SLIDER, 0, iY+=20, 130, 22, 1, 20, 8);

	g_SampleUI.AddRadioButton( IDC_ALL_SLICES, IDC_SLICES, L"Draw All Slices", 0, iY += 36, 170, 22);
	g_SampleUI.AddRadioButton( IDC_ONE_SLICE, IDC_SLICES, L"Draw One Slice", 0, iY += 26, 170, 22);
	g_SampleUI.GetRadioButton( IDC_ALL_SLICES )->SetChecked(true);
	g_SampleUI.GetRadioButton(IDC_ALL_SLICES)->SetVisible(false);
	g_SampleUI.GetRadioButton(IDC_ONE_SLICE)->SetVisible(false);

	StringCchPrintf( sz, 100, L"Sliceindex: %d", g_iSliceIndex);
	g_SampleUI.AddStatic(IDC_SLICEINDEX_STATIC, sz, 0, iY+=26, 100, 22);
	g_SampleUI.AddSlider( IDC_SLICEINDEX_SLIDER, 0, iY+=20, 130, 22);
	g_SampleUI.GetStatic(IDC_SLICEINDEX_STATIC)->SetVisible(false);
	g_SampleUI.GetSlider(IDC_SLICEINDEX_SLIDER)->SetVisible(false);

	g_SampleUI.AddCheckBox(IDC_ISO_CHECK, L"Show Isosurface", 0, iY+=36, 170, 22);
	g_SampleUI.GetCheckBox(IDC_ISO_CHECK)->SetChecked(false);
	g_SampleUI.GetCheckBox(IDC_ISO_CHECK)->SetVisible(false);

	StringCchPrintf( sz, 100, L"IsoValue: %.2f", g_fIsoValue);
	g_SampleUI.AddStatic(IDC_ISO_SLIDER_STATIC, sz, 0, iY += 26, 100, 22);
	g_SampleUI.AddSlider(IDC_ISO_SLIDER, 0, iY+=20, 130, 22);
	g_SampleUI.GetStatic(IDC_ISO_SLIDER_STATIC)->SetVisible(false);
	g_SampleUI.GetSlider(IDC_ISO_SLIDER)->SetVisible(false);

	// Setup the camera's view parameters
    D3DXVECTOR3 vecEye( 0.0f, 0.0f, -4.0f );
    D3DXVECTOR3 vecAt ( 0.0f, 0.0f, 0.0f );
    g_Camera.SetViewParams( &vecEye, &vecAt );
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // Uncomment this to get debug information from D3D11
    //pDeviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	g_fElapsedTime = fElapsedTime;
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

	g_pTxtHelper->End();//important for SAFE_DELETE

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
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass all remaining windows messages to camera so it can respond to user input
	if(g_bCameraActive)
		g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	switch(nChar) {
        case '+':
			g_zNear += 1.0f;
			g_Camera.SetProjParams( D3DX_PI / 4, g_fAspectRatio, g_zNear, g_zFar);
            break;
        case '-':
			if(g_zNear < 2.0)
				break;
			g_zNear -= 1.0f;
			g_Camera.SetProjParams( D3DX_PI / 4, g_fAspectRatio, g_zNear, g_zFar);
            break;
		
    }
}

//--------------------------------------------------------------------------------------
// Handle mouse
//--------------------------------------------------------------------------------------
void CALLBACK OnMouseEvent( bool bLeftDown, bool bRightDown, bool bMiddleDown, bool bSide1Down, bool bSide2Down, int iWheelDelta, int iX, int iY, void* pUserContext)
{
	if(g_bCameraActive)
		return;

	if(!bLeftDown)
		g_bBlockMouseDragging = false;

	if(g_bBlockMouseDragging)
		return;
	
	if(g_mouseX == 0 && g_mouseY == 0)
	{
		g_mouseX = iX;
		g_mouseY = iY;
	}

	const D3DXMATRIX* mView = g_Camera.GetViewMatrix();
	D3DXVECTOR3 lookAt = D3DXVECTOR3(mView->_13, mView->_23,mView->_33);
	D3DXVECTOR3 lookRight = D3DXVECTOR3(mView->_11, mView->_21,mView->_31);
	D3DXVECTOR3 lookUp = D3DXVECTOR3(mView->_12, mView->_22,mView->_32);

	if(g_bRotatesWithMouse)//Rotate&Scale object
	{
		if(bLeftDown)
		{
			g_pScene->RotateCurrentSurface(lookRight, (g_mouseY-iY)*g_fElapsedTime*g_mouseSpeed);
			g_pScene->RotateCurrentSurface(lookUp, (g_mouseX-iX)*g_fElapsedTime*g_mouseSpeed);
		}
		
		if(iWheelDelta>0)
			g_pScene->ScaleCurrentSurface(1.02f);
		else if(iWheelDelta<0)
			g_pScene->ScaleCurrentSurface(0.98f);
	}
	else//Move object
	{
		

		if(bLeftDown)
		{
			g_pScene->TranslateCurrentSurface(g_mouseSpeed*(iX-g_mouseX)*g_fElapsedTime*lookRight.x, g_mouseSpeed*(iX-g_mouseX)*g_fElapsedTime*lookRight.y, g_mouseSpeed*(iX-g_mouseX)*g_fElapsedTime*lookRight.z);
			g_pScene->TranslateCurrentSurface(g_mouseSpeed*(g_mouseY-iY)*g_fElapsedTime*lookUp.x, g_mouseSpeed*(g_mouseY-iY)*g_fElapsedTime*lookUp.y, g_mouseSpeed*(g_mouseY-iY)*g_fElapsedTime*lookUp.z);
		}

		if(iWheelDelta>0)
			g_pScene->TranslateCurrentSurface(100*lookAt.x, 100*lookAt.y, 100*lookAt.z);
		else if(iWheelDelta<0)
			g_pScene->TranslateCurrentSurface(-100*lookAt.x, -100*lookAt.y, -100*lookAt.z);
	}

	g_mouseX = iX;
	g_mouseY = iY;

	WCHAR sz[100];

	g_iTextureWidth = g_pScene->GetTextureWidth();
	g_iTextureHeight = g_pScene->GetTextureHeight();
	g_iTextureDepth = g_pScene->GetTextureDepth();
	StringCchPrintf( sz, 100, L"Size: (%d,%d,%d)", g_iTextureWidth, g_iTextureHeight, g_iTextureDepth); 
	g_SampleUI.GetStatic(IDC_TEXTRES_STATIC)->SetText(sz);
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
    switch( nControlID )
    {
		case IDC_CHANGE_CONTROL:
			g_pScene->ChangeControlledSurface();
			break;
		case IDC_ROTATE:
			g_bRotatesWithMouse = true;
			g_bCameraActive = false;
			g_SampleUI.GetRadioButton(IDC_ALL_SLICES)->SetVisible(false);
			g_SampleUI.GetRadioButton(IDC_ONE_SLICE)->SetVisible(false);
			g_SampleUI.GetStatic(IDC_SLICEINDEX_STATIC)->SetVisible(false);
			g_SampleUI.GetSlider(IDC_SLICEINDEX_SLIDER)->SetVisible(false);
			g_SampleUI.GetCheckBox(IDC_ISO_CHECK)->SetVisible(false);
			g_SampleUI.GetStatic(IDC_ISO_SLIDER_STATIC)->SetVisible(false);
			g_SampleUI.GetSlider(IDC_ISO_SLIDER)->SetVisible(false);
			g_pScene->Render3DTexture(false);
			break;
		case IDC_MOVE:
			g_bRotatesWithMouse = false;
			g_bCameraActive = false;
			g_SampleUI.GetRadioButton(IDC_ALL_SLICES)->SetVisible(false);
			g_SampleUI.GetRadioButton(IDC_ONE_SLICE)->SetVisible(false);
			g_SampleUI.GetStatic(IDC_SLICEINDEX_STATIC)->SetVisible(false);
			g_SampleUI.GetSlider(IDC_SLICEINDEX_SLIDER)->SetVisible(false);
			g_SampleUI.GetCheckBox(IDC_ISO_CHECK)->SetVisible(false);
			g_SampleUI.GetStatic(IDC_ISO_SLIDER_STATIC)->SetVisible(false);
			g_SampleUI.GetSlider(IDC_ISO_SLIDER)->SetVisible(false);
			g_pScene->Render3DTexture(false);
			break;
		case IDC_CAMERA:
			g_bCameraActive = true;
			break;
		case IDC_TEXTRES_MAX_SLIDER:
			g_bBlockMouseDragging = true;
			g_iTextureMaximum = g_SampleUI.GetSlider(IDC_TEXTRES_MAX_SLIDER)->GetValue();
			WCHAR sz[100];
			StringCchPrintf( sz, 100, L"Max. Texture Res: %d", g_iTextureMaximum);
            g_SampleUI.GetStatic( IDC_TEXTRES_MAX_STATIC )->SetText( sz );
			g_pScene->UpdateTextureResolution(g_iTextureMaximum);
			g_iTextureWidth = g_pScene->GetTextureWidth();
			g_iTextureHeight = g_pScene->GetTextureHeight();
			g_iTextureDepth = g_pScene->GetTextureDepth();
			StringCchPrintf( sz, 100, L"Size: (%d,%d,%d)", g_iTextureWidth, g_iTextureHeight, g_iTextureDepth); 
			g_SampleUI.GetStatic(IDC_TEXTRES_STATIC)->SetText(sz);
			
			g_SampleUI.GetRadioButton(IDC_ALL_SLICES)->SetVisible(false);
			g_SampleUI.GetRadioButton(IDC_ONE_SLICE)->SetVisible(false);
			g_SampleUI.GetStatic(IDC_SLICEINDEX_STATIC)->SetVisible(false);
			g_SampleUI.GetSlider(IDC_SLICEINDEX_SLIDER)->SetVisible(false);
			g_SampleUI.GetCheckBox(IDC_ISO_CHECK)->SetVisible(false);
			g_SampleUI.GetStatic(IDC_ISO_SLIDER_STATIC)->SetVisible(false);
			g_SampleUI.GetSlider(IDC_ISO_SLIDER)->SetVisible(false);
			g_pScene->Render3DTexture(false);
			break;
		case IDC_ALL_SLICES:
			g_SampleUI.GetStatic(IDC_SLICEINDEX_STATIC)->SetVisible(false);
			g_SampleUI.GetSlider(IDC_SLICEINDEX_SLIDER)->SetVisible(false);
			g_pScene->ChangeRenderingToAllSlices();
			break;
		case IDC_ONE_SLICE:
			g_SampleUI.GetStatic(IDC_SLICEINDEX_STATIC)->SetVisible(true);
			g_SampleUI.GetSlider(IDC_SLICEINDEX_SLIDER)->SetVisible(true);
			g_iSliceIndex = int((g_SampleUI.GetSlider(IDC_SLICEINDEX_SLIDER)->GetValue()/100.0f)*g_iTextureDepth + 0.5);
			g_pScene->ChangeRenderingToOneSlice(g_iSliceIndex);
			break;
		case IDC_SLICEINDEX_SLIDER:
			g_bBlockMouseDragging = true;
			g_iSliceIndex = int((g_SampleUI.GetSlider(IDC_SLICEINDEX_SLIDER)->GetValue()/100.0f)*(g_iTextureDepth-1) + 0.5);
			StringCchPrintf( sz, 100, L"Sliceindex: %d", g_iSliceIndex);
			g_SampleUI.GetStatic( IDC_SLICEINDEX_STATIC )->SetText( sz );
			g_pScene->ChangeRenderingToOneSlice(g_iSliceIndex);
			break;
		case IDC_SHOW_SURFACES:
			g_bShowSurfaces = !g_bShowSurfaces;
			break;
		case IDC_DIFFUSION:
			g_bShowSurfaces = true;
			g_SampleUI.GetRadioButton(IDC_ALL_SLICES)->SetVisible(true);
			g_SampleUI.GetRadioButton(IDC_ONE_SLICE)->SetVisible(true);
			g_SampleUI.GetCheckBox(IDC_ISO_CHECK)->SetVisible(true);
			g_SampleUI.GetStatic(IDC_ISO_SLIDER_STATIC)->SetVisible(true);
			g_SampleUI.GetSlider(IDC_ISO_SLIDER)->SetVisible(true);
			if(g_SampleUI.GetRadioButton(IDC_ONE_SLICE)->GetChecked())
			{
				g_SampleUI.GetStatic(IDC_SLICEINDEX_STATIC)->SetVisible(true);
				g_SampleUI.GetSlider(IDC_SLICEINDEX_SLIDER)->SetVisible(true);
			}
			g_bCameraActive = true;
			g_SampleUI.GetRadioButton(IDC_CAMERA)->SetChecked(true);
			g_pScene->GenerateVoronoi();
			break;
		case IDC_DIFFSTEPS_SLIDER:
			g_bBlockMouseDragging = true;
			g_iDiffusionSteps = g_SampleUI.GetSlider(IDC_DIFFSTEPS_SLIDER)->GetValue();
			StringCchPrintf(sz, 100, L"Steps: %d", g_iDiffusionSteps);
			g_SampleUI.GetStatic(IDC_DIFFSTEPS_STATIC)->SetText(sz);
			g_pScene->ChangeDiffusionSteps(g_iDiffusionSteps);
			g_pScene->Render3DTexture(false);
			break;
		case IDC_ISO_CHECK:
			g_pScene->ChangeIsoBehaviour();
			break;
		case IDC_ISO_SLIDER:
			g_bBlockMouseDragging = true;
			g_fIsoValue = g_SampleUI.GetSlider(IDC_ISO_SLIDER)->GetValue()/100.0f;
			StringCchPrintf( sz, 100, L"IsoValue: %.2f", g_fIsoValue);
			g_SampleUI.GetStatic( IDC_ISO_SLIDER_STATIC )->SetText( sz );
			g_pScene->ChangeIsoValue(g_fIsoValue);
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
    
	g_pScene = new Scene(pd3dDevice, pd3dImmediateContext);
	V_RETURN(g_pScene->Initialize(g_iTextureWidth, g_iTextureHeight, g_iTextureDepth));
	V_RETURN(g_pScene->SetScreenSize(g_Width, g_Height));

	// Initialize the view matrix
    g_Camera.SetViewParams( &g_Eye, &g_At );
    g_Camera.SetEnablePositionMovement(true);
    g_Camera.SetScalers(0.004f, 20.0f);
    g_View = *g_Camera.GetViewMatrix();
    g_Proj = *g_Camera.GetProjMatrix();

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
    g_fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI / 4, g_fAspectRatio, g_zNear, g_zFar);
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );

	g_Width = pBackBufferSurfaceDesc->Width;
	g_Height = pBackBufferSurfaceDesc->Height;
	g_pScene->SetScreenSize(g_Width, g_Height);

    g_SampleUI.SetLocation( g_Width - 170, 0 );
    g_SampleUI.SetSize( 170, 300 );
	
	V_RETURN(ReinitWindowSizeDependentRenderTargets(pd3dDevice));

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Initialize any textures that must match the window size
//--------------------------------------------------------------------------------------
HRESULT ReinitWindowSizeDependentRenderTargets(ID3D11Device* pd3dDevice)
{
    HRESULT hr;

    // Create resources to enable writing the scene depth using MRT, as well as to 
    //  enable reading as a shader resource
    ID3D11RenderTargetView *pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11Resource *pRTVResource;
    pRTV->GetResource(&pRTVResource);
    ID3D11Texture2D *pRTVTex2D = static_cast<ID3D11Texture2D*>(pRTVResource);
    assert(pRTVTex2D);
    D3D11_TEXTURE2D_DESC pRTVTex2DDesc;
    pRTVTex2D->GetDesc(&pRTVTex2DDesc);
    pRTVResource->Release();    


    D3D11_TEXTURE2D_DESC desc;
    desc.ArraySize = 1;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MipLevels = 1;
    desc.MiscFlags = 0;
    desc.SampleDesc = pRTVTex2DDesc.SampleDesc;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Width = g_Width;
    desc.Height = g_Height;
    desc.Format = DXGI_FORMAT_R32_FLOAT;
    V_RETURN(pd3dDevice->CreateTexture2D(&desc,NULL,&g_pSceneDepthTex2D));



    return S_OK;

}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                  float fElapsedTime, void* pUserContext )
{
	// If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and depth stencil
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();

    pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

	
	// Create a viewport to match the screen size
    D3D11_VIEWPORT rtViewport;
    rtViewport.TopLeftX = 0;
    rtViewport.TopLeftY = 0;
    rtViewport.MinDepth = 0;
    rtViewport.MaxDepth = 1;
    rtViewport.Width = float(g_Width);
    rtViewport.Height = float(g_Height);

    // If the settings dialog is being shown, then
    //  render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        // Set the viewport
        pd3dImmediateContext->RSSetViewports(1,&rtViewport);
        // Render the scene to the screen
        pd3dImmediateContext->OMSetRenderTargets( 1, &pRTV , pDSV ); 
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        pd3dImmediateContext->OMSetRenderTargets( 0, NULL, NULL );
        return;
    }

	// UPDATE GLOBAL VARIABLES FOR VOLUME RENDERING
	g_View = *g_Camera.GetViewMatrix();
	g_Proj = *g_Camera.GetProjMatrix();

	D3DXMATRIX mViewProjection = g_View * g_Proj;
	
	g_pScene->Render(mViewProjection, g_bShowSurfaces);

	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    g_SampleUI.OnRender( fElapsedTime );
    RenderText();
    DXUT_EndPerfEvent();


}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE(g_pTxtHelper);

    SAFE_DELETE(g_pScene);

}