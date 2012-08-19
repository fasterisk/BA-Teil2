#include "TextureManager.h"
#include "Scene.h"



TextureManager* TextureManager::s_pInstance = NULL;

/****************************************************************************
 ****************************************************************************/
TextureManager * TextureManager::GetInstance()
{
	if(s_pInstance == NULL)
		s_pInstance = new TextureManager();
	return s_pInstance;
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::DeleteInstance()
{
	if(s_pInstance != NULL)
		SAFE_DELETE(s_pInstance)
}

/****************************************************************************
 ****************************************************************************/
TextureManager::TextureManager()
{
	m_pOldRTV = NULL;
	m_pOldDSV = NULL;
	m_nOldViewports = 0;
	m_pViewports[100];

	m_nCurrentTextureID = 1;
	m_nCurrentDepthBufferID = 1;
}

/****************************************************************************
 ****************************************************************************/
TextureManager::~TextureManager()
{
	for(unsigned int i = 1; i < m_nCurrentTextureID; i++)
	{
		SAFE_RELEASE(m_TextureMap[i]);
		SAFE_RELEASE(m_TextureRTVs[i]);
		SAFE_RELEASE(m_TextureSRVs[i]);
	}

	for(unsigned int i = 1; i < m_nCurrentDepthBufferID; i++)
	{
		SAFE_RELEASE(m_DepthBufferMap[i]);
		SAFE_RELEASE(m_DepthBufferViewMap[i]);
	}
}

/****************************************************************************
 ****************************************************************************/
unsigned int	TextureManager::Create2DTexture(const std::string sDebugName,
												const int iWidth,
												const int iHeight)
{
	unsigned int nID = ItlGetNewTextureID();

	ItlCreate2DTexture(sDebugName, nID, iWidth, iHeight);

	return nID;
}

/****************************************************************************
 ****************************************************************************/
unsigned int	TextureManager::Create3DTexture(const std::string sDebugName,
												const int iWidth,
												const int iHeight,
												const int iDepth)
{
	unsigned int nID = ItlGetNewTextureID();

	ItlCreate3DTexture(sDebugName, nID, iWidth, iHeight, iDepth);

	return nID;
}

/****************************************************************************
 ****************************************************************************/
unsigned int	TextureManager::Create2DDepthBuffer(const std::string sDebugName,
													const int iWidth,
													const int iHeight)
{
	unsigned int nID = ItlGetNewDepthBufferID();

	ItlCreate2DDepthBuffer(sDebugName, nID, iWidth, iHeight);

	return nID;
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::Update2DTexture(const unsigned int nID,
										const int iWidth,
										const int iHeight)
{
	assert(m_TextureMap.find(nID) != m_TextureMap.end());

	SAFE_RELEASE(m_TextureMap[nID]);
	SAFE_RELEASE(m_TextureRTVs[nID]);
	SAFE_RELEASE(m_TextureSRVs[nID]);

	ItlCreate2DTexture(m_TextureStateMap[nID].sDebugName, nID, iWidth, iHeight);

}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::Update3DTexture(const unsigned int nID,
										const int iWidth,
										const int iHeight, 
										const int iDepth)
{
	assert(m_TextureMap.find(nID) != m_TextureMap.end());

	SAFE_RELEASE(m_TextureMap[nID]);
	SAFE_RELEASE(m_TextureRTVs[nID]);
	SAFE_RELEASE(m_TextureSRVs[nID]);

	ItlCreate3DTexture(m_TextureStateMap[nID].sDebugName, nID, iWidth, iHeight, iDepth);
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::Update2DDepthBuffer(const unsigned int nID,
											const int iWidth,
											const int iHeight)
{
	assert(m_DepthBufferMap.find(nID) != m_DepthBufferMap.end());

	SAFE_RELEASE(m_DepthBufferMap[nID]);
	SAFE_RELEASE(m_DepthBufferViewMap[nID]);

	ItlCreate2DDepthBuffer(m_DepthBufferStateMap[nID].sDebugName, nID, iWidth, iHeight);
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::Clear2DDepthBuffer(const unsigned int nID)
{
	assert(m_DepthBufferMap.find(nID) != m_DepthBufferMap.end());

	Scene::GetInstance()->GetContext()->ClearDepthStencilView(m_DepthBufferViewMap[nID], D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::BindTextureAsRTV(const unsigned int nID)
{
	assert(m_TextureMap.find(nID) != m_TextureMap.end());

	ItlStoreOldRenderState();

	TEXTURESTATE state = GetTextureState(nID);

	SAFE_RELEASE(m_TextureRTVs[nID]);

	D3D11_RENDER_TARGET_VIEW_DESC desc;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if(state.nType == 0)
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
	}
	else
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipSlice = 0;
		desc.Texture3D.FirstWSlice = 0;
		desc.Texture3D.WSize = state.iDepth;
	}

	Scene::GetInstance()->GetDevice()->CreateRenderTargetView(m_TextureMap[nID], &desc, &m_TextureRTVs[nID]);

	Scene::GetInstance()->GetContext()->OMSetRenderTargets(1, &m_TextureRTVs[nID], NULL);

}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::BindTextureAsRTV(const unsigned int nID1, 
										 const unsigned int nID2)
{
	assert(m_TextureMap.find(nID1) != m_TextureMap.end());
	assert(m_TextureMap.find(nID2) != m_TextureMap.end());

	ItlStoreOldRenderState();

	TEXTURESTATE state1 = GetTextureState(nID1);
	TEXTURESTATE state2 = GetTextureState(nID2);

	SAFE_RELEASE(m_TextureRTVs[nID1]);
	SAFE_RELEASE(m_TextureRTVs[nID2]);

	D3D11_RENDER_TARGET_VIEW_DESC desc;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if(state1.nType == 0)
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
	}
	else
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipSlice = 0;
		desc.Texture3D.FirstWSlice = 0;
		desc.Texture3D.WSize = state1.iDepth;
	}

	if(state2.nType == 0)
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
	}
	else
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipSlice = 0;
		desc.Texture3D.FirstWSlice = 0;
		desc.Texture3D.WSize = state2.iDepth;
	}

	Scene::GetInstance()->GetDevice()->CreateRenderTargetView(m_TextureMap[nID1], &desc, &m_TextureRTVs[nID1]);
	Scene::GetInstance()->GetDevice()->CreateRenderTargetView(m_TextureMap[nID2], &desc, &m_TextureRTVs[nID2]);

	ID3D11RenderTargetView* destRTVs[2];
	destRTVs[0] = m_TextureRTVs[nID1];
	destRTVs[1] = m_TextureRTVs[nID2];
	Scene::GetInstance()->GetContext()->OMSetRenderTargets(2, destRTVs, NULL);
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::BindTextureAsRTV(const unsigned int nID1, 
										 const unsigned int nID2,
										 const unsigned int nDepthBufferID)
{
	assert(m_TextureMap.find(nID1) != m_TextureMap.end());
	assert(m_TextureMap.find(nID2) != m_TextureMap.end());
	assert(m_DepthBufferMap.find(nDepthBufferID) != m_DepthBufferMap.end());

	ItlStoreOldRenderState();

	TEXTURESTATE state1 = GetTextureState(nID1);
	TEXTURESTATE state2 = GetTextureState(nID2);

	SAFE_RELEASE(m_TextureRTVs[nID1]);
	SAFE_RELEASE(m_TextureRTVs[nID2]);

	D3D11_RENDER_TARGET_VIEW_DESC desc;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if(state1.nType == 0)
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
	}
	else
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipSlice = 0;
		desc.Texture3D.FirstWSlice = 0;
		desc.Texture3D.WSize = state1.iDepth;
	}

	if(state2.nType == 0)
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
	}
	else
	{
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipSlice = 0;
		desc.Texture3D.FirstWSlice = 0;
		desc.Texture3D.WSize = state2.iDepth;
	}

	Scene::GetInstance()->GetDevice()->CreateRenderTargetView(m_TextureMap[nID1], &desc, &m_TextureRTVs[nID1]);
	Scene::GetInstance()->GetDevice()->CreateRenderTargetView(m_TextureMap[nID2], &desc, &m_TextureRTVs[nID2]);

	ID3D11RenderTargetView* destRTVs[2];
	destRTVs[0] = m_TextureRTVs[nID1];
	destRTVs[1] = m_TextureRTVs[nID2];
	Scene::GetInstance()->GetContext()->OMSetRenderTargets(2, destRTVs, m_DepthBufferViewMap[nDepthBufferID]);
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::UnBindRTVs()
{
	//assert(m_pOldRTV != NULL && m_pOldDSV != NULL);

	ItlRestoreOldRenderState();

	m_pOldRTV = NULL;
	m_pOldDSV = NULL;
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::BindTextureAsSRV(const unsigned int nID, 
										 ID3DX11EffectShaderResourceVariable* pSRVar)
{
	TEXTURESTATE state = GetTextureState(nID);

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	if(state.nType == 0)
	{
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MostDetailedMip = 0;
		desc.Texture2D.MipLevels = 1;
	}
	else
	{
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MostDetailedMip = 0;
		desc.Texture3D.MipLevels = 1;
	}

	SAFE_RELEASE(m_TextureSRVs[nID]);
	
	Scene::GetInstance()->GetDevice()->CreateShaderResourceView(m_TextureMap[nID], &desc, &m_TextureSRVs[nID]);

	pSRVar->SetResource(m_TextureSRVs[nID]);
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::UnBindSRV(ID3DX11EffectShaderResourceVariable* pSRVar)
{
	pSRVar->SetResource(NULL);
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::Render2DTextureInto3DSlice(const unsigned int n2DTexture,
												   const unsigned int n3DTexture,
												   const int iSliceIndex)
{
	Scene::GetInstance()->GetContext()->CopySubresourceRegion(m_TextureMap[n3DTexture], 0, 0, 0, iSliceIndex, m_TextureMap[n2DTexture], 0, NULL);
}

/****************************************************************************
 ****************************************************************************/
TextureManager::TEXTURESTATE	TextureManager::GetTextureState(const unsigned int nID)
{
	return m_TextureStateMap[nID];
}

/****************************************************************************
 ****************************************************************************/
TextureManager::DEPTHBUFFERSTATE	TextureManager::GetDepthBufferState(const unsigned int nID)
{
	return m_DepthBufferStateMap[nID];
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::ItlCreate2DTexture(const std::string sDebugName,
										   const unsigned int nID,
										   const int iWidth, 
										   const int iHeight)
{
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = iWidth;
	desc.Height = iHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	ID3D11Texture2D* pTexture2D;

	hr = Scene::GetInstance()->GetDevice()->CreateTexture2D(&desc, NULL, &pTexture2D);
	assert(hr == S_OK);

	DXUT_SetDebugName(pTexture2D, sDebugName.c_str());


	TEXTURESTATE state;
	state.sDebugName = sDebugName;
	state.nID = nID;
	state.bBoundAsRTV = false;
	state.bBoundAsSRV = false;
	state.nType = 0;
	state.iWidth = iWidth;
	state.iHeight = iHeight;
	state.iDepth = 1;

	m_TextureStateMap[state.nID] = state;
	m_TextureMap[state.nID] = pTexture2D;
	m_TextureRTVs[state.nID] = NULL;
	m_TextureSRVs[state.nID] = NULL;
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::ItlCreate3DTexture(const std::string sDebugName,
										   const unsigned int nID,
										   const int iWidth, 
										   const int iHeight, 
										   const int iDepth)
{
	HRESULT hr;

	D3D11_TEXTURE3D_DESC desc;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = iWidth;
	desc.Height = iHeight;
	desc.Depth = iDepth;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	
	ID3D11Texture3D* pTexture3D;

	hr = Scene::GetInstance()->GetDevice()->CreateTexture3D(&desc, NULL, &pTexture3D);
	assert(hr == S_OK);

	DXUT_SetDebugName(pTexture3D, sDebugName.c_str());

	TEXTURESTATE state;
	state.sDebugName = sDebugName;
	state.nID = nID;
	state.bBoundAsRTV = false;
	state.bBoundAsSRV = false;
	state.nType = 1;
	state.iWidth = iWidth;
	state.iHeight = iHeight;
	state.iDepth = iDepth;

	m_TextureStateMap[state.nID] = state;
	m_TextureMap[state.nID] = pTexture3D;
	m_TextureRTVs[state.nID] = NULL;
	m_TextureSRVs[state.nID] = NULL;
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::ItlCreate2DDepthBuffer(const std::string sDebugName,
											   const unsigned int nID,
											   const int iWidth,
											   const int iHeight)
{
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = iWidth;
	desc.Height = iHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	ID3D11Texture2D			*p2DDepthBufferTex2D;
	ID3D11DepthStencilView	*p2DDepthBufferView;

	hr = Scene::GetInstance()->GetDevice()->CreateTexture2D(&desc, NULL, &p2DDepthBufferTex2D);
	assert(hr == S_OK);
	hr = Scene::GetInstance()->GetDevice()->CreateDepthStencilView(p2DDepthBufferTex2D, NULL, &p2DDepthBufferView);

	DXUT_SetDebugName(p2DDepthBufferTex2D, sDebugName.c_str());

	DEPTHBUFFERSTATE state;
	state.sDebugName = sDebugName;
	state.nID = nID;
	state.bBound = false;

	m_DepthBufferStateMap[state.nID] = state;
	m_DepthBufferMap[state.nID] = p2DDepthBufferTex2D;
	m_DepthBufferViewMap[state.nID] = p2DDepthBufferView;
}

/****************************************************************************
 ****************************************************************************/
unsigned int	TextureManager::ItlGetNewTextureID()
{
	return m_nCurrentTextureID++;
}

/****************************************************************************
 ****************************************************************************/
unsigned int	TextureManager::ItlGetNewDepthBufferID()
{
	return m_nCurrentDepthBufferID++;
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::ItlStoreOldRenderState()
{
	/*m_pOldRTV = DXUTGetD3D11RenderTargetView();
	m_pOldDSV = DXUTGetD3D11DepthStencilView();
	Scene::GetInstance()->GetContext()->RSGetViewports( &m_nOldViewports, &m_pViewports[0]);*/
}

/****************************************************************************
 ****************************************************************************/
void	TextureManager::ItlRestoreOldRenderState()
{
	/*Scene::GetInstance()->GetContext()->OMSetRenderTargets( 1,  &m_pOldRTV,  m_pOldDSV );
	Scene::GetInstance()->GetContext()->RSSetViewports( m_nOldViewports, &m_pViewports[0]);*/
}