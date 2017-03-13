REM ECHO **** Command line: "Post Build.bat" "$(SolutionDir)" "$(TargetDir)" "$(OutDir)"

REM Parameters:
REM 	%1 - $(SolutionDir)
REM 	%2 - $(TargetDir)
REM 	%3 - $(OutDir)
ECHO.

REM Make sure parameters were entered.
SET params=false
IF NOT "%~1" == "" IF NOT "%~2" == "" IF NOT "%~3" == "" SET params=true
IF %params% == false (
	ECHO **** Missing parameters. Parameters should be: $^(SolutionDir^) $^(TargetDir^) $^(OutDir^)
	GOTO Abort
)

REM Variables:
SET SOLUTION_DIR=%~1
SET TARGET_DIR=%~2
SET OUT_DIR=%~3

REM SET COPY_DIR=%SOLUTION_DIR%LCDHardwareMonitor.Core\%OUT_DIR%Renderers\D3D11\
SET COPY_DIR=%SOLUTION_DIR%LCDHardwareMonitor.Presentation\%OUT_DIR%Renderers\D3D11\
ECHO **** Copying out to: "%COPY_DIR%"

REM Create target directories if necessary and empty them
ECHO **** Creating and clearing directory
MKDIR "%COPY_DIR%" > NUL 2>&1
DEL /Q /S "%COPY_DIR%*.*" > NUL

REM TODO FOR loop over file types
ECHO **** Copying files
XCOPY /Y "%TARGET_DIR%*.dll" "%COPY_DIR%" > NUL
IF errorlevel 1 GOTO Abort

XCOPY /Y "%TARGET_DIR%*.pdb" "%COPY_DIR%" > NUL
IF errorlevel 1 GOTO Abort

XCOPY /Y "%TARGET_DIR%*.cso" "%COPY_DIR%" > NUL
IF errorlevel 1 GOTO Abort

ECHO **** Deployment successful
ECHO.
EXIT /B 0

REM Errors lead here. Visual Studio only cares about the last error when executing a batch file.
:Abort
ECHO **** Warning: Deployment failed
ECHO.
EXIT /B 0