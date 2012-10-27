#ifndef __NIFGUI_H__
#define __NIFGUI_H__

/* Combo box control wrapper. */
class NpComboBox
{

public:

	NpComboBox(HWND hWnd=NULL);

	void			init(HWND hWnd);

	int				add(const char *);

	int				count() const;
	void			select(int i);
	int 			selection() const;

private:

	HWND			mWnd;
};

#endif __NIFGUI_H__
