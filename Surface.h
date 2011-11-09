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

	Surface();
	~Surface();

	void ReadVectorFile(char *s);
};