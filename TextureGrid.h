struct VS_INPUT_DIFFUSION_STRUCT
{
	D3DXVECTOR3 Pos; // Clip space position for slice vertices
    D3DXVECTOR3 Tex; // Cell coordinates in 0-"texture dimension" range
};

class TextureGrid
{
public:
	TextureGrid(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
	~TextureGrid();

	HRESULT Initialize(int iTextureWidth, int iTextureHeight, int iTextureDepth, ID3DX11EffectTechnique* pTechnique);

	void DrawSlices();
    
    /*int  GetCols(){return m_iCols;};
    int  GetRows(){return m_iRows;};*/

protected:
	ID3D11Device*			m_pd3dDevice;
	ID3D11DeviceContext*	m_pd3dImmediateContext;

	ID3D11InputLayout*      m_pLayout;
    ID3D11Buffer*           m_pSlicesBuffer;

	int m_iTextureWidth;
	int m_iTextureHeight;
	int m_iTextureDepth;
	int m_iMaxDim;

    int m_iNumVerticesSlices;
	int m_iCols;
    int m_iRows;

	HRESULT CreateVertexBuffers(ID3DX11EffectTechnique* pTechnique);

	// Helper functions
	HRESULT CreateLayout		(ID3DX11EffectTechnique* pTechnique);
	HRESULT CreateVertexBuffer	(int ByteWidth, UINT bindFlags, ID3D11Buffer** vertexBuffer,VS_INPUT_DIFFUSION_STRUCT* vertices,int numVertices);
	void InitSlice				(int z, VS_INPUT_DIFFUSION_STRUCT** vertices, int& index);

	void DrawPrimitive(D3D11_PRIMITIVE_TOPOLOGY PrimitiveType, ID3D11InputLayout* layout, ID3D11Buffer** vertexBuffer,UINT* stride, UINT* offset, UINT StartVertex, UINT VertexCount );
};