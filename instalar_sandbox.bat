@echo off
pushd "%~dp0"
dir /b %SystemRoot%\servicing\Packages\Microsoft-Windows-InternetExplorer-Optional-Package~3*.mum >sandbox.txt
dir /b %SystemRoot%\servicing\Packages\Microsoft-Windows-SenseClient-Package~3*.mum >>sandbox.txt
for /f %%i in ('findstr /i . sandbox.txt 2^>nul') do dism /online /norestart /add-package:"%SystemRoot%\servicing\Packages\%%i"
dism /online /enable-feature /featurename:Containers-DisposableClientVM /all /norestart
pause