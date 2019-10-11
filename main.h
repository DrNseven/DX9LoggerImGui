#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "winmm.lib")//time

//detours
#include "detours.h"
#if defined _M_X64
#pragma comment(lib, "detours.X64/detours.lib")
#elif defined _M_IX86
#pragma comment(lib, "detours.X86/detours.lib")
#endif

//DX Includes
//#include <DirectXMath.h>
//using namespace DirectX;

//dx sdk if files are in ..\DXSDK dir
//#include "DXSDK\d3dx9.h"
//#if defined _M_X64
//#pragma comment(lib, "DXSDK/x64/d3dx9.lib") 
//#elif defined _M_IX86
//#pragma comment(lib, "DXSDK/x86/d3dx9.lib")
//#endif

#pragma warning (disable: 4244) 

//==========================================================================================================================

//features

//item states
//bool wallhack = 1;			
int wallhacktype = 0;
//int chamtype = 1;
bool aimbot = 1;
int aimkey = 1;
DWORD Daimkey = VK_SHIFT;
int aimsens = 2;
int aimfov = 3;
int aimheight = 3;
bool autoshoot = 0;
unsigned int asdelay = 49;
bool IsPressed = false;
DWORD astime = timeGetTime();

//==========================================================================================================================

HMODULE Hand;

UINT Stride;
D3DVERTEXBUFFER_DESC vdesc;

IDirect3DVertexDeclaration9* pDecl;
D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH];
UINT numElements;

UINT mStartregister;
UINT mVectorCount;

IDirect3DVertexShader9* vShader;
UINT vSize;

IDirect3DPixelShader9* pShader;
UINT pSize;

IDirect3DTexture9 *texture;
D3DSURFACE_DESC sDesc;
DWORD qCRC;
D3DLOCKED_RECT pLockedRect;

D3DVIEWPORT9 Viewport;
float ScreenCX;
float ScreenCY;

int countStride = -1;
int countnumElements = -1;
int countNumVertices = -1;
int countvSize = -1;
int countpSize = -1;
int countmStartregister = -1;
int countmVectorCount = -1;
int countnumPrimitives = -1;

LPDIRECT3DTEXTURE9 Red, Green, Blue, Yellow;

bool ShowMenu = false;
bool info = true;
bool is_imgui_initialised = false;
bool is_wndproc_initialised = false;
DWORD wndproctime = timeGetTime();

//==========================================================================================================================

// getdir & log
using namespace std;
char dlldir[320];
char* GetDirFile(char *name)
{
	static char pldir[320];
	strcpy_s(pldir, dlldir);
	strcat_s(pldir, name);
	return pldir;
}

