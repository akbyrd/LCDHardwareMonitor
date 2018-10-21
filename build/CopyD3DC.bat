REM Variables:
SET WIN_SDK_DIR=%~1
SET WIN_SDK_ARCH=%~2

SET D3D_COMPILER=%WIN_SDK_DIR%Redist\D3D\%WIN_SDK_ARCH%\d3dcompiler_47.dll
SET RUN_DIR=..\run

ECHO  **** Copying shader compiler

COPY "%D3D_COMPILER%" "%RUN_DIR%" > NUL
IF ERRORLEVEL 1 GOTO Abort

ECHO  **** Copy successful
goto Exit

:Abort
  ECHO  **** WARNING - Copy failed
  EXIT /B 1

:Exit
  EXIT /B 0

