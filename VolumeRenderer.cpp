#include "Globals.h"
#include "VolumeRenderer.h"

struct VsInput
{
    D3DXVECTOR3 pos;  
};

VolumeRenderer::VolumeRenderer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pEffect)
{
	m_pd3dDevice = pd3dDevice;
	m_pd3dImmediateContext = pd3dImmediateContext;
	m_pEffect = pEffect;

	memset(m_vGridDim,0, sizeof(m_vGridDim));
    memset(pRayDataTex2D, 0, sizeof(pRayDataTex2D));
    memset(pRayDataRTV, 0, sizeof(pRayDataRTV));
    memset(pRayDataSRV, 0, sizeof(pRayDataSRV));

	pQuadVertexBuffer = NULL;
	pQuadLayout = NULL;

	pGridBoxLayout = NULL;
    pGridBoxVertexBuffer = NULL;
    pGridBoxIndexBuffer = NULL;
	
	pRayDataTex2D[0] = NULL;
    pRayDataTex2D[1] = NULL;
    pRayDataSRV[0] = NULL;
    pRayDataSRV[1] = NULL;
    pRayDataRTV[0] = NULL;
    pRayDataRTV[1] = NULL;
    pRayDataSmallTex2D = NULL;
    pRayDataSmallRTV = NULL;
    pRayDataSmallSRV = NULL;
    pRayCastTex2D = NULL;
    pRayCastSRV = NULL;
    pRayCastRTV = NULL;
    pEdgeTex2D = NULL;
    pEdgeSRV = NULL;
    pEdgeRTV = NULL;

    D3DXMatrixIdentity(&m_mGridMatrix);
}

VolumeRenderer::~VolumeRenderer()
{
    SAFE_RELEASE(pGridBoxLayout);
    SAFE_RELEASE(pGridBoxVertexBuffer);
    SAFE_RELEASE(pGridBoxIndexBuffer);
    
    SAFE_RELEASE(pQuadLayout);
    SAFE_RELEASE(pQuadVertexBuffer);
  
    SAFE_RELEASE(pRayDataTex2D[0]);
    SAFE_RELEASE(pRayDataTex2D[1]);
    SAFE_RELEASE(pRayDataSRV[0]);
    SAFE_RELEASE(pRayDataSRV[1]);
    SAFE_RELEASE(pRayDataRTV[0]);
    SAFE_RELEASE(pRayDataRTV[1]);
    SAFE_RELEASE(pRayDataSmallTex2D);
    SAFE_RELEASE(pRayDataSmallRTV);
    SAFE_RELEASE(pRayDataSmallSRV);
    SAFE_RELEASE(pRayCastTex2D);
    SAFE_RELEASE(pRayCastSRV);
    SAFE_RELEASE(pRayCastRTV);
    SAFE_RELEASE(pEdgeTex2D); 
    SAFE_RELEASE(pEdgeSRV);
    SAFE_RELEASE(pEdgeRTV);
}

HRESULT VolumeRenderer::Initialize(int gridWidth, int gridHeight, int gridDepth, VERTEX vMin, VERTEX vMax)
{
    HRESULT hr;

    m_vGridDim[0] = float(gridWidth);
    m_vGridDim[1] = float(gridHeight);
    m_vGridDim[2] = float(gridDepth);

    m_iMaxDim = max( max( gridWidth, gridHeight ), gridDepth );

    // Initialize the grid offset matrix
    {
        // Make a scale matrix to scale the unit-sided box to be unit-length on the 
        //  side/s with maximum dimension 
        D3DXMATRIX scaleM;
        D3DXMatrixIdentity(&scaleM);
        D3DXMatrixScaling(&scaleM, m_vGridDim[0] / m_iMaxDim, m_vGridDim[1] / m_iMaxDim, m_vGridDim[2] / m_iMaxDim);
        // offset grid to be centered at origin
        D3DXMATRIX translationM;
        D3DXMatrixTranslation(&translationM, -0.5, -0.5, -0.5);

        m_mGridMatrix = translationM * scaleM;
    }

    // Check if the device supports FP32 blending to choose the right codepath depending on this
    UINT rgba32fFormatSupport;
    m_pd3dDevice->CheckFormatSupport(DXGI_FORMAT_R32G32B32A32_FLOAT, &rgba32fFormatSupport);
    m_bUseFP32Blending = (rgba32fFormatSupport & D3D11_FORMAT_SUPPORT_BLENDABLE) ? true : false;
    m_bUseFP32Blending = false;

    V_RETURN(InitShaders());
    V_RETURN(CreateGridBox(vMin, vMax));
    V_RETURN(CreateScreenQuad(vMin, vMax));

    return S_OK;
}

