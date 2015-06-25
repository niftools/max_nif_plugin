
#include "stdafx.h"

NpComboBox::NpComboBox( HWND hWnd )
{
	init( hWnd );
}

void NpComboBox::init( HWND hWnd )
{
	mWnd = hWnd;
}

int NpComboBox::add( const wchar_t* s )
{
	SendMessage( mWnd, CB_ADDSTRING, 0, (LPARAM) s );
	return count( ) - 1;
}

int NpComboBox::count( ) const
{
	return SendMessage( mWnd, CB_GETCOUNT, 0, 0 );
}

void NpComboBox::select( int i )
{
	SendMessage( mWnd, CB_SETCURSEL, (WPARAM) i, 0 );
}

int NpComboBox::selection( ) const
{
	return SendMessage( mWnd, CB_GETCURSEL, 0, 0 );
}

void NpComboBox::remove( int i )
{
	SendMessage( mWnd, CB_DELETESTRING, 0, (LPARAM) i );
}