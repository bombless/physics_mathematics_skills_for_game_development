//------------------------------------------------------------
// Scroll_5_1.cpp
// 遠近ラスタスクロール
// 
//------------------------------------------------------------

#include <windows.h>
#include <math.h>

#define PI					3.14159265f			// 円周率
#define VIEW_WIDTH			640					// 画面幅
#define VIEW_HEIGHT			480					// 画面高さ
#define PIC_WIDTH_UP		960					// 絵の上の幅
#define PIC_WIDTH_DOWN		1920				// 絵の下の幅
#define CAMERA_VEL			10.0f				// カメラ速さ

float		fCamera_x;
float		fBack_x;

struct DRAWPOINT {
	float			x, y;
};

DRAWPOINT		v2Points[VIEW_HEIGHT];			// 描画ラインの位置


int InitBack( void )							// 最初に１回だけ呼ばれる
{
	fCamera_x = VIEW_WIDTH / 2.0f;				// カメラの初期位置
	fBack_x = 0.0f;								// 背景の初期位置

	return 0;
}


int MoveBack( void )							// 毎フレーム呼ばれる
{
	int						i;
	float					fLineWidth;			// 現在のラインの絵の幅
	float					fLineBase;			// ラインのベース位置

	// 左キーが押されていれば左へ
	if ( GetAsyncKeyState( VK_LEFT ) ) {
		fCamera_x -= CAMERA_VEL;
		if ( fCamera_x < VIEW_WIDTH / 2.0f ) {
			fCamera_x = VIEW_WIDTH / 2.0f;
		}
	}
	// 右キーが押されていれば右へ
	if ( GetAsyncKeyState( VK_RIGHT ) ) {
		fCamera_x += CAMERA_VEL;
		if ( fCamera_x > ( float )( PIC_WIDTH_DOWN - VIEW_WIDTH / 2.0f ) ) {
			fCamera_x = ( float )( PIC_WIDTH_DOWN - VIEW_WIDTH / 2.0f );
		}
	}

	fBack_x = VIEW_WIDTH / 2.0f - fCamera_x;

	// ラスタ位置セット
	fLineWidth = PIC_WIDTH_UP;
	fLineBase = ( PIC_WIDTH_DOWN - PIC_WIDTH_UP ) / 2.0f;
	for ( i = 0; i < VIEW_HEIGHT; i++ ) {
		v2Points[i].y = ( float )i;
		v2Points[i].x = fBack_x * ( fLineWidth - VIEW_WIDTH ) / ( PIC_WIDTH_DOWN - VIEW_WIDTH ) - fLineBase;
		fLineWidth += ( float )( PIC_WIDTH_DOWN - PIC_WIDTH_UP ) / VIEW_HEIGHT;
		fLineBase -= ( float )( PIC_WIDTH_DOWN - PIC_WIDTH_UP ) / VIEW_HEIGHT / 2.0f;
	}

	return 0;
}


//------------------------------------------------------------
// 以下、DirectXによる表示プログラム

#include <stdio.h>
#include <windows.h>
#include <tchar.h>								// Unicode・マルチバイト文字関係

#include <D3D11.h>
#include <D3DX11.h>
#include <D3Dcompiler.h>
#include <xnamath.h>


#define MAX_BUFFER_VERTEX				10000	// 最大バッファ頂点数


// リンクライブラリ
#pragma comment( lib, "d3d11.lib" )   // D3D11ライブラリ
#pragma comment( lib, "d3dx11.lib" )


// セーフリリースマクロ
#ifndef SAFE_RELEASE
#define SAFE_RELEASE( p )      { if ( p ) { ( p )->Release(); ( p )=NULL; } }
#endif


// 頂点構造体
struct CUSTOMVERTEX {
    XMFLOAT4	v4Pos;
    XMFLOAT4	v4Color;
	XMFLOAT2	v2UV;
};

// シェーダ定数構造体
struct CBNeverChanges
{
    XMMATRIX mView;
};

