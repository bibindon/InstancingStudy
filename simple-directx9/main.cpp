#pragma comment( lib, "d3d9.lib" )
#if defined(DEBUG) || defined(_DEBUG)
#pragma comment( lib, "d3dx9d.lib" )
#else
#pragma comment( lib, "d3dx9.lib" )
#endif

#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <tchar.h>
#include <cassert>
#include <crtdbg.h>
#include <vector>

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = NULL; } }

LPDIRECT3D9 g_pD3D = NULL;
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
LPD3DXFONT g_pFont = NULL;
LPD3DXMESH g_pMesh = NULL;
std::vector<D3DMATERIAL9> g_pMaterials;
std::vector<LPDIRECT3DTEXTURE9> g_pTextures;
DWORD g_dwNumMaterials = 0;
LPD3DXEFFECT g_pEffect = NULL;
bool g_bClose = false;

struct WorldPos
{
    float x;
    float y;
};

WorldPos* m_worldPos;
IDirect3DVertexBuffer9* m_worldPosBuf;
IDirect3DVertexDeclaration9* m_decl;

const int W = 100;
const int H = 100;
const int tipNum = W * H;               // �X�N���[����̃`�b�v����

static void TextDraw(LPD3DXFONT pFont, TCHAR* text, int X, int Y);
static void InitD3D(HWND hWnd);
static void Cleanup();
static void Render();
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void copyBuf(unsigned sz, void* src, IDirect3DVertexBuffer9* buf)
{
    void* p = 0;
    buf->Lock(0, 0, &p, 0);
    memcpy(p, src, sz);
    buf->Unlock();
}

extern int WINAPI _tWinMain(_In_ HINSTANCE hInstance,
                            _In_opt_ HINSTANCE hPrevInstance,
                            _In_ LPTSTR lpCmdLine,
                            _In_ int nCmdShow);

int WINAPI _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR lpCmdLine,
                     _In_ int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    WNDCLASSEX wc { };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = MsgProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T("Window1");
    wc.hIconSm = NULL;

    ATOM atom = RegisterClassEx(&wc);
    assert(atom != 0);

    RECT rect;
    SetRect(&rect, 0, 0, 1920, 1080);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    rect.right = rect.right - rect.left;
    rect.bottom = rect.bottom - rect.top;
    rect.top = 0;
    rect.left = 0;

    HWND hWnd = CreateWindow(_T("Window1"),
                             _T("Hello DirectX9 World !!"),
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             rect.right,
                             rect.bottom,
                             NULL,
                             NULL,
                             wc.hInstance,
                             NULL);

    InitD3D(hWnd);
    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    MSG msg;

    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            DispatchMessage(&msg);
        }
        else
        {
            Sleep(16);
            Render();
        }

        if (g_bClose)
        {
            break;
        }
    }

    Cleanup();

    UnregisterClass(_T("Window1"), wc.hInstance);
    return 0;
}

void TextDraw(LPD3DXFONT pFont, TCHAR* text, int X, int Y)
{
    RECT rect = { X, Y, 0, 0 };

    // DrawText�̖߂�l�͕������ł���B
    // ���̂��߁AhResult�̒��g�������ł��G���[���N���Ă���킯�ł͂Ȃ��B
    HRESULT hResult = pFont->DrawText(NULL,
                                      text,
                                      -1,
                                      &rect,
                                      DT_LEFT | DT_NOCLIP,
                                      D3DCOLOR_ARGB(255, 0, 0, 0));

    assert((int)hResult >= 0);
}

