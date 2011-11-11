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


	Surface();
	~Surface();

	void ReadVectorFile(char *s);
};