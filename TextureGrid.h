
class TextureGrid
{
public:
	TextureGrid(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	~TextureGrid();

	HRESULT Initialize(int iTextureWidth, int iTextureHeight, int iTextureDepth, ID3DX11EffectTechnique* technique);

}