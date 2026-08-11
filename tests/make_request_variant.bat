@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl /nologo /EHsc /W4 /std:c++14 tests\make_request_variant.cpp source\home_menu_request.cpp /Fo:"%TEMP%\\" /Fe:"%TEMP%\cthulhu_make_request_variant.exe"
if errorlevel 1 exit /b %errorlevel%
"%TEMP%\cthulhu_make_request_variant.exe" %*
exit /b %errorlevel%