// テクスチャ絵構造体
struct TEX_PICTURE {
	ID3D11ShaderResourceView	*pSRViewTexture;
	D3D11_TEXTURE2D_DESC		tdDesc;
	int							nWidth, nHeight;
};


// グローバル変数
UINT  g_nClientWidth;							// 描画領域の横幅
UINT  g_nClientHeight;							// 描画領域の高さ

HWND        g_hWnd;         // ウィンドウハンドル


ID3D11Device			*g_pd3dDevice;			// デバイス
IDXGISwapChain			*g_pSwapChain;			// DXGIスワップチェイン
ID3D11DeviceContext		*g_pImmediateContext;	// デバイスコンテキスト
ID3D11RasterizerState	*g_pRS;					// ラスタライザ
ID3D11RenderTargetView	*g_pRTV;				// レンダリングターゲット
D3D_FEATURE_LEVEL       g_FeatureLevel;			// フィーチャーレベル

ID3D11Buffer			*g_pD3D11VertexBuffer;
ID3D11BlendState		*g_pbsAlphaBlend;
ID3D11VertexShader		*g_pVertexShader;
ID3D11PixelShader		*g_pPixelShader;
ID3D11InputLayout		*g_pInputLayout;
ID3D11SamplerState		*g_pSamplerState;

ID3D11Buffer			*g_pCBNeverChanges = NULL;

TEX_PICTURE				g_tBack;

// 描画頂点バッファ
CUSTOMVERTEX g_cvVertices[MAX_BUFFER_VERTEX];
int							g_nVertexNum = 0;
ID3D11ShaderResourceView	*g_pNowTexture = NULL;


// Direct3Dの初期化
HRESULT InitD3D( void )
{
    HRESULT hr = S_OK;
	D3D_FEATURE_LEVEL  FeatureLevelsRequested[6] = { D3D_FEATURE_LEVEL_11_0,
													 D3D_FEATURE_LEVEL_10_1,
													 D3D_FEATURE_LEVEL_10_0,
													 D3D_FEATURE_LEVEL_9_3,
													 D3D_FEATURE_LEVEL_9_2,
													 D3D_FEATURE_LEVEL_9_1 };
	UINT               numLevelsRequested = 6;
	D3D_FEATURE_LEVEL  FeatureLevelsSupported;

	// デバイス作成
	hr = D3D11CreateDevice( NULL,
					D3D_DRIVER_TYPE_HARDWARE, 
					NULL, 
					0,
					FeatureLevelsRequested, 
					numLevelsRequested,
					D3D11_SDK_VERSION, 
					&g_pd3dDevice,
					&FeatureLevelsSupported,
					&g_pImmediateContext );
	if( FAILED ( hr ) ) {
		return hr;
	}

	// ファクトリの取得
	IDXGIDevice * pDXGIDevice;
	hr = g_pd3dDevice->QueryInterface( __uuidof( IDXGIDevice ), ( void ** )&pDXGIDevice );
	IDXGIAdapter * pDXGIAdapter;
	hr = pDXGIDevice->GetParent( __uuidof( IDXGIAdapter ), ( void ** )&pDXGIAdapter );
	IDXGIFactory * pIDXGIFactory;
	pDXGIAdapter->GetParent( __uuidof( IDXGIFactory ), ( void ** )&pIDXGIFactory);

	// スワップチェインの作成
    DXGI_SWAP_CHAIN_DESC	sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_nClientWidth;
	sd.BufferDesc.Height = g_nClientHeight;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	hr = pIDXGIFactory->CreateSwapChain( g_pd3dDevice, &sd, &g_pSwapChain );

	pDXGIDevice->Release();
	pDXGIAdapter->Release();
	pIDXGIFactory->Release();

	if( FAILED ( hr ) ) {
		return hr;
	}

    // レンダリングターゲットの生成
    ID3D11Texture2D			*pBackBuffer = NULL;
    D3D11_TEXTURE2D_DESC BackBufferSurfaceDesc;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if ( FAILED( hr ) ) {
		MessageBox( NULL, _T( "Can't get backbuffer." ), _T( "Error" ), MB_OK );
        return hr;
    }
    pBackBuffer->GetDesc( &BackBufferSurfaceDesc );
    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRTV );
    SAFE_RELEASE( pBackBuffer );
    if ( FAILED( hr ) ) {
		MessageBox( NULL, _T( "Can't create render target view." ), _T( "Error" ), MB_OK );
        return hr;
    }

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRTV, NULL );

    // ラスタライザの設定
    D3D11_RASTERIZER_DESC drd;
	ZeroMemory( &drd, sizeof( drd ) );
	drd.FillMode				= D3D11_FILL_SOLID;
	drd.CullMode				= D3D11_CULL_NONE;
	drd.FrontCounterClockwise	= FALSE;
	drd.DepthClipEnable			= TRUE;
    hr = g_pd3dDevice->CreateRasterizerState( &drd, &g_pRS );
    if ( FAILED( hr ) ) {
		MessageBox( NULL, _T( "Can't create rasterizer state." ), _T( "Error" ), MB_OK );
        return hr;
    }
    g_pImmediateContext->RSSetState( g_pRS );

    // ビューポートの設定
    D3D11_VIEWPORT vp;
    vp.Width    = ( FLOAT )g_nClientWidth;
    vp.Height   = ( FLOAT )g_nClientHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    return S_OK;
}


