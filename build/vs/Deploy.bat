SETLOCAL
REM @ECHO OFF

REM TODO: Add clean function

REM Usage
REM Deploy.bat SolutionPreBuild "$(WindowsSdkDir)" "$(PlatformTarget)"
REM Deploy.bat ProjectPostBuild "Project folder" "$(OutDir)" "Sub-directory" "Shader directory"
REM Deploy.bat GenerateTLB      "$(OutDir)" "$(TargetName)" "$(PlatformTarget)"

REM NOTE: Must use CRLF. LF results in "the system cannot find the batch label specified"

SET FUNCTION=%~1
SET RUN_DIR=run

ECHO.
GOTO :%FUNCTION%

:SolutionPreBuild
	SET WIN_SDK_DIR=%~2
	SET WIN_SDK_ARCH=%~3
	SET D3D_COMPILER=%WIN_SDK_DIR%Redist\D3D\%WIN_SDK_ARCH%\d3dcompiler_47.dll

	ECHO  **** Copying shader compiler
	XCOPY /Y /I /S /B "%D3D_COMPILER%" "%RUN_DIR%\" > NUL
	IF ERRORLEVEL 1 GOTO Abort
	GOTO Exit

:ProjectPostBuild
	SET PROJECT_DIR=%~2
	SET OUT_DIR=%~3
	SET SUB_DIR=%~4
	SET SHADER_DIR=%~5

	ECHO  **** Copying output to run
	XCOPY /Y /I /S /B /EXCLUDE:exclude_from_copy.txt "%OUT_DIR%*" "%RUN_DIR%\%SUB_DIR%" > NUL
	IF ERRORLEVEL 1 GOTO Abort
	XCOPY /Y /I /S /B "%OUT_DIR%*.cso" "%RUN_DIR%\%SUB_DIR%\%SHADER_DIR%" > NUL 2>&1
	IF ERRORLEVEL 1 GOTO Abort

	IF EXIST "%PROJECT_DIR%/include" (
		ECHO  **** Copying public headers to run
		XCOPY /Y /I /S /B "%PROJECT_DIR%/include" "%RUN_DIR%/Include" > NUL
	)
	GOTO Exit

:GenerateTLB
	REM $(OutDir) ends with a \ which breaks tlbexp. It will complain about an
	REM invalid character in the path. Appending a . is a hack to avoid the error.

	REM It sucks that we have to hardcode the path to other project assemblies.
	REM This means we have to be careful about renames and that $(OutDir) matches
	REM for all involved projects.

	SET OUT_DIR=%~2
	SET TARGET_NAME=%~3
	SET WIN_SDK_ARCH=%~4
	SET TARGET_FILE_NO_EXT=%OUT_DIR%%TARGET_NAME%
	SET DEPENDENCY_DIR=..\LCDHardwareMonitor API CLR\%OUT_DIR%
	IF      "%WIN_SDK_ARCH%" == "x86" ( SET TLB_ARCH=win32 ) ^
	ELSE IF "%WIN_SDK_ARCH%" == "x64" ( SET TLB_ARCH=win64 )

	ECHO  **** Generating TLB
	tlbexp ^
		"%TARGET_FILE_NO_EXT%.dll" ^
		/out:"%TARGET_FILE_NO_EXT%.tlb" ^
		/asmpath:"%DEPENDENCY_DIR%." ^
		/%TLB_ARCH% ^
		/nologo > NUL
	IF ERRORLEVEL 1 GOTO Abort
	GOTO Exit

:Abort
	ECHO  **** WARNING - Failed
	EXIT /B 1

:Exit
	ECHO.
	EXIT /B 0
