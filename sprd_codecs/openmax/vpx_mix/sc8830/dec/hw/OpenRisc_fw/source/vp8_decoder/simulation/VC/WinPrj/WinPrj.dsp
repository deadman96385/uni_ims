# Microsoft Developer Studio Project File - Name="WinPrj" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=WinPrj - Win32 ORSC
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WinPrj.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WinPrj.mak" CFG="WinPrj - Win32 ORSC"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WinPrj - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "WinPrj - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "WinPrj - Win32 ORSC" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WinPrj - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\code\inc\common" /I "..\..\..\code\inc\decoder" /I "..\..\..\..\vsp_drv" /I "..\..\..\..\c_model\vld" /I "..\..\..\..\c_model\bsm" /I "..\..\..\..\c_model\mbc" /I "..\..\..\..\c_model\dct" /I "..\..\..\..\c_model\ppa" /I "..\..\..\..\c_model\parser" /I "..\..\..\..\c_model\coeff_vld" /I "..\..\..\..\c_model\global" /I "..\..\..\..\c_model\ahbm" /I "..\..\..\..\c_model\mca" /I "..\..\..\..\c_model\dcam" /I "..\..\..\..\c_model\buffer" /I "..\..\..\..\c_model\common" /I "..\..\..\..\c_model\vlc" /I "..\..\..\..\c_model\dbk" /I "..\..\..\..\c_model\mea" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_SIMULATION_" /D "VP8_DEC" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinPrj___Win32_ORSC"
# PROP BASE Intermediate_Dir "WinPrj___Win32_ORSC"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ORSC"
# PROP Intermediate_Dir "ORSC"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\code\inc\common" /I "..\..\..\code\inc\decoder" /I "..\..\..\..\vsp_drv" /I "..\..\..\..\c_model\vld" /I "..\..\..\..\c_model\bsm" /I "..\..\..\..\c_model\mbc" /I "..\..\..\..\c_model\dct" /I "..\..\..\..\c_model\ppa" /I "..\..\..\..\c_model\parser" /I "..\..\..\..\c_model\coeff_vld" /I "..\..\..\..\c_model\global" /I "..\..\..\..\c_model\ahbm" /I "..\..\..\..\c_model\mca" /I "..\..\..\..\c_model\dcam" /I "..\..\..\..\c_model\buffer" /I "..\..\..\..\c_model\common" /I "..\..\..\..\c_model\vlc" /I "..\..\..\..\c_model\dbk" /I "..\..\..\..\c_model\mea" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_SIMULATION_" /D "VP8_DEC" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W4 /Gm /GX /ZI /Od /I "..\..\..\code\inc\common" /I "..\..\..\code\inc\decoder" /I "..\..\..\..\vsp_drv" /I "..\..\..\..\c_model\vld" /I "..\..\..\..\c_model\bsm" /I "..\..\..\..\c_model\mbc" /I "..\..\..\..\c_model\dct" /I "..\..\..\..\c_model\ppa" /I "..\..\..\..\c_model\parser" /I "..\..\..\..\c_model\coeff_vld" /I "..\..\..\..\c_model\global" /I "..\..\..\..\c_model\ahbm" /I "..\..\..\..\c_model\mca" /I "..\..\..\..\c_model\dcam" /I "..\..\..\..\c_model\buffer" /I "..\..\..\..\c_model\common" /I "..\..\..\..\c_model\vlc" /I "..\..\..\..\c_model\dbk" /I "..\..\..\..\c_model\mea" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "VP8_DEC" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "WinPrj - Win32 Release"
# Name "WinPrj - Win32 Debug"
# Name "WinPrj - Win32 ORSC"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\code\src\common\md5.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_alloc_common.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_blockd.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /WX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_coef_update_probs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_entropy.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_entropy_mode.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_entropy_mv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_extend.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_filter.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_find_nearmv.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_idct.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_init.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_mbpitch.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_mode_context.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_mode_count.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# SUBTRACT CPP /WX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_quant_common.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_recon.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_reconinter.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_reconintra.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_reconintra4x4.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_segmentation_common.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_setup_intra_recon.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_swap_yv12buffer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_test_vectors.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_treecoder.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_yv12config.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\common\vp8_yv12extend.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "decoder"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_dboolhuff.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_demode.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_dequant.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_detokenize.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_global.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_interface.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_malloc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_mb.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_mv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_reg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\decoder\vp8dec_treereader.c
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "common_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\code\inc\common\mmcodec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\sc8810_video_header.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\sci_types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\video_common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_alloc_common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_basic.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_blockd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_default_coef_probs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_entropy.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_entropy_mode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_entropy_mv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_extend.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_find_nearmv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_idct.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_init.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_loopfilter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_mode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_mode_count.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_mv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_postproc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_quant_common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_recon.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_reconinter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_reconintra.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_reconintra4x4.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_segmentation_common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_setup_intra_recon.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_subpix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_swap_yv12buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_test_vectors.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_treecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_yv12config.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vp8_yv12extend.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\vpx_codec.h
# End Source File
# End Group
# Begin Group "decoder_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_basic.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_dboolhuff.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_demode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_dequant.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_detokenize.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_global.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_malloc.H
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_mb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_mode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_mv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_reg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\decoder\vp8dec_treereader.h
# End Source File
# End Group
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "vsp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\os_api.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_ahbm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_bsm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_dbk.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_dcam.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_dct.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_drv.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_drv_sc8800g.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_frame_header.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_global.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_global_define.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_ip_syntax.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_mbc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_mca.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_mea.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_parser.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_ppa.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_vlc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_vld.h
# End Source File
# End Group
# Begin Group "cmodel"