// プログラマブルシェーダ作成
HRESULT MakeShaders( void )
{
    HRESULT hr;
    ID3DBlob* pVertexShaderBuffer = NULL;
    ID3DBlob* pPixelShaderBuffer = NULL;
    ID3DBlob* pError = NULL;

    DWORD dwShaderFlags = 0;
#ifdef _DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif
    // コンパイル
    hr = D3DX11CompileFromFile( _T( "Basic_2D.fx" ), NULL, NULL, "VS", "vs_4_0_level_9_1",
								dwShaderFlags, 0, NULL, &pVertexShaderBuffer, &pError, NULL );
    if ( FAILED( hr ) ) {
		MessageBox( NULL, _T( "Can't open Basic_2D.fx" ), _T( "Error" ), MB_OK );
        SAFE_RELEASE( pError );
        return hr;
    }
    hr = D3DX11CompileFromFile( _T( "Basic_2D.fx" ), NULL, NULL, "PS", "ps_4_0_level_9_1",
								dwShaderFlags, 0, NULL, &pPixelShaderBuffer, &pError, NULL );
    if ( FAILED( hr ) ) {
        SAFE_RELEASE( pVertexShaderBuffer );
        SAFE_RELEASE( pError );
        return hr;
    }
    SAFE_RELEASE( pError );
    
    // VertexShader作成
    hr = g_pd3dDevice->CreateVertexShader( pVertexShaderBuffer->GetBufferPointer(),
										   pVertexShaderBuffer->GetBufferSize(),
										   NULL, &g_pVertexShader );
    if ( FAILED( hr ) ) {
        SAFE_RELEASE( pVertexShaderBuffer );
        SAFE_RELEASE( pPixelShaderBuffer );
        return hr;
    }
    // PixelShader作成
    hr = g_pd3dDevice->CreatePixelShader( pPixelShaderBuffer->GetBufferPointer(),
										  pPixelShaderBuffer->GetBufferSize(),
										  NULL, &g_pPixelShader );
    if ( FAILED( hr ) ) {
        SAFE_RELEASE( pVertexShaderBuffer );
        SAFE_RELEASE( pPixelShaderBuffer );
        return hr;
    }

    // 入力バッファの入力形式
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXTURE",  0, DXGI_FORMAT_R32G32_FLOAT,       0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
	UINT numElements = ARRAYSIZE( layout );
	// 入力バッファの入力形式作成
    hr = g_pd3dDevice->CreateInputLayout( layout, numElements,
										  pVertexShaderBuffer->GetBufferPointer(),
										  pVertexShaderBuffer->GetBufferSize(),
										  &g_pInputLayout );
    SAFE_RELEASE( pVertexShaderBuffer );
    SAFE_RELEASE( pPixelShaderBuffer );
    if ( FAILED( hr ) ) {
        return hr;
    }

    // シェーダ定数バッファ作成
    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof( bd ) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( CBNeverChanges );
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, NULL, &g_pCBNeverChanges );
    if( FAILED( hr ) )
        return hr;

	// 変換行列
    CBNeverChanges	cbNeverChanges;
	XMMATRIX		mScreen;
    mScreen = XMMatrixIdentity();
	mScreen._11 =  2.0f / g_nClientWidth;
	mScreen._22 = -2.0f / g_nClientHeight;
	mScreen._41 = -1.0f;
	mScreen._42 =  1.0f;
	cbNeverChanges.mView = XMMatrixTranspose( mScreen );
	g_pImmediateContext->UpdateSubresource( g_pCBNeverChanges, 0, NULL, &cbNeverChanges, 0, 0 );

    return S_OK;
}


