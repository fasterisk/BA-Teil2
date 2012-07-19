#ifndef SURFACE_H
#define SURFACE_H

class Surface
{
public:
	/*
	 *  Constructor
	 */
	Surface(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, ID3DX11Effect* pSurfaceEffect);

	/*
	 *  Destrictpr
	 */
	~Surface();
	
	/*
	 *  Model matrix
	 */
	D3DXMATRIX m_mModel;

	/*
	 *  Translation, rotation and scale functions
	 */
	void Translate(float fX, float fY, float fZ);
	void Rotate(D3DXVECTOR3 axis, float fFactor);
	void RotateX(float fFactor);
	void RotateY(float fFactor);
	void RotateZ(float fFactor);
	void Scale(float fFactor);

	/*
	 *  Getter and setter for the iso color
	 *		One of the surfaces has the color 1.0, the other 0.0
	 */
	float GetIsoColor();
	void SetIsoColor(float fIsoColor);

	/*
	 *  Loads a new mesh for this surface
	 */
	HRESULT LoadMesh(std::string strMeshName, std::string strTextureName);

	/*
	 *  Initialize the surface (only called at creation)
	 */
	HRESULT Initialize(std::string strMeshName, std::string strTextureName);

	/*
	 *  Render surface
	 */
	void Render(D3DXMATRIX mViewProjection);

	/*
	 *  Gets called from the voronoi algorithm, renders the surface with its shader
	 */
	void RenderVoronoi(ID3DX11EffectTechnique* pTechnique, ID3DX11EffectShaderResourceVariable *pSurfaceTextureVar);

	/*
	 *  Returns the bounding box of this surface
	 */
	BOUNDINGBOX GetBoundingBox();

protected:
	/*
	 *  Initializes the shader
	 */
	HRESULT InitializeShader();

	/*
	 *  Device and shader effects, inputlayout and the shader variables
	 */
	ID3D11Device*					m_pd3dDevice;
	ID3D11DeviceContext*			m_pd3dImmediateContext;
	ID3DX11Effect*					m_pSurfaceEffect;
	ID3DX11EffectTechnique*			m_pTechnique;
	ID3D11InputLayout*				m_pInputLayout;
	ID3DX11EffectMatrixVariable		*m_pModelViewProjectionVar;
	ID3DX11EffectShaderResourceVariable *m_pSurfaceTextureVar;

	/*
	 *  Vertices of this surface
	 */
	SURFACE_VERTEX* m_pVertices;

	/*
	 *  Vertexbuffer - two index buffers are needed because of the differences in the voronoi 
	 *  algorithm of triangle and edge rendering
	 */
	ID3D11Buffer*	m_pTriangleVertexBuffer;
	ID3D11Buffer*	m_pTriangleIndexBuffer;
	ID3D11Buffer*	m_pEdgeIndexBuffer;
	unsigned int m_mNumVertices;
	unsigned int m_mNumIndices;

	/*
	 *  Texture of the surface and its SRV
	 */
	ID3D11Texture2D	*m_pDiffuseTexture;
	ID3D11ShaderResourceView	*m_pDiffuseTextureSRV;

	/*
	 *  Helping matrices for storing translation and rotation
	 */
	D3DXMATRIX m_mRot;
	D3DXMATRIX m_mTrans;
	D3DXMATRIX m_mTransInv;
	D3DXVECTOR3 m_vTranslation;

	/*
	 *  Isocolor of this surface
	 */
	float m_fIsoColor;

};

#endif