HRESULT VolumeRenderer::SetScreenSize( int width, int height )
{
    HRESULT hr;

    V_RETURN(CreateRayDataResources(width, height));
    return S_OK;
}

void VolumeRenderer::Draw(ID3D11ShaderResourceView * pSourceTexSRV, VERTEX vMin, VERTEX vMax)
{
	//update box according to boundingbox
	//CreateGridBox(vMin, vMax);
	//CreateScreenQuad(vMin, vMax);

    pColorTexVar->SetResource(pSourceTexSRV);

    // The near and far planes are used to unproject the scene's z-buffer values
    pZNearVar->SetFloat(g_zNear);
    pZFarVar->SetFloat(g_zFar);

	
    D3DXMATRIX worldView = g_View;

    // The length of one of the axis of the worldView matrix is the length of longest side of the box
    //  in view space. This is used to convert the length of a ray from view space to grid space.
    D3DXVECTOR3 worldXaxis = D3DXVECTOR3(worldView._11, worldView._12, worldView._13);
    float worldScale = D3DXVec3Length(&worldXaxis);
    pGridScaleFactorVar->SetFloat( worldScale );

    // We prepend the current world matrix with this other matrix which adds an offset (-0.5, -0.5, -0.5)
    //  and scale factors to account for unequal number of voxels on different sides of the volume box. 
    // This is because we want to preserve the aspect ratio of the original simulation grid when 
    //  raytracing through it.
    //worldView = m_mGridMatrix * worldView;
    (m_pEffect->GetVariableByName("WorldView")->AsMatrix())->SetMatrix((float*)&worldView); 
   
    (m_pEffect->GetVariableByName("tan_FovYhalf")->AsScalar())->SetFloat( float(tan(g_Fovy/2.0)) );
    (m_pEffect->GetVariableByName("tan_FovXhalf")->AsScalar())->SetFloat( float(tan(g_Fovy/2.0))*m_iRenderTextureWidth/m_iRenderTextureHeight );

	// worldViewProjection is used to transform the volume box to screen space
    D3DXMATRIX worldViewProjection;
    worldViewProjection = worldView * g_Proj;
    pWorldViewProjectionVar->SetMatrix( (float*)&worldViewProjection );

    // invWorldViewProjection is used to transform positions in the "near" plane into grid space
    D3DXMATRIX invWorldViewProjection;
    D3DXMatrixInverse(&invWorldViewProjection, NULL, &worldViewProjection);
    pInvWorldViewProjectionVar->SetMatrix((float*)&invWorldViewProjection);

    // Compute the inverse of the worldView matrix 
    D3DXMATRIX worldViewInv;
    D3DXMatrixInverse(&worldViewInv, NULL, &worldView);
    // Compute the eye's position in "grid space" (the 0-1 texture coordinate cube)
    D3DXVECTOR4 eyeInGridSpace;
    D3DXVECTOR3 origin(0,0,0);
    D3DXVec3Transform(&eyeInGridSpace, &origin, &worldViewInv);
    pEyeOnGridVar->SetFloatVector((float*)&eyeInGridSpace);

    float color[4] = {0, 0, 0, 0 };


    // Ray cast and render to a temporary buffer
    //=========================================================================

    // Partial init of viewport struct used below
    D3D11_VIEWPORT rtViewport;
    rtViewport.TopLeftX = 0;
    rtViewport.TopLeftY = 0;
    rtViewport.MinDepth = 0;
    rtViewport.MaxDepth = 1;


    // Compute the ray data required by the raycasting pass below.
    //  This function will render to a buffer of float4 vectors, where
    //  xyz is starting position of the ray in grid space
    //  w is the length of the ray in view space
    ComputeRayData();

	// Do edge detection on this image to find any
    // problematic areas where we need to raycast at higher resolution
    ComputeEdgeTexture();
	
    // Raycast into the temporary render target: 
    //  raycasting is done at the smaller resolution, using a fullscreen quad
    m_pd3dImmediateContext->ClearRenderTargetView( pRayCastRTV, color );
    m_pd3dImmediateContext->OMSetRenderTargets( 1, &pRayCastRTV , NULL ); 

    rtViewport.Width = float(m_iRenderTextureWidth);
    rtViewport.Height = float(m_iRenderTextureHeight);
    m_pd3dImmediateContext->RSSetViewports(1,&rtViewport);

    pRTWidthVar->SetFloat((float)m_iRenderTextureWidth);
    pRTHeightVar->SetFloat((float)m_iRenderTextureHeight);

    pRayDataSmallVar->SetResource(pRayDataSmallSRV);
   
       m_pTechnique->GetPassByName("QuadRaycast")->Apply(0, m_pd3dImmediateContext);
    DrawScreenQuad();

   
    // Render to the back buffer sampling from the raycast texture that we just created
    // If and edge was detected at the current pixel we will raycast again to avoid
    // smoke aliasing artifacts at scene edges
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    m_pd3dImmediateContext->OMSetRenderTargets( 1, &pRTV , pDSV ); 

    rtViewport.Width = float(g_Width);
    rtViewport.Height = float(g_Height);
    m_pd3dImmediateContext->RSSetViewports(1,&rtViewport);

    pRTWidthVar->SetFloat((float)g_Width);
    pRTHeightVar->SetFloat((float)g_Height);

    pRayCastVar->SetResource(pRayCastSRV);
    pEdgeVar->SetResource(pEdgeSRV);
    
    m_pTechnique->GetPassByName("QuadRaycastCopy")->Apply(0, m_pd3dImmediateContext);
    DrawScreenQuad();
	
}

