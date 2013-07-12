#include "ExampleMainPch.h"

#include "Components/ComponentsPch.h"
#include "EditorSupport/EditorSupportPch.h"
#include "Bullet/BulletPch.h"

#include "Ois/OisSystem.h"

#include "Framework/SceneDefinition.h"
#include "Framework/WorldManager.h"
#include "Engine/AssetPath.h"

#include "Rendering/Renderer.h"
#include "Windowing/Window.h"


#include "ExampleGame/Components/Graphics/Sprite.h"


#include "Bullet/BulletEngine.h"
#include "Bullet/BulletWorld.h"
#include "Bullet/BulletWorldDefinition.h"
#include "Bullet/BulletBodyDefinition.h"
#include "Bullet/BulletBodyComponent.h"
#include "Bullet/BulletShapes.h"
#include "Bullet/BulletBody.h"
#include "Bullet/BulletWorldComponent.h"

#include "Persist/ArchiveJson.h"
#include "Foundation/MemoryStream.h"



using namespace Helium;

/// Windows application entry point.
///
/// @param[in] hInstance      Handle to the current instance of the application.
/// @param[in] hPrevInstance  Handle to the previous instance of the application (always null; ignored).
/// @param[in] lpCmdLine      Command line for the application, excluding the program name.
/// @param[in] nCmdShow       Flags specifying how the application window should be shown.
///
/// @return  Result code of the application.
int APIENTRY _tWinMain( HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nCmdShow )
{
	ForceLoadBulletDll();
	ForceLoadComponentsDll();
	ForceLoadExampleGameDll();

#if HELIUM_TOOLS
	ForceLoadEditorSupportDll();
#endif

	HELIUM_TRACE_SET_LEVEL( TraceLevels::Debug );

	int32_t result = 0;

	{
		// Initialize a GameSystem instance.
		CommandLineInitializationWin commandLineInitialization;
		MemoryHeapPreInitializationWin memoryHeapPreInitialization;
		AssetLoaderInitializationWin assetLoaderInitialization;
		ConfigInitializationWin configInitialization;
		WindowManagerInitializationWin windowManagerInitialization( hInstance, nCmdShow );
		RendererInitializationWin rendererInitialization;
		AssetPath systemDefinitionPath( "/ExampleGames/ShapeShooter:System" );
		//NullRendererInitialization rendererInitialization;

		GameSystem* pGameSystem = GameSystem::CreateStaticInstance();
		HELIUM_ASSERT( pGameSystem );
		bool bSystemInitSuccess = pGameSystem->Initialize(
			commandLineInitialization,
			memoryHeapPreInitialization,
			assetLoaderInitialization,
			configInitialization,
			windowManagerInitialization,
			rendererInitialization,
			systemDefinitionPath
			);

		{
			Helium::AssetLoader *pAssetLoader = AssetLoader::GetStaticInstance();
			BulletBodyComponentDefinitionPtr spBulletBody;

			AssetPath ap( "/ExampleGames/ShapeShooter:Bullet_BulletBody" );
			pAssetLoader->LoadObject(ap, spBulletBody);

			BulletBodyComponentDefinition *pBulletBody = spBulletBody.Get();

			Reflect::ObjectPtr spCOpy = pBulletBody->Clone();
			BulletBodyComponentDefinition *pBulletBodyCopy = Reflect::AssertCast<BulletBodyComponentDefinition>(spCOpy.Get());
			int i = 0;

		}
		
		{
			Helium::AssetLoader *pAssetLoader = AssetLoader::GetStaticInstance();
			Helium::SceneDefinitionPtr spSceneDefinition;

			AssetPath scenePath( TXT( "/ExampleGames/ShapeShooter/Scenes/TestScene:SceneDefinition" ) );
			pAssetLoader->LoadObject(scenePath, spSceneDefinition );

			HELIUM_ASSERT( !spSceneDefinition->GetAllFlagsSet( Asset::FLAG_BROKEN ) );

			pGameSystem->LoadScene(spSceneDefinition.Get());
		}

		if( bSystemInitSuccess )
		{


			void *windowHandle = rendererInitialization.GetMainWindow()->GetHandle();
			Input::Initialize(&windowHandle, false);
			Input::SetWindowSize( 
				rendererInitialization.GetMainWindow()->GetWidth(),
				rendererInitialization.GetMainWindow()->GetHeight());

			// Run the application.
			result = pGameSystem->Run();
		}

		// Shut down and destroy the system.
		pGameSystem->Shutdown();
		System::DestroyStaticInstance();
	}

	// Perform final cleanup.
	ThreadLocalStackAllocator::ReleaseMemoryHeap();

#if HELIUM_ENABLE_MEMORY_TRACKING
	DynamicMemoryHeap::LogMemoryStats();
	ThreadLocalStackAllocator::ReleaseMemoryHeap();
#endif

	return result;
}