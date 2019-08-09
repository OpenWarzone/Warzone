cmake_minimum_required(VERSION 2.8.8)

# Make sure the user is not executing this script directly
if(NOT InWarzone)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InWarzone)

set(MPWarzoneIncludeDirectories ${MPDir})
set(MPWarzoneIncludeDirectories ${MPWarzoneIncludeDirectories} "${MPDir}/rd-warzone")

set(MPWarzoneFiles
  "${MPDir}/client/fast_mutex.h"
	"${MPDir}/client/tinythread.cpp"
	"${MPDir}/client/tinythread.h"
	"${MPDir}/qcommon/inifile.h"
	"${MPDir}/qcommon/inifile.cpp"
	"${MPDir}/rd-warzone/iqm.h"
	"${MPDir}/rd-warzone/qgl.h"
	"${MPDir}/rd-warzone/G2_API.cpp"
	"${MPDir}/rd-warzone/G2_bolts.cpp"
	"${MPDir}/rd-warzone/G2_bones.cpp"
	"${MPDir}/rd-warzone/G2_misc.cpp"
	"${MPDir}/rd-warzone/G2_surfaces.cpp"
	"${MPDir}/rd-warzone/glsl_optimizer.h"
	"${MPDir}/rd-warzone/nuklear.h"
	"${MPDir}/rd-warzone/stb_image.h"
	"${MPDir}/rd-warzone/stb_truetype.h"
	"${MPDir}/rd-warzone/tr_animation.cpp"
	"${MPDir}/rd-warzone/tr_awesomium.cpp"
	"${MPDir}/rd-warzone/tr_backend.cpp"
	"${MPDir}/rd-warzone/tr_bsp.cpp"
	"${MPDir}/rd-warzone/tr_cache.cpp"
	"${MPDir}/rd-warzone/tr_clouds.cpp"
	"${MPDir}/rd-warzone/tr_cmds.cpp"
	"${MPDir}/rd-warzone/tr_cubemaps.cpp"
	"${MPDir}/rd-warzone/tr_curve.cpp"
	"${MPDir}/rd-warzone/tr_debug.cpp"
	"${MPDir}/rd-warzone/tr_debug.h"
	"${MPDir}/rd-warzone/tr_decals.cpp"
	"${MPDir}/rd-warzone/tr_extensions.cpp"
	"${MPDir}/rd-warzone/tr_extramath.cpp"
	"${MPDir}/rd-warzone/tr_extramath.h"
	"${MPDir}/rd-warzone/tr_extratypes.h"
	"${MPDir}/rd-warzone/tr_fbo.cpp"
	"${MPDir}/rd-warzone/tr_fbo.h"
	"${MPDir}/rd-warzone/tr_foliage.cpp"
	"${MPDir}/rd-warzone/tr_flares.cpp"
	"${MPDir}/rd-warzone/tr_ghoul2.cpp"
	"${MPDir}/rd-warzone/tr_glsl.cpp"
	"${MPDir}/rd-warzone/tr_image.cpp"
	"${MPDir}/rd-warzone/tr_image_dds.cpp"
	"${MPDir}/rd-warzone/tr_init.cpp"
	"${MPDir}/rd-warzone/tr_instancing.cpp"
	"${MPDir}/rd-warzone/tr_instancing.h"
	"${MPDir}/rd-warzone/tr_lasermanager.cpp"
	"${MPDir}/rd-warzone/tr_lasermanager.h"
	"${MPDir}/rd-warzone/tr_light.cpp"
	"${MPDir}/rd-warzone/tr_local.h"
	"${MPDir}/rd-warzone/tr_main.cpp"
	"${MPDir}/rd-warzone/tr_marks.cpp"
	"${MPDir}/rd-warzone/tr_mapping.cpp"
	"${MPDir}/rd-warzone/tr_mapping.h"
	"${MPDir}/rd-warzone/tr_matrix.cpp"
	"${MPDir}/rd-warzone/tr_matrix.h"
	"${MPDir}/rd-warzone/tr_mesh.cpp"
	"${MPDir}/rd-warzone/tr_model.cpp"
	"${MPDir}/rd-warzone/tr_model_iqm.cpp"
	"${MPDir}/rd-warzone/tr_nuklear_gui.cpp"
	"${MPDir}/rd-warzone/tr_occlusion.cpp"
	"${MPDir}/rd-warzone/tr_ocean.cpp"
	"${MPDir}/rd-warzone/tr_ocean.h"
	"${MPDir}/rd-warzone/tr_optimization.cpp"
	"${MPDir}/rd-warzone/tr_postprocess.cpp"
	"${MPDir}/rd-warzone/tr_postprocess.h"
	"${MPDir}/rd-warzone/tr_quicksprite.cpp"
	"${MPDir}/rd-warzone/tr_quicksprite.h"
	"${MPDir}/rd-warzone/tr_scene.cpp"
	"${MPDir}/rd-warzone/tr_shade.cpp"
	"${MPDir}/rd-warzone/tr_shade_calc.cpp"
	"${MPDir}/rd-warzone/tr_shader.cpp"
	"${MPDir}/rd-warzone/tr_shadows.cpp"
	"${MPDir}/rd-warzone/tr_skin.cpp"
	"${MPDir}/rd-warzone/tr_sky.cpp"
	"${MPDir}/rd-warzone/tr_subs.cpp"
	"${MPDir}/rd-warzone/tr_surface.cpp"
	"${MPDir}/rd-warzone/tr_surfacesprites.cpp"
	"${MPDir}/rd-warzone/tr_vao.cpp"
	"${MPDir}/rd-warzone/tr_vbo.cpp"
	"${MPDir}/rd-warzone/tr_world.cpp"
	"${MPDir}/rd-warzone/tr_WorldEffects.cpp"
	"${MPDir}/rd-warzone/tr_WorldEffects.h"
	"${MPDir}/rd-warzone/VectorUtils3.cpp"
	"${MPDir}/rd-warzone/VectorUtils3.h"
	"${MPDir}/rd-warzone/compose_models.cpp"
	"${MPDir}/rd-warzone/compose_models.h"
	"${MPDir}/game/ckheap.h"
	)