HRESULT VolumeRenderer::InitShaders()
{
    HRESULT hr;

    m_pTechnique = m_pEffect->GetTechniqueByName("VolumeRenderer");

    pColorTexVar = m_pEffect->GetVariableByName("colorTex")->AsShaderResource();

    pZNearVar = m_pEffect->GetVariableByName("ZNear")->AsScalar();
    pZFarVar = m_pEffect->GetVariableByName("ZFar")->AsScalar();
    pGridScaleFactorVar = m_pEffect->GetVariableByName( "gridScaleFactor")->AsScalar();
    pEyeOnGridVar = m_pEffect->GetVariableByName("eyeOnGrid")->AsVector();
    pWorldViewProjectionVar = m_pEffect->GetVariableByName("WorldViewProjection")->AsMatrix();
    pInvWorldViewProjectionVar = m_pEffect->GetVariableByName("InvWorldViewProjection")->AsMatrix();
    pRTWidthVar = m_pEffect->GetVariableByName("RTWidth")->AsScalar();  
    pRTHeightVar = m_pEffect->GetVariableByName("RTHeight")->AsScalar();

	D3DXVECTOR3 recGridDim(1.0f/m_vGridDim[0], 1.0f/m_vGridDim[1], 1.0f/m_vGridDim[2]);
    V_RETURN(m_pEffect->GetVariableByName("gridDim")->AsVector()->SetFloatVector(m_vGridDim));
    V_RETURN(m_pEffect->GetVariableByName("recGridDim")->AsVector()->SetFloatVector(recGridDim));
    V_RETURN(m_pEffect->GetVariableByName("maxGridDim")->AsScalar()->SetFloat(float(m_iMaxDim)));


    return S_OK;
}

