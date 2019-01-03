cmake_minimum_required(VERSION 2.8.8)

# Make sure the user is not executing this script directly
if(NOT InWarzone)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InWarzone)

set(MPGameIncludeDirectories "${MPDir}")
if(WIN32)
	set(MPGameLibraries "winmm" "../../../lib/navlib")
endif(WIN32)
set(MPGameDefines ${MPSharedDefines} "_GAME" )

set(MPGameDetourFiles
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
source_group("Detour" FILES ${MPGameDetourFiles})
set(MPGameFiles ${MPCGameFiles} ${MPGameDetourFiles})

set(MPGameRecastFiles
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
source_group("Recast" FILES ${MPGameRecastFiles})
set(MPGameFiles ${MPGameFiles} ${MPGameRecastFiles})

set(MPGameGameFiles
  "${MPDir}/game/ai_dmq3.cpp"
	"${MPDir}/game/ai_dmq3.h"
	"${MPDir}/game/ai_dominance.cpp"
	"${MPDir}/game/ai_dominance.h"
	"${MPDir}/game/ai_dominance_astar.cpp"
	"${MPDir}/game/ai_dominance_main.h"
	"${MPDir}/game/ai_dominance_navmesh.cpp"
	"${MPDir}/game/ai_dominance_navmesh.h"
	"${MPDir}/game/ai_dominance_wpnav.cpp"
	"${MPDir}/game/bg_g2_utils.cpp"
	"${MPDir}/game/bg_inventory.cpp"
	"${MPDir}/game/bg_inventory.h"
	"${MPDir}/game/bg_inventoryItem.cpp"
	"${MPDir}/game/bg_inventoryItem.h"
	"${MPDir}/game/bg_misc.cpp"
	"${MPDir}/game/bg_panimate.cpp"
	"${MPDir}/game/bg_pmove.cpp"
	"${MPDir}/game/bg_saber.cpp"
	"${MPDir}/game/bg_saberLoad.cpp"
	"${MPDir}/game/bg_saga.cpp"
	"${MPDir}/game/bg_slidemove.cpp"
	"${MPDir}/game/bg_vehicleLoad.cpp"
	"${MPDir}/game/bg_weapons.cpp"
	"${MPDir}/game/g_active.cpp"
	"${MPDir}/game/g_bot.cpp"
	"${MPDir}/game/g_client.cpp"
	"${MPDir}/game/g_cmds.cpp"
	"${MPDir}/game/g_combat.cpp"
	"${MPDir}/game/g_cvar.cpp"
	"${MPDir}/game/g_exphysics.cpp"
	"${MPDir}/game/g_foliage.cpp"
	"${MPDir}/game/g_ICARUScb.cpp"
	"${MPDir}/game/g_items.cpp"
	"${MPDir}/game/g_log.cpp"
	"${MPDir}/game/g_main.cpp"
	"${MPDir}/game/g_mem.cpp"
	"${MPDir}/game/g_misc.cpp"
	"${MPDir}/game/g_missile.cpp"
	"${MPDir}/game/g_mover.cpp"
	"${MPDir}/game/g_nav.cpp"
	"${MPDir}/game/g_navnew.cpp"
	"${MPDir}/game/g_object.cpp"
	"${MPDir}/game/g_saga.cpp"
	"${MPDir}/game/g_session.cpp"
	"${MPDir}/game/g_spawn.cpp"
	"${MPDir}/game/g_svcmds.cpp"
	"${MPDir}/game/g_syscalls.cpp"
	"${MPDir}/game/g_target.cpp"
	"${MPDir}/game/g_team.cpp"
	"${MPDir}/game/g_timer.cpp"
	"${MPDir}/game/g_trigger.cpp"
	"${MPDir}/game/g_turret.cpp"
	"${MPDir}/game/g_turret_G2.cpp"
	"${MPDir}/game/g_utils.cpp"
	"${MPDir}/game/g_vehicles.cpp"
	"${MPDir}/game/g_vehicleTurret.cpp"
	"${MPDir}/game/g_weapon.cpp"
	"${MPDir}/game/NPC.cpp"
	"${MPDir}/game/NPC_AI_Animal.cpp"
	"${MPDir}/game/NPC_AI_AssassinDroid.cpp"
	"${MPDir}/game/NPC_AI_Atst.cpp"
	"${MPDir}/game/NPC_AI_Cower.cpp"
	"${MPDir}/game/NPC_AI_Default.cpp"
	"${MPDir}/game/NPC_AI_Droid.cpp"
	"${MPDir}/game/NPC_AI_Fighter.cpp"
	"${MPDir}/game/NPC_AI_GalakMech.cpp"
	"${MPDir}/game/NPC_AI_Grenadier.cpp"
	"${MPDir}/game/NPC_AI_Howler.cpp"
	"${MPDir}/game/NPC_AI_ImperialProbe.cpp"
	"${MPDir}/game/NPC_AI_Interrogator.cpp"
	"${MPDir}/game/NPC_AI_Jedi.cpp"
	"${MPDir}/game/NPC_AI_Jetpack.cpp"
	"${MPDir}/game/NPC_AI_Mark1.cpp"
	"${MPDir}/game/NPC_AI_Mark2.cpp"
	"${MPDir}/game/NPC_AI_MineMonster.cpp"
	"${MPDir}/game/NPC_AI_Padawan.cpp"
	"${MPDir}/game/NPC_AI_Path.cpp"
	"${MPDir}/game/NPC_AI_Patrol.cpp"
	"${MPDir}/game/NPC_AI_Rancor.cpp"
	"${MPDir}/game/NPC_AI_Remote.cpp"
	"${MPDir}/game/NPC_AI_RocketTrooper.cpp"
	"${MPDir}/game/NPC_AI_SaberDroid.cpp"
	"${MPDir}/game/NPC_AI_Sand_Creature.cpp"
	"${MPDir}/game/NPC_AI_Seeker.cpp"
	"${MPDir}/game/NPC_AI_Sentry.cpp"
	"${MPDir}/game/NPC_AI_Sniper.cpp"
	"${MPDir}/game/NPC_AI_Speeder.cpp"
	"${MPDir}/game/NPC_AI_Stormtrooper.cpp"
	"${MPDir}/game/NPC_AI_Utils.cpp"
	"${MPDir}/game/NPC_AI_Walker.cpp"
	"${MPDir}/game/NPC_AI_Wampa.cpp"
	"${MPDir}/game/NPC_Behavior.cpp"
	"${MPDir}/game/NPC_Combat.cpp"
	"${MPDir}/game/NPC_Conversations.cpp"
	"${MPDir}/game/NPC_Goal.cpp"
	"${MPDir}/game/NPC_Headers.h"
	"${MPDir}/game/NPC_Misc.cpp"
	"${MPDir}/game/NPC_Move.cpp"
	"${MPDir}/game/NPC_Reactions.cpp"
	"${MPDir}/game/NPC_Senses.cpp"
	"${MPDir}/game/NPC_Sounds.cpp"
	"${MPDir}/game/NPC_Spawn.cpp"
	"${MPDir}/game/NPC_Stats.cpp"
	"${MPDir}/game/NPC_Utils.cpp"
	"${MPDir}/game/tri_coll_test.cpp"
	"${MPDir}/game/w_force.cpp"
	"${MPDir}/game/w_saber.cpp"
	"${MPDir}/game/w_saberblock.cpp"
	"${MPDir}/game/anims.h"
	"${MPDir}/game/b_local.h"
	"${MPDir}/game/b_public.h"
	"${MPDir}/game/bg_local.h"
	"${MPDir}/game/bg_public.h"
	"${MPDir}/game/bg_saga.h"
	"${MPDir}/game/bg_vehicles.h"
	"${MPDir}/game/bg_weapons.h"
	"${MPDir}/game/chars.h"
	"${MPDir}/game/g_ICARUScb.h"
	"${MPDir}/game/g_local.h"
	"${MPDir}/game/g_nav.h"
	"${MPDir}/game/g_public.h"
	"${MPDir}/game/g_team.h"
	"${MPDir}/game/g_xcvar.h"
	"${MPDir}/game/inv.h"
	"${MPDir}/game/jkg_damagetypes.cpp"
  "${MPDir}/game/jkg_damagetypes.h"
	"${MPDir}/game/match.h"
	"${MPDir}/game/say.h"
	"${MPDir}/game/surfaceflags.h"
	"${MPDir}/game/teams.h"
	"${MPDir}/game/w_saber.h"
	"${MPDir}/game/ckheap.h"
	"${MPDir}/client/tinythread.cpp"
	"${MPDir}/client/tinythread.h"
	"${MPDir}/client/fast_mutex.h"
	"${MPDir}/navlib/navlib_api.h"
	)
source_group("game" FILES ${MPGameGameFiles})
set(MPGameFiles ${MPGameFiles} ${MPGameGameFiles})

set(MPGameCommonFiles
	"${MPDir}/qcommon/inifile.cpp"
	"${MPDir}/qcommon/inifile.h"
	"${MPDir}/qcommon/q_math.cpp"
	"${MPDir}/qcommon/q_shared.cpp"
	"${MPDir}/qcommon/disablewarnings.h"
	"${MPDir}/qcommon/q_shared.h"
	"${MPDir}/qcommon/q_platform.h"
	"${MPDir}/qcommon/tags.h"
	)
source_group("common" FILES ${MPGameCommonFiles})
set(MPGameFiles ${MPGameFiles} ${MPGameCommonFiles})

set(MPGameCgameFiles
	"${MPDir}/cgame/animtable.h"
	"${MPDir}/cgame/cg_public.h"
	)
source_group("cgame" FILES ${MPGameCgameFiles})
set(MPGameFiles ${MPGameFiles} ${MPGameCgameFiles})

set(MPGameRendererFiles
	"${MPDir}/rd-common/tr_types.h"
	)
source_group("rd-common" FILES ${MPGameRendererFiles})
set(MPGameFiles ${MPGameFiles} ${MPGameRendererFiles})

set(MPGameBotlibFiles
	"${MPDir}/botlib/be_aas.h"
	"${MPDir}/botlib/be_ai_char.h"
	"${MPDir}/botlib/be_ai_chat.h"
	"${MPDir}/botlib/be_ai_gen.h"
	"${MPDir}/botlib/be_ai_goal.h"
	"${MPDir}/botlib/be_ai_move.h"
	"${MPDir}/botlib/be_ai_weap.h"
	"${MPDir}/botlib/be_ea.h"
	"${MPDir}/botlib/botlib.h"
	)
source_group("botlib" FILES ${MPGameBotlibFiles})
set(MPGameFiles ${MPGameFiles} ${MPGameBotlibFiles})

set(MPGameG2Files
	"${MPDir}/ghoul2/G2.h"
	)
source_group("ghoul2" FILES ${MPGameG2Files})
set(MPGameFiles ${MPGameFiles} ${MPGameG2Files})

set(MPGameIcarusFiles
	"${MPDir}/icarus/interpreter.h"
	"${MPDir}/icarus/Q3_Interface.h"
	)
source_group("icarus" FILES ${MPGameIcarusFiles})
set(MPGameFiles ${MPGameFiles} ${MPGameIcarusFiles})

set(MPGameUiFiles
	"${MPDir}/ui/menudef.h"
	)
source_group("ui" FILES ${MPGameUiFiles})
set(MPGameFiles ${MPGameFiles} ${MPGameUiFiles})

add_library(${MPGame} SHARED ${MPGameFiles})

if(NOT WIN32)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPGame} PROPERTIES PREFIX "")
endif(NOT WIN32)

if(WIN32)
	install(TARGETS ${MPGame} RUNTIME DESTINATION "Warzone")
else(WIN32)
	if(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPGame} LIBRARY DESTINATION "${MPEngine}.app/Contents/MacOS/Warzone")
		install(TARGETS ${MPGame} LIBRARY DESTINATION "${MPEngine}.app/Contents/MacOS/base")
	else(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPGame} LIBRARY DESTINATION "Warzone")
	endif(MakeApplicationBundles AND BuildMPEngine)
endif(WIN32)
set_target_properties(${MPGame} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${MPGameDefines};${ReleaseDefines}")
set_target_properties(${MPGame} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${MPGameDefines};${ReleaseDefines}")
set_target_properties(${MPGame} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${MPGameDefines};${ReleaseDefines}")
set_target_properties(${MPGame} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${MPGameDefines};${DebugDefines}")
set_target_properties(${MPGame} PROPERTIES INCLUDE_DIRECTORIES "${MPGameIncludeDirectories}")
set_target_properties(${MPGame} PROPERTIES PROJECT_LABEL "MP Game Library")
# no libraries used
if(MPGameLibraries)
	target_link_libraries(${MPGame} ${MPGameLibraries})
endif(MPGameLibraries)
