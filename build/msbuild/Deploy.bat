SETLOCAL EnableDelayedExpansion
REM @ECHO OFF

REM TODO: Add clean function

REM Usage
REM Deploy.bat CopyShaderCompiler "$(PlatformTarget)" "$(WindowsSdkDir)" "$(OutDir)"
REM Deploy.bat ProjectPostBuild "$(PlatformTarget)" "$(OutDir)" "<PROJECT_DIR>" ["<SUB_DIR>" <ext directory names>]
REM Deploy.bat GenerateTLH      "$(PlatformTarget)" "$(OutDir)" "$(TargetName)" "<PROJECT_DIR>"

REM NOTE: Must use CRLF. LF results in "the system cannot find the batch label specified"

SET FUNCTION=%~1

ECHO.
GOTO :%FUNCTION%

:CopyShaderCompiler
	SET PLATFORM_ARCH=%~2
	SET WIN_SDK_DIR=%~3
	SET OUT_DIR=%~4
	SET RUN_DIR=%OUT_DIR%..\..\run
	SET D3D_COMPILER=%WIN_SDK_DIR%Redist\D3D\%PLATFORM_ARCH%\d3dcompiler_47.dll

	ECHO  **** Copying shader compiler
	XCOPY /Y /I /S /B "%D3D_COMPILER%" "%RUN_DIR%\" > NUL
	IF ERRORLEVEL 1 GOTO Abort
	GOTO Exit

:ProjectPostBuild
	SET PLATFORM_ARCH=%~2
	SET OUT_DIR=%~3
	SET PROJECT_DIR=%~4
	SET SUB_DIR=%~5
	SET RUN_DIR=%OUT_DIR%..\..\run

	ECHO  **** Copying output to run
	XCOPY /Y /I /S /B /EXCLUDE:exclude_from_copy.txt "%OUT_DIR%*" "%RUN_DIR%\%SUB_DIR%" > NUL
	IF ERRORLEVEL 1 GOTO Abort

	XCOPY /Y /I /S /B "%OUT_DIR%*.cso" "%RUN_DIR%\%SUB_DIR%\Shaders" > NUL 2>&1
	IF ERRORLEVEL 1 GOTO Abort

	IF EXIST "%PROJECT_DIR%\include" (
		ECHO  **** Copying public headers to run
		XCOPY /Y /I /S /B /EXCLUDE:exclude_from_copy.txt "%PROJECT_DIR%\include" "%RUN_DIR%\%SUB_DIR%\Include" > NUL
		IF ERRORLEVEL 1 GOTO Abort
	)

	ECHO  **** Copying external dependencies to run
	:Loop
		IF NOT "%~6"=="" (
			SET EXT_DIR=%~6
			ECHO "%PROJECT_DIR%\ext\!EXT_DIR!\lib_%PLATFORM_ARCH%"
			XCOPY /Y /I /S /B /EXCLUDE:exclude_from_copy.txt "%PROJECT_DIR%\ext\!EXT_DIR!\lib_%PLATFORM_ARCH%" "%RUN_DIR%\%SUB_DIR%" > NUL
			IF ERRORLEVEL 1 GOTO Abort
			SHIFT /6
			GOTO Loop
		)
	GOTO Exit

:GenerateTLH
	REM $(OutDir) ends with a \ which breaks tlbexp and cl. It will complain about an
	REM invalid character in the path. Appending a . is a hack to avoid the error.

	REM Warning TX00131175 is disabled because it appears to be spurious and I can't find a way to
	REM prevent it. It's complaining that all library references should be specified when cross-
	REM compiling but 1) I'm not cross-compiling and 2) I shouldn't have any dependencies.

	SET PLATFORM_ARCH=%~2
	SET OUT_DIR=%~3
	SET TARGET_NAME=%~4
	SET PROJECT_DIR=%~5
	SET TARGET_FILE_NO_EXT=%OUT_DIR%%TARGET_NAME%
	SET TLB_ARCH=win64

	ECHO  **** Generating TLB
	tlbexp ^
		"%TARGET_FILE_NO_EXT%.dll" ^
		/out:"%TARGET_FILE_NO_EXT%.tlb" ^
		/%TLB_ARCH% ^
		/silence:00131175 ^
		/nologo > NUL
	IF ERRORLEVEL 1 GOTO Abort

	ECHO  **** Generating TLH
	cl ^
		"%PROJECT_DIR%\src\pluginloader_clr_interface_tlh.cpp" ^
		/Fo"%TARGET_FILE_NO_EXT%.obj" ^
		/I"%OUT_DIR%." ^
		/c ^
		/nologo
	IF ERRORLEVEL 1 GOTO Abort
	GOTO Exit

:Abort
	ECHO  **** WARNING - Failed
	EXIT /B 1

:Exit
	ECHO.
	EXIT /B 0
