REM erase the .ncb
erase *.ncb
erase *.sdf
erase *.opensdf

rd .\ipch /s /q

REM remove the build folders (has compiled .exe files in it)
rd .\Debug /s /q
rd .\Release /s /q

rd .\gtp\Debug /s /q
rd .\gtp\Release /s /q

REM Remove the x64 build folder
rd .\x64 /s /q
rd .\gtp\x64 /s /q


REM delete all "lastRunLog.txt" 's
erase .\gtp\lastRunLog.txt

move .\gtp\*.png "C:\Dropbox\code\gtp snapshots\images"
move .\gtp\*.wsh "C:\Dropbox\code\gtp snapshots\images"

