#include <fbxvpch.h>

#include <App.h>
#include <AppSurface.h>
#include <Input.h>
#include <NuklearSdl.h>

#include <Scene.h>

using namespace fbxv;

struct Camera {
    Camera( ) {
        reset( );
    }

    void reset( ) {
        m_target.curr[ 0 ] = 0.0f;
        m_target.curr[ 1 ] = 0.0f;
        m_target.curr[ 2 ] = 0.0f;
        m_target.dest[ 0 ] = 0.0f;
        m_target.dest[ 1 ] = 0.0f;
        m_target.dest[ 2 ] = 0.0f;

        m_pos.curr[ 0 ] = 0.0f;
        m_pos.curr[ 1 ] = 0.0f;
        m_pos.curr[ 2 ] = -300.0f;
        m_pos.dest[ 0 ] = 0.0f;
        m_pos.dest[ 1 ] = 0.0f;
        m_pos.dest[ 2 ] = -300.0f;

        m_orbit[ 0 ] = 0.0f;
        m_orbit[ 1 ] = 0.0f;
    }

    void mtxLookAt( float* _outViewMtx ) {
        //bx::mtxLookAtRh( _outViewMtx, m_pos.curr, m_target.curr );
        bx::mtxLookAt( _outViewMtx, m_pos.curr, m_target.curr );
    }

    void orbit( float _dx, float _dy ) {
        m_orbit[ 0 ] += _dx;
        m_orbit[ 1 ] += _dy;
    }

    void dolly( float _dz ) {
        const float cnear = 1.0f;
        const float cfar  = 1000.0f;

        const float toTarget[ 3 ] = {
            m_target.dest[ 0 ] - m_pos.dest[ 0 ], m_target.dest[ 1 ] - m_pos.dest[ 1 ], m_target.dest[ 2 ] - m_pos.dest[ 2 ],
        };
        const float toTargetLen       = bx::vec3Length( toTarget );
        const float invToTargetLen    = 1.0f / ( toTargetLen + FLT_MIN );
        const float toTargetNorm[ 3 ] = {
            toTarget[ 0 ] * invToTargetLen, toTarget[ 1 ] * invToTargetLen, toTarget[ 2 ] * invToTargetLen,
        };

        float delta  = toTargetLen * _dz;
        float newLen = toTargetLen + delta;
        if ( ( cnear < newLen || _dz < 0.0f ) && ( newLen < cfar || _dz > 0.0f ) ) {
            m_pos.dest[ 0 ] += toTargetNorm[ 0 ] * delta;
            m_pos.dest[ 1 ] += toTargetNorm[ 1 ] * delta;
            m_pos.dest[ 2 ] += toTargetNorm[ 2 ] * delta;
        }
    }

    void consumeOrbit( float _amount ) {
        float consume[ 2 ];
        consume[ 0 ] = m_orbit[ 0 ] * _amount;
        consume[ 1 ] = m_orbit[ 1 ] * _amount;
        m_orbit[ 0 ] -= consume[ 0 ];
        m_orbit[ 1 ] -= consume[ 1 ];

        const float toPos[ 3 ] = {
            m_pos.curr[ 0 ] - m_target.curr[ 0 ], m_pos.curr[ 1 ] - m_target.curr[ 1 ], m_pos.curr[ 2 ] - m_target.curr[ 2 ],
        };
        const float toPosLen       = bx::vec3Length( toPos );
        const float invToPosLen    = 1.0f / ( toPosLen + FLT_MIN );
        const float toPosNorm[ 3 ] = {
            toPos[ 0 ] * invToPosLen, toPos[ 1 ] * invToPosLen, toPos[ 2 ] * invToPosLen,
        };

        float ll[ 2 ];
        latLongFromVec( ll[ 0 ], ll[ 1 ], toPosNorm );
        ll[ 0 ] += consume[ 0 ];
        ll[ 1 ] -= consume[ 1 ];
        ll[ 1 ] = bx::fclamp( ll[ 1 ], 0.02f, 0.98f );

        float tmp[ 3 ];
        vecFromLatLong( tmp, ll[ 0 ], ll[ 1 ] );

        float diff[ 3 ];
        diff[ 0 ] = ( tmp[ 0 ] - toPosNorm[ 0 ] ) * toPosLen;
        diff[ 1 ] = ( tmp[ 1 ] - toPosNorm[ 1 ] ) * toPosLen;
        diff[ 2 ] = ( tmp[ 2 ] - toPosNorm[ 2 ] ) * toPosLen;

        m_pos.curr[ 0 ] += diff[ 0 ];
        m_pos.curr[ 1 ] += diff[ 1 ];
        m_pos.curr[ 2 ] += diff[ 2 ];
        m_pos.dest[ 0 ] += diff[ 0 ];
        m_pos.dest[ 1 ] += diff[ 1 ];
        m_pos.dest[ 2 ] += diff[ 2 ];
    }