source_group("renderer" FILES ${MPWarzoneFiles})

file(GLOB MPWarzoneGLSLFiles "${MPDir}/rd-warzone/glsl/*.glsl")
source_group("renderer\\glsl" FILES ${MPWarzoneGLSLFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneGLSLFiles})

file(GLOB MPWarzoneGLSLFiles2 "${MPDir}/rd-warzone/glsl2/*.glsl")
source_group("renderer\\glsl2" FILES ${MPWarzoneGLSLFiles2})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneGLSLFiles2})

set(MPWarzoneGhoul2Files
	"${MPDir}/ghoul2/g2_local.h"
	"${MPDir}/ghoul2/ghoul2_shared.h"
	"${MPDir}/ghoul2/G2_gore.cpp"
	"${MPDir}/ghoul2/G2_gore.h")
source_group("ghoul2" FILES ${MPWarzoneGhoul2Files})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneGhoul2Files})

#set(MPWarzoneAwesomiumFiles
#	"${MPDir}/rd-warzone/Awesomium/BitmapSurface.h"
#	"${MPDir}/rd-warzone/Awesomium/ChildProcess.h"
#	"${MPDir}/rd-warzone/Awesomium/DataPak.h"
#	"${MPDir}/rd-warzone/Awesomium/DataSource.h"
#	"${MPDir}/rd-warzone/Awesomium/JSArray.h"
#	"${MPDir}/rd-warzone/Awesomium/JSObject.h"
#	"${MPDir}/rd-warzone/Awesomium/JSValue.h"
#	"${MPDir}/rd-warzone/Awesomium/Platform.h"
#	"${MPDir}/rd-warzone/Awesomium/PrintConfig.h"
#	"${MPDir}/rd-warzone/Awesomium/ResourceInterceptor.h"
#	"${MPDir}/rd-warzone/Awesomium/STLHelpers.h"
#	"${MPDir}/rd-warzone/Awesomium/Surface.h"
#	"${MPDir}/rd-warzone/Awesomium/WebConfig.h"
#	"${MPDir}/rd-warzone/Awesomium/WebCore.h"
#	"${MPDir}/rd-warzone/Awesomium/WebKeyboardCodes.h"
#	"${MPDir}/rd-warzone/Awesomium/WebKeyboardEvent.h"
#	"${MPDir}/rd-warzone/Awesomium/WebMenuItem.h"
#	"${MPDir}/rd-warzone/Awesomium/WebPreferences.h"
#	"${MPDir}/rd-warzone/Awesomium/WebSession.h"
#	"${MPDir}/rd-warzone/Awesomium/WebString.h"
#	"${MPDir}/rd-warzone/Awesomium/WebStringArray.h"
#	"${MPDir}/rd-warzone/Awesomium/WebTouchEvent.h"
#	"${MPDir}/rd-warzone/Awesomium/WebURL.h"
#	"${MPDir}/rd-warzone/Awesomium/WebView.h"
#	"${MPDir}/rd-warzone/Awesomium/WebViewListener.h")
#source_group("Awesomium Headers" FILES ${MPWarzoneAwesomiumFiles})
#set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneAwesomiumFiles})

