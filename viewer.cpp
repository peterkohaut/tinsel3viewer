#include <SDL.h>
#include <GL/glew.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "imgui_memory_editor.h"

#include <string>
#include <vector>
#include <map>

#include "base.hpp"
#include "read.hpp"
#include "tinsel.hpp"

#include <fstream>

using namespace std;
using namespace ImGui;

void TextP(u32 padding, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[1024];
	memset(buf, ' ', sizeof(buf));
	buf[sizeof(buf) - 1] = 0;
	vsprintf(buf + (padding * 4), fmt, args);
	TextUnformatted(buf);
	va_end(args);
}

Tinsel tinsel;
map<u32, GLuint> glimages;

void render_image(::Image& image, u32 padding = 0)
{
	PushID(image.handle);
	TextP(padding, "Image: %08x", image.handle);
	TextP(padding+1, "width: %d", image.width);
	TextP(padding+1, "height: %d", image.height);
	TextP(padding+1, "aniOffX: %d", image.aniOffX);
	TextP(padding+1, "aniOffY: %d", image.aniOffY);
	TextP(padding+1, "hImgBits: %08x", image.hImgBits);
	TextP(padding+1, "isRLE: %d", image.isRLE);
	TextP(padding+1, "colorFlags: %08x", image.colorFlags);

	if (glimages.count(image.handle) == 0)
	{
		auto data = tinsel.decode_image(image);
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0,  GL_RGBA, GL_UNSIGNED_BYTE, data.data());
		glBindTexture(GL_TEXTURE_2D, 0);
		glimages[image.handle] = texture;
	}
	ImGui::Image((ImTextureID)(uintptr_t)glimages[image.handle], { (float)image.width, (float)image.height });
	PopID();
}

void render_frames(Frames& f, u32 padding = 0)
{
	PushID(f.handle);
	for (auto &image : f.images)
	{
		render_image(image, padding+1);
	}
	PopID();
}

void render_multi_init(MultiInit &mi, u32 padding = 0)
{
	PushID(mi.handle);
	TextP(padding, "MultiInitObject: %08x", mi.handle);
	TextP(padding+1, "hMulFrame: %08x", mi.hMulFrame);
	TextP(padding+1, "mulFlags: %08x", mi.mulFlags);
	TextP(padding+1, "mulID: %08x", mi.mulID);
	TextP(padding+1, "mulX: %d", mi.mulX);
	TextP(padding+1, "mulY: %d", mi.mulY);
	TextP(padding+1, "mulZ: %d", mi.mulZ);
	TextP(padding+1, "otherFlags: %08x", mi.otherFlags);
	TextP(padding+1, "frames:");
	render_frames(mi.frames, padding+1);
	PopID();
}

void render_anim_script(AnimScript& as, u32 padding = 0, bool sound = false)
{
	PushID(as.handle);
	TextP(padding, "AnimScript: %08x", as.handle);
	for (auto &line : as.lines)
	{
		if (line.hFrame)
		{
			TextP(padding+1, "%4x: frame %08x", line.ip, line.hFrame);
			if (!sound)
			{
				render_frames(line.frame, padding+1);
			}
		}
		else
		{
			char buf[1024];
			TextP(padding+1, "%4x: %-15s %-s", line.ip, line.opcodeStr.c_str(), line.argumentStr.c_str());
		}
	}
	PopID();
}

void render_reel(Reel &reel, u32 padding = 0)
{
	PushID(reel.handle);
	TextP(padding, "Reel: %08x", reel.handle);
	TextP(padding+1, "mobj: %08x", reel.mobj);
	TextP(padding+1, "script: %08x", reel.script);
	render_multi_init(reel.obj, padding+1);
	render_anim_script(reel.animScript, padding+1, reel.obj.mulID == -2);
	PopID();
}

