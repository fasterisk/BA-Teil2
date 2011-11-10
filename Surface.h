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

struct BEZIER_CONTROL_POINT
{
	float x;
	float y;
	float z;
};

class Surface
{
public:
	int m_pNum;
	BEZIER_CONTROL_POINT *m_controlpoints;
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