set(MPWarzoneMeshOptimizationFiles
  "${MPDir}/rd-warzone/meshOptimization/meshoptimizer.h"
	"${MPDir}/rd-warzone/meshOptimization/triListOpt.cpp"
	"${MPDir}/rd-warzone/meshOptimization/triListOpt.h")
source_group("MeshOptimization" FILES ${MPWarzoneMeshOptimizationFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneMeshOptimizationFiles})

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
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneTinyImageLoaderFiles})

set(MPWarzoneAssimpFiles
  "${MPDir}/rd-warzone/assimp/ai_assert.h"
	"${MPDir}/rd-warzone/assimp/anim.h"
	"${MPDir}/rd-warzone/assimp/BaseImporter.h"
	"${MPDir}/rd-warzone/assimp/Bitmap.h"
	"${MPDir}/rd-warzone/assimp/BlobIOSystem.h"
	"${MPDir}/rd-warzone/assimp/ByteSwapper.h"
	"${MPDir}/rd-warzone/assimp/camera.h"
	"${MPDir}/rd-warzone/assimp/cexport.h"
	"${MPDir}/rd-warzone/assimp/cfileio.h"
	"${MPDir}/rd-warzone/assimp/cimport.h"
	"${MPDir}/rd-warzone/assimp/color4.h"
	"${MPDir}/rd-warzone/assimp/color4.inl"
	"${MPDir}/rd-warzone/assimp/Compiler"
	"${MPDir}/rd-warzone/assimp/config.h"
	"${MPDir}/rd-warzone/assimp/CreateAnimMesh.h"
	"${MPDir}/rd-warzone/assimp/DefaultIOStream.h"
	"${MPDir}/rd-warzone/assimp/DefaultIOSystem.h"
	"${MPDir}/rd-warzone/assimp/DefaultLogger.hpp"
	"${MPDir}/rd-warzone/assimp/Defines.h"
	"${MPDir}/rd-warzone/assimp/defs.h"
	"${MPDir}/rd-warzone/assimp/Exceptional.h"
	"${MPDir}/rd-warzone/assimp/Exporter.hpp"
	"${MPDir}/rd-warzone/assimp/fast_atof.h"
	"${MPDir}/rd-warzone/assimp/GenericProperty.h"
	"${MPDir}/rd-warzone/assimp/Hash.h"
	"${MPDir}/rd-warzone/assimp/Importer.hpp"
	"${MPDir}/rd-warzone/assimp/importerdesc.h"
	"${MPDir}/rd-warzone/assimp/IOStream.hpp"
	"${MPDir}/rd-warzone/assimp/IOStreamBuffer.h"
	"${MPDir}/rd-warzone/assimp/IOSystem.hpp"
	"${MPDir}/rd-warzone/assimp/irrXMLWrapper.h"
	"${MPDir}/rd-warzone/assimp/light.h"
	"${MPDir}/rd-warzone/assimp/LineSplitter.h"
	"${MPDir}/rd-warzone/assimp/LogAux.h"
	"${MPDir}/rd-warzone/assimp/Logger.hpp"
	"${MPDir}/rd-warzone/assimp/LogStream.hpp"
	"${MPDir}/rd-warzone/assimp/Macros.h"
	"${MPDir}/rd-warzone/assimp/material.h"
	"${MPDir}/rd-warzone/assimp/material.inl"
	"${MPDir}/rd-warzone/assimp/MathFunctions.h"
	"${MPDir}/rd-warzone/assimp/matrix3x3.h"
	"${MPDir}/rd-warzone/assimp/matrix3x3.inl"
	"${MPDir}/rd-warzone/assimp/matrix4x4.h"
	"${MPDir}/rd-warzone/assimp/matrix4x4.inl"
	"${MPDir}/rd-warzone/assimp/MemoryIOWrapper.h"
	"${MPDir}/rd-warzone/assimp/mesh.h"
	"${MPDir}/rd-warzone/assimp/metadata.h"
	"${MPDir}/rd-warzone/assimp/NullLogger.hpp"
	"${MPDir}/rd-warzone/assimp/ParsingUtils.h"
	"${MPDir}/rd-warzone/assimp/pbrmaterial.h"
	"${MPDir}/rd-warzone/assimp/port"
	"${MPDir}/rd-warzone/assimp/postprocess.h"
	"${MPDir}/rd-warzone/assimp/Profiler.h"
	"${MPDir}/rd-warzone/assimp/ProgressHandler.hpp"
	"${MPDir}/rd-warzone/assimp/qnan.h"
	"${MPDir}/rd-warzone/assimp/quaternion.h"
	"${MPDir}/rd-warzone/assimp/quaternion.inl"
	"${MPDir}/rd-warzone/assimp/RemoveComments.h"
	"${MPDir}/rd-warzone/assimp/scene.h"
	"${MPDir}/rd-warzone/assimp/SceneCombiner.h"
	"${MPDir}/rd-warzone/assimp/SGSpatialSort.h"
	"${MPDir}/rd-warzone/assimp/SkeletonMeshBuilder.h"
	"${MPDir}/rd-warzone/assimp/SmoothingGroups.h"
	"${MPDir}/rd-warzone/assimp/SmoothingGroups.inl"
	"${MPDir}/rd-warzone/assimp/SpatialSort.h"
	"${MPDir}/rd-warzone/assimp/StandardShapes.h"
	"${MPDir}/rd-warzone/assimp/StreamReader.h"
	"${MPDir}/rd-warzone/assimp/StreamWriter.h"
	"${MPDir}/rd-warzone/assimp/StringComparison.h"
	"${MPDir}/rd-warzone/assimp/StringUtils.h"
	"${MPDir}/rd-warzone/assimp/Subdivision.h"
	"${MPDir}/rd-warzone/assimp/texture.h"
	"${MPDir}/rd-warzone/assimp/TinyFormatter.h"
	"${MPDir}/rd-warzone/assimp/types.h"
	"${MPDir}/rd-warzone/assimp/vector2.h"
	"${MPDir}/rd-warzone/assimp/vector2.inl"
	"${MPDir}/rd-warzone/assimp/vector3.h"
	"${MPDir}/rd-warzone/assimp/vector3.inl"
	"${MPDir}/rd-warzone/assimp/version.h"
	"${MPDir}/rd-warzone/assimp/Vertex.h"
	"${MPDir}/rd-warzone/assimp/XMLTools.h")