void render_film(Film &film, u32 padding = 0)
{
	PushID(film.handle);
	TextP(padding, "Film: %08x", film.handle);
	TextP(padding+1, "framerate: %d", film.framerate);
	TextP(padding+1, "reels:");
	for (auto& reel : film.reels)
	{
		render_reel(reel, padding+2);
	}
	PopID();
}

void render_scene(Scene &scene, u32 padding = 0)
{
	TextP(padding, "Scene:");
	TextP(padding+1, "defRefer: %d", scene.defRefer);
	TextP(padding+1, "hSceneScript: %08x", scene.hSceneScript);
	TextP(padding+1, "hSceneDesc: %08x", scene.hSceneDesc);
	TextP(padding+1, "numEntrance: %d", scene.numEntrance);
	TextP(padding+1, "hEntrance: %08x", scene.hEntrance);
	TextP(padding+1, "numCameras: %d", scene.numCameras);
	TextP(padding+1, "hCamera: %08x", scene.hCamera);
	TextP(padding+1, "numLights: %d", scene.numLights);
	TextP(padding+1, "hLight: %08x", scene.hLight);
	TextP(padding+1, "numPoly: %d", scene.numPoly);
	TextP(padding+1, "hPoly: %08x", scene.hPoly);
	TextP(padding+1, "numTaggedActor: %d", scene.numTaggedActor);
	TextP(padding+1, "hTaggedActor: %08x", scene.hTaggedActor);
	TextP(padding+1, "numProcess: %d", scene.numProcess);
	TextP(padding+1, "hProcess: %08x", scene.hProcess);
	TextP(padding+1, "hMusicScript: %08x", scene.hMusicScript);
	TextP(padding+1, "hMusicSegment: %08x", scene.hMusicSegment);
}