void Log(const char *fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile(GetDirFile((PCHAR)"log.txt"), ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}

DWORD QuickChecksum(DWORD *pData, int size)
{
	if (!pData) { return 0x0; }

	DWORD sum;
	DWORD tmp;
	sum = *pData;

	for (int i = 1; i < (size / 4); i++)
	{
		tmp = pData[i];
		tmp = (DWORD)(sum >> 29) + tmp;
		tmp = (DWORD)(sum >> 17) + tmp;
		sum = (DWORD)(sum << 3) ^ tmp;
	}

	return sum;
}

// The main window handle of the game.
//HWND game_hwnd = FindWindowA(0, "Deadpool");
//HWND game_hwnd = FindWindowA(0, "Star Wars Battlefront II");

// The main window handle of the game.
HWND game_hwnd = NULL;

// Used to find windows belonging to the game process.
BOOL CALLBACK find_game_hwnd(HWND hwnd, LPARAM game_pid) {
	// Skip windows not belonging to the game process.
	DWORD hwnd_pid = NULL;

	GetWindowThreadProcessId(hwnd, &hwnd_pid);

	if (hwnd_pid != game_pid)
		return TRUE;

	// Set the target window handle and stop the callback.
	game_hwnd = hwnd;

	return FALSE;
}

//==========================================================================================================================

//calc distance
float GetDistance(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

struct ModelEspInfo_t
{
	float pOutX, pOutY, RealDistance, pOut2X, pOut2Y, vSize;
	float CrosshairDistance;
};
std::vector<ModelEspInfo_t>ModelEspInfo;
/*
//w2s
void AddModels(LPDIRECT3DDEVICE9 Device)
{
	D3DMATRIX viewMatrix, worldmatrix;
	D3DXVECTOR4 position;
	D3DXVECTOR2 out;
	D3DXVECTOR4 vTransformed;

	Device->GetVertexShaderConstantF(8, (D3DXMATRIX)viewMatrix, 4); //different in every game
	Device->GetVertexShaderConstantF(16, (D3DXMATRIX)worldmatrix, 4); //different in every game

	position.x = worldmatrix._44;
	position.y = worldmatrix._34;
	position.z = worldmatrix._24;
	position.w = worldmatrix._14;

	D3DXMatrixTranspose(&(D3DXMATRIX)viewMatrix, &(D3DXMATRIX)viewMatrix);

	vTransformed.x = (float)(position.y * viewMatrix.m[0][1]) + (float)(position.x * viewMatrix.m[0][0]) + (float)(position.z * viewMatrix.m[0][2]) + viewMatrix.m[0][3];
	vTransformed.y = (float)(position.y * viewMatrix.m[1][1]) + (float)(position.x * viewMatrix.m[1][0]) + (float)(position.z * viewMatrix.m[1][2]) + viewMatrix.m[1][3];
	vTransformed.z = (float)(position.y * viewMatrix.m[2][1]) + (float)(position.x * viewMatrix.m[2][0]) + (float)(position.z * viewMatrix.m[2][2]) + viewMatrix.m[2][3];
	vTransformed.w = (float)(position.y * viewMatrix.m[3][1]) + (float)(position.x * viewMatrix.m[3][0]) + (float)(position.z * viewMatrix.m[3][2]) + viewMatrix.m[3][3];

	vTransformed.x *= 1.0 / vTransformed.w;
	vTransformed.y *= 1.0 / vTransformed.w;

	const int width = Viewport.Width;
	const int height = Viewport.Height;

	out.x = width / 2 - vTransformed.x * (width / 2);
	out.y = height / 2 - vTransformed.y * (height / 2);
	//out.y = height * 6/10 - vTransformed.y * (-height * 6 / 10);

	ModelEspInfo_t pModelEspInfo = { static_cast<float>(out.x), static_cast<float>(out.y), static_cast<float>(vTransformed.w) };
	ModelEspInfo.push_back(pModelEspInfo);
}
*/
//==========================================================================================================================

HRESULT GenerateTexture(IDirect3DDevice9 *pDevice, IDirect3DTexture9 **ppD3Dtex, DWORD colour32)
{
	if (FAILED(pDevice->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
		return E_FAIL;

	WORD colour16 = ((WORD)((colour32 >> 28) & 0xF) << 12)
		| (WORD)(((colour32 >> 20) & 0xF) << 8)
		| (WORD)(((colour32 >> 12) & 0xF) << 4)
		| (WORD)(((colour32 >> 4) & 0xF) << 0);

	D3DLOCKED_RECT d3dlr;
	(*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
	WORD *pDst16 = (WORD*)d3dlr.pBits;

	for (int xy = 0; xy < 8 * 8; xy++)
		*pDst16++ = colour16;

	(*ppD3Dtex)->UnlockRect(0);

	return S_OK;
}

//==========================================================================================================================

void SaveCfg()
{
	ofstream fout;
	fout.open(GetDirFile((PCHAR)"dx9imgui.ini"), ios::trunc);
	fout << "wallhack " << wallhacktype << endl;
	//fout << "chamtypes " << chamtype << endl;
	fout << "aimbot " << aimbot << endl;
	fout << "aimkey " << aimkey << endl;
	fout << "aimsens " << aimsens << endl;
	fout << "aimfov " << aimfov << endl;
	fout << "aimheight " << aimheight << endl;
	fout << "autoshoot " << autoshoot << endl;
	fout.close();
}

void LoadCfg()
{
	ifstream fin;
	string Word = "";
	fin.open(GetDirFile((PCHAR)"dx9imgui.ini"), ifstream::in);
	fin >> Word >> wallhacktype;
	//fin >> Word >> chamtype;
	fin >> Word >> aimbot;
	fin >> Word >> aimkey;
	fin >> Word >> aimsens;
	fin >> Word >> aimfov;
	fin >> Word >> aimheight;
	fin >> Word >> autoshoot;
	fin.close();
}

//==========================================================================================================================