source_group("Assimp" FILES ${MPWarzoneAssimpFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneAssimpFiles})

set(MPWarzoneImGuiFiles
	"${MPDir}/rd-warzone/imgui/ccall/ccall.h"
	"${MPDir}/rd-warzone/imgui/MagicFile.cpp/magicfile.cpp"
	"${MPDir}/rd-warzone/imgui/MagicFile.cpp/magicfile.h"
	"${MPDir}/rd-warzone/imgui/ffi.cpp"
	"${MPDir}/rd-warzone/imgui/imconfig.h"
	"${MPDir}/rd-warzone/imgui/imgui.cpp"
	"${MPDir}/rd-warzone/imgui/imgui.h"
	"${MPDir}/rd-warzone/imgui/imgui_api.cpp"
	"${MPDir}/rd-warzone/imgui/imgui_api.h"
	"${MPDir}/rd-warzone/imgui/imgui_c_utils.cpp"
	"${MPDir}/rd-warzone/imgui/imgui_c_utils.h"
	"${MPDir}/rd-warzone/imgui/imgui_default_docks.cpp"
	"${MPDir}/rd-warzone/imgui/imgui_default_docks.h"
	"${MPDir}/rd-warzone/imgui/imgui_demo.cpp"
	"${MPDir}/rd-warzone/imgui/imgui_dock.cpp"
	"${MPDir}/rd-warzone/imgui/imgui_dock.h"
	"${MPDir}/rd-warzone/imgui/imgui_draw.cpp"
	"${MPDir}/rd-warzone/imgui/imgui_fixedarea.cpp"
	"${MPDir}/rd-warzone/imgui/imgui_fixedarea.h"
	"${MPDir}/rd-warzone/imgui/imgui_impl_idtech3_gles.cpp"
	"${MPDir}/rd-warzone/imgui/imgui_impl_idtech3_gles.h"
	"${MPDir}/rd-warzone/imgui/imgui_internal.h"
	"${MPDir}/rd-warzone/imgui/imgui_utils.cpp"
	"${MPDir}/rd-warzone/imgui/imgui_utils.h"
	"${MPDir}/rd-warzone/imgui/include_imgui.h"
	"${MPDir}/rd-warzone/imgui/include_quakegl.h"
	"${MPDir}/rd-warzone/imgui/stb_rect_pack.h"
	"${MPDir}/rd-warzone/imgui/stb_textedit.h"
	"${MPDir}/rd-warzone/imgui/stb_truetype.h")
