@echo off

set CODE_BASE=../../../../../

set DEBUG_FLAG=%1
if not "%DEBUG_FLAG%"=="release" (set DEBUG_FLAG=debug)

set CC_FLAGS=/nologo /utf-8 -D_X86 -DWIN32 -D_CONSOLE -I%CODE_BASE%/mycode/h -I%CODE_BASE%/open/h -I%CODE_BASE%/open/h/winpcap
set USE_LIBS="Ws2_32.lib" "libutl.lib" "libbs.lib" "Iphlpapi.lib" "libmysql.lib" "libeay32.lib" "ssleay32.lib" "pcre.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib"
set MYLIB_PATH=%CODE_BASE%/mycode/build_win/_lib/%DEBUG_FLAG%/
set OPENLIB_PATH=%CODE_BASE%/open/lib/
set LINK_FLAGS=

if "%DEBUG_FLAG%"=="debug" (set CC_FLAGS=%CC_FLAGS% /MDd /ZI) else (set CC_FLAGS=%CC_FLAGS% /MD)
if "%DEBUG_FLAG%"=="debug" (set LINK_FLAGS=%LINK_FLAGS% /DEBUG)

if not exist "obj" md "obj"
if not exist "..\out" md "..\out"

cl /c %CC_FLAGS% *.c /Foobj/obj.o
Link %LINK_FLAGS% /OUT:"../out/scm.dll" /dll obj/*.o  %USE_LIBS% /LIBPATH:%MYLIB_PATH% /LIBPATH:%OPENLIB_PATH%

rd /s/q obj
del ..\out\*.exp
del ..\out\*.lib

