#include "pch.h"

#define REGPATH "Software\\NifTools\\MaxPlugins"

void regSet(HKEY hKey, const char *value, float f);
void regSet(HKEY hKey, const char *value, bool b);
void regSet(HKEY hKey, const char *value, DWORD dw);
void regSet(HKEY hKey, const char *value, int i);
void regSet(HKEY hKey, const char *value, const string &str);
void regGet(HKEY hKey, const char *value, float &f);
void regGet(HKEY hKey, const char *value, bool &b);
void regGet(HKEY hKey, const char *value, DWORD &v);
void regGet(HKEY hKey, const char *value, int &i);
void regGet(HKEY hKey, const char *value, string &str);

void Exporter::writeConfig(INode *node)
{
}

void Exporter::writeConfig()
{
    HKEY hKey;
    if (RegCreateKey(HKEY_CURRENT_USER, REGPATH, &hKey) != ERROR_SUCCESS)
		return;

	regSet(hKey, "npx_ver", mVersion);
	regSet(hKey, "npx_tristrips", mTriStrips);
	regSet(hKey, "npx_hidden", mExportHidden);
	regSet(hKey, "npx_furn", mExportFurn);
	regSet(hKey, "npx_lights", mExportLights);
	regSet(hKey, "npx_vcolors", mVertexColors);
//	regSet(hKey, "npx_wthresh", mWeldThresh);
	regSet(hKey, "npx_tprefix", mTexPrefix);
	regSet(hKey, "npx_coll", mExportCollision);
	regSet(hKey, "npx_remap", mRemapIndices);
}

void Exporter::readConfig(INode *node)
{

}

void Exporter::readConfig()
{
    HKEY hKey;
    if (RegCreateKey(HKEY_CURRENT_USER, REGPATH, &hKey) != ERROR_SUCCESS)
		return;

	DWORD ver;
	regGet(hKey, "npx_ver", ver);
	regGet(hKey, "npx_tristrips", mTriStrips);
	regGet(hKey, "npx_hidden", mExportHidden);
	regGet(hKey, "npx_furn", mExportFurn);
	regGet(hKey, "npx_lights", mExportLights);
	regGet(hKey, "npx_vcolors", mVertexColors);
//	regGet(hKey, "npx_wthresh", mWeldThresh);
	regGet(hKey, "npx_tprefix", mTexPrefix);
	regGet(hKey, "npx_coll", mExportCollision);
	regGet(hKey, "npx_remap", mRemapIndices);
}


void regSet(HKEY hKey, const char *value, float f)
{
	DWORD dw = *((DWORD*)&f);
    RegSetValueEx(hKey, value, NULL, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
}

void regSet(HKEY hKey, const char *value, bool b)
{
	DWORD dw = (DWORD)b;
    RegSetValueEx(hKey, value, NULL, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
}

void regSet(HKEY hKey, const char *value, DWORD dw)
{
    RegSetValueEx(hKey, value, NULL, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
}

void regSet(HKEY hKey, const char *value, int v)
{
	DWORD dw = (DWORD)v;
    RegSetValueEx(hKey, value, NULL, REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));
}

void regSet(HKEY hKey, const char *value, const std::string &str)
{
	RegSetValueEx(hKey, value, NULL, REG_SZ, (LPBYTE)str.c_str(), str.length()+1);
}

void regGet(HKEY hKey, const char *value, float &f)
{
	DWORD dw, type, cdata = sizeof(DWORD);
    if (RegQueryValueEx(hKey, value, NULL, &type, (LPBYTE)&dw, &cdata)==ERROR_SUCCESS && type==REG_DWORD)
		f = *((float*)&dw);
}

void regGet(HKEY hKey, const char *value, bool &b)
{
	DWORD dw, type, cdata = sizeof(DWORD);
    if (RegQueryValueEx(hKey, value, NULL, &type, (LPBYTE)&dw, &cdata)==ERROR_SUCCESS && type==REG_DWORD)
		b = (bool)dw;
}

void regGet(HKEY hKey, const char *value, DWORD &v)
{
	DWORD dw, type, cdata = sizeof(DWORD);
    if (RegQueryValueEx(hKey, value, NULL, &type, (LPBYTE)&dw, &cdata)==ERROR_SUCCESS && type==REG_DWORD)
		v = dw;
}

void regGet(HKEY hKey, const char *value, int &v)
{
	DWORD dw, type, cdata = sizeof(DWORD);
    if (RegQueryValueEx(hKey, value, NULL, &type, (LPBYTE)&dw, &cdata)==ERROR_SUCCESS && type==REG_DWORD)
		v = dw;
}

void regGet(HKEY hKey, const char *value, string &str)
{
	char buff[MAX_PATH];
	DWORD type, cdata = MAX_PATH;
    if (RegQueryValueEx(hKey, value, NULL, &type, (LPBYTE)buff, &cdata)==ERROR_SUCCESS && type==REG_SZ)
		str = buff;
}