HRESULT VolumeRenderer::CreateGridBox(VERTEX vMin, VERTEX vMax) 
{
    HRESULT hr;

    /*VsInput vertices[] =
    {
        { D3DXVECTOR3( vMin.x, vMin.y, vMin.z ) },
        { D3DXVECTOR3( vMin.x, vMin.y, vMax.z ) },
        { D3DXVECTOR3( vMin.x, vMax.y, vMin.z ) },
        { D3DXVECTOR3( vMin.x, vMax.y, vMax.z ) },
        { D3DXVECTOR3( vMax.x, vMin.y, vMin.z ) },
        { D3DXVECTOR3( vMax.x, vMin.y, vMax.z ) },
        { D3DXVECTOR3( vMax.x, vMax.y, vMin.z ) },
        { D3DXVECTOR3( vMax.x, vMax.y, vMax.z ) },
    };*/
	
	VsInput vertices[] =
    {
        { D3DXVECTOR3( -1.0, -1.0, -1.0 ) },
        { D3DXVECTOR3( -1.0, -1.0, 1.0 ) },
        { D3DXVECTOR3( -1.0, 1.0, -1.0 ) },
        { D3DXVECTOR3( -1.0, 1.0, 1.0 ) },
        { D3DXVECTOR3( 1.0, -1.0, -1.0 ) },
        { D3DXVECTOR3( 1.0, -1.0, 1.0 ) },
        { D3DXVECTOR3( 1.0, 1.0, -1.0 ) },
        { D3DXVECTOR3( 1.0, 1.0, 1.0 ) },
    };

	SAFE_RELEASE(pGridBoxVertexBuffer);
	SAFE_RELEASE(pGridBoxIndexBuffer);
	SAFE_RELEASE(pGridBoxLayout);

    D3D11_BUFFER_DESC bd;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = vertices;
    V_RETURN( m_pd3dDevice->CreateBuffer( &bd, &InitData, &pGridBoxVertexBuffer ) );


    // Create index buffer
    DWORD indices[] =
    {
        0, 4, 1, 1, 4, 5,
        0, 1, 2, 2, 1, 3,
        4, 6, 5, 6, 7, 5,
        2, 3, 6, 3, 7, 6,
        1, 5, 3, 3, 5, 7,
        0, 2, 4, 2, 6, 4
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(indices);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    InitData.pSysMem = indices;
    V_RETURN( m_pd3dDevice->CreateBuffer( &bd, &InitData, &pGridBoxIndexBuffer ) );

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },  
    };

	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pTechnique->GetPassByName("CompRayData_Back")->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

	V_RETURN(m_pd3dDevice->CreateInputLayout(layout, _countof(layout), vsCodePtr, vsCodeLen, &pGridBoxLayout));


    return S_OK;
}


HRESULT VolumeRenderer::CreateScreenQuad(VERTEX vMin, VERTEX vMax) 
{
    HRESULT hr;
    
	SAFE_RELEASE(pQuadLayout);
	SAFE_RELEASE(pQuadVertexBuffer);

	// Create our quad input layout
    const D3D11_INPUT_ELEMENT_DESC quadlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

	D3DX11_PASS_SHADER_DESC passVsDesc;
	m_pTechnique->GetPassByName("QuadRaycast")->GetVertexShaderDesc(&passVsDesc);
	D3DX11_EFFECT_SHADER_DESC effectVsDesc;
	passVsDesc.pShaderVariable->GetShaderDesc(passVsDesc.ShaderIndex, &effectVsDesc);
	const void *vsCodePtr = effectVsDesc.pBytecode;
	unsigned vsCodeLen = effectVsDesc.BytecodeLength;

    // Create the input layout
	V_RETURN(m_pd3dDevice->CreateInputLayout(quadlayout, _countof(quadlayout), vsCodePtr, vsCodeLen, &pQuadLayout));

    // Create a screen quad for all render to texture operations
    VsInput svQuad[4];
	svQuad[0].pos = D3DXVECTOR3(0.0f, 1.0f, 0.0f );
	svQuad[1].pos = D3DXVECTOR3(1.0f, 1.0f, 0.0f );
	svQuad[2].pos = D3DXVECTOR3(0.0f, 0.0f, 0.0f );
	svQuad[3].pos = D3DXVECTOR3(1.0f, 0.0f, 0.0f );

    D3D11_BUFFER_DESC vbdesc =
    {
        4*sizeof(VsInput),
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER,
        0,
        0
    };

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = svQuad;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;
    V_RETURN( m_pd3dDevice->CreateBuffer( &vbdesc, &InitData, &pQuadVertexBuffer ) );

    return hr;
}