void render_ui(Tinsel &tinsel)
{
	ShowDemoWindow();

	static MemHandle* selected_memhandle = nullptr;
	static PcodeScript *selected_script = nullptr;
	static u32 selected_handle = 0;
	static u32 selected_film = 0;

	u32 flags =
			ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
			| ImGuiTableFlags_SizingFixedFit;

	if (Begin("Handles"))
	{
		BeginChild("list", ImVec2(400, 0));

		if (Button("Load all"))
		{
			for (auto& memHandle : tinsel.memHandles)
			{
				tinsel.load_memhandle(memHandle.id);
			}
		}
		SameLine();
		if (Button("Unload all"))
		{
		}

		if (BeginTable("handles", 6, flags))
		{
			TableSetupColumn("ID");
			TableSetupColumn("Name");
			TableSetupColumn("Size");
			TableSetupColumn("Flags");
			TableSetupColumn("Loaded");
			TableSetupColumn("Action");
			TableHeadersRow();

			u32 i = 0;
			for (auto &handle : tinsel.memHandles)
			{
				PushID(i);
				TableNextColumn();
				char label[32] {};
				sprintf(label, "%02x", i << 1);

				bool selected = &handle == selected_memhandle;
				if (Selectable(label, selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap))
				{
					selected_memhandle = &handle;
					if (handle.loaded)
					{
						selected_handle = (i << 25);
						selected_script = nullptr;
					}
				}
				TableNextColumn();
				TextUnformatted(handle.name.c_str());
				TableNextColumn();
				Text("%10d", handle.size);
				TableNextColumn();
				Text("0x%08x", handle.flags);
				TableNextColumn();
				if (handle.loaded)
				{
					Text("x");
				}
				TableNextColumn();
				if (handle.loaded)
				{
					if ((handle.flags & (u32)MemHandleFlags::Preload) == 0)
					{
						if (SmallButton("Unload"))
						{
							if (selected_memhandle == &handle)
							{
								selected_memhandle = nullptr;
							}
						}
					}
				}
				else
				{
					if (SmallButton("Load"))
					{
						tinsel.load_memhandle(i);
					}
				}
				++i;
				PopID();
			}
			EndTable();
		}
		EndChild(); // list

		SameLine();

		if (selected_memhandle != nullptr)
		{
			if (BeginChild("Properties", ImVec2(400, 0)))
			{
				Text("Properties");
				if (selected_memhandle->loaded)
				{
					if (BeginTable("chunks", 4, flags))
					{
						TableSetupColumn("Pos");
						TableSetupColumn("Size");
						TableSetupColumn("Type");
						TableSetupColumn("Type");
						TableHeadersRow();

						u32 i = 0;
						for (auto &chunk : selected_memhandle->chunks)
						{
							PushID(i);
							TableNextColumn();
							char label[32] {};
							sprintf(label, "%08x", chunk.pos);
							if (Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns))
							{
								selected_handle = (selected_memhandle->id << 25) | chunk.pos;
							}
							TableNextColumn();
							Text("%10d", chunk.size);
							TableNextColumn();
							Text("%08x", chunk.type);
							TableNextColumn();
							TextUnformatted(tinsel.chunkTypeNames[chunk.type].c_str());
							++i;
							PopID();
						}
						EndTable();
					}

					if (selected_memhandle->hasScene)
					{
						render_scene(selected_memhandle->scene);

						Text("Entrances:");
						if (BeginTable("entrances", 5, flags))
						{
							TableSetupColumn("handle");
							TableSetupColumn("eNumber");
							TableSetupColumn("hScript");
							TableSetupColumn("hEntDesc");
							TableSetupColumn("flags");
							TableHeadersRow();

							u32 i = 0;
							for (auto &ent : selected_memhandle->scene.entrances)
							{
								PushID(i);
								TableNextColumn();
								char label[32] {};
								sprintf(label, "%08x", ent.handle);
								if (Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns))
								{
									selected_handle = ent.handle;
								}
								TableNextColumn();
								Text("%d", ent.eNumber);
								TableNextColumn();
								Text("%08x", ent.hScript);
								TableNextColumn();
								Text("%08x", ent.hEntDesc);
								TableNextColumn();
								Text("%08x", ent.flags);
								++i;
								PopID();
							}

							EndTable();
						}
					}

					if (selected_memhandle->hasObjects)
					{
						Text("Objects:");
						if (BeginTable("objects", 6, flags))
						{
							TableSetupColumn("id");
							TableSetupColumn("hIconFilm");
							TableSetupColumn("hScript");
							TableSetupColumn("attribute");
							TableSetupColumn("title");
							TableSetupColumn("notClue");
							TableHeadersRow();

							u32 i = 0;
							for (auto &obj : selected_memhandle->objects)
							{
								PushID(i);
								TableNextColumn();
								char label[32] {};
								sprintf(label, "%x", obj.id);
								if (Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns))
								{
									selected_handle = obj.handle;
									if (obj.hIconFilm > 0x160)  // meh
									{
										selected_film = obj.hIconFilm;
									}
								}
								TableNextColumn();
								Text("%08x", obj.hIconFilm);
								TableNextColumn();
								Text("%08x", obj.hScript);
								TableNextColumn();
								Text("%08x", obj.attribute);
								TableNextColumn();
								Text("%8x", obj._u);
								TableNextColumn();
								Text("%8x", obj.notClue);
								++i;
								PopID();
							}

							EndTable();
						}
					}
				}
			}
			EndChild(); // Properties

			SameLine();

			if (BeginChild("Scripts"))
			{
				Text("Scripts");
				if (BeginTable("scripts", 2, flags | ImGuiTableFlags_ScrollY , ImVec2(0.0f, 200.0f)))
				{
					TableSetupColumn("Handle");
					TableSetupColumn("Description");
					TableHeadersRow();

					for (auto& script : selected_memhandle->scripts)
					{
						PushID(script.handle);
						TableNextColumn();
						char label[32] {};
						sprintf(label, "%08x", script.handle);
						if (Selectable(label, selected_script == &script, ImGuiSelectableFlags_SpanAllColumns))
						{
							selected_script = &script;
							selected_handle = script.handle;
						}
						TableNextColumn();
						TextUnformatted(script.name.c_str());
						PopID();
					}
					EndTable();
				}

				BeginChild("Disassembly");
				if (selected_script != nullptr)
				{
					for (auto& line : selected_script->disassembly)
					{
						char buf[1024];
						sprintf(buf, "%4x: %-15s %-s", line.ip, line.opcodeStr.c_str(), line.argumentStr.c_str());
						PushID(line.ip);
						if (line.opcode == OP_FILM)
						{
							PushStyleColor(ImGuiCol_Text, {0, 1.0f, 0, 1.0f});
							if (Selectable(buf, false))
							{
								selected_film = line.argument;
								selected_handle = line.argument;
							}
							PopStyleColor(1);
						}
						else if (line.opcode == OP_LIBCALL)
						{
							PushStyleColor(ImGuiCol_Text, {0, 0.5f, 1.0f, 1.0f});
							TextUnformatted(buf);
							PopStyleColor(1);
						}
						else
						{
							TextUnformatted(buf);
						}
						PopID();
					}

				}
				EndChild();
			}
			EndChild(); // Scripts
		}
	}
	End();

	if (selected_film != 0)
	{
		static u32 loaded_film = 0;
		static Film cached_film;

		if (loaded_film != selected_film)
		{
			for(auto glimage : glimages)
			{
				glDeleteTextures(1, (GLuint*)&glimage.second);
			}
			glimages.clear();
			cached_film = tinsel.parse_film(selected_film);
			loaded_film = selected_film;
		}

		if (Begin("Film"))
		{
			render_film(cached_film);
		}
		End();
	}

	if (Begin("Text decoder"))
	{
		static char textIdStr[1024];
		static u32 textId;
		InputText("hex", &textIdStr[0], sizeof(textIdStr));
		SameLine();
		if (Button("Decode"))
		{
			textId = strtol(textIdStr, nullptr, 16);
		}
		if (textId != 0)
		{
			TextUnformatted(tinsel.get_string(textId).c_str());
		}
		End();
	}

	if (selected_handle != 0)
	{
		MemHandle* memHandle = tinsel.get_memhandle(selected_handle);
		if (memHandle && memHandle->loaded)
		{
			static u32 current_handle = 0;
			static MemoryEditor mem_edit;
			mem_edit.HighlightColor = 0xff0000ff;
			if (selected_handle != current_handle)
			{
				u32 offset = tinsel.get_offset(selected_handle);
				mem_edit.GotoAddrAndHighlight(offset, offset + 1);
				current_handle = selected_handle;
			}
			mem_edit.ReadOnly = true;
			mem_edit.DrawWindow("Memory Editor", memHandle->data.data(), memHandle->size);
		}
	}
}

