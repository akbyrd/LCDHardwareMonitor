REM Parameters:
REM 	%1 - $(SolutionDir)
REM 	%2 - $(TargetDir)
REM 	%3 - Run tree sub-directory

REM ECHO **** Command line: Deploy.bat %1 %2 %3
ECHO.

REM Variables:
SET SOLUTION_DIR=%~1
SET TARGET_DIR=%~2
SET SUB_DIR=%~3

REM Working directory is the project directory
PUSHD "%SOLUTION_DIR%"

REM Check for errors?
ECHO  **** Copying output to Run Tree
XCOPY /Y /I /S /B /EXCLUDE:ignored.txt "%TARGET_DIR%*" "Run Tree\%SUB_DIR%" > NUL
IF errorlevel 1 GOTO Abort

ECHO  **** Deployment successful

:Exit
ECHO.
POPD
EXIT /B 0

:Abort
ECHO  **** WARNING - Deployment failed
GOTO Exit