// テクスチャロード
int LoadTexture( TCHAR *szFileName, TEX_PICTURE *pTexPic )
{
    HRESULT						hr;
	D3DX11_IMAGE_INFO			ImageInfo;
	D3DX11_IMAGE_LOAD_INFO		liLoadInfo;
	ID3D11Texture2D				*pTexture;
	int							nTexWidth, nTexHeight;

	// 画像情報取得
	hr = D3DX11GetImageInfoFromFile( szFileName, NULL, &ImageInfo, NULL );
    if ( FAILED( hr ) ) {
        return hr;
    }
	nTexWidth  = ( int )pow( 2.0, floor( log( ( double )ImageInfo.Width )  / log( 2.0 ) + 1.01 ) );
	if ( ( nTexWidth  / 2 ) == ImageInfo.Width  ) nTexWidth  /= 2;
	nTexHeight = ( int )pow( 2.0, floor( log( ( double )ImageInfo.Height ) / log( 2.0 ) + 1.01 ) );
	if ( ( nTexHeight / 2 ) == ImageInfo.Height ) nTexHeight /= 2;

	// ロード
	ZeroMemory( &liLoadInfo, sizeof( D3DX11_IMAGE_LOAD_INFO ) );
	liLoadInfo.Width = nTexWidth;
	liLoadInfo.Height = nTexHeight;
	liLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	liLoadInfo.Format = ImageInfo.Format;

	hr = D3DX11CreateShaderResourceViewFromFile( g_pd3dDevice, szFileName, &liLoadInfo, NULL, &( pTexPic->pSRViewTexture ), NULL );
    if ( FAILED( hr ) ) {
        return hr;
    }
	pTexPic->pSRViewTexture->GetResource( ( ID3D11Resource ** )&( pTexture ) );
	pTexture->GetDesc( &( pTexPic->tdDesc ) );
	pTexture->Release();

	pTexPic->nWidth = ImageInfo.Width;
	pTexPic->nHeight = ImageInfo.Height;

	return S_OK;
}


// 描画モードオブジェクト初期化
int InitDrawModes( void )
{
    HRESULT				hr;

	// ブレンドステート
    D3D11_BLEND_DESC BlendDesc;
	BlendDesc.AlphaToCoverageEnable = FALSE;
	BlendDesc.IndependentBlendEnable = FALSE;
    BlendDesc.RenderTarget[0].BlendEnable           = TRUE;
    BlendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    BlendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    BlendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    BlendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
    BlendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = g_pd3dDevice->CreateBlendState( &BlendDesc, &g_pbsAlphaBlend );
    if ( FAILED( hr ) ) {
        return hr;
    }

    // サンプラ
    D3D11_SAMPLER_DESC samDesc;
    ZeroMemory( &samDesc, sizeof( samDesc ) );
    samDesc.Filter          = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samDesc.AddressU        = D3D11_TEXTURE_ADDRESS_WRAP;
    samDesc.AddressV        = D3D11_TEXTURE_ADDRESS_WRAP;
    samDesc.AddressW        = D3D11_TEXTURE_ADDRESS_WRAP;
    samDesc.ComparisonFunc  = D3D11_COMPARISON_ALWAYS;
    samDesc.MaxLOD          = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState( &samDesc, &g_pSamplerState );
    if ( FAILED( hr ) ) {
        return hr;
    }

    return S_OK;
}