# PROP Default_Filter ""
# Begin Group "mca"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\mca\mca_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mca\mca_core_vp8.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mca\mca_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mca\mca_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mca\mca_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "bsm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\bsm\bsm_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\bsm\bsm_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\bsm\bsm_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\bsm\bsm_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "vspcommon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\common\common_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\common\common_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\common\common_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "dct"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\dct_asic.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\dct_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\dct_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\dct_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\dct_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\iict_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\iict_global.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\iict_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\iict_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "global"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\global\global_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\global\global_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "ahbm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\ahbm\ahbm_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\ahbm\ahbm_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "vld"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_arith_lev_infor_dec.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_bi_arith_dec.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_blk.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_cabac_blk.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_global.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_ipcm.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_lev.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_mbctr.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_mode.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_reg.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_run.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_sig_map_decoder.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_test_vector.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_test_vector.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_trace.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_two_bin_arith_dec.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\vld_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\vld_jpeg_rld.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\vld_mpeg4_dcac_pred.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\vld_mpeg4_rld.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\vld_mpeg4_rvld.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\vld_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\vld_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "dcam"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\dcam\dcam_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dcam\dcam_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "mbc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\mbc\ipred_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mbc\ipred_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mbc\ipred_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mbc\ipred_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mbc\mbc_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mbc\mbc_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mbc\mbc_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mbc\mbc_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "buffer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\buffer\buffer_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\buffer\buffer_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "dbk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\h264dbk_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\h264dbk_trace.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\hdbk_core_ctr.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\hdbk_filter.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\hdbk_global.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\hdbk_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\hdbk_mode.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\hdbk_pix_arr.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\hdbk_reg.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\hdbk_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\vp8dbk_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\vp8dbk_test.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\vp8dbk_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\vp8dbk_trace.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "vlc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\vlc_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\vlc_dc_pred.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\vlc_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\vlc_jpeg_rlc.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\vlc_mpeg4_rlc.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\vlc_mpeg4_table.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\vlc_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\vlc_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "ppa"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\ppa\ppa_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\ppa\ppa_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "parser"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\parser\parser.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\parser\parser_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=..\..\..\code\src\main.c
# End Source File
# Begin Source File

SOURCE=..\seq\vp8_decoder.cfg
# End Source File
# Begin Source File

SOURCE=.\vp8dec_main.c
# End Source File
# End Target
# End Project
