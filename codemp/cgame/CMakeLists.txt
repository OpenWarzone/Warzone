cmake_minimum_required(VERSION 2.8.8)

# Make sure the user is not executing this script directly
if(NOT InWarzone)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InWarzone)

set(MPCGameIncludeDirectories "${MPDir}")
if(WIN32)
	set(MPCGameLibraries "odbc32" "odbccp32") # what are these even?
endif(WIN32)
set(MPCGameDefines ${MPSharedDefines} "_CGAME" )
set(MPCGameGameFiles
	"${MPDir}/game/NPC_AI_Animal.cpp"
	"${MPDir}/game/NPC_AI_Fighter.cpp"
	"${MPDir}/game/NPC_AI_Speeder.cpp"
	"${MPDir}/game/NPC_AI_Walker.cpp"
	"${MPDir}/game/bg_g2_utils.cpp"
	"${MPDir}/game/bg_misc.cpp"
	"${MPDir}/game/bg_panimate.cpp"
	"${MPDir}/game/bg_pmove.cpp"
	"${MPDir}/game/bg_saber.cpp"
	"${MPDir}/game/bg_saberLoad.cpp"
	"${MPDir}/game/bg_saga.cpp"
	"${MPDir}/game/bg_slidemove.cpp"
	"${MPDir}/game/bg_vehicleLoad.cpp"
	"${MPDir}/game/bg_weapons.cpp"
	"${MPDir}/game/anims.h"
	"${MPDir}/game/bg_local.h"
	"${MPDir}/game/bg_public.h"
	"${MPDir}/game/bg_saga.h"
	"${MPDir}/game/bg_weapons.h"
	"${MPDir}/game/surfaceflags.h"
	)
source_group("game" FILES ${MPCGameGameFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameGameFiles})

set(MPCGameCgameFiles
  "${MPDir}/game/bg_inventory.cpp"
	"${MPDir}/game/bg_inventory.h"
	"${MPDir}/game/bg_inventoryItem.cpp"
	"${MPDir}/game/bg_inventoryItem.h"
	"${MPDir}/game/bg_inventoryReforge.cpp"
	"${MPDir}/game/bg_inventoryReforge.h"
	"${MPDir}/cgame/cg_consolecmds.cpp"
	"${MPDir}/cgame/cg_cvar.cpp"
	"${MPDir}/cgame/cg_cubemaps.cpp"
	"${MPDir}/cgame/cg_cull.cpp"
	"${MPDir}/cgame/cg_draw.cpp"
	"${MPDir}/cgame/cg_drawtools.cpp"
	"${MPDir}/cgame/cg_effects.cpp"
	"${MPDir}/cgame/cg_ents.cpp"
	"${MPDir}/cgame/cg_event.cpp"
	"${MPDir}/cgame/cg_foliage.cpp"
	"${MPDir}/cgame/cg_info.cpp"
	"${MPDir}/cgame/cg_laserbolts.cpp"
	"${MPDir}/cgame/cg_light.cpp"
	"${MPDir}/cgame/cg_localents.cpp"
	"${MPDir}/cgame/cg_main.cpp"
	"${MPDir}/cgame/cg_mapefx.cpp"
	"${MPDir}/cgame/cg_marks.cpp"
	"${MPDir}/cgame/cg_nav_autowaypoint.cpp"
	"${MPDir}/cgame/cg_nav_navmesh_creator.cpp"
	"${MPDir}/cgame/cg_newDraw.cpp"
	"${MPDir}/cgame/cg_players.cpp"
	"${MPDir}/cgame/cg_playerstate.cpp"
	"${MPDir}/cgame/cg_predict.cpp"
	"${MPDir}/cgame/cg_saga.cpp"
	"${MPDir}/cgame/cg_scoreboard.cpp"
	"${MPDir}/cgame/cg_servercmds.cpp"
	"${MPDir}/cgame/cg_snapshot.cpp"
	"${MPDir}/cgame/cg_spawn.cpp"
	"${MPDir}/cgame/cg_syscalls.cpp"
	"${MPDir}/cgame/cg_texttospeech.cpp"
	"${MPDir}/cgame/cg_turret.cpp"
	"${MPDir}/cgame/cg_view.cpp"
	"${MPDir}/cgame/cg_weaponinit.cpp"
	"${MPDir}/cgame/cg_weather.cpp"
	"${MPDir}/cgame/cg_holster.cpp"
	"${MPDir}/cgame/cg_holster.h"
	"${MPDir}/cgame/cg_weapons.cpp"
	"${MPDir}/cgame/fx_force.cpp"
	"${MPDir}/cgame/fx_weapons.cpp"
	"${MPDir}/cgame/fx_weapons_demp2.cpp"
	"${MPDir}/cgame/fx_weapons_disruptor.cpp"
	"${MPDir}/cgame/fx_weapons_heavyrepeater.cpp"
	"${MPDir}/cgame/fx_weapons_pistol.cpp"
	"${MPDir}/cgame/fx_weapons_rocketlauncher.cpp"
	"${MPDir}/cgame/animtable.h"
	"${MPDir}/cgame/cg_local.h"
	"${MPDir}/cgame/cg_public.h"
	"${MPDir}/cgame/cg_xcvar.h"
	"${MPDir}/cgame/fx_local.h"
	"${MPDir}/game/ckheap.h"
	"${MPDir}/client/tinythread.cpp"
	"${MPDir}/client/tinythread.h"
	"${MPDir}/client/fast_mutex.h"
	)
source_group("cgame" FILES ${MPCGameCgameFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameCgameFiles})