// ジオメトリの初期化
HRESULT InitGeometry( void )
{
    HRESULT hr = S_OK;

    // 頂点バッファ作成
    D3D11_BUFFER_DESC BufferDesc;
    BufferDesc.Usage                = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth            = sizeof( CUSTOMVERTEX ) * MAX_BUFFER_VERTEX;
    BufferDesc.BindFlags            = D3D11_BIND_VERTEX_BUFFER;
    BufferDesc.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags            = 0;

    D3D11_SUBRESOURCE_DATA SubResourceData;
    SubResourceData.pSysMem             = g_cvVertices;
    SubResourceData.SysMemPitch         = 0;
    SubResourceData.SysMemSlicePitch    = 0;
    hr = g_pd3dDevice->CreateBuffer( &BufferDesc, &SubResourceData, &g_pD3D11VertexBuffer );
    if ( FAILED( hr ) ) {
        return hr;
    }

	// テクスチャ作成
	g_tBack.pSRViewTexture =  NULL;
	hr = LoadTexture( _T( "11.BMP" ), &g_tBack );
    if ( FAILED( hr ) ) {
 		MessageBox( NULL, _T( "Can't open 11.BMP" ), _T( "Error" ), MB_OK );
       return hr;
    }

	return S_OK;
}


// 終了処理
int Cleanup( void )
{
    SAFE_RELEASE( g_tBack.pSRViewTexture );
    SAFE_RELEASE( g_pD3D11VertexBuffer );

    SAFE_RELEASE( g_pSamplerState );
    SAFE_RELEASE( g_pbsAlphaBlend );
    SAFE_RELEASE( g_pInputLayout );
    SAFE_RELEASE( g_pPixelShader );
    SAFE_RELEASE( g_pVertexShader );
    SAFE_RELEASE( g_pCBNeverChanges );

    SAFE_RELEASE( g_pRS );									// ラスタライザ

	// ステータスをクリア
	if ( g_pImmediateContext ) {
		g_pImmediateContext->ClearState();
		g_pImmediateContext->Flush();
	}

    SAFE_RELEASE( g_pRTV );									// レンダリングターゲット

    // スワップチェーン
    if ( g_pSwapChain != NULL ) {
        g_pSwapChain->SetFullscreenState( FALSE, 0 );
    }
    SAFE_RELEASE( g_pSwapChain );

    SAFE_RELEASE( g_pImmediateContext );					// デバイスコンテキスト
    SAFE_RELEASE( g_pd3dDevice );							// デバイス

	return 0;
}


// ウィンドウプロシージャ
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}


// 絵の描画待ち行列フラッシュ
int FlushDrawingPictures( void )
{
	HRESULT			hr;

	if ( ( g_nVertexNum > 0 ) && g_pNowTexture ) {
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		hr = g_pImmediateContext->Map( g_pD3D11VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
		if ( SUCCEEDED( hr ) ) {
			CopyMemory( mappedResource.pData, &( g_cvVertices[0] ), sizeof( CUSTOMVERTEX ) * g_nVertexNum );
			g_pImmediateContext->Unmap( g_pD3D11VertexBuffer, 0 );
		}
		g_pImmediateContext->PSSetShaderResources( 0, 1, &g_pNowTexture );
		g_pImmediateContext->Draw( g_nVertexNum, 0 );
	}
	g_nVertexNum = 0;
	g_pNowTexture = NULL;

	return 0;
}


// 絵の描画
int DrawPicture( float x, float y, TEX_PICTURE *pTexPic )
{
	if ( g_nVertexNum > ( MAX_BUFFER_VERTEX - 6 ) ) return -1;	// 頂点がバッファからあふれる場合は描画せず

	// テクスチャが切り替えられていれば待ち行列フラッシュ
	if ( ( pTexPic->pSRViewTexture != g_pNowTexture ) && g_pNowTexture ) {
		FlushDrawingPictures();
	}

	// 頂点セット
	g_cvVertices[g_nVertexNum + 0].v4Pos   = XMFLOAT4( x,                   y,                    0.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 0].v4Color = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 0].v2UV    = XMFLOAT2( 0.0f, 0.0f );
	g_cvVertices[g_nVertexNum + 1].v4Pos   = XMFLOAT4( x,                   y + pTexPic->nHeight, 0.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 1].v4Color = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 1].v2UV    = XMFLOAT2( 0.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 2].v4Pos   = XMFLOAT4( x + pTexPic->nWidth, y,                    0.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 2].v4Color = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 2].v2UV    = XMFLOAT2( 1.0f, 0.0f );
	g_cvVertices[g_nVertexNum + 3] = g_cvVertices[g_nVertexNum + 1];
	g_cvVertices[g_nVertexNum + 4].v4Pos   = XMFLOAT4( x + pTexPic->nWidth, y + pTexPic->nHeight, 0.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 4].v4Color = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 4].v2UV    = XMFLOAT2( 1.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 5] = g_cvVertices[g_nVertexNum + 2];
	g_nVertexNum += 6;
	g_pNowTexture = pTexPic->pSRViewTexture;

	return 0;
}