HRESULT VolumeRenderer::CreateRayDataResources( int width, int height )
{
    HRESULT hr;

    // clean up all resources first
    SAFE_RELEASE(pRayDataTex2D[0]);
    SAFE_RELEASE(pRayDataTex2D[1]);
    SAFE_RELEASE(pRayDataSRV[0]);
    SAFE_RELEASE(pRayDataSRV[1]);
    SAFE_RELEASE(pRayDataRTV[0]);
    SAFE_RELEASE(pRayDataRTV[1]);
    SAFE_RELEASE(pRayDataSmallTex2D);
    SAFE_RELEASE(pRayDataSmallSRV);
    SAFE_RELEASE(pRayDataSmallRTV);
    SAFE_RELEASE(pRayCastTex2D);
    SAFE_RELEASE(pRayCastSRV);
    SAFE_RELEASE(pRayCastRTV);
    SAFE_RELEASE(pEdgeTex2D);
    SAFE_RELEASE(pEdgeSRV);
    SAFE_RELEASE(pEdgeRTV);
	

    // find a good resolution for raycasting purposes
    CalculateRenderTextureSize(width, height);

    DXGI_FORMAT rayDataFmt = DXGI_FORMAT_R32G32B32A32_FLOAT;
    
    //create the textures
    D3D11_TEXTURE2D_DESC desc;
    desc.ArraySize = 1;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MipLevels = 1;
    desc.MiscFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Width = width;
    desc.Height = height;
    desc.Format = rayDataFmt;
    V_RETURN(m_pd3dDevice->CreateTexture2D(&desc,NULL, &pRayDataTex2D[0]));
    if( !m_bUseFP32Blending )
        V_RETURN(m_pd3dDevice->CreateTexture2D(&desc,NULL, &pRayDataTex2D[1]));
    desc.Width = m_iRenderTextureWidth;
    desc.Height = m_iRenderTextureHeight;
    V_RETURN(m_pd3dDevice->CreateTexture2D(&desc,NULL,&pRayDataSmallTex2D));
    V_RETURN(m_pd3dDevice->CreateTexture2D(&desc,NULL,&pRayCastTex2D));
    desc.Format = DXGI_FORMAT_R32_FLOAT;
    V_RETURN(m_pd3dDevice->CreateTexture2D(&desc,NULL,&pEdgeTex2D));

    //create the render target views
    D3D11_RENDER_TARGET_VIEW_DESC DescRT;
    DescRT.Format = rayDataFmt;
    DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    DescRT.Texture2D.MipSlice = 0;
    V_RETURN( m_pd3dDevice->CreateRenderTargetView(pRayDataTex2D[0], &DescRT, &pRayDataRTV[0]));
    if( !m_bUseFP32Blending )
        V_RETURN( m_pd3dDevice->CreateRenderTargetView(pRayDataTex2D[1], &DescRT, &pRayDataRTV[1]));
    V_RETURN( m_pd3dDevice->CreateRenderTargetView(pRayDataSmallTex2D, &DescRT, &pRayDataSmallRTV));   
    V_RETURN( m_pd3dDevice->CreateRenderTargetView(pRayCastTex2D, &DescRT, &pRayCastRTV));
    DescRT.Format = DXGI_FORMAT_R32_FLOAT;
    V_RETURN( m_pd3dDevice->CreateRenderTargetView(pEdgeTex2D, &DescRT, &pEdgeRTV));

    //create the shader resource views
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = 1;
    SRVDesc.Format = rayDataFmt;
    V_RETURN(m_pd3dDevice->CreateShaderResourceView(pRayDataTex2D[0], &SRVDesc, &pRayDataSRV[0]));
    if( !m_bUseFP32Blending )
        V_RETURN(m_pd3dDevice->CreateShaderResourceView(pRayDataTex2D[1], &SRVDesc, &pRayDataSRV[1]));
    V_RETURN(m_pd3dDevice->CreateShaderResourceView(pRayDataSmallTex2D, &SRVDesc, &pRayDataSmallSRV));
    V_RETURN(m_pd3dDevice->CreateShaderResourceView(pRayCastTex2D, &SRVDesc, &pRayCastSRV));
    SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
    V_RETURN(m_pd3dDevice->CreateShaderResourceView(pEdgeTex2D, &SRVDesc, &pEdgeSRV));


    pRayDataVar = m_pEffect->GetVariableByName("rayDataTex")->AsShaderResource();
    pRayDataSmallVar = m_pEffect->GetVariableByName("rayDataTexSmall")->AsShaderResource();
    pRayCastVar = m_pEffect->GetVariableByName("rayCastTex")->AsShaderResource();
    pEdgeVar    = m_pEffect->GetVariableByName("edgeTex")->AsShaderResource();

    return hr;
}

