class Surface;
class BoundingBox;
class VolumeRenderer;
class TextureGrid;


class Scene {
public:

	Scene(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	~Scene();

	HRESULT Initialize(int iTexWidth, int iTexHeight, int iTexDepth);
	HRESULT SetScreenSize(int iWidth, int iHeight);

	void Render(D3DXMATRIX mViewProjection);

	void ChangeControlledSurface();
	void Translate(float fX, float fY, float fZ);
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void Scale(float fFactor);
	

protected:
	// Device
	ID3D11Device*			m_pd3dDevice;
	ID3D11DeviceContext*	m_pd3dImmediateContext;

	// TextureGrid
	TextureGrid*			m_pTextureGrid;

	// VolumeRenderer
	VolumeRenderer*			m_pVolumeRenderer;
	
	// Bounding Box (contains surfaces)
	BoundingBox*			m_pBoundingBox;

	HRESULT CreateEffect(WCHAR* name, ID3DX11Effect **ppEffect);
	HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );
};