    void update( float _dt ) {
        const float amount = bx::fmin( _dt / 0.12f, 1.0f );

        consumeOrbit( amount );

        m_target.curr[ 0 ] = bx::flerp( m_target.curr[ 0 ], m_target.dest[ 0 ], amount );
        m_target.curr[ 1 ] = bx::flerp( m_target.curr[ 1 ], m_target.dest[ 1 ], amount );
        m_target.curr[ 2 ] = bx::flerp( m_target.curr[ 2 ], m_target.dest[ 2 ], amount );
        m_pos.curr[ 0 ]    = bx::flerp( m_pos.curr[ 0 ], m_pos.dest[ 0 ], amount );
        m_pos.curr[ 1 ]    = bx::flerp( m_pos.curr[ 1 ], m_pos.dest[ 1 ], amount );
        m_pos.curr[ 2 ]    = bx::flerp( m_pos.curr[ 2 ], m_pos.dest[ 2 ], amount );
    }

    void envViewMtx( float* _mtx ) {
        const float toTarget[ 3 ] = {
            m_target.curr[ 0 ] - m_pos.curr[ 0 ], m_target.curr[ 1 ] - m_pos.curr[ 1 ], m_target.curr[ 2 ] - m_pos.curr[ 2 ],
        };

        const float toTargetLen       = bx::vec3Length( toTarget );
        const float invToTargetLen    = 1.0f / ( toTargetLen + FLT_MIN );
        const float toTargetNorm[ 3 ] = {
            toTarget[ 0 ] * invToTargetLen, toTarget[ 1 ] * invToTargetLen, toTarget[ 2 ] * invToTargetLen,
        };

        float       tmp[ 3 ];
        const float fakeUp[ 3 ] = {0.0f, 1.0f, 0.0f};

        float right[ 3 ];
        bx::vec3Cross( tmp, fakeUp, toTargetNorm );
        bx::vec3Norm( right, tmp );

        float up[ 3 ];
        bx::vec3Cross( tmp, toTargetNorm, right );
        bx::vec3Norm( up, tmp );

        _mtx[ 0 ]  = right[ 0 ];
        _mtx[ 1 ]  = right[ 1 ];
        _mtx[ 2 ]  = right[ 2 ];
        _mtx[ 3 ]  = 0.0f;
        _mtx[ 4 ]  = up[ 0 ];
        _mtx[ 5 ]  = up[ 1 ];
        _mtx[ 6 ]  = up[ 2 ];
        _mtx[ 7 ]  = 0.0f;
        _mtx[ 8 ]  = toTargetNorm[ 0 ];
        _mtx[ 9 ]  = toTargetNorm[ 1 ];
        _mtx[ 10 ] = toTargetNorm[ 2 ];
        _mtx[ 11 ] = 0.0f;
        _mtx[ 12 ] = 0.0f;
        _mtx[ 13 ] = 0.0f;
        _mtx[ 14 ] = 0.0f;
        _mtx[ 15 ] = 1.0f;
    }

    static inline void vecFromLatLong( float _vec[ 3 ], float _u, float _v ) {
        const float phi   = _u * 2.0f * bx::pi;
        const float theta = _v * bx::pi;

        const float st = bx::fsin( theta );
        const float sp = bx::fsin( phi );
        const float ct = bx::fcos( theta );
        const float cp = bx::fcos( phi );

        _vec[ 0 ] = -st * sp;
        _vec[ 1 ] = ct;
        _vec[ 2 ] = -st * cp;
    }

    static inline void latLongFromVec( float& _u, float& _v, const float _vec[ 3 ] ) {
        const float phi   = atan2f( _vec[ 0 ], _vec[ 2 ] );
        const float theta = acosf( _vec[ 1 ] );

        _u = ( bx::pi + phi ) * bx::invPi * 0.5f;
        _v = theta * bx::invPi;
    }

    struct Interp3f {
        float curr[ 3 ];
        float dest[ 3 ];
    };

    Interp3f m_target;
    Interp3f m_pos;
    float    m_orbit[ 2 ];
};

struct Mouse {
    Mouse( ) : m_dx( 0.0f ), m_dy( 0.0f ), m_prevMx( 0.0f ), m_prevMy( 0.0f ), m_scroll( 0 ), m_scrollPrev( 0 ) {
    }