int main(int argc, char* argv[])
{
	tinsel.load_index();
	tinsel.load_strings();


	// SDL setup
	SDL_Init(SDL_INIT_VIDEO);

	u32 width = 1000;
	u32 height = 1000;
	u32 running = 1;
	u32 fullscreen = 0;

	u32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
	SDL_Window *window = SDL_CreateWindow("Tinsel viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, windowFlags);
	SDL_SetWindowMinimumSize(window, 500, 300);

	// OpenGL setup
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, glContext);

	glewInit();

	// setup ImGui
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init(nullptr);

	// main loop
	u32 frame = 0;
	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						running = 0;
						break;
					case SDLK_F11:
						fullscreen = !fullscreen;
						if (fullscreen)
						{
							SDL_SetWindowFullscreen(window, windowFlags | SDL_WINDOW_FULLSCREEN_DESKTOP);
						}
						else
						{
							SDL_SetWindowFullscreen(window, windowFlags);
						}
						break;

					default:
						break;
				}
			}
			else if (event.type == SDL_QUIT)
			{
				running = 0;
			}
		}

		glClearColor(0.5, 0.5, 0.5, 0.5);
		glClear(GL_COLOR_BUFFER_BIT);


		// imgui rendering
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		render_ui(tinsel);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);
		frame++;
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}