cmake_minimum_required(VERSION 2.8.8)

# Make sure the user is not executing this script directly
if(NOT InWarzone)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InWarzone)

set(MPUIIncludeDirectories "${MPDir}")
if(WIN32)
	set(MPUILibraries "odbc32" "odbccp32") # what are these even?
endif(WIN32)
set(MPUIDefines ${MPSharedDefines} "_UI")

set(MPUIGameFiles
	"${MPDir}/game/bg_misc.cpp"
	"${MPDir}/game/bg_saberLoad.cpp"
	"${MPDir}/game/bg_saga.cpp"
	"${MPDir}/game/bg_vehicleLoad.cpp"
	"${MPDir}/game/bg_weapons.cpp"
	"${MPDir}/game/anims.h"
	"${MPDir}/game/bg_local.h"
	"${MPDir}/game/bg_public.h"
	"${MPDir}/game/bg_saga.h"
	"${MPDir}/game/bg_weapons.h"
	"${MPDir}/game/surfaceflags.h"
	)
source_group("game" FILES ${MPUIGameFiles})
set(MPUIFiles ${MPUIFiles} ${MPUIGameFiles})

set(MPUICommonFiles
  "${MPDir}/qcommon/inifile.cpp"
  "${MPDir}/qcommon/inifile.h"
	"${MPDir}/qcommon/q_math.cpp"
	"${MPDir}/qcommon/q_shared.cpp"
	"${MPDir}/qcommon/disablewarnings.h"
	"${MPDir}/qcommon/game_version.h"
	"${MPDir}/qcommon/q_shared.h"
	"${MPDir}/qcommon/q_platform.h"
	"${MPDir}/qcommon/qfiles.h"
	"${MPDir}/qcommon/tags.h"
	)
source_group("common" FILES ${MPUICommonFiles})
set(MPUIFiles ${MPUIFiles} ${MPUICommonFiles})

set(MPUIUIFiles
	"${MPDir}/ui/ui_atoms.cpp"
	"${MPDir}/ui/ui_cvar.cpp"
	"${MPDir}/ui/ui_force.cpp"
	"${MPDir}/ui/ui_gameinfo.cpp"
	"${MPDir}/ui/ui_main.cpp"
	"${MPDir}/ui/ui_saber.cpp"
	"${MPDir}/ui/ui_shared.cpp"
	"${MPDir}/ui/ui_syscalls.cpp"
	"${MPDir}/ui/keycodes.h"
	"${MPDir}/ui/menudef.h"
	"${MPDir}/ui/ui_force.h"
	"${MPDir}/ui/ui_local.h"
	"${MPDir}/ui/ui_public.h"
	"${MPDir}/ui/ui_shared.h"
	"${MPDir}/ui/ui_xcvar.h"
	"${MPDir}/game/ckheap.h"
	)
source_group("ui" FILES ${MPUIUIFiles})
set(MPUIFiles ${MPUIFiles} ${MPUIUIFiles})

set(MPUIRendererFiles
	"${MPDir}/rd-common/tr_types.h"
	)
source_group("rd-common" FILES ${MPUIRendererFiles})
set(MPUIFiles ${MPUIFiles} ${MPUIRendererFiles})

add_library(${MPUI} SHARED ${MPUIFiles})

if(NOT WIN32)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPUI} PROPERTIES PREFIX "")
endif(NOT WIN32)

if(WIN32)
	install(TARGETS ${MPUI} RUNTIME DESTINATION "Warzone")
else(WIN32)
	if(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPUI} LIBRARY DESTINATION "${MPEngine}.app/Contents/MacOS/Warzone")
		install(TARGETS ${MPUI} LIBRARY DESTINATION "${MPEngine}.app/Contents/MacOS/base")
	else(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPUI} LIBRARY DESTINATION "Warzone")
	endif(MakeApplicationBundles AND BuildMPEngine)
endif(WIN32)
set_target_properties(${MPUI} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${MPUIDefines};${ReleaseDefines}")
set_target_properties(${MPUI} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${MPUIDefines};${ReleaseDefines}")
set_target_properties(${MPUI} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${MPUIDefines};${ReleaseDefines}")
set_target_properties(${MPUI} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${MPUIDefines};${DebugDefines}")
set_target_properties(${MPUI} PROPERTIES INCLUDE_DIRECTORIES "${MPUIIncludeDirectories}")
set_target_properties(${MPUI} PROPERTIES PROJECT_LABEL "MP UI Library")
# no libraries used
if(MPUILibraries)
	target_link_libraries(${MPUI} ${MPUILibraries})
endif(MPUILibraries)
