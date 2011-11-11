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

	ID3D11Buffer* m_vertexbuffer;

	D3DXMATRIX m_mModel;

	Surface();
	~Surface();

	HRESULT InitVertexBuffer(ID3D11Device* pd3dDevice);
	void Render(ID3D11DeviceContext* pd3dImmediateContext);

	void ReadVectorFile(char *s);
};