void VolumeRenderer::CalculateRenderTextureSize(int screenWidth, int screenHeight)
{
    int maxProjectedSide = int(3.0 * sqrt(3.0)*m_iMaxDim);
    int maxScreenDim = max(screenWidth, screenHeight);
    
    float screenAspectRatio = ((float)screenWidth)/screenHeight;

    if( maxScreenDim > maxProjectedSide)
    {
        if(screenHeight > screenWidth)
        {
            m_iRenderTextureHeight = maxProjectedSide;
            m_iRenderTextureWidth = (int)(screenAspectRatio * maxProjectedSide);
        }
        else
        {
            m_iRenderTextureWidth = maxProjectedSide;
            m_iRenderTextureHeight = (int)((1.0f/screenAspectRatio) * maxProjectedSide);
        }
    }
    else
    {
        m_iRenderTextureWidth = screenWidth;
        m_iRenderTextureHeight = screenHeight;
    }
}

void VolumeRenderer::ComputeRayData()
{
    // Clear the color buffer to 0
    float blackColor[4] = {0, 0, 0, 0 };
    m_pd3dImmediateContext->ClearRenderTargetView(pRayDataRTV[0], blackColor);
    if( !m_bUseFP32Blending )
        m_pd3dImmediateContext->ClearRenderTargetView(pRayDataRTV[1], blackColor);

    m_pd3dImmediateContext->OMSetRenderTargets(1, &pRayDataRTV[0], NULL);
    m_pEffect->GetVariableByName("sceneDepthTex")->AsShaderResource()->SetResource(g_pSceneDepthSRV);
    
    // Setup viewport to match the window's backbuffer
    D3D11_VIEWPORT rtViewport;
    rtViewport.TopLeftX = 0;
    rtViewport.TopLeftY = 0;
    rtViewport.MinDepth = 0;
    rtViewport.MaxDepth = 1;
    rtViewport.Width = float(g_Width);
    rtViewport.Height = float(g_Height);
    m_pd3dImmediateContext->RSSetViewports(1,&rtViewport);
    pRTWidthVar->SetFloat((float)g_Width);
    pRTHeightVar->SetFloat((float)g_Height);

    // Render volume back faces
    // We output xyz=(0,-1,0) and w=min(sceneDepth, boxDepth)
    m_pTechnique->GetPassByName("CompRayData_Back")->Apply(0, m_pd3dImmediateContext);
    DrawBox();

    if( !m_bUseFP32Blending )
    {
        // repeat the back face pass in pRayDataRTV[1] - we could also do CopySubResource
        m_pd3dImmediateContext->OMSetRenderTargets(1, &pRayDataRTV[1], NULL);
        DrawBox();
    }

    // Render volume front faces using subtractive blending or doing texture lookups
    //  depending on hw support for FP32 blending. Note that an FP16 RGBA buffer is
    //  does not have enough precision to represent the different XYZ postions 
    //  for each ray entry point in most common circumstances.
    // We output xyz="position in grid space" and w=boxDepth,
    //  unless the pixel is occluded by the scene, in which case we output xyzw=(1,0,0,0)
    if( m_bUseFP32Blending )
    {
        m_pTechnique->GetPassByName("CompRayData_Front")->Apply(0, m_pd3dImmediateContext);
    }
    else
    {
        pRayDataVar->SetResource(pRayDataSRV[0]);
        m_pTechnique->GetPassByName("CompRayData_FrontNOBLEND")->Apply(0, m_pd3dImmediateContext);
    }
    DrawBox();
}