    void update( float _mx, float _my, int32_t _mz, uint32_t _width, uint32_t _height ) {
        const float widthf  = float( int32_t( _width ) );
        const float heightf = float( int32_t( _height ) );

        // Delta movement.
        m_dx = float( _mx - m_prevMx ) / widthf;
        m_dy = float( _my - m_prevMy ) / heightf;

        m_prevMx = _mx;
        m_prevMy = _my;

        // Scroll.
        m_scroll     = _mz - m_scrollPrev;
        m_scrollPrev = _mz;
    }

    float   m_dx; // Screen space.
    float   m_dy;
    float   m_prevMx;
    float   m_prevMy;
    int32_t m_scroll;
    int32_t m_scrollPrev;
};

struct Uniforms {
    enum { NumVec4 = 12 };

    void init( ) {
        u_params = bgfx::createUniform( "u_params", bgfx::UniformType::Vec4, NumVec4 );
    }

    void submit( ) {
        bgfx::setUniform( u_params, m_params, NumVec4 );
    }

    void destroy( ) {
        bgfx::destroyUniform( u_params );
    }

    union {
        struct {
            union {
                float m_mtx[ 16 ];
                /* 0*/ struct { float m_mtx0[ 4 ]; };
                /* 1*/ struct { float m_mtx1[ 4 ]; };
                /* 2*/ struct { float m_mtx2[ 4 ]; };
                /* 3*/ struct { float m_mtx3[ 4 ]; };
            };
            /* 4*/ struct { float m_glossiness, m_reflectivity, m_exposure, m_bgType; };
            /* 5*/ struct { float m_metalOrSpec, m_unused5[ 3 ]; };
            /* 6*/ struct { float m_doDiffuse, m_doSpecular, m_doDiffuseIbl, m_doSpecularIbl; };
            /* 7*/ struct { float m_cameraPos[ 3 ], m_unused7[ 1 ]; };
            /* 8*/ struct { float m_rgbDiff[ 4 ]; };
            /* 9*/ struct { float m_rgbSpec[ 4 ]; };
            /*10*/ struct { float m_lightDir[ 3 ], m_unused10[ 1 ]; };
            /*11*/ struct { float m_lightCol[ 3 ], m_unused11[ 1 ]; };
        };

        float m_params[ NumVec4 * 4 ];
    };

    bgfx::UniformHandle u_params;
};

struct PosTexCoord0Vertex {
    float m_x;
    float m_y;
    float m_z;
    float m_u;
    float m_v;

    static void init( ) {
        decl.begin( )
            .add( bgfx::Attrib::Position, 3, bgfx::AttribType::Float )
            .add( bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float )
            .end( );
    }

    static bgfx::VertexDecl decl;
};

bgfx::VertexDecl PosTexCoord0Vertex::decl;

void screenSpaceQuad( float _textureWidth,
                      float _textureHeight,
                      float _texelHalf,
                      bool  _originBottomLeft,
                      float _width  = 1.0f,
                      float _height = 1.0f ) {
    if ( 3 == bgfx::getAvailTransientVertexBuffer( 3, PosTexCoord0Vertex::decl ) ) {
        bgfx::TransientVertexBuffer vb;
        bgfx::allocTransientVertexBuffer( &vb, 3, PosTexCoord0Vertex::decl );
        PosTexCoord0Vertex* vertex = (PosTexCoord0Vertex*) vb.data;

        const float minx = -_width;
        const float maxx = _width;
        const float miny = 0.0f;
        const float maxy = _height * 2.0f;

        const float texelHalfW = _texelHalf / _textureWidth;
        const float texelHalfH = _texelHalf / _textureHeight;
        const float minu       = -1.0f + texelHalfW;
        const float maxu       = 1.0f + texelHalfH;

        const float zz = 0.0f;

        float minv = texelHalfH;
        float maxv = 2.0f + texelHalfH;

        if ( _originBottomLeft ) {
            std::swap( minv, maxv );
            minv -= 1.0f;
            maxv -= 1.0f;
        }

        vertex[ 0 ].m_x = minx;
        vertex[ 0 ].m_y = miny;
        vertex[ 0 ].m_z = zz;
        vertex[ 0 ].m_u = minu;
        vertex[ 0 ].m_v = minv;

        vertex[ 1 ].m_x = maxx;
        vertex[ 1 ].m_y = miny;
        vertex[ 1 ].m_z = zz;
        vertex[ 1 ].m_u = maxu;
        vertex[ 1 ].m_v = minv;

        vertex[ 2 ].m_x = maxx;
        vertex[ 2 ].m_y = maxy;
        vertex[ 2 ].m_z = zz;
        vertex[ 2 ].m_u = maxu;
        vertex[ 2 ].m_v = maxv;

        bgfx::setVertexBuffer( &vb );
    }
}