set(MPCGameCommonFiles
	"${MPDir}/qcommon/inifile.cpp"
	"${MPDir}/qcommon/inifile.h"
	"${MPDir}/qcommon/q_math.cpp"
	"${MPDir}/qcommon/q_shared.cpp"
	"${MPDir}/qcommon/disablewarnings.h"
	"${MPDir}/qcommon/q_shared.h"
	"${MPDir}/qcommon/q_platform.h"
	"${MPDir}/qcommon/qfiles.h"
	"${MPDir}/qcommon/tags.h"
	)
source_group("common" FILES ${MPCGameCommonFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameCommonFiles})

set(MPCGameUiFiles
	"${MPDir}/ui/ui_shared.cpp"
	"${MPDir}/ui/keycodes.h"
	"${MPDir}/ui/menudef.h"
	"${MPDir}/ui/ui_shared.h"
	)
source_group("ui" FILES ${MPCGameUiFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameUiFiles})

set(MPCGameDetourFiles
	"${MPDir}/Recast/Detour/DetourAlloc.cpp"
	"${MPDir}/Recast/Detour/DetourAlloc.h"
	"${MPDir}/Recast/Detour/DetourAssert.h"
	"${MPDir}/Recast/Detour/DetourCommon.cpp"
	"${MPDir}/Recast/Detour/DetourCommon.h"
	"${MPDir}/Recast/Detour/DetourMath.h"
	"${MPDir}/Recast/Detour/DetourNavMesh.cpp"
	"${MPDir}/Recast/Detour/DetourNavMesh.h"
	"${MPDir}/Recast/Detour/DetourNavMeshBuilder.cpp"
	"${MPDir}/Recast/Detour/DetourNavMeshBuilder.h"
	"${MPDir}/Recast/Detour/DetourNavMeshQuery.cpp"
	"${MPDir}/Recast/Detour/DetourNavMeshQuery.h"
	"${MPDir}/Recast/Detour/DetourNode.cpp"
	"${MPDir}/Recast/Detour/DetourNode.h"
	"${MPDir}/Recast/Detour/DetourStatus.h"
	)
source_group("Detour" FILES ${MPCGameDetourFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameDetourFiles})

set(MPCGameRecastFiles
	"${MPDir}/Recast/Recast/Recast.cpp"
	"${MPDir}/Recast/Recast/Recast.h"
	"${MPDir}/Recast/Recast/RecastAlloc.cpp"
	"${MPDir}/Recast/Recast/RecastAlloc.h"
	"${MPDir}/Recast/Recast/RecastArea.cpp"
	"${MPDir}/Recast/Recast/RecastAssert.h"
	"${MPDir}/Recast/Recast/RecastContour.cpp"
	"${MPDir}/Recast/Recast/RecastFilter.cpp"
	"${MPDir}/Recast/Recast/RecastLayers.cpp"
	"${MPDir}/Recast/Recast/RecastMesh.cpp"
	"${MPDir}/Recast/Recast/RecastMeshDetail.cpp"
	"${MPDir}/Recast/Recast/RecastRasterization.cpp"
	"${MPDir}/Recast/Recast/RecastRegion.cpp"
	)
source_group("Recast" FILES ${MPCGameRecastFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameRecastFiles})

set(MPWarzoneTinyImageLoaderFiles
	"${MPDir}/rd-warzone/TinyImageLoader/TILColors.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILFileStream.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TILFileStream.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILFileStreamStd.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TILFileStreamStd.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImage.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImage.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageBMP.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageBMP.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageDDS.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageDDS.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageGIF.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageGIF.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageICO.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageICO.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImagePNG.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImagePNG.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageTemplate.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageTemplate.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageTGA.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TILImageTGA.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILInternal.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TILSettings.h"
	"${MPDir}/rd-warzone/TinyImageLoader/TinyImageLoader.cpp"
	"${MPDir}/rd-warzone/TinyImageLoader/TinyImageLoader.h")
source_group("TinyImageLoader" FILES ${MPWarzoneTinyImageLoaderFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPWarzoneTinyImageLoaderFiles})

set(MPCGameRendererFiles
	"${MPDir}/rd-common/tr_types.h"
	)
source_group("rd-common" FILES ${MPCGameRendererFiles})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameRendererFiles})

set(MPCGameGFiles
	"${MPDir}/ghoul2/G2.h"
	)
source_group("ghoul2" FILES ${MPCGameG2Files})
set(MPCGameFiles ${MPCGameFiles} ${MPCGameG2Files})

add_library(${MPCGame} SHARED ${MPCGameFiles})

if(NOT WIN32)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPCGame} PROPERTIES PREFIX "")
endif(NOT WIN32)

if(WIN32)
	install(TARGETS ${MPCGame} RUNTIME DESTINATION "Warzone")
else(WIN32)
	if(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPCGame} LIBRARY DESTINATION "${MPEngine}.app/Contents/MacOS/Warzone")
		install(TARGETS ${MPCGame} LIBRARY DESTINATION "${MPEngine}.app/Contents/MacOS/base")
	else(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPCGame} LIBRARY DESTINATION "Warzone")
	endif(MakeApplicationBundles AND BuildMPEngine)
endif(WIN32)
set_target_properties(${MPCGame} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${MPCGameDefines};${ReleaseDefines}")
set_target_properties(${MPCGame} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${MPCGameDefines};${ReleaseDefines}")
set_target_properties(${MPCGame} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${MPCGameDefines};${ReleaseDefines}")
set_target_properties(${MPCGame} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${MPCGameDefines};${DebugDefines}")
set_target_properties(${MPCGame} PROPERTIES INCLUDE_DIRECTORIES "${MPCGameIncludeDirectories}")
set_target_properties(${MPCGame} PROPERTIES PROJECT_LABEL "MP Client Game Library")
# no libraries used
if(MPCGameLibraries)
	target_link_libraries(${MPCGame} ${MPCGameLibraries})
endif(MPCGameLibraries)
