:: BUILD.BAT - build project on Windows
:: try running 'build test' OR 'build debug' for test/debug builds
:: default build is release
@ECHO OFF

::SET COMPILE_VS=cl
SET COMPILE_GNU=g++

:: define common object files shared between configurations
SET OBJECTS=src\lexer.cpp src\ast.cpp src\codegen.cpp src\gccbuild_win32.cpp src\parser.cpp src\ramsey-error.cpp src\ramsey.cpp src\semantics.cpp src\stable.cpp

:: define object files used for testing
SET TEST_OBJECTS=src\test.cpp

:: define object files used for main program
:: 	SET MAIN_OBJECTS=

:: build test; link to a different main function for testing
IF '%1'=='test' (
	::IF '%2'=='vs' %COMPILE_VS% /Feramsey-test.exe /DRAMSEY_DEBUG /Zi /EHsc %OBJECTS% %TEST_OBJECTS% && GOTO end
	%COMPILE_GNU% -oramsey-test.exe -DRAMSEY_DEBUG -g -std=c++11 %OBJECTS% %TEST_OBJECTS%
	GOTO end
)

:: build debug; same as normal default release except include compiler debug information
IF '%1'=='debug' (
	::IF '%2'=='vs %COMPILE_VS% /Feramsey-debug.exe /DRAMSEY_DEBUG /Zi /EHsc %OBJECTS% %MAIN_OBJECTS% && GOTO end
	%COMPILE_GNU% -oramsey-debug.exe -DRAMSEY_DEBUG -g -std=c++11 %OBJECTS%
	GOTO end
)

:: build default release binary
::IF '%2'=='vs' %COMPILE_VS% /Feramsey.exe /EHsc %OBJECTS% %MAIN_OBJECTS% ELSE %COMPILE_GNU% -oramsey.exe -std=c++11 %OBJECTS% %MAIN_OBJECTS%
%COMPILE_GNU% -oramsey.exe -std=c++11 %OBJECTS%

:end

:: unload env vars
SET COMPILE_GNU=
SET OBJECTS=
SET TEST_OBJECTS=
