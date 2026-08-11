@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%

cl /nologo /EHsc /W4 /std:c++14 tests\home_menu_request_tests.cpp source\home_menu_request.cpp source\home_menu_layout.cpp /Fo:"%TEMP%\\" /Fe:"%TEMP%\cthulhu_request_tests.exe"
if errorlevel 1 exit /b %errorlevel%

"%TEMP%\cthulhu_request_tests.exe"
exit /b %errorlevel%
