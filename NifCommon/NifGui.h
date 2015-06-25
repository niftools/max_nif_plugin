
#pragma once

/* Combo box control wrapper */
class NpComboBox
{
public:
	NpComboBox( HWND hWnd = NULL );

	void		init( HWND hWnd );
	int			add( const wchar_t* );
	int			count( ) const;
	void		select( int i );
	int			selection( ) const;
	void		remove( int i );

private:

	HWND		mWnd;
};