source_group("imgui" FILES ${MPWarzoneImGuiFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneImGuiFiles})

set(MPWarzoneImGuiOpenJKFiles
	"${MPDir}/rd-warzone/imgui_openjk/imgui_openjk_default_docks.h"
	"${MPDir}/rd-warzone/imgui_openjk/imgui_openjk_default_docks.cpp"
	"${MPDir}/rd-warzone/imgui_openjk/gluecode.h"
	"${MPDir}/rd-warzone/imgui_openjk/gluecode.cpp"
)
source_group("imgui_openjk" FILES ${MPWarzoneImGuiOpenJKFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneImGuiOpenJKFiles})

set(MPWarzoneImGuiDockFiles
	"${MPDir}/rd-warzone/imgui_docks/dock.cpp"
	"${MPDir}/rd-warzone/imgui_docks/dock.h"
	"${MPDir}/rd-warzone/imgui_docks/dock_console.cpp"
	"${MPDir}/rd-warzone/imgui_docks/dock_console.h"
	"${MPDir}/rd-warzone/imgui_docks/dock_julia.cpp"
	"${MPDir}/rd-warzone/imgui_docks/dock_julia.h"
	"${MPDir}/rd-warzone/imgui_docks/dock_node.cpp"
	"${MPDir}/rd-warzone/imgui_docks/dock_node.h"
	"${MPDir}/rd-warzone/imgui_docks/dock_repl.cpp"
	"${MPDir}/rd-warzone/imgui_docks/dock_repl.h"
	"${MPDir}/rd-warzone/imgui_docks/dock_skeleton.cpp"
	"${MPDir}/rd-warzone/imgui_docks/dock_skeleton.h")
source_group("imgui_docks" FILES ${MPWarzoneImGuiDockFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneImGuiDockFiles})

set(MPWarzoneImGuiDockOpenJKFiles
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_shadereditor.cpp"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_shadereditor.h"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_mapinfo.cpp"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_mapinfo.h"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_postprocess.cpp"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_postprocess.h"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_perf.cpp"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_perf.h"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_controlflow.cpp"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_controlflow.h"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_models.cpp"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_models.h"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_all.cpp"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_all.h"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_mdxm.cpp"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_mdxm.h"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_mdxa.cpp"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_mdxa.h"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_characterEditor.cpp"
	"${MPDir}/rd-warzone/imgui_docks_openjk/dock_characterEditor.h"
)
source_group("imgui_docks_openjk" FILES ${MPWarzoneImGuiDockOpenJKFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneImGuiDockOpenJKFiles})

set(MPWarzoneRdCommonFiles
	"${MPDir}/rd-common/mdx_format.h"
	"${MPDir}/rd-common/tr_common.h"
	"${MPDir}/rd-common/tr_font.cpp"
	"${MPDir}/rd-common/tr_font.h"
	"${MPDir}/rd-common/tr_image_load.cpp"
	"${MPDir}/rd-common/tr_image_jpg.cpp"
	"${MPDir}/rd-common/tr_image_tga.cpp"
	"${MPDir}/rd-common/tr_image_png.cpp"
	"${MPDir}/rd-common/tr_noise.cpp"
	"${MPDir}/rd-common/tr_public.h")
source_group("rd-common" FILES ${MPWarzoneRdCommonFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneRdCommonFiles})

set(MPWarzoneQcommonFiles
  "${MPDir}/qcommon/inifile.cpp"
  "${MPDir}/qcommon/inifile.h"
	"${MPDir}/qcommon/GenericParser2.cpp"
	"${MPDir}/qcommon/matcomp.cpp"
	"${MPDir}/qcommon/q_math.cpp"
	"${MPDir}/qcommon/q_shared.cpp")