void InitD3D(HWND hWnd)
{
    HRESULT hResult = E_FAIL;

    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    assert(g_pD3D != NULL);

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.BackBufferCount = 1;
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality = 0;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.hDeviceWindow = hWnd;
    d3dpp.Flags = 0;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    hResult = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                                   D3DDEVTYPE_HAL,
                                   hWnd,
                                   D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                   &d3dpp,
                                   &g_pd3dDevice);

    if (FAILED(hResult))
    {
        hResult = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT,
                                       D3DDEVTYPE_HAL,
                                       hWnd,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                       &d3dpp,
                                       &g_pd3dDevice);

        assert(hResult == S_OK);
    }

    hResult = D3DXCreateFont(g_pd3dDevice,
                             20,
                             0,
                             FW_HEAVY,
                             1,
                             FALSE,
                             SHIFTJIS_CHARSET,
                             OUT_TT_ONLY_PRECIS,
                             CLEARTYPE_NATURAL_QUALITY,
                             FF_DONTCARE,
                             _T("�l�r �S�V�b�N"),
                             &g_pFont);

    assert(hResult == S_OK);

    LPD3DXBUFFER pD3DXMtrlBuffer = NULL;

    hResult = D3DXLoadMeshFromX(_T("cube.x"),
                                D3DXMESH_SYSTEMMEM,
                                g_pd3dDevice,
                                NULL,
                                &pD3DXMtrlBuffer,
                                NULL,
                                &g_dwNumMaterials,
                                &g_pMesh);

    assert(hResult == S_OK);

    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    g_pMaterials.resize(g_dwNumMaterials);
    g_pTextures.resize(g_dwNumMaterials);

    for (DWORD i = 0; i < g_dwNumMaterials; i++)
    {
        g_pMaterials[i] = d3dxMaterials[i].MatD3D;
        g_pMaterials[i].Ambient = g_pMaterials[i].Diffuse;
        g_pTextures[i] = NULL;
        
        //--------------------------------------------------------------
        // Unicode�����Z�b�g�ł��}���`�o�C�g�����Z�b�g�ł�
        // "d3dxMaterials[i].pTextureFilename"�̓}���`�o�C�g�����Z�b�g�ɂȂ�B
        // 
        // ����ŁAD3DXCreateTextureFromFile�̓v���W�F�N�g�ݒ��
        // Unicode�����Z�b�g���}���`�o�C�g�����Z�b�g���ς��B
        //--------------------------------------------------------------

        std::string pTexPath(d3dxMaterials[i].pTextureFilename);

        if (!pTexPath.empty())
        {
            bool bUnicode = false;

#ifdef UNICODE
            bUnicode = true;
#endif

            if (!bUnicode)
            {
                hResult = D3DXCreateTextureFromFileA(g_pd3dDevice, pTexPath.c_str(), &g_pTextures[i]);
                assert(hResult == S_OK);
            }
            else
            {
                int len = MultiByteToWideChar(CP_ACP, 0, pTexPath.c_str(), -1, nullptr, 0);
                std::wstring pTexPathW(len, 0);
                MultiByteToWideChar(CP_ACP, 0, pTexPath.c_str(), -1, &pTexPathW[0], len);

                hResult = D3DXCreateTextureFromFileW(g_pd3dDevice, pTexPathW.c_str(), &g_pTextures[i]);
                assert(hResult == S_OK);
            }
        }
    }

    hResult = pD3DXMtrlBuffer->Release();
    assert(hResult == S_OK);

    hResult = D3DXCreateEffectFromFile(g_pd3dDevice,
                                       _T("simple.fx"),
                                       NULL,
                                       NULL,
                                       D3DXSHADER_DEBUG,
                                       NULL,
                                       &g_pEffect,
                                       NULL);

    assert(hResult == S_OK);

    // ���[���h���W�ʒu�o�b�t�@
    WorldPos* worldPos = new WorldPos[tipNum];

    for (int w = 0; w < W; w++)
    {
        for (int h = 0; h < H; h++)
        {
            int e = h * W + w;
            worldPos[e].x = 5.f * (w - 50);
            worldPos[e].y = 5.f * (h - 50);
        }
    }

    g_pd3dDevice->CreateVertexBuffer(sizeof(WorldPos) * tipNum,
                                     0,
                                     0,
                                     D3DPOOL_MANAGED,
                                     &m_worldPosBuf,
                                     0);

    copyBuf(sizeof(WorldPos) * tipNum, worldPos, m_worldPosBuf);

    delete[] worldPos;

    // ���_�錾�쐬
    D3DVERTEXELEMENT9 declElems[] =
    {
        // POSITION
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },

        // NORMAL0
        { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },

        // TEXCOORD0
        { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },

        // TEXCOORD1
        // ���[���h�ʒu
        { 1, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },

        D3DDECL_END()
    };

    g_pd3dDevice->CreateVertexDeclaration(declElems, &m_decl);
}