struct fbxv::AppContent {
    nk_context* nk          = nullptr;
    nk_color    diffColor;
    nk_color    specColor;
    uint32_t    width       = 0;
    uint32_t    height      = 0;
    uint32_t    resetFlags  = 0;
    uint32_t    envId       = 1;
    uint32_t    sceneId     = 0;
    uint32_t    maskId      = 0;
    uint32_t    blurId      = 0;
    int32_t     enableBlur  = false;
    int32_t     blurItCount = 5;
    float       blurRadius  = 5;

    Camera   camera;
    Mouse    mouse;
    Uniforms uniforms;

    Scene* scenes[ 2 ];

    bgfx::ProgramHandle programMesh;
    bgfx::ProgramHandle programSky;
    bgfx::ProgramHandle programMaskCombine;
    bgfx::ProgramHandle programBlur[ 3 ];
    bgfx::TextureHandle colorTextureHandle;
    bgfx::TextureHandle depthTextureHandle;
    bgfx::TextureHandle maskTextureHandles[ 7 ];
    bgfx::TextureHandle radianceTextureHandles[ 2 ];
    bgfx::TextureHandle irradianceTextureHandles[ 2 ];
    bgfx::UniformHandle u_mtx;
    bgfx::UniformHandle u_params;
    bgfx::UniformHandle u_flags;
    bgfx::UniformHandle u_camPos;
    bgfx::UniformHandle u_blurParams;
    bgfx::UniformHandle s_texCube;
    bgfx::UniformHandle s_texCubeIrr;
    bgfx::UniformHandle s_colorTexture;
    bgfx::UniformHandle s_blurredTexture;
    bgfx::UniformHandle s_maskTexture;

    bgfx::FrameBufferHandle colorHandle;
    bgfx::FrameBufferHandle blurredHandles[ 2 ];
};

App::App( ) : content( new AppContent( ) ) {
}

App::~App( ) {
}

AppSurfaceBase* App::CreateAppSurface( ) {
    return new AppSurface( );
}

