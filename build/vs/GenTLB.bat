REM $(OutDir) ends with a \ which breaks tlbexp. It will complain about an
REM invalid character in the path. Appending a . is a hack to avoid the error.

REM It sucks that we have to hardcode the path to other project assemblies.
REM This means we have to be careful about renames and that the $(OutDir)s
REM for all involved projects must match.

REM Variables:
SET OUT_DIR=%~1
SET TARGET_NAME=%~2

SET TARGET_FILE_NO_EXT=%OUT_DIR%%TARGET_NAME%
SET DEPENDENCY_DIR=..\LCDHardwareMonitor API CLR\%OUT_DIR%

ECHO  **** Generating TLB

REM TODO: use /win32 and /win64?
tlbexp /nologo ^
	"%TARGET_FILE_NO_EXT%.dll" ^
	/out:"%TARGET_FILE_NO_EXT%.tlb" ^
	/asmpath:"%DEPENDENCY_DIR%." > NUL
IF ERRORLEVEL 1 GOTO Abort

ECHO  **** Generation successful
goto Exit

:Abort
	ECHO  **** WARNING - Generation failed
	EXIT /B 1

:Exit
	EXIT /B 0
