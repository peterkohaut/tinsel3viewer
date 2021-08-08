#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "base.hpp"
#include "read.hpp"

using namespace std;

int decompressLZSS(string &filename, u8 *output);

enum class ChunkType : u32
{
	CHUNK_STRING 			= 0x33340001L,
	CHUNK_BITMAP			= 0x33340002L,
	CHUNK_CHARPTR			= 0x33340003L,
	CHUNK_CHARMATRIX		= 0x33340004L,
	CHUNK_PALETTE			= 0x33340005L,
	CHUNK_IMAGE				= 0x33340006L,
	CHUNK_ANI_FRAME			= 0x33340007L,
	CHUNK_FILM				= 0x33340008L,
	CHUNK_FONT				= 0x33340009L,
	CHUNK_PCODE				= 0x3334000AL,
	CHUNK_ENTRANCE			= 0x3334000BL,
	CHUNK_POLYGONS			= 0x3334000CL,
	CHUNK_ACTORS			= 0x3334000DL,
	CHUNK_PROCESSES			= 0x3334000EL,
	CHUNK_SCENE				= 0x3334000FL,
	CHUNK_TOTAL_ACTORS		= 0x33340010L,
	CHUNK_TOTAL_GLOBALS		= 0x33340011L,
	CHUNK_TOTAL_OBJECTS		= 0x33340012L,
	CHUNK_OBJECTS			= 0x33340013L,
	CHUNK_MIDI				= 0x33340014L,
	CHUNK_SAMPLE			= 0x33340015L,
	CHUNK_TOTAL_POLY		= 0x33340016L,
	CHUNK_NUM_PROCESSES		= 0x33340017L,
	CHUNK_MASTER_SCRIPT		= 0x33340018L,
	CHUNK_CDPLAY_FILENUM	= 0x33340019L,
	CHUNK_CDPLAY_HANDLE		= 0x3334001AL,
	CHUNK_CDPLAY_FILENAME	= 0x3334001BL,
	CHUNK_MUSIC_FILENAME	= 0x3334001CL,
	CHUNK_MUSIC_SCRIPT		= 0x3334001DL,
	CHUNK_MUSIC_SEGMENT		= 0x3334001EL,
	CHUNK_SCENE_HOPPER		= 0x3334001FL,
	CHUNK_SCENE_HOPPER2		= 0x33340030L,
	CHUNK_TIME_STAMPS		= 0x33340020L,
	CHUNK_MBSTRING			= 0x33340022L,
	CHUNK_GAME				= 0x33340031L,
	CHUNK_GRAB_NAME			= 0x33340100L,
};

struct Chunk
{
	ChunkType type;
	u32 pos;
	u32 size;

	u8* data;
};

enum class MemHandleFlags
{
	Preload		= 0x01000000L,	///< preload memory
	Discard		= 0x02000000L,	///< discard memory
	Sound		= 0x04000000L,	///< sound data
	Graphic		= 0x08000000L,	///< graphic data
	Compressed	= 0x10000000L,	///< compressed data
	Loaded		= 0x20000000L
};

struct GameVariables
{
	u32	un0;
	u32	un4;
	u32	un8;
	u32	numActors;
	u32	numGlobals;
	u32	numPolygons;
	u32	numGlobalProcesses;
	u32	cdPlayHandle;
	u32	numIcons;
};

struct Process
{
	u32 pid;
	u32 handle;
};

struct Image
{
	u32 handle;

	u16 width;
	u16 height;
	u16 aniOffX;
	u16 aniOffY;
	u32 hImgBits;
	u16 isRLE;
	u16 colorFlags;
};

struct Frames
{
	u32 handle;

	vector<Image> images;
};

enum AnimScriptOpcode {
	ANI_END = 0,
	ANI_JUMP,
	ANI_HFLIP,
	ANI_VFLIP,
	ANI_HVFLIP,
	ANI_ADJUSTX,
	ANI_ADJUSTY,
	ANI_ADJUSTXY,
	ANI_NOSLEEP,
	ANI_CALL,
	ANI_HIDE,
	ANI_STOP,
};

struct AnimScriptLine {
	u32 ip;
	u32 opcode;
	string opcodeStr;
	string argumentStr;

	u32 hFrame;
	Frames frame;

	AnimScriptLine(u32 ip, u32 opcode, string argument);
	AnimScriptLine(u32 ip, u32 hFrame_);
};

struct AnimScript
{
	u32 handle;

	vector<AnimScriptLine> lines;
};

static vector<AnimScriptLine> animscript_disassemble(istream &code);

struct MultiInit
{
	u32 handle;

	u32 hMulFrame;
	i32 mulFlags;
	i32 mulID;
	i32 mulX;
	i32 mulY;
	i32 mulZ;
	u32 otherFlags;

	Frames frames;
};

struct Reel
{
	u32 handle;

	u32 mobj;
	u32 script;

	MultiInit obj;
	AnimScript animScript;
};

struct Film
{
	u32 handle;