bool App::Initialize( int Args, char* ppArgs[] ) {
    if ( AppBase::Initialize( Args, ppArgs ) ) {
        totalSecs = 0.0f;

        content->nk = nk_sdl_init( (SDL_Window*) GetSurface( )->GetWindowHandle( ) );

        content->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/MercedesBenzA45AMG.fbxp" );
        content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/mech-m-6k.fbxp" );

        content->uniforms.init( );

        PosTexCoord0Vertex::init( );

        content->resetFlags = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X16;

        content->u_mtx        = bgfx::createUniform( "u_mtx", bgfx::UniformType::Mat4 );
        content->u_params     = bgfx::createUniform( "u_params", bgfx::UniformType::Vec4 );
        content->u_flags      = bgfx::createUniform( "u_flags", bgfx::UniformType::Vec4 );
        content->u_camPos     = bgfx::createUniform( "u_camPos", bgfx::UniformType::Vec4 );
        content->s_texCube    = bgfx::createUniform( "s_texCube", bgfx::UniformType::Int1 );
        content->s_texCubeIrr = bgfx::createUniform( "s_texCubeIrr", bgfx::UniformType::Int1 );

        content->u_blurParams     = bgfx::createUniform( "u_blurParams", bgfx::UniformType::Vec4 );
        content->s_colorTexture   = bgfx::createUniform( "s_colorTexture", bgfx::UniformType::Int1 );
        content->s_blurredTexture = bgfx::createUniform( "s_blurredTexture", bgfx::UniformType::Int1 );
        content->s_maskTexture    = bgfx::createUniform( "s_maskTexture", bgfx::UniformType::Int1 );

#include <vs_ibl_mesh_android.bin.h>
#include <fs_ibl_mesh_android.bin.h>

        content->programMesh = bgfx::createProgram(
            bgfx::createShader( bgfx::makeRef( (uint8_t*) vs_ibl_mesh_android_bin_h, sizeof( vs_ibl_mesh_android_bin_h ) ) ),
            bgfx::createShader( bgfx::makeRef( (uint8_t*) fs_ibl_mesh_android_bin_h, sizeof( fs_ibl_mesh_android_bin_h ) ) ),
            true );

#include <vs_blur_android.bin.h>
#include <fs_blur5_android.bin.h>
#include <fs_blur9_android.bin.h>
#include <fs_blur13_android.bin.h>
#include <fs_blur_masked_combine_android.bin.h>

        content->programBlur[ 2 ] = bgfx::createProgram(
            bgfx::createShader( bgfx::makeRef( (uint8_t*) vs_blur_android_bin_h, sizeof( vs_blur_android_bin_h ) ) ),
            bgfx::createShader( bgfx::makeRef( (uint8_t*) fs_blur5_android_bin_h, sizeof( fs_blur5_android_bin_h ) ) ),
            true );
        content->programBlur[ 1 ] = bgfx::createProgram(
            bgfx::createShader( bgfx::makeRef( (uint8_t*) vs_blur_android_bin_h, sizeof( vs_blur_android_bin_h ) ) ),
            bgfx::createShader( bgfx::makeRef( (uint8_t*) fs_blur9_android_bin_h, sizeof( fs_blur9_android_bin_h ) ) ),
            true );
        content->programBlur[ 0 ] = bgfx::createProgram(
            bgfx::createShader( bgfx::makeRef( (uint8_t*) vs_blur_android_bin_h, sizeof( vs_blur_android_bin_h ) ) ),
            bgfx::createShader( bgfx::makeRef( (uint8_t*) fs_blur13_android_bin_h, sizeof( fs_blur13_android_bin_h ) ) ),
            true );
        content->programMaskCombine = bgfx::createProgram(
            bgfx::createShader( bgfx::makeRef( (uint8_t*) vs_blur_android_bin_h, sizeof( vs_blur_android_bin_h ) ) ),
            bgfx::createShader( bgfx::makeRef( (uint8_t*) fs_blur_masked_combine_android_bin_h, sizeof( fs_blur_masked_combine_android_bin_h ) ) ),
            true );

#include <vs_ibl_skybox_android.bin.h>
#include <fs_ibl_skybox_android.bin.h>

        content->programSky = bgfx::createProgram(
            bgfx::createShader( bgfx::makeRef( (uint8_t*) vs_ibl_skybox_android_bin_h, sizeof( vs_ibl_skybox_android_bin_h ) ) ),
            bgfx::createShader( bgfx::makeRef( (uint8_t*) fs_ibl_skybox_android_bin_h, sizeof( fs_ibl_skybox_android_bin_h ) ) ),
            true );

        uint32_t maskFileId = 0;
        std::string fileBuffer;
        for ( auto maskFilename : {"../../../assets/UbisoftMask.dds",
                                   "../../../assets/CircleGradient.dds",
                                   "../../../assets/DotsGradient.dds",
                                   "../../../assets/HorzGradient.dds",
                                   "../../../assets/HorzInvGradient.dds",
                                   "../../../assets/VertGradient.dds",
                                   "../../../assets/VertInvGradient.dds"} ) {
            if ( flatbuffers::LoadFile( maskFilename, true, &fileBuffer ) ) {
                auto mem = bgfxUtils::makeReleasableCopy( fileBuffer.data( ), fileBuffer.size( ) );
                content->maskTextureHandles[ maskFileId++ ] = bgfx::createTexture( mem );
            }
        }

        if ( flatbuffers::LoadFile( "../../../assets/kyoto_irr.dds", true, &fileBuffer ) ) {
            auto mem = bgfxUtils::makeReleasableCopy( fileBuffer.data( ), fileBuffer.size( ) );
            content->irradianceTextureHandles[ 0 ] = bgfx::createTexture( mem );
        }

        if ( flatbuffers::LoadFile( "../../../assets/bolonga_irr.dds", true, &fileBuffer ) ) {
            auto mem = bgfxUtils::makeReleasableCopy( fileBuffer.data( ), fileBuffer.size( ) );
            content->irradianceTextureHandles[ 1 ] = bgfx::createTexture( mem );
        }

        if ( flatbuffers::LoadFile( "../../../assets/kyoto_lod.dds", true, &fileBuffer ) ) {
            auto mem = bgfxUtils::makeReleasableCopy( fileBuffer.data( ), fileBuffer.size( ) );
            content->radianceTextureHandles[ 0 ] = bgfx::createTexture( mem );
        }

        if ( flatbuffers::LoadFile( "../../../assets/bolonga_lod.dds", true, &fileBuffer ) ) {
            auto mem = bgfxUtils::makeReleasableCopy( fileBuffer.data( ), fileBuffer.size( ) );
            content->radianceTextureHandles[ 1 ] = bgfx::createTexture( mem );
        }

        content->uniforms.m_glossiness    = 0.5;
        content->uniforms.m_reflectivity  = 1;
        content->uniforms.m_exposure      = 2;
        content->uniforms.m_bgType        = 0;
        content->uniforms.m_metalOrSpec   = float( 0 );
        content->uniforms.m_doDiffuse     = float( 1 );
        content->uniforms.m_doSpecular    = float( 1 );
        content->uniforms.m_doDiffuseIbl  = float( 1 );
        content->uniforms.m_doSpecularIbl = float( 1 );
        content->uniforms.m_lightDir[ 0 ] = 1;
        content->uniforms.m_lightDir[ 1 ] = 1;
        content->uniforms.m_lightDir[ 2 ] = 0;
        content->uniforms.m_lightCol[ 0 ] = 1;
        content->uniforms.m_lightCol[ 1 ] = 1;
        content->uniforms.m_lightCol[ 2 ] = 1;
        content->uniforms.m_lightCol[ 3 ] = 1;
        content->uniforms.m_rgbDiff[ 0 ]  = 1;
        content->uniforms.m_rgbDiff[ 1 ]  = 1;
        content->uniforms.m_rgbDiff[ 2 ]  = 1;
        content->uniforms.m_rgbDiff[ 3 ]  = 1;
        content->uniforms.m_rgbSpec[ 0 ]  = 1;
        content->uniforms.m_rgbSpec[ 1 ]  = 1;
        content->uniforms.m_rgbSpec[ 2 ]  = 1;
        content->uniforms.m_rgbSpec[ 3 ]  = 1;

        return true;
    }

    return false;
}

