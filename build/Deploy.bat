REM Variables:
SET OUT_DIR=%~1
SET SUB_DIR=%~2

ECHO.
ECHO  **** Copying output to run

XCOPY /Y /I /S /B /EXCLUDE:..\build\ignored.txt "%OUT_DIR%*" "..\run\%SUB_DIR%" > NUL
IF ERRORLEVEL 1 GOTO Abort

ECHO  **** Deployment successful
GOTO Exit

:Abort
	ECHO  **** WARNING - Deployment failed
	EXIT /B 1

:Exit
	EXIT /B 0
