#ifndef _TEXTUREMANAGER_H_
#define _TEXTUREMANAGER_H_

#include "Globals.h"
#include <map>
#include <string>

class TextureManager
{
public:
	struct TEXTURESTATE
	{
		std::string sDebugName;
		unsigned int nID;
		bool bBoundAsSRV;
		bool bBoundAsRTV;
		unsigned int nType; // 0 if 2D, 1 if 3D
		int iWidth;
		int iHeight;
		int iDepth; //1 if 2D
	};

	struct DEPTHBUFFERSTATE
	{
		std::string sDebugName;
		unsigned int nID;
		bool bBound;
	};

	static TextureManager* GetInstance();
	static void DeleteInstance();

	
	unsigned int	Create2DTexture(const std::string sDebugName,
									const int iWidth, 
									const int iHeight);

	unsigned int	Create3DTexture(const std::string sDebugName,
									const int iWidth, 
									const int iHeight, 
									const int iDepth);

	unsigned int	Create2DDepthBuffer(const std::string sDebugName,
										const int iWidth, 
										const int iHeight);

	void	Update2DTexture(const unsigned int nID, 
							const int iWidth, 
							const int iHeight);

	void	Update3DTexture(const unsigned int nID, 
							const int iWidth, 
							const int iHeight, 
							const int iDepth);

	void	Update2DDepthBuffer(const unsigned int nID, 
								const int iWidth, 
								const int iHeight);

	void	Clear2DDepthBuffer(const unsigned int nID);

	void	BindTextureAsRTV(const unsigned int nID);

	void	BindTextureAsRTV(const unsigned int nID1, 
							 const unsigned int nID2);

	void	BindTextureAsRTV(const unsigned int nID1, 
							 const unsigned int nID2, 
							 const unsigned int nDepthBufferID);

	void	UnBindRTVs();

	void	BindTextureAsSRV(const unsigned int nID, 
							 ID3DX11EffectShaderResourceVariable* pSRVar);

	void	UnBindSRV(ID3DX11EffectShaderResourceVariable* pSRVar);
	
	void	Render2DTextureInto3DSlice(const unsigned int n2DTexture, 
									   const unsigned int n3DTexture,
									   const int iSliceIndex);

	TEXTURESTATE		GetTextureState(const unsigned int nID);
	DEPTHBUFFERSTATE	GetDepthBufferState(const unsigned int nID);

protected:
	TextureManager();
	~TextureManager();

	void	ItlCreate2DTexture(const std::string sDebugName, 
							   const unsigned int nID, 
							   const int iWidth, 
							   const int iHeight);

	void	ItlCreate3DTexture(const std::string sDebugName, 
							   const unsigned int nID, 
							   const int iWidth, 
							   const int iHeight, 
							   const int iDepth);

	void	ItlCreate2DDepthBuffer(const std::string sDebugName, 
								   const unsigned int nID, 
								   const int iWidth, 
								   const int iHeight);
	
	unsigned int	ItlGetNewTextureID();
	unsigned int	ItlGetNewDepthBufferID();
	
	void	ItlStoreOldRenderState();
	void	ItlRestoreOldRenderState();


	static TextureManager* s_pInstance;

	ID3D11RenderTargetView*	m_pOldRTV;
	ID3D11DepthStencilView* m_pOldDSV;
	unsigned int m_nOldViewports;
	D3D11_VIEWPORT *m_pViewports;

	unsigned int m_nCurrentTextureID;
	unsigned int m_nCurrentDepthBufferID;

	std::map<unsigned int, ID3D11Resource*>				m_TextureMap;
	std::map<unsigned int, ID3D11RenderTargetView*>		m_TextureRTVs;
	std::map<unsigned int, ID3D11ShaderResourceView*>	m_TextureSRVs;
	std::map<unsigned int, TEXTURESTATE>				m_TextureStateMap;

	std::map<unsigned int, ID3D11Texture2D*>			m_DepthBufferMap;
	std::map<unsigned int, ID3D11DepthStencilView*>		m_DepthBufferViewMap;
	std::map<unsigned int, DEPTHBUFFERSTATE>			m_DepthBufferStateMap;
};
#endif //_TEXTUREMANAGER_H_