	u32 framerate;
	vector<Reel> reels;
};

struct Entrance
{
	u32 handle;

	u32 eNumber;
	u32 hScript;
	u32 hEntDesc;
	u32 flags;
};

struct Camera
{
	u32 handle;

};

struct Light
{
	u32 handle;

};

struct Poly
{
	u32 handle;

	u32 type;
	u32 x[4];
	u32 y[4];
	u32 xOff;
	u32 yOff;
	u32 id;
	u32 _ws;
	u32 field;
	u32 reftype;
	u32 tagx;
	u32 tagy;
	u32 hTagText;
	u32 nodeX;
	u32 nodeY;
	u32 hFilm;
	u32 scale1;
	u32 scale2;
	u32 level1;
	u32 level2;
	u32 bright1;
	u32 bright2;
	u32 reelType;
	u32 zFactor;
	u32 nodeCount;
	u32 nodeListX;
	u32 nodeListY;
	u32 lineList;
	u32 hScript;
};

struct Actor
{
	u32 handle;

	u32 id;
	u32 hTagText;
	u32 tagPortionV;
	u32 tagPortionH;
	u32 hActorCode;
	u32 tagFlags;
	u32 hOverrideTag;
};

struct Scene
{
	u32 handle;

	u32 defRefer;
	u32 hSceneScript;
	u32 hSceneDesc;
	u32 numEntrance;
	u32 hEntrance;
	u32 numCameras;
	u32 hCamera;
	u32 numLights;
	u32 hLight;
	u32 numPoly;
	u32 hPoly;
	u32 numTaggedActor;
	u32 hTaggedActor;
	u32 numProcess;
	u32 hProcess;
	u32 hMusicScript;
	u32 hMusicSegment;

	vector<Entrance> entrances;
	vector<Poly> polys;
	vector<Actor> actors;
};

struct Object
{
	u32 handle;

	u32 id;
	u32 hIconFilm;
	u32 hScript;
	u32 attribute;
	u32 _u;
	u32 notClue;
};

enum PcodeOpCode {
	OP_NOOP = 0,
	OP_HALT,
	OP_IMM,
	OP_ZERO,
	OP_ONE,
	OP_MINUSONE,
	OP_STR,
	OP_FILM,
	OP_FONT,
	OP_PAL,
	OP_LOAD,
	OP_GLOAD,
	OP_STORE,
	OP_GSTORE,
	OP_CALL,
	OP_LIBCALL,
	OP_RET,
	OP_ALLOC,
	OP_JUMP,
	OP_JMPFALSE,
	OP_JMPTRUE,
	OP_EQUAL,
	OP_LESS,
	OP_LEQUAL,
	OP_NEQUAL,
	OP_GEQUAL,
	OP_GREAT,
	OP_PLUS,
	OP_MINUS,
	OP_LOR,
	OP_MULT,
	OP_DIV,
	OP_MOD,
	OP_AND,
	OP_OR,
	OP_EOR,
	OP_LAND,
	OP_NOT,
	OP_COMP,
	OP_NEG,
	OP_DUP,
	OP_ESCON,
	OP_ESCOFF,
	OP_CIMM,
	OP_CDFILM
};

struct PcodeScriptLine
{
	u32 ip;
	u32 opcode;
	u32 argument;
	bool hasArgument;

	string opcodeStr;
	string argumentStr;

	PcodeScriptLine(u32 ip, u32 opcode);
	PcodeScriptLine(u32 ip, u32 opcode, u32 argument);
	PcodeScriptLine(u32 ip, string text);
};

struct PcodeScript
{
	u32 handle;
	string name;
	vector <PcodeScriptLine> disassembly;
};

static vector<PcodeScriptLine> pcode_disassemble(istream &code);

struct MemHandle
{
	u32 id;
	string name;
	u32 size;
	u32 flags;

	bool loaded;
	vector<u8> data;


	vector<Chunk> chunks;
	vector<PcodeScript> scripts;

	bool hasScene;
	Scene scene;

	bool hasObjects;
	vector<Object> objects;
};

struct Tinsel
{
	map<ChunkType, string> chunkTypeNames;
	vector<MemHandle> memHandles;
	u32 stringsId;

	GameVariables gameVars;

	Tinsel();

	void load_index();
	void load_memhandle(u32 i);
	void unload_memhandle(u32 i);

	MemHandle* get_memhandle(u32 h);
	u32 get_offset(u32 h);
	unique_ptr<istream> get_memory(u32 h);

	void load_chunks(u32 i);
	void load_game_vars(u32 i);
	void load_scene(u32 i);
	void load_objects(u32 i);
	void load_processes(u32 i);

	vector<u8> decode_image(Image &image);

	Image parse_image(u32 handle);
	Frames parse_frame(u32 handle);
	MultiInit parse_multi_init(u32 handle);
	AnimScript parse_anim_script(u32 handle, bool sound);
	Film parse_film(u32 handle);


	void load_strings();
	string get_string(u32 id);
};