// 絵の一部分描画
int DrawPicturePart( float x1, float y1, float x2, float y2, float px, float py, TEX_PICTURE *pTexPic )
{
	float			u1, v1, u2, v2;

	if ( g_nVertexNum > ( MAX_BUFFER_VERTEX - 6 ) ) return -1;	// 頂点がバッファからあふれる場合は描画せず

	// テクスチャが切り替えられていれば待ち行列フラッシュ
	if ( ( pTexPic->pSRViewTexture != g_pNowTexture ) && g_pNowTexture ) {
		FlushDrawingPictures();
	}

	u1 = px / pTexPic->nWidth;
	v1 = py / pTexPic->nHeight;
	u2 = u1 + ( x2 - x1 ) / pTexPic->nWidth;
	v2 = v1 + ( y2 - y1 ) / pTexPic->nHeight;

	// 頂点セット
	g_cvVertices[g_nVertexNum + 0].v4Pos   = XMFLOAT4( x1, y1, 0.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 0].v4Color = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 0].v2UV    = XMFLOAT2( u1, v1 );
	g_cvVertices[g_nVertexNum + 1].v4Pos   = XMFLOAT4( x1, y2, 0.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 1].v4Color = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 1].v2UV    = XMFLOAT2( u1, v2 );
	g_cvVertices[g_nVertexNum + 2].v4Pos   = XMFLOAT4( x2, y1, 0.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 2].v4Color = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 2].v2UV    = XMFLOAT2( u2, v1 );
	g_cvVertices[g_nVertexNum + 3] = g_cvVertices[g_nVertexNum + 1];
	g_cvVertices[g_nVertexNum + 4].v4Pos   = XMFLOAT4( x2, y2, 0.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 4].v4Color = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );
	g_cvVertices[g_nVertexNum + 4].v2UV    = XMFLOAT2( u2, v2 );
	g_cvVertices[g_nVertexNum + 5] = g_cvVertices[g_nVertexNum + 2];
	g_nVertexNum += 6;
	g_pNowTexture = pTexPic->pSRViewTexture;

	return 0;
}


// レンダリング
HRESULT Render( void )
{
	int						i;
    // 画面クリア
	XMFLOAT4	v4Color = XMFLOAT4( 0.0f, 0.0f, 1.0f, 1.0f );
    g_pImmediateContext->ClearRenderTargetView( g_pRTV, ( float * )&v4Color );

    // サンプラ・ラスタライザセット
    g_pImmediateContext->PSSetSamplers( 0, 1, &g_pSamplerState );
    g_pImmediateContext->RSSetState( g_pRS );
    
    // 描画設定
    UINT nStrides = sizeof( CUSTOMVERTEX );
    UINT nOffsets = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pD3D11VertexBuffer, &nStrides, &nOffsets );
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    g_pImmediateContext->IASetInputLayout( g_pInputLayout );

    // シェーダ設定
    g_pImmediateContext->VSSetShader( g_pVertexShader, NULL, 0 );
    g_pImmediateContext->VSSetConstantBuffers( 0, 1, &g_pCBNeverChanges );
    g_pImmediateContext->PSSetShader( g_pPixelShader, NULL, 0 );

    // 描画
    g_pImmediateContext->OMSetBlendState( NULL, NULL, 0xFFFFFFFF );