source_group("qcommon" FILES ${MPWarzoneQcommonFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneQcommonFiles})

set(MPWarzoneSharedMemoryFiles
		"${MPDir}/SharedMemory/sharedmemory.h"
		"${MPDir}/SharedMemory/sharedmemory.cpp")
source_group("SharedMemory" FILES ${MPWarzoneSharedMemoryFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneSharedMemoryFiles})

file(GLOB_RECURSE MPWarzoneJpegFiles "${WarzoneLibDir}/jpeg-8c/*.c" "${WarzoneLibDir}/jpeg-8c/*.h")
source_group("jpeg-8c" FILES ${MPWarzoneJpegFiles})
set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneJpegFiles})

if(UseInternalPNG)
	set(MPWarzoneLibPngFiles
		"${WarzoneLibDir}/libpng/png.c"
		"${WarzoneLibDir}/libpng/pngerror.c"
		"${WarzoneLibDir}/libpng/pngget.c"
		"${WarzoneLibDir}/libpng/pngmem.c"
		"${WarzoneLibDir}/libpng/pngpread.c"
		"${WarzoneLibDir}/libpng/pngread.c"
		"${WarzoneLibDir}/libpng/pngrio.c"
		"${WarzoneLibDir}/libpng/pngrtran.c"
		"${WarzoneLibDir}/libpng/pngrutil.c"
		"${WarzoneLibDir}/libpng/pngset.c"
		"${WarzoneLibDir}/libpng/pngtrans.c"
		"${WarzoneLibDir}/libpng/pngwio.c"
		"${WarzoneLibDir}/libpng/pngwrite.c"
		"${WarzoneLibDir}/libpng/pngwtran.c"
		"${WarzoneLibDir}/libpng/pngwutil.c")
	source_group("libpng" FILES ${MPWarzoneLibPngFiles})
	set(MPWarzoneIncludeDirectories ${MPWarzoneIncludeDirectories} "${WarzoneLibDir}/libpng")
	set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneLibPngFiles})
else(UseInternalPNG)
	find_package(PNG REQUIRED)
	set(MPWarzoneIncludeDirectories ${MPWarzoneIncludeDirectories} ${PNG_INCLUDE_DIR})
	set(MPWarzoneLibraries ${MPWarzoneLibraries} ${PNG_LIBRARIES})
endif(UseInternalPNG)

if(UseInternalZlib)
	set(MPWarzoneZLibFiles
		"${WarzoneLibDir}/zlib/adler32.c"
		"${WarzoneLibDir}/zlib/compress.c"
		"${WarzoneLibDir}/zlib/crc32.c"
		"${WarzoneLibDir}/zlib/deflate.c"
		"${WarzoneLibDir}/zlib/gzclose.c"
		"${WarzoneLibDir}/zlib/gzlib.c"
		"${WarzoneLibDir}/zlib/gzread.c"
		"${WarzoneLibDir}/zlib/gzwrite.c"
		"${WarzoneLibDir}/zlib/infback.c"
		"${WarzoneLibDir}/zlib/inffast.c"
		"${WarzoneLibDir}/zlib/inflate.c"
		"${WarzoneLibDir}/zlib/inftrees.c"
		"${WarzoneLibDir}/zlib/trees.c"
		"${WarzoneLibDir}/zlib/uncompr.c"
		"${WarzoneLibDir}/zlib/zutil.c"
		"${WarzoneLibDir}/zlib/crc32.h"
		"${WarzoneLibDir}/zlib/deflate.h"
		"${WarzoneLibDir}/zlib/gzguts.h"
		"${WarzoneLibDir}/zlib/inffast.h"
		"${WarzoneLibDir}/zlib/inffixed.h"
		"${WarzoneLibDir}/zlib/inflate.h"
		"${WarzoneLibDir}/zlib/inftrees.h"
		"${WarzoneLibDir}/zlib/trees.h"
		"${WarzoneLibDir}/zlib/zconf.h"
		"${WarzoneLibDir}/zlib/zlib.h"
		"${WarzoneLibDir}/zlib/zutil.h")
	source_group("zlib" FILES ${MPWarzoneZLibFiles})
	set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneZLibFiles})

	set(MPWarzoneIncludeDirectories ${MPWarzoneIncludeDirectories} "${WarzoneLibDir}/zlib")
