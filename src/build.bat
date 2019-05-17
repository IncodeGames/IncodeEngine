@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
set path=w:\HandmadeWork\misc;%path%

set Pincludes=W:\src\include
set Plibs=W:\src\lib

set PDBLockPrevention=-PDB:handmade_%random%.pdb
set EXPORTS=-EXPORT:GameUpdateAndRender -EXPORT:GameGetSoundSamples

w:
if not defined DevEnvDir (
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
)

set CommonCompilerFlags=-MD -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 /EHsc -FC -Z7 /I%Pincludes% -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1
set CommonLinkerFlags=-incremental:no -opt:ref /LIBPATH:%Plibs% SDL2.lib SDL2main.lib opengl32.lib user32.lib

pushd ..\build

del *.pdb > NUL 2> NUL
cl  %CommonCompilerFlags% ..\src\handmade.cpp -Fmair.map -LD /link -incremental:no /subsystem:windows,6.1 /incremental:no %PDBLockPrevention% %EXPORTS%
cl  %CommonCompilerFlags% ..\src\sdl_handmade.cpp -FmSDL_handmade.map /link /subsystem:console %CommonLinkerFlags%

popd

echo -- Finished compiling --