void App::OnFrameMove( ) {
    SDL_Event evt;
    nk_input_begin( content->nk );
    while ( SDL_PollEvent( &evt ) ) {
        nk_sdl_handle_event( &evt );
    }
    nk_input_end( content->nk );

    AppBase::OnFrameMove( );
}

void App::Update( float deltaSecs, Input const& inputState ) {
    totalSecs += deltaSecs;

    const uint32_t width  = GetSurface( )->GetWidth( );
    const uint32_t height = GetSurface( )->GetHeight( );

    bool hovered = false;
    bool reset   = false;

    if ( nk_begin( content->nk,
                   "Blur Demo.",
                   nk_rect( 10, 60, 400, 600 ),
                   NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE ) ) {
        nk_layout_row_dynamic( content->nk, 20, 1 );
        nk_labelf_wrap( content->nk, "Orbit - left button" );
        nk_labelf_wrap( content->nk, "Zoom - right button" );
        nk_labelf_wrap( content->nk, "Statistics - tab key" );

        nk_layout_row_dynamic( content->nk, 30, 1 );
        nk_property_float( content->nk, "Glossiness", 0, &content->uniforms.m_glossiness, 1, 0.01, 0.001 );
        nk_property_float( content->nk, "Reflectivity", 0, &content->uniforms.m_reflectivity, 1, 0.01, 0.001 );

        nk_layout_row_dynamic( content->nk, 30, 1 );
        nk_label_wrap( content->nk, "Diffuse Color" );

        nk_layout_row_dynamic( content->nk, 100, 1 );
        content->diffColor = nk_color_picker( content->nk, content->diffColor, nk_color_format::NK_RGB );
        nk_color_fv( content->uniforms.m_rgbDiff, content->diffColor );

        nk_layout_row_dynamic( content->nk, 10, 1 );
        nk_spacing( content->nk, 1 );

        nk_layout_row_dynamic( content->nk, 30, 1 );
        nk_combobox_string( content->nk,
                            "assets/UbisoftMask.dds\0"
                            "assets/CircleGradient.dds\0"
                            "assets/DotsGradient.dds\0"
                            "assets/HorzGradient.dds\0"
                            "assets/HorzInvGradient.dds\0"
                            "assets/VertGradient.dds\0"
                            ".assets/VertInvGradient.dds\0\0",
                            (int*) &content->maskId,
                            7,
                            30,
                            nk_vec2( 300, 300 ) );

        nk_combobox_string( content->nk,
                            "scene/A45-AMG\0"
                            "scene/Mech-6k\0\0",
                            (int*) &content->sceneId,
                            2,
                            30,
                            nk_vec2( 300, 300 ) );
        nk_combobox_string( content->nk,
                            "env/kyoto\0"
                            "env/bolonga\0\0",
                            (int*) &content->envId,
                            2,
                            30,
                            nk_vec2( 300, 300 ) );

        nk_layout_row_dynamic( content->nk, 10, 1 );
        nk_spacing( content->nk, 1 );

        nk_layout_row_dynamic( content->nk, 30, 1 );

        const bool prevEnabled = content->enableBlur;
        nk_checkbox_label( content->nk, "Enable Blur", &content->enableBlur );
        reset |= prevEnabled != content->enableBlur;

        if ( content->enableBlur ) {
            const auto prevItCount = content->blurItCount;
            nk_property_int( content->nk, "Blur Iterations", 0, &content->blurItCount, 16, 1, 1 );
            reset |= prevItCount != content->blurItCount;

            nk_property_float( content->nk, "Blur Radius", 0, &content->blurRadius, 16, 0.01, 0.001 );
        }

        hovered = nk_window_is_any_hovered( content->nk );
    }
    nk_end( content->nk );

    //
    // Handle resizing.
    //

    if ( width != content->width || height != content->height || reset ) {
        content->width  = width;
        content->height = height;
        bgfx::reset( content->width, content->height, content->resetFlags );

        if ( content->enableBlur ) {
            const uint32_t samplerFlags = BGFX_TEXTURE_RT_MSAA_X16 | BGFX_TEXTURE_MIN_POINT | BGFX_TEXTURE_MAG_POINT |
                                          BGFX_TEXTURE_MIP_POINT | BGFX_TEXTURE_U_CLAMP | BGFX_TEXTURE_V_CLAMP;

            content->colorTextureHandle = bgfx::createTexture2D( width, height, false, 1, bgfx::TextureFormat::BGRA8, samplerFlags );
            content->depthTextureHandle = bgfx::createTexture2D( width, height, false, 1, bgfx::TextureFormat::D24, samplerFlags );
            bgfx::TextureHandle textureHandles[] = {content->colorTextureHandle, content->depthTextureHandle};

            content->colorHandle = bgfx::createFrameBuffer( 2, textureHandles, true );
            content->blurredHandles[ 0 ] = bgfx::createFrameBuffer( width, height, bgfx::TextureFormat::BGRA8, samplerFlags );
            content->blurredHandles[ 1 ] = bgfx::createFrameBuffer( width, height, bgfx::TextureFormat::BGRA8, samplerFlags );
        }
    }

    if ( inputState.Buttons[ 0 ][ kDigitalInput_KeyTab ] ) {
        bgfx::setDebug( BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS );
    } else {
        bgfx::setDebug( BGFX_DEBUG_TEXT );
    }

    if ( !hovered ) {
        content->mouse.update( inputState.Analogs[ kAnalogInput_MouseX ],
                               inputState.Analogs[ kAnalogInput_MouseY ],
                               inputState.Analogs[ kAnalogInput_MouseScroll ],
                               width,
                               height );

        if ( inputState.Buttons[ 0 ][ kDigitalInput_Mouse0 ] ) {
            content->camera.orbit( content->mouse.m_dx, content->mouse.m_dy );
        } else if ( inputState.Buttons[ 0 ][ kDigitalInput_Mouse1 ] ) {
            content->camera.dolly( content->mouse.m_dx + content->mouse.m_dy );
        }

    }
    content->camera.update( deltaSecs );
    memcpy( content->uniforms.m_cameraPos, content->camera.m_pos.curr, 3 * sizeof( float ) );

    bgfx::dbgTextClear( );
    bgfx::dbgTextPrintf( 0, 1, 0x4f, "fbxv/ubisoft demo" );
    bgfx::dbgTextPrintf( 0, 2, 0x2f, "Frame: % 7.3f[ms]", deltaSecs * 1000.0f );

    float proj[16];
    float view[16];

    // Env mtx.
    float mtxEnvView[ 16 ];
    content->camera.envViewMtx( content->uniforms.m_mtx );
    bx::mtxIdentity( view );
    bx::mtxOrtho( proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f );

    bgfx::setViewClear( 0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 1.0f, 0, 0 );
    bgfx::setViewRect( 0, 0, 0, uint16_t( width ), uint16_t( height ) );
    if ( content->enableBlur )
        bgfx::setViewFrameBuffer( 0, content->colorHandle );
    bgfx::setViewTransform( 0, view, proj );
    bgfx::setTexture( 0, content->s_texCube, content->radianceTextureHandles[ content->envId ] );
    bgfx::setTexture( 1, content->s_texCubeIrr, content->irradianceTextureHandles[ content->envId ] );
    bgfx::setState( BGFX_STATE_RGB_WRITE | BGFX_STATE_ALPHA_WRITE );
    screenSpaceQuad( (float) width, (float) height, 0, bgfx::getCaps( )->originBottomLeft );
    content->uniforms.submit( );
    bgfx::submit( 0, content->programSky );

    //
    // Geometry pass
    //

    content->camera.mtxLookAt( view );
    bx::mtxProj( proj, 45.0f, float( width ) / float( height ), 0.1f, 1000.0f );

    auto scene = content->scenes[ content->sceneId ];

    scene->ForEachNode( [&]( uint32_t nodeId ) {
        assert( nodeId != uint32_t( -1 ) );
        auto& node = scene->nodes[ nodeId ];
        if ( node.meshId != uint32_t( -1 ) ) {
            auto& mesh = scene->meshes[ node.meshId ];

            float mtx[ 16 ];
            memcpy( mtx, &scene->worldMatrices[ nodeId ], sizeof( mtx ) );

            bgfx::setTexture( 0, content->s_texCube, content->radianceTextureHandles[ content->envId ] );
            bgfx::setTexture( 1, content->s_texCubeIrr, content->irradianceTextureHandles[ content->envId ] );
            content->uniforms.submit( );

            auto state = BGFX_STATE_RGB_WRITE |
                         BGFX_STATE_ALPHA_WRITE |
                         BGFX_STATE_DEPTH_WRITE |
                         BGFX_STATE_DEPTH_TEST_LESS |
                         BGFX_STATE_MSAA;

            bgfx::setViewRect( 1, 0, 0, uint16_t( width ), uint16_t( height ) );

            if ( content->enableBlur ) {
                bgfx::setViewFrameBuffer( 1, content->colorHandle );
            }

            bgfx::setViewTransform( 1, view, proj );
            bgfx::setTransform( mtx );
            bgfx::setState( state );
            bgfx::setVertexBuffer( mesh.vertexBufferHandle );
            bgfx::submit( 1, content->programMesh, 0, false );
        }
    } );

    uint32_t id = 2;

    if ( content->enableBlur ) {
        //
        // Blurring pass
        //

        ++id;

        bx::mtxOrtho( proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f );

        bgfx::ProgramHandle programBlur  = content->programBlur[ content->blurId ];
        bgfx::TextureHandle colorTexture = bgfx::getTexture( content->colorHandle );

        float blurParams[ 4 ];
        blurParams[ 0 ] = (float) width;
        blurParams[ 1 ] = (float) height;

        for ( uint32_t blurIt = 0; blurIt < content->blurItCount; ++blurIt ) {
            //
            // Vertical blurring
            //

            blurParams[ 2 ] = (float) 0;
            blurParams[ 3 ] = (float) content->blurRadius;

            bgfx::setViewClear( id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 1.0f, 0, 0 );
            bgfx::setViewRect( id, 0, 0, uint16_t( width ), uint16_t( height ) );
            bgfx::setViewTransform( id, nullptr, proj );
            bgfx::setViewFrameBuffer( id, content->blurredHandles[ 0 ] );
            bgfx::setUniform( content->u_blurParams, blurParams );

            if ( blurIt )
                bgfx::setTexture( 0, content->s_colorTexture, bgfx::getTexture( content->blurredHandles[ 1 ] ) );
            else
                bgfx::setTexture( 0, content->s_colorTexture, colorTexture );

            bgfx::setState( BGFX_STATE_RGB_WRITE );
            screenSpaceQuad( (float) width, (float) height, 0, bgfx::getCaps( )->originBottomLeft );
            bgfx::submit( id, programBlur );
            ++id;

            //
            // Horizontal blurring
            //

            blurParams[ 2 ] = (float) content->blurRadius;
            blurParams[ 3 ] = (float) 0;

            bgfx::setViewClear( id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 1.0f, 0, 0 );
            bgfx::setViewRect( id, 0, 0, uint16_t( width ), uint16_t( height ) );
            bgfx::setViewFrameBuffer( id, content->blurredHandles[ 1 ] );
            bgfx::setViewTransform( id, nullptr, proj );
            bgfx::setUniform( content->u_blurParams, blurParams );
            bgfx::setTexture( 0, content->s_colorTexture, bgfx::getTexture( content->blurredHandles[ 0 ] ) );
            bgfx::setState( BGFX_STATE_RGB_WRITE );
            screenSpaceQuad( (float) width, (float) height, 0, bgfx::getCaps( )->originBottomLeft );
            bgfx::submit( id, programBlur );
            ++id;
        }

        //
        // Combine pass
        //

        bgfx::setViewClear( id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 1.0f, 0, 0 );
        bgfx::setViewRect( id, 0, 0, uint16_t( width ), uint16_t( height ) );
        bgfx::setViewTransform( id, nullptr, proj );
        bgfx::setTexture( 0, content->s_colorTexture, colorTexture );
        bgfx::setTexture( 1, content->s_blurredTexture, bgfx::getTexture( content->blurredHandles[ 1 ] ) );
        bgfx::setTexture( 2, content->s_maskTexture, content->maskTextureHandles[ content->maskId ] );
        bgfx::setState( BGFX_STATE_RGB_WRITE );
        screenSpaceQuad( (float) width, (float) height, 0, bgfx::getCaps( )->originBottomLeft );
        bgfx::submit( id, content->programMaskCombine );
        ++id;
    }

    nk_sdl_render( NK_ANTI_ALIASING_ON, 36 );
}

bool App::IsRunning( ) {
    return AppBase::IsRunning( );
}

extern "C" AppBase* CreateApp( ) {
    return new App( );
}