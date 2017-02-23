#include <fbxvpch.h>

#include <AppBase.h>
#include <SDL.h>
#include <SDL_main.h>

extern "C" fbxv::AppBase* CreateApp( );

extern "C" int SDL_main( int Args, char* ppArgs[] ) {
    fbxv::AppBase* pAppImpl = CreateApp( );

    if ( pAppImpl && pAppImpl->Initialize( Args, ppArgs ) ) {
        while ( pAppImpl->IsRunning( ) )
            pAppImpl->OnFrameMove( );
    }

    if ( pAppImpl )
        delete pAppImpl;
    return 0;
}
