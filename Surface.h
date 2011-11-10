#pragma once

struct COLORPOINT
{
	D3DXVECTOR3 col;
	int			off;
};

struct BLURRPOINT
{
	float		blurr;
	int			off;
};

struct CB_PER_FRAME_CONSTANTS
{
    D3DXMATRIX mViewProjection;
    D3DXVECTOR3 vCameraPosWorld;
    float fTessellationFactor;
};

class Surface
{
public:
	int m_pNum;
	D3DXVECTOR3 *m_controlpoints;
	int m_clNum;
	COLORPOINT *m_colors_left;
	int m_crNum;
	COLORPOINT *m_colors_right;
	int m_bNum;
	BLURRPOINT *m_blurrpoints;

	ID3D11Buffer *m_vertexBuffer;


	Surface();
	~Surface();

	void InitBuffers(ID3D11Device *pd3dDevice);
	void Render(ID3D11Device *pd3dDevice);

	void ReadVectorFile(char *s);
	void ConstructSurface(ID3D11Device *pd3dDevice);
};