//	DrawPicture( fBack_x, 0.0f, &g_tBack );
	for ( i = 0; i < VIEW_HEIGHT; i++ ) {
		DrawPicturePart( v2Points[i].x, v2Points[i].y, v2Points[i].x + PIC_WIDTH_DOWN, v2Points[i].y + 1.0f,
						 0.0f, v2Points[i].y, &g_tBack );
	}
    g_pImmediateContext->OMSetBlendState( g_pbsAlphaBlend, NULL, 0xFFFFFFFF );

    // 表示
	FlushDrawingPictures();

    return S_OK;
}


// エントリポイント
int WINAPI _tWinMain( HINSTANCE hInst, HINSTANCE, LPTSTR, int )
{
	LARGE_INTEGER			nNowTime, nLastTime;		// 現在とひとつ前の時刻
	LARGE_INTEGER			nTimeFreq;					// 時間単位

    // 画面サイズ
    g_nClientWidth  = VIEW_WIDTH;						// 幅
    g_nClientHeight = VIEW_HEIGHT;						// 高さ

	// Register the window class
    WNDCLASSEX wc = { sizeof( WNDCLASSEX ), CS_CLASSDC, MsgProc, 0L, 0L,
                      GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
                      _T( "D3D Sample" ), NULL };
    RegisterClassEx( &wc );

	RECT rcRect;
	SetRect( &rcRect, 0, 0, g_nClientWidth, g_nClientHeight );
	AdjustWindowRect( &rcRect, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( _T( "D3D Sample" ), _T( "Scroll_5_1" ),
						   WS_OVERLAPPEDWINDOW, 100, 20, rcRect.right - rcRect.left, rcRect.bottom - rcRect.top,
						   GetDesktopWindow(), NULL, wc.hInstance, NULL );

    // Initialize Direct3D
    if( SUCCEEDED( InitD3D() ) && SUCCEEDED( MakeShaders() ) )
    {
        // Create the shaders
        if( SUCCEEDED( InitDrawModes() ) )
        {
			if ( SUCCEEDED( InitGeometry() ) ) {					// ジオメトリ作成

				// Show the window
				ShowWindow( g_hWnd, SW_SHOWDEFAULT );
				UpdateWindow( g_hWnd );

				InitBack();									// キャラクタ初期化
				
				QueryPerformanceFrequency( &nTimeFreq );			// 時間単位
				QueryPerformanceCounter( &nLastTime );				// 1フレーム前時刻初期化

				// Enter the message loop
				MSG msg;
				ZeroMemory( &msg, sizeof( msg ) );
				while( msg.message != WM_QUIT )
				{
					Render();
					MoveBack();
					do {
						if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
						{
							TranslateMessage( &msg );
							DispatchMessage( &msg );
						}
						QueryPerformanceCounter( &nNowTime );
					} while( ( ( nNowTime.QuadPart - nLastTime.QuadPart ) < ( nTimeFreq.QuadPart / 90 ) ) &&
							 ( msg.message != WM_QUIT ) );
					while( ( ( nNowTime.QuadPart - nLastTime.QuadPart ) < ( nTimeFreq.QuadPart / 60 ) ) &&
						   ( msg.message != WM_QUIT ) )
					{
						QueryPerformanceCounter( &nNowTime );
					}
					nLastTime = nNowTime;
					g_pSwapChain->Present( 0, 0 );					// 表示
				}
			}
        }
    }

    // Clean up everything and exit the app
    Cleanup();
    UnregisterClass( _T( "D3D Sample" ), wc.hInstance );
    return 0;
}