void Cleanup()
{
    for (auto& texture : g_pTextures)
    {
        SAFE_RELEASE(texture);
    }

    SAFE_RELEASE(g_pMesh);
    SAFE_RELEASE(g_pEffect);
    SAFE_RELEASE(g_pFont);
    SAFE_RELEASE(g_pd3dDevice);
    SAFE_RELEASE(g_pD3D);
}

void Render()
{
    HRESULT hResult = E_FAIL;

    static float f = 0.0f;
    f += 0.025f;

    D3DXMATRIX mat;
    D3DXMATRIX View, Proj;

    D3DXMatrixPerspectiveFovLH(&Proj,
                               D3DXToRadian(45),
                               1920.0f / 1080.0f,
                               1.0f,
                               10000.0f);

    D3DXVECTOR3 vec1(10 * sinf(f), 10, -10 * cosf(f));
    D3DXVECTOR3 vec2(0, 0, 0);
    D3DXVECTOR3 vec3(0, 1, 0);
    D3DXMatrixLookAtLH(&View, &vec1, &vec2, &vec3);
    D3DXMatrixIdentity(&mat);
    mat = mat * View * Proj;

    hResult = g_pEffect->SetMatrix("g_matWorldViewProj", &mat);
    assert(hResult == S_OK);

    LPDIRECT3DVERTEXBUFFER9 pVB = nullptr;
    g_pMesh->GetVertexBuffer(&pVB);
    g_pd3dDevice->SetStreamSource(0, pVB, 0, g_pMesh->GetNumBytesPerVertex());
    pVB->Release();

    g_pd3dDevice->SetStreamSource(1, m_worldPosBuf, 0, sizeof(WorldPos));

    g_pd3dDevice->SetVertexDeclaration(m_decl);

    g_pd3dDevice->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | tipNum);
    g_pd3dDevice->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1);

    // �C���f�b�N�X�o�b�t�@���Z�b�g
    LPDIRECT3DINDEXBUFFER9 pIB = nullptr;
    g_pMesh->GetIndexBuffer(&pIB);
    g_pd3dDevice->SetIndices(pIB);
    pIB->Release();

    hResult = g_pd3dDevice->Clear(0,
                                  NULL,
                                  D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                                  D3DCOLOR_XRGB(100, 100, 100),
                                  1.0f,
                                  0);

    assert(hResult == S_OK);

    hResult = g_pd3dDevice->BeginScene();
    assert(hResult == S_OK);

    TCHAR msg[100];
    _tcscpy_s(msg, 100, _T("X�t�@�C���̓ǂݍ��݂ƕ\��"));
    TextDraw(g_pFont, msg, 0, 0);

    hResult = g_pEffect->SetTechnique("Technique1");
    assert(hResult == S_OK);

    UINT numPass;
    hResult = g_pEffect->Begin(&numPass, 0);
    assert(hResult == S_OK);

    hResult = g_pEffect->BeginPass(0);
    assert(hResult == S_OK);

    for (DWORD i = 0; i < g_dwNumMaterials; i++)
    {
        hResult = g_pEffect->SetTexture("texture1", g_pTextures[i]);
        assert(hResult == S_OK);

        hResult = g_pEffect->CommitChanges();
        assert(hResult == S_OK);

        DWORD numVertices = g_pMesh->GetNumVertices();
        DWORD numFaces = g_pMesh->GetNumFaces();

        // �`�� (�C���X�^���X���� SetStreamSourceFreq �Ō��܂�)
        g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                                           0,
                                           0,
                                           g_pMesh->GetNumVertices(),
                                           0,
                                           g_pMesh->GetNumFaces() );

        assert(hResult == S_OK);
    }

    hResult = g_pEffect->EndPass();
    assert(hResult == S_OK);

    hResult = g_pEffect->End();
    assert(hResult == S_OK);

    g_pd3dDevice->SetStreamSourceFreq(1, 0);

    hResult = g_pd3dDevice->EndScene();
    assert(hResult == S_OK);

    hResult = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    assert(hResult == S_OK);
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        g_bClose = true;
        return 0;
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

