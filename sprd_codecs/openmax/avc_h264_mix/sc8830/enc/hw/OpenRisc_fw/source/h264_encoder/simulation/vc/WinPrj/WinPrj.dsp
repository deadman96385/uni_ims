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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\code\inc\common" /I "..\..\..\code\inc\encoder" /I "..\..\..\code\vsp" /I "..\..\..\..\c_model\vld" /I "..\..\..\..\c_model\bsm" /I "..\..\..\..\c_model\mbc" /I "..\..\..\..\c_model\dct" /I "..\..\..\..\c_model\global" /I "..\..\..\..\c_model\ahbm" /I "..\..\..\..\c_model\mca" /I "..\..\..\..\c_model\dcam" /I "..\..\..\..\c_model\buffer" /I "..\..\..\..\c_model\common" /I "..\..\..\..\c_model\vlc" /I "..\..\..\..\c_model\dbk" /I "..\..\..\..\c_model\mea" /I "..\..\..\..\vsp_drv" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_SIMULATION_" /D "H264_ENC" /FR /YX /FD /GZ /c
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
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ORSC"
# PROP Intermediate_Dir "ORSC"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\code\inc\common" /I "..\..\..\code\inc\encoder" /I "..\..\..\code\vsp" /I "..\..\..\..\c_model\vld" /I "..\..\..\..\c_model\bsm" /I "..\..\..\..\c_model\mbc" /I "..\..\..\..\c_model\dct" /I "..\..\..\..\c_model\global" /I "..\..\..\..\c_model\ahbm" /I "..\..\..\..\c_model\mca" /I "..\..\..\..\c_model\dcam" /I "..\..\..\..\c_model\buffer" /I "..\..\..\..\c_model\common" /I "..\..\..\..\c_model\vlc" /I "..\..\..\..\c_model\dbk" /I "..\..\..\..\c_model\mea" /I "..\..\..\..\vsp_drv" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_SIMULATION_" /D "H264_ENC" /FR /YX /FD /GZ /c
# ADD CPP /nologo /W4 /Gm /GX /ZI /Od /I "..\..\..\code\inc\common" /I "..\..\..\code\inc\encoder" /I "..\..\..\code\vsp" /I "..\..\..\..\c_model\vld" /I "..\..\..\..\c_model\bsm" /I "..\..\..\..\c_model\mbc" /I "..\..\..\..\c_model\dct" /I "..\..\..\..\c_model\global" /I "..\..\..\..\c_model\ahbm" /I "..\..\..\..\c_model\mca" /I "..\..\..\..\c_model\dcam" /I "..\..\..\..\c_model\buffer" /I "..\..\..\..\c_model\common" /I "..\..\..\..\c_model\vlc" /I "..\..\..\..\c_model\dbk" /I "..\..\..\..\c_model\mea" /I "..\..\..\..\vsp_drv" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "H264_ENC" /FR /YX /FD /GZ /c
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
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_bfrctrl.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_bitstrm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_global.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_interface.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_malloc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_mb.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_mv.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_rc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_reg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_set.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_slice.c
# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_table.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\code\src\encoder\h264enc_vlc.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_bfrctrl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_bitstrm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_frame.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_global.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_malloc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_mb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_mode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_mv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_rc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_reg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_set.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_slice.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\encoder\h264enc_vlc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\mmcodec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\sc6800x_video_header.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\sci_types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\code\inc\common\video_common.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "vsp"

# PROP Default_Filter ""
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

SOURCE=..\..\..\..\vsp_drv\vsp_vlc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\vsp_drv\vsp_vld.h
# End Source File
# End Group
# Begin Group "c_model"

# PROP Default_Filter ""
# Begin Group "vld"

# PROP Default_Filter ""
# Begin Group "hvld"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\vld\hvld_blk.c

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
# End Group
# Begin Group "mj_vld"

# PROP Default_Filter ""
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
# Begin Group "dct"

# PROP Default_Filter ""
# Begin Group "mj_dct"

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
# End Group
# Begin Group "iict"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\iict_core.c

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
# Begin Group "hdct"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\hdct_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\hdct_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\hdct_mode.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dct\hdct_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "mbc"

# PROP Default_Filter ""
# Begin Group "ipred"

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
# End Group
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

SOURCE=..\..\..\..\c_model\mca\mca_core_h264.c

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
# Begin Group "common"

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
# Begin Group "vlc"

# PROP Default_Filter ""
# Begin Group "hvlc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\hvlc_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\hvlc_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\hvlc_mode.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\hvlc_table.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\hvlc_top.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\vlc\hvlc_tv.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
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

SOURCE=..\..\..\..\c_model\dbk\hdbk_core_ctrl_jpeg.c

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

SOURCE=..\..\..\..\c_model\dbk\hdbk_test_vector.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\dbk\hdbk_test_vector.h

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
# End Group
# Begin Group "mea"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\analyse_trace.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_block.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_ctr.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_global.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_global.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_interface.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_ip_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_mca_core.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_mode.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_mv_cost.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_PE.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_PE_array.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\hmea_prefilter.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\intra_pred.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\tv_mea.c

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\c_model\mea\tv_mea.h

!IF  "$(CFG)" == "WinPrj - Win32 Release"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinPrj - Win32 ORSC"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=..\seq\h264_encoder.cfg
# End Source File
# Begin Source File

SOURCE=.\h264enc_main.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\main.c
# End Source File
# End Target
# End Project
