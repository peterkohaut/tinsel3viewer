@set SRC=viewer.cpp tinsel.cpp imgui/backends/imgui_impl_sdl.cpp imgui/backends/imgui_impl_opengl3.cpp imgui/imgui*.cpp 
@set INCLUDES=/I imgui /I imgui/backends /I include /I include/SDL2 /I imgui_club/imgui_memory_editor
@set LIBS=lib/x64/SDL2main.lib lib/x64/SDL2.lib lib/x64/glew32.lib user32.lib shell32.lib opengl32.lib
cl /std:c++17 /Zi /EHsc /nologo %SRC% %INCLUDES% /link /SUBSYSTEM:CONSOLE %LIBS%