void VolumeRenderer::ComputeEdgeTexture(void)
{
    // First setup viewport to match the size of the destination low-res texture
    D3D11_VIEWPORT rtViewport;
    rtViewport.TopLeftX = 0;
    rtViewport.TopLeftY = 0;
    rtViewport.MinDepth = 0;
    rtViewport.MaxDepth = 1;
    rtViewport.Width = float(m_iRenderTextureWidth);
    rtViewport.Height = float(m_iRenderTextureHeight);
    m_pd3dImmediateContext->RSSetViewports(1,&rtViewport);
    pRTWidthVar->SetFloat((float)m_iRenderTextureWidth);
    pRTHeightVar->SetFloat((float)m_iRenderTextureHeight);

    // Downsample the rayDataTexture to a new small texture, simply using point sample (no filtering)
    m_pd3dImmediateContext->OMSetRenderTargets( 1, &pRayDataSmallRTV , NULL ); 
    if( m_bUseFP32Blending )
        pRayDataVar->SetResource(pRayDataSRV[0]);
    else
        pRayDataVar->SetResource(pRayDataSRV[1]);
    m_pTechnique->GetPassByName("QuadDownSampleRayDataTexture")->Apply(0, m_pd3dImmediateContext);
    DrawScreenQuad();

    // Create an edge texture, performing edge detection on 'rayDataTexSmall'
    m_pd3dImmediateContext->OMSetRenderTargets( 1, &pEdgeRTV , NULL ); 
    pRayDataSmallVar->SetResource(pRayDataSmallSRV);
    m_pTechnique->GetPassByName("QuadEdgeDetect")->Apply(0, m_pd3dImmediateContext);
    DrawScreenQuad();
}

void VolumeRenderer::DrawBox(void)
{
    UINT stride = sizeof( VsInput );
    UINT offset = 0;
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, &pGridBoxVertexBuffer, &stride, &offset );
    m_pd3dImmediateContext->IASetIndexBuffer( pGridBoxIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );
    m_pd3dImmediateContext->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    m_pd3dImmediateContext->IASetInputLayout(pGridBoxLayout);
    m_pd3dImmediateContext->DrawIndexed(36, 0, 0);
}


void VolumeRenderer::DrawScreenQuad(void)
{
    UINT strides = sizeof(VsInput);
    UINT offsets = 0;
    m_pd3dImmediateContext->IASetInputLayout( pQuadLayout );
    m_pd3dImmediateContext->IASetVertexBuffers( 0, 1, &pQuadVertexBuffer, &strides, &offsets );
    m_pd3dImmediateContext->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
    m_pd3dImmediateContext->Draw( 4, 0 );
}