else(UseInternalZlib)
	find_package(ZLIB REQUIRED)
	set(MPWarzoneIncludeDirectories ${MPWarzoneIncludeDirectories} ${ZLIB_INCLUDE_DIR})
	set(MPWarzoneLibraries ${MPWarzoneLibraries} ${ZLIB_LIBRARY})
endif(UseInternalZlib)

if(NOT WIN32)
	find_package(OpenGL REQUIRED)
	set(MPWarzoneIncludeDirectories ${MPWarzoneIncludeDirectories} ${OPENGL_INCLUDE_DIR})
	set(MPWarzoneLibraries ${MPWarzoneLibraries} ${OPENGL_LIBRARIES})

	find_package(SDL2 REQUIRED)
	set(MPWarzoneIncludeDirectories ${MPWarzoneIncludeDirectories} ${SDL2_INCLUDE_DIR})
	set(MPWarzoneLibraries ${MPWarzoneLibraries} ${SDL2_LIBRARY})
endif(NOT WIN32)


if(WIN32)
	set(MPWarzoneWin32Files
		"${MPDir}/win32/win_gamma.cpp"
		"${MPDir}/win32/win_glimp.cpp"
		"${MPDir}/win32/win_qgl.cpp")
	source_group("win32" FILES ${MPWarzoneWin32Files})
	set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneWin32Files})
else(WIN32)
	set(MPWarzoneSysFiles
		"${MPDir}/sdl/sdl_glimp.cpp")
	source_group("sys" FILES ${MPWarzoneSysFiles})
	set(MPWarzoneFiles ${MPWarzoneFiles} ${MPWarzoneSysFiles})
endif(WIN32)

source_group("renderer" FILES ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp)
set(MPWarzoneFiles ${MPWarzoneFiles} ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp)

source_group("renderer" FILES ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders2.cpp)
set(MPWarzoneFiles ${MPWarzoneFiles} ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders2.cpp)

set(MPWarzoneIncludeDirectories ${MPWarzoneIncludeDirectories} ${WarzoneLibDir})
add_library(${MPWarzone} SHARED ${MPWarzoneFiles})

if(NOT WIN32)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${MPWarzone} PROPERTIES PREFIX "")
endif(NOT WIN32)

if(WIN32)
	install(TARGETS ${MPWarzone} RUNTIME DESTINATION ".")
else(WIN32)
	if(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPWarzone} LIBRARY DESTINATION "${MPEngine}.app/Contents/MacOS/")
	else(MakeApplicationBundles AND BuildMPEngine)
		install(TARGETS ${MPWarzone} LIBRARY DESTINATION ".")
	endif(MakeApplicationBundles AND BuildMPEngine)
endif(WIN32)
set_target_properties(${MPWarzone} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPWarzone} PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPWarzone} PROPERTIES COMPILE_DEFINITIONS_RELEASE "${SharedDefines};${ReleaseDefines}")
set_target_properties(${MPWarzone} PROPERTIES COMPILE_DEFINITIONS_DEBUG "${SharedDefines};${DebugDefines}")
set_target_properties(${MPWarzone} PROPERTIES INCLUDE_DIRECTORIES "${MPWarzoneIncludeDirectories}")
set_target_properties(${MPWarzone} PROPERTIES PROJECT_LABEL "MP Warzone Renderer")
#target_link_libraries(${MPWarzone} ${MPWarzoneLibraries} "../../../lib/awesomium;../../../lib/glsl_optimizer_lib;../../../lib/assimp-vc140-mt;../../../lib/meshoptimizer")
target_link_libraries(${MPWarzone} ${MPWarzoneLibraries} "../../../lib/glsl_optimizer_lib;../../../lib/assimp-vc140-mt;../../../lib/meshoptimizer")

# GLSL shader file generator
add_executable(compact_glsl ${MPDir}/rd-warzone/glsl/compact.cpp)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders2.cpp
	COMMAND compact_glsl ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders.cpp ${MPWarzoneGLSLFiles}
	COMMAND compact_glsl ${CMAKE_CURRENT_BINARY_DIR}/glsl_shaders2.cpp ${MPWarzoneGLSLFiles2}
	DEPENDS compact_glsl ${MPWarzoneGLSLFiles} ${MPWarzoneGLSLFiles2})
