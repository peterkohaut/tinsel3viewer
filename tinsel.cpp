#include "tinsel.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <cassert>

using namespace std;

static u8 HIGH_BITS(u8 byteValue, int numBits) {
	u32 mask = ((1 << numBits) - 1) << (8 - numBits);
	return (byteValue & mask) >> (8 - numBits);
}

static u8 LOW_BITS(u8 byteValue, int numBits) {
	u32 mask = ((1 << numBits) - 1);
	return byteValue & mask;
}

int decompressLZSS(string &filename, u8 *output) {
	static const u32 kDictionarySize = 4096;
	u8 dictionary[kDictionarySize] = {};
	u32 dictionaryOffset = 1;
	u32 outputOffset = 0;

	ifstream input { string { "data/" } + filename, ios::binary | ios::ate };
	if (!input.is_open()) {
		return 0;
	}

	size_t inputSize = input.tellg();
	input.seekg(0);

	u8 *data = new u8[inputSize];
	input.read((char*)data, inputSize);
	u32 offset = 0;

	u32 bitShift = 0;
	u32 bytesWritten = 0;

	while (true) {
		u8 value = data[offset];
		u8 bitMask = 0x80 >> bitShift++;
		// First bit:
		// 0 -> Copy data from dictionary
		// 1 -> Copy raw byte from input
		bool useRawByte = value & bitMask;
		if (bitShift == 8) {
			bitShift = 0;
			offset++;
		}
		if (!useRawByte) {
			u32 bitsFromFirst = 8 - bitShift;
			u32 bitsFromLast = 16 - 8 - bitsFromFirst;

			// The dictionary lookup is 16 bit:
			// 12 bits for the offset
			// 4 bits for the run-length

			// Combined with the first bit this makes for 17 bits,
			// So we will be reading from three bytes, except when
			// the first bit was read from the end of a byte, then
			// bitShift will be 0, and bitsFromLast will be 8.

			// We make the assumption that we can dereference the third byte
			// even if we aren't using it. We will check "offset" after decompression
			// to verify this assumption.
			u32 byte1 = LOW_BITS(data[offset], bitsFromFirst);
			u32 byte2 = data[offset + 1];
			u32 byte3 = HIGH_BITS(data[offset + 2], bitsFromLast);

			u32 lookup = (byte1 << (8 + bitsFromLast)) | (byte2 << bitsFromLast) | byte3;

			u32 lookupOffset = (lookup >> 4) & 0xFFF;
			if (lookupOffset == 0) {
				break;
			}
			u32 lookupRunLength = (lookup & 0xF) + 2;
			for (u32 j = 0; j < lookupRunLength; j++) {
				output[outputOffset++] = dictionary[(lookupOffset + j) % kDictionarySize];
				dictionary[dictionaryOffset++] = dictionary[(lookupOffset + j) % kDictionarySize];
				dictionaryOffset %= kDictionarySize;
			}

			offset += 2;
			bytesWritten += lookupRunLength;
		} else {
			// Raw byte, but since we spent a bit first,
			// we must reassemble it from potentially two bytes.
			u32 bitsFromFirst = 8 - bitShift;
			u32 bitsFromLast = 8 - bitsFromFirst;

			u8 byteValue = LOW_BITS(data[offset], bitsFromFirst) << bitsFromLast;
			byteValue |= HIGH_BITS(data[offset + 1], bitsFromLast);

			offset++;

			output[outputOffset++] = byteValue;
			dictionary[dictionaryOffset++] = byteValue;
			dictionaryOffset %= kDictionarySize;

			bytesWritten++;
		}

	}
	delete[] data;

	return bytesWritten;
}

static const char * AnimScriptOpCodes[] = {
	"ANI_END", "ANI_JUMP", "ANI_HFLIP", "ANI_VFLIP", "ANI_HVFLIP", "ANI_ADJUSTX", "ANI_ADJUSTY", "ANI_ADJUSTXY", "ANI_NOSLEEP", "ANI_CALL", "ANI_HIDE", "ANI_STOP",
};

AnimScriptLine::AnimScriptLine(u32 ip_, u32 opcode_, string argument_)
: ip { ip_ }
, opcode { opcode_ }
, opcodeStr { AnimScriptOpCodes[opcode_] }
, argumentStr { argument_ }
, hFrame { 0 }
{
}

AnimScriptLine::AnimScriptLine(u32 ip_, u32 hFrame_)
: ip { ip_ }
, opcode { 0 }
, opcodeStr { "frame" }
, argumentStr { "" }
, hFrame { hFrame_ }
{
}

vector<AnimScriptLine> animscript_disassemble(istream &code)
{
	bool bHalt = false;
	vector<AnimScriptLine> out;
	do
	{
		u32 ip = code.tellg();
		u32 opcode = read_u32(code);

		switch (opcode) {
		case ANI_END:
			bHalt = true;
		case ANI_HFLIP:
		case ANI_VFLIP:
		case ANI_HVFLIP:
		case ANI_NOSLEEP:
		case ANI_CALL:
		case ANI_HIDE:
		case ANI_STOP:
			out.emplace_back(ip, opcode, "");
			break;

		case ANI_JUMP:
		{
			ostringstream oss;
			i32 jump = read_i32(code);
			if (jump < 0)
			{
				bHalt = true; // just repeat the animation
			}
			oss << jump;
			out.emplace_back(ip, opcode, oss.str());
			skip(code, jump * 4);
			break;
		}
		case ANI_ADJUSTX:
		case ANI_ADJUSTY:
		{
			ostringstream oss;
			oss << read_i32(code);
			out.emplace_back(ip, opcode, oss.str());
			break;
		}
		case ANI_ADJUSTXY:
		{
			ostringstream oss;
			oss << read_i32(code)  << ", " << read_i32(code);
			out.emplace_back(ip, opcode, oss.str());
			break;
		}
		default:
		{
			out.emplace_back(ip, opcode);
			break;
		}

		}

	} while (!bHalt);

	return out;
}

static const char * PcodeOpCodes[] = {
	"OP_NOOP", "OP_HALT", "OP_IMM", "OP_ZERO", "OP_ONE", "OP_MINUSONE", "OP_STR", "OP_FILM", "OP_FONT", "OP_PAL", "OP_LOAD", "OP_GLOAD", "OP_STORE", "OP_GSTORE", "OP_CALL", "OP_LIBCALL", "OP_RET", "OP_ALLOC", "OP_JUMP", "OP_JMPFALSE", "OP_JMPTRUE", "OP_EQUAL", "OP_LESS", "OP_LEQUAL", "OP_NEQUAL", "OP_GEQUAL", "OP_GREAT", "OP_PLUS", "OP_MINUS", "OP_LOR", "OP_MULT", "OP_DIV", "OP_MOD", "OP_AND", "OP_OR", "OP_EOR", "OP_LAND", "OP_NOT", "OP_COMP", "OP_NEG", "OP_DUP", "OP_ESCON", "OP_ESCOFF", "OP_CIMM", "OP_CDFILM",
};

static const char * PcodeLibCodes[] = {
	"NOFUNCTION", "ACTORBRIGHTNESS", "ACTORDIRECTION", "ACTORPRIORITY", "ACTORREF", "ACTORRGB", "ACTORXPOS", "ACTORYPOS", "ADDNOTEBOOK", "ADDCONV", "ADDHIGHLIGHT", "ADDINV8_T3", "ADDINV1", "ADDINV2", "ADDINV7_T3", "ADDINV4_T3", "ADDINV3_T3", "ADDTOPIC", "BACKGROUND", "BLOCKING", "UNKNOWN_14h", "CALLACTOR", "CALLGLOBALPROCESS", "CALLOBJECT", "CALLPROCESS", "CALLSCENE", "CALLTAG", "CAMERA", "CDCHANGESCENE", "CDDOCHANGE", "CDENDACTOR", "CDLOAD", "CDPLAY", "UNKNOWN_21h", "CLEARHOOKSCENE", "CLOSEINVENTORY", "CLOSEINVENTORY_24h", "CONTROL", "CONVERSATION", "UNKNOWN_27h", "CURSOR", "CURSORXPOS", "CURSORYPOS", "DECINVMAIN", "DECINV2", "DECLARELANGUAGE", "DECLEAD", "DEC3D", "DECTAGFONT", "DECTALKFONT", "DELTOPIC", "UNKNOWN_33h", "DIMMUSIC", "DROP", "DROPEVERYTHING", "DROPOUT", "EFFECTACTOR", "ENABLEMENU", "ENDACTOR", "ESCAPEOFF", "ESCAPEON", "EVENT", "FACETAG", "FADEIN", "FADEMUSIC_T3", "FADEOUT", "FRAMEGRAB", "FREEZECURSOR", "GETINVLIMIT", "GHOST", "GLOBALVAR", "GRABMOVIE", "HAILSCENE", "HASRESTARTED", "HAVE", "HELDOBJECT?", "HELDOBJECT2?", "HIDEACTOR", "HIDEBLOCK", "HIDEEFFECT", "HIDEPATH", "HIDEREFER", "HIDE_UNKNOWN_T3", "HIDETAG", "HOLD", "HOOKSCENE", "HYPERLINK_T3", "IDLETIME", "INSTANTSCROLL", "INVENTORY", "INVPLAY", "INWHICHINV", "KILLACTOR", "KILLGLOBALPROCESS", "KILLPROCESS", "LOCALVAR", "MOVECURSOR", "MOVETAG", "MOVETAGTO", "NEWSCENE", "NOBLOCKING", "NOPAUSE", "NOSCROLL", "UNKNOWN_67h", "OFFSET", "INVENTORY4_T3", "INVENTORY3_T3", "OTHEROBJECT", "PAUSE", "HOLD_T3?", "PLAY", "PLAYMOVIE", "PLAYMUSIC", "PLAYSAMPLE", "POINTACTOR", "POINTTAG", "POSTACTOR", "UNKNOWN75h", "POSTGLOBALPROCESS", "POSTOBJECT", "POSTPROCESS", "POSTTAG", "PREPAREMOVIE", "PRINT", "PRINTCURSOR", "PRINTOBJ", "PRINTTAG", "QUITGAME", "RANDOM", "RESETIDLETIME", "RESTARTGAME", "RESTORESCENE", "RESUMELASTGAME", "RUNMODE", "SAVESCENE", "SAY", "SAYAT", "SCREENXPOS", "SCREENYPOS", "SCOLL", "SCROLLPARAMETERS", "SENDACTOR", "SENDGLOBALPROCESS", "SENDOBJECT", "SENDPROCESS", "SENDTAG", "SETBRIGHTNESS", "SETINVLIMIT", "SETINVSIZE", "SETLANGUAGE", "UNKNOWN_96h", "SETSYSTEMREEL", "SETSYSTEMSTRING", "SETSYSTEMVAR", "SETVIEW_T3", "SHELL", "SHOWACTOR", "SHOWBLOCK", "SHOWEFFECT", "SHOWMENU", "SHOWPATH", "SHOWREFER", "SHOW_UNKNOWN", "SHOWTAG", "STAND", "STANDTAG", "STARTGLOBALPROCESS", "STARTPROCESS", "STARTTIMER", "STOPALLSAMPLES", "STOPSAMPLE", "STOPWALK", "SUBTITLES", "SWALK", "SWALKZ", "SYSTEMVAR", "TAGTAGXPOS", "TAGTAGYPOS", "TAGWALKXPOS", "TAGWALKYPOS", "TALK", "TALKAT", "TALKRGB", "TALKVIA", "TEMPTAGFONT", "TEMPTALKFONT", "THISOBJECT", "THISTAG", "TIMER", "TOPIC", "TOPPLAY", "TOPWINDOW", "UNDIMMUSIC", "UNHOOKSCENE", "WAITFRAME", "WAITKEY", "WAITSCROLL", "WAITTIME", "WALK", "WALKED", "WALKEDPOLY", "WALKEDTAG", "WALKINGACTOR", "WALKPOLY", "WALKTAG", "WALKXPOS", "WALKYPOS", "WHICHCD", "WHICHINVENTORY", "ZZZZZZ", "NTBPOLYENTRY", "PLAYSEQUENCE", "NTBPOLYPREVPAGE", "NTBPOLYNEXTPAGE", "SET3DTEXTURE_T3", "UNKNOWN_D7h", "UNKNOWN_D8h", "VOICEOVER", "TALK_DAh", "TALK_DBh", "TALK_DCh", "SAY_DDh", "SAY_DEh", "SAY_DFh", "LOAD3DOVERLAY", "PLAYMOVIEu_T3", "WAITSPRITER", "UNKNOWN_E3h", "UNKNOWN_E4h", "UNKNOWN_E5h", "UNKNOWN_E6h"
};

PcodeScriptLine::PcodeScriptLine(u32 ip_, u32 opcode_)
: ip { ip_ }
, opcode { opcode_ }
, hasArgument { false }
, opcodeStr { PcodeOpCodes[opcode_] }
, argumentStr { "" }
{}

PcodeScriptLine::PcodeScriptLine(u32 ip_, u32 opcode_, u32 argument_)
: ip { ip_ }
, opcode { opcode_ }
, argument { argument_ }
, hasArgument { true }
, opcodeStr { PcodeOpCodes[opcode_] }
{
	if (opcode == OP_LIBCALL)
	{
		argumentStr = PcodeLibCodes[argument_];
	} else {
		std::ostringstream ss;
		ss << hex << argument_ << "; = " << dec << argument_;
		argumentStr = ss.str();
	}
}

PcodeScriptLine::PcodeScriptLine(u32 ip_, string text_)
: ip { ip_ }
, opcode { 0 }
, hasArgument { false }
, opcodeStr { text_ }
{}

static u32 get_bytes(istream &code, u32 numBytes)
{
	u32 tmp;
	switch (numBytes)
	{
	case 0:
		tmp = read_u8(code);
		break;
	case 1:
		tmp = read_u8(code);
		break;
	case 2:
		tmp = read_u16(code);
		break;
	default:
		tmp = read_u32(code);
		break;
	}

	return tmp;
}

static u32 fetch(u8 opcode, istream &code) {
	if (opcode & 0x40)
	{
		return get_bytes(code, 1);
	}
	else if (opcode & 0x80)
	{
		return get_bytes(code, 2);
	}
	else
	{
		return get_bytes(code, 4);
	}
}

vector<PcodeScriptLine> pcode_disassemble(istream &code)
{
	bool bHalt = false;
	vector<PcodeScriptLine> out;;
	do
	{
		u32 ip = code.tellg();
		u8 opcode = (u8)get_bytes(code, 0);

		switch (opcode & 0x3F) {
		case OP_HALT:
			bHalt = true;
		case OP_ZERO:
		case OP_ONE:
		case OP_MINUSONE:
		case OP_RET:
		case OP_EQUAL:
		case OP_LESS:
		case OP_LEQUAL:
		case OP_NEQUAL:
		case OP_GEQUAL:
		case OP_GREAT:
		case OP_LOR:
		case OP_LAND:
		case OP_PLUS:
		case OP_MINUS:
		case OP_MULT:
		case OP_DIV:
		case OP_MOD:
		case OP_AND:
		case OP_OR:
		case OP_EOR:
		case OP_NOT:
		case OP_COMP:
		case OP_NEG:
		case OP_DUP:
		case OP_ESCON:
		case OP_ESCOFF:
		case OP_NOOP:
			out.emplace_back(ip, opcode & 0x3F);
			break;

		case OP_IMM:
		case OP_STR:
		case OP_FILM:
		case OP_CDFILM:
		case OP_FONT:
		case OP_PAL:
		case OP_LOAD:
		case OP_GLOAD:
		case OP_STORE:
		case OP_GSTORE:
		case OP_CALL:
		case OP_LIBCALL:
		case OP_ALLOC:
		case OP_JUMP:
		case OP_JMPFALSE:
		case OP_JMPTRUE:
			out.emplace_back(ip, opcode & 0x3F, fetch(opcode, code));
			break;

		default:
			out.emplace_back(ip, "???");
		}

	} while (!bHalt);

	return out;
}

Tinsel::Tinsel(): chunkTypeNames {
		{ ChunkType::CHUNK_STRING, "CHUNK_STRING" },
		{ ChunkType::CHUNK_BITMAP, "CHUNK_BITMAP" },
		{ ChunkType::CHUNK_CHARPTR, "CHUNK_CHARPTR" },
		{ ChunkType::CHUNK_CHARMATRIX, "CHUNK_CHARMATRIX" },
		{ ChunkType::CHUNK_PALETTE, "CHUNK_PALETTE" },
		{ ChunkType::CHUNK_IMAGE, "CHUNK_IMAGE" },
		{ ChunkType::CHUNK_ANI_FRAME, "CHUNK_ANI_FRAME" },
		{ ChunkType::CHUNK_FILM, "CHUNK_FILM" },
		{ ChunkType::CHUNK_FONT, "CHUNK_FONT" },
		{ ChunkType::CHUNK_PCODE, "CHUNK_PCODE" },
		{ ChunkType::CHUNK_ENTRANCE, "CHUNK_ENTRANCE" },
		{ ChunkType::CHUNK_POLYGONS, "CHUNK_POLYGONS" },
		{ ChunkType::CHUNK_ACTORS, "CHUNK_ACTORS" },
		{ ChunkType::CHUNK_PROCESSES, "CHUNK_PROCESSES" },
		{ ChunkType::CHUNK_SCENE, "CHUNK_SCENE" },
		{ ChunkType::CHUNK_TOTAL_ACTORS, "CHUNK_TOTAL_ACTORS" },
		{ ChunkType::CHUNK_TOTAL_GLOBALS, "CHUNK_TOTAL_GLOBALS" },
		{ ChunkType::CHUNK_TOTAL_OBJECTS, "CHUNK_TOTAL_OBJECTS" },
		{ ChunkType::CHUNK_OBJECTS, "CHUNK_OBJECTS" },
		{ ChunkType::CHUNK_MIDI, "CHUNK_MIDI" },
		{ ChunkType::CHUNK_SAMPLE, "CHUNK_SAMPLE" },
		{ ChunkType::CHUNK_TOTAL_POLY, "CHUNK_TOTAL_POLY" },
		{ ChunkType::CHUNK_NUM_PROCESSES, "CHUNK_NUM_PROCESSES" },
		{ ChunkType::CHUNK_MASTER_SCRIPT, "CHUNK_MASTER_SCRIPT" },
		{ ChunkType::CHUNK_CDPLAY_FILENUM, "CHUNK_CDPLAY_FILENUM" },
		{ ChunkType::CHUNK_CDPLAY_HANDLE, "CHUNK_CDPLAY_HANDLE" },
		{ ChunkType::CHUNK_CDPLAY_FILENAME, "CHUNK_CDPLAY_FILENAME" },
		{ ChunkType::CHUNK_MUSIC_FILENAME, "CHUNK_MUSIC_FILENAME" },
		{ ChunkType::CHUNK_MUSIC_SCRIPT, "CHUNK_MUSIC_SCRIPT" },
		{ ChunkType::CHUNK_MUSIC_SEGMENT, "CHUNK_MUSIC_SEGMENT" },
		{ ChunkType::CHUNK_SCENE_HOPPER, "CHUNK_SCENE_HOPPER" },
		{ ChunkType::CHUNK_SCENE_HOPPER2, "CHUNK_SCENE_HOPPER2" },
		{ ChunkType::CHUNK_TIME_STAMPS, "CHUNK_TIME_STAMPS" },
		{ ChunkType::CHUNK_MBSTRING, "CHUNK_MBSTRING" },
		{ ChunkType::CHUNK_GAME, "CHUNK_GAME" },
		{ ChunkType::CHUNK_GRAB_NAME, "CHUNK_GRAB_NAME" },
	}
{
}

void Tinsel::load_index()
{
	ifstream input {"data/index", ios::binary | ios::ate };

	size_t count = input.tellg() / 24;
	input.seekg(0);

	memHandles.reserve(count);

	for(u32 i = 0; i < count; ++i)
	{
		MemHandle &memHandle = memHandles.emplace_back();
		memHandle.id = i;
		memHandle.name = read_string(input, 12);
		memHandle.size = read_u32(input);
		skip(input, 4);
		memHandle.flags = read_u32(input);

		memHandle.loaded = false;
		memHandle.data.clear();

		if (memHandle.flags & (u32)MemHandleFlags::Preload)
		{
			load_memhandle(i);

			ofstream output { memHandle.name + ".uncompressed", ios::binary};
			output.write((char*)memHandle.data.data(), memHandle.size);
		}

	}
}

void Tinsel::load_memhandle(u32 i)
{
	MemHandle &memHandle = memHandles[i];
	if (memHandle.loaded)
	{
		return;
	}

	memHandle.data.resize(memHandle.size);

	if (decompressLZSS(memHandle.name, memHandle.data.data()))
	{
		memHandle.loaded = true;
		memHandle.hasScene = false;
		memHandle.hasObjects = false;

		load_chunks(i);

		if (i == 0)
		{
			load_game_vars(i);
		}
		else if (i == 1)
		{
			load_objects(i);
		}
		else
		{
			load_scene(i);
		}

		load_processes(i);
	}
}

void Tinsel::unload_memhandle(u32 i)
{
	MemHandle &memHandle = memHandles[i];
	if (!memHandle.loaded)
	{
		return;
	}
}

MemHandle* Tinsel::get_memhandle(u32 h)
{
	u32 index = h >> 25;
	assert(index < memHandles.size());

	return &memHandles[h >> 25];
}

u32 Tinsel::get_offset(u32 h)
{
	return h & 0x01ffffff;
}

unique_ptr<istream> Tinsel::get_memory(u32 h)
{
	u32 index = h >> 25;
	assert(index < memHandles.size());
	MemHandle &memHandle = memHandles[index];
	if (!memHandle.loaded)
	{
		load_memhandle(index);
	}

	u32 offset = get_offset(h);
	string buf { (char*)(memHandle.data.data() + offset), memHandle.size - offset - 1 };
	return make_unique<istringstream>(buf);
}


void Tinsel::load_chunks(u32 i)
{
	MemHandle &memHandle = memHandles[i];
	if (!memHandle.loaded)
	{
		return;
	}
	u32 offset = 0;
	while(true)
	{
		u32 next = *(u32*)(memHandle.data.data() + offset + 4);

		Chunk& chunk = memHandle.chunks.emplace_back();
		chunk.type = *(ChunkType*)(memHandle.data.data() + offset);
		chunk.pos = offset;
		if (next != 0)
		{
			chunk.size = next - offset;
		}
		else
		{
			chunk.size = memHandle.size - offset;
		}
		chunk.data = memHandle.data.data() + offset + 8;

		offset = next;

		if (offset == 0)
		{
			break;
		}
	}
}

void Tinsel::load_game_vars(u32 i)
{
	MemHandle& memHandle = memHandles[i];
	for (auto& chunk : memHandle.chunks)
	{
		if (chunk.type == ChunkType::CHUNK_GAME)
		{
			gameVars = *(GameVariables*)chunk.data;
		}
	}
}

void Tinsel::load_scene(u32 i)
{
	MemHandle& memHandle = memHandles[i];
	for (auto& chunk : memHandle.chunks)
	{
		if (chunk.type == ChunkType::CHUNK_SCENE)
		{
			istringstream data { { (char*)chunk.data, chunk.size } };

			Scene& scene = memHandle.scene;

			scene.defRefer = read_u32(data);
			scene.hSceneScript = read_u32(data);
			scene.hSceneDesc = read_u32(data);
			scene.numEntrance = read_u32(data);
			scene.hEntrance = read_u32(data);
			scene.numCameras = read_u32(data);
			scene.hCamera = read_u32(data);
			scene.numLights = read_u32(data);
			scene.hLight = read_u32(data);
			scene.numPoly = read_u32(data);
			scene.hPoly = read_u32(data);
			scene.numTaggedActor = read_u32(data);
			scene.hTaggedActor = read_u32(data);
			scene.numProcess = read_u32(data);
			scene.hProcess = read_u32(data);
			scene.hMusicScript = read_u32(data);
			scene.hMusicSegment = read_u32(data);

			if (scene.numEntrance != 0 && scene.hEntrance != 0)
			{
				auto data = get_memory(scene.hEntrance);
				for (u32 i = 0; i < scene.numEntrance; ++i)
				{
					Entrance& entrance = scene.entrances.emplace_back();
					entrance.handle = scene.hEntrance + (i * 16);
					entrance.eNumber = read_u32(*data);
					entrance.hScript = read_u32(*data);
					entrance.hEntDesc = read_u32(*data);
					entrance.flags = read_u32(*data);
				}
			}

			if (scene.numPoly != 0 && scene.hPoly != 0)
			{
				auto data = get_memory(scene.hPoly);
				for (u32 i = 0; i < scene.numPoly; ++i)
				{
					Poly& poly = scene.polys.emplace_back();
					poly.handle = scene.hPoly + (i * 136);

					poly.type = read_u32(*data);
					poly.x[0] = read_u32(*data);
					poly.x[1] = read_u32(*data);
					poly.x[2] = read_u32(*data);
					poly.x[3] = read_u32(*data);
					poly.y[0] = read_u32(*data);
					poly.y[1] = read_u32(*data);
					poly.y[2] = read_u32(*data);
					poly.y[3] = read_u32(*data);
					poly.xOff = read_u32(*data);
					poly.yOff = read_u32(*data);
					poly.id = read_u32(*data);
					poly._ws = read_u32(*data);
					poly.field = read_u32(*data);
					poly.reftype = read_u32(*data);
					poly.tagx = read_u32(*data);
					poly.tagy = read_u32(*data);
					poly.hTagText = read_u32(*data);
					poly.nodeX = read_u32(*data);
					poly.nodeY = read_u32(*data);
					poly.hFilm = read_u32(*data);
					poly.scale1 = read_u32(*data);
					poly.scale2 = read_u32(*data);
					poly.level1 = read_u32(*data);
					poly.level2 = read_u32(*data);
					poly.bright1 = read_u32(*data);
					poly.bright2 = read_u32(*data);
					poly.reelType = read_u32(*data);
					poly.zFactor = read_u32(*data);
					poly.nodeCount = read_u32(*data);
					poly.nodeListX = read_u32(*data);
					poly.nodeListY = read_u32(*data);
					poly.lineList = read_u32(*data);
					poly.hScript = read_u32(*data);
				}
			}


			if (scene.numTaggedActor != 0 && scene.hTaggedActor != 0)
			{
				auto data = get_memory(scene.hTaggedActor);
				for (u32 i = 0; i < scene.numTaggedActor; ++i)
				{
					Actor& actor = scene.actors.emplace_back();
					actor.handle = scene.hTaggedActor + (i * 28);

					actor.id = read_u32(*data);
					actor.hTagText = read_u32(*data);
					actor.tagPortionV = read_u32(*data);
					actor.tagPortionH = read_u32(*data);
					actor.hActorCode = read_u32(*data);
					actor.tagFlags = read_u32(*data);
					actor.hOverrideTag = read_u32(*data);
				}
			}

			memHandle.hasScene = true;
		}
	}
}

void Tinsel::load_objects(u32 i)
{
	MemHandle& memHandle = memHandles[i];
	for (auto& chunk : memHandle.chunks)
	{
		if (chunk.type == ChunkType::CHUNK_OBJECTS)
		{
			istringstream data { { (char*)chunk.data, chunk.size } };

			for (u32 i = 0; i < gameVars.numIcons; ++i)
			{
				Object& object = memHandle.objects.emplace_back();

				object.handle = i * 24;

				object.id = read_u32(data);
				object.hIconFilm = read_u32(data);
				object.hScript = read_u32(data);
				object.attribute = read_u32(data);
				object._u = read_u32(data);
				object.notClue = read_u32(data);
			}

			memHandle.hasObjects = true;
		}
	}
}

void Tinsel::load_processes(u32 i)
{
	MemHandle& memHandle = memHandles[i];
	if (i == 0)
	{
		for (auto& chunk : memHandle.chunks)
		{
			if (chunk.type == ChunkType::CHUNK_MASTER_SCRIPT)
			{
				u32 handle = *(u32*)chunk.data;

				PcodeScript &src = memHandle.scripts.emplace_back();
				src.handle = handle;
				src.name = "master script";
				src.disassembly = pcode_disassemble(*get_memory(handle));
			}

			if (chunk.type == ChunkType::CHUNK_PROCESSES)
			{
				istringstream data { { (char*)chunk.data, chunk.size } };
				for (u32 i = 0; i < gameVars.numGlobalProcesses; ++i)
				{
					u32 pid = read_u32(data);
					u32 handle = read_u32(data);

					ostringstream name;
					name << "global process script " << i << ", pid: "  << hex << setw(4) << right << setfill('0') << pid;

					PcodeScript &src = memHandle.scripts.emplace_back();
					src.handle = handle;
					src.name = name.str();
					src.disassembly = pcode_disassemble(*get_memory(handle));
				}
			}
		}
	}

	if (memHandle.hasObjects)
	{
		for (auto& object : memHandle.objects)
		{
				ostringstream name;
				name << "object " << hex << object.id << " script";

				PcodeScript &src = memHandle.scripts.emplace_back();
				src.handle = object.hScript;
				src.name = name.str();
				src.disassembly = pcode_disassemble(*get_memory(object.hScript));
		}
	}

	if (memHandle.hasScene)
	{
		if (memHandle.scene.hSceneScript != 0)
		{
			ostringstream name;
			name << "scene script " << memHandle.name;

			PcodeScript &src = memHandle.scripts.emplace_back();
			src.handle = memHandle.scene.hSceneScript;
			src.name = name.str();
			src.disassembly = pcode_disassemble(*get_memory(memHandle.scene.hSceneScript));
		}

		if (memHandle.scene.numProcess > 0)
		{
			auto data = get_memory(memHandle.scene.hProcess);
			for (u32 i = 0; i < memHandle.scene.numProcess; ++i)
			{
				u32 pid = read_u32(*data);
				u32 handle = read_u32(*data);

				ostringstream name;
				name << "scene process script " << i << ", pid: "  << hex << setw(4) << right << setfill('0') << pid;

				PcodeScript &src = memHandle.scripts.emplace_back();
				src.handle = handle;
				src.name = name.str();
				src.disassembly = pcode_disassemble(*get_memory(handle));
			}
		}

		for (auto& ent : memHandle.scene.entrances)
		{
			if (ent.hScript != 0)
			{
				ostringstream name;
				name << "entrance " << hex << ent.eNumber << " script";

				PcodeScript &src = memHandle.scripts.emplace_back();
				src.handle = ent.hScript;
				src.name = name.str();
				src.disassembly = pcode_disassemble(*get_memory(ent.hScript));
			}
		}

		for (auto& poly : memHandle.scene.polys)
		{
			if (poly.hScript != 0)
			{
				ostringstream name;
				name << "poly " << hex << poly.id << " script";

				PcodeScript &src = memHandle.scripts.emplace_back();
				src.handle = poly.hScript;
				src.name = name.str();
				src.disassembly = pcode_disassemble(*get_memory(poly.hScript));
			}
		}

		for (auto& actor : memHandle.scene.actors)
		{
			if (actor.hActorCode != 0)
			{
				ostringstream name;
				name << "actor " << hex << actor.id << " script";

				PcodeScript &src = memHandle.scripts.emplace_back();
				src.handle = actor.hActorCode;
				src.name = name.str();
				src.disassembly = pcode_disassemble(*get_memory(actor.hActorCode));
			}
		}
	}
}

void get_rgb(u16 color, u8& r, u8& g, u8& b)
{
	r = ((color >> 11) & 0x1F) << 3;
	g = ((color >> 5)  & 0x3F) << 2;
	b = ((color >> 0)  & 0x1F) << 3;
}

vector<u8> Tinsel::decode_image(Image &image) {
	auto src = get_memory(image.hImgBits);

	vector<u8> result(image.width * image.height * 4, 0);
	u8* dst = result.data();

	if (image.isRLE)
	{
		for (int y = 0; y < image.height; ++y)
		{
			int width = image.width;
			int x = 0;
			while (x < width) {
				int numPixels = read_u16(*src);

				if (numPixels & 0x8000)
				{
					numPixels &= 0x7FFF;
					u16 color = read_u16(*src);
					u8 r,g,b;
					get_rgb(color, r, g, b);
					for (int xp = 0; xp < numPixels; ++xp, ++x)
					{
						*(dst++) = r;
						*(dst++) = g;
						*(dst++) = b;
						*(dst++) = 0;
					}
				}
				else
				{
					for (int xp = 0; xp < numPixels; ++xp, ++x)
					{
						u16 color = read_u16(*src);
						u8 r,g,b;
						get_rgb(color, r, g, b);
						*(dst++) = r;
						*(dst++) = g;
						*(dst++) = b;
						*(dst++) = 0;
					}
				}
			}
		}
	}
	else
	{
		for (int y = 0; y < image.height; ++y)
		{
			for (int x = 0; x < image.width; ++x)
			{
				u16 color = read_u16(*src);
				u8 r,g,b;
				get_rgb(color, r, g, b);

				*(dst++) = r;
				*(dst++) = g;
				*(dst++) = b;
				*(dst++) = 0;
			}
		}
	}
	return result;
}

Image Tinsel::parse_image(u32 handle)
{
	auto data = get_memory(handle);

	Image i {};
	i.handle = handle;

	i.width = read_u16(*data);
	i.height = read_u16(*data);
	i.aniOffX = read_u16(*data);
	i.aniOffY = read_u16(*data);
	i.hImgBits = read_u32(*data);
	i.isRLE = read_u16(*data);
	i.colorFlags = read_u16(*data);

	return i;
}

Frames Tinsel::parse_frame(u32 handle)
{
	auto data = get_memory(handle);

	Frames f {};
	f.handle = handle;

	while(true)
	{
		u32 pFrame = read_u32(*data);
		if (pFrame == 0 || ((pFrame >> 25) >= memHandles.size()))
		{
			break;
		}

		f.images.push_back(parse_image(pFrame));
	}

	return f;
}

MultiInit Tinsel::parse_multi_init(u32 handle)
{
	auto data = get_memory(handle);

	MultiInit mi {};
	mi.handle = handle;
	mi.hMulFrame = read_u32(*data);
	mi.mulFlags = read_i32(*data);
	mi.mulID = read_i32(*data);
	mi.mulX = read_i32(*data);
	mi.mulY = read_i32(*data);
	mi.mulZ = read_i32(*data);
	mi.otherFlags = read_u32(*data);

	if (mi.hMulFrame != 0)
	{
		mi.frames = parse_frame(mi.hMulFrame);
	}
	return mi;
}

AnimScript Tinsel::parse_anim_script(u32 handle, bool sound)
{
	auto data = get_memory(handle);

	AnimScript animScript {};
	animScript.handle = handle;
	animScript.lines = animscript_disassemble(*data);

	for (auto& line : animScript.lines)
	{
		if (line.hFrame && !sound)
		{
			line.frame = parse_frame(line.hFrame);
		}
	}
	return animScript;
}

Film Tinsel::parse_film(u32 handle)
{
	auto data = get_memory(handle);

	Film film {};
	film.handle = handle;
	film.framerate = read_u32(*data);
	u32 numreels = read_u32(*data);
	film.reels.reserve(numreels);

	for (u32 i = 0; i < numreels; ++i)
	{
		Reel reel {};
		reel.handle = i;
		reel.mobj = read_u32(*data);
		reel.script = read_u32(*data);

		reel.obj = parse_multi_init(reel.mobj);
		reel.animScript = parse_anim_script(reel.script, reel.obj.mulID == -2);

		film.reels.push_back(reel);
	}
	return film;
}


void Tinsel::load_strings()
{
	ifstream input {"data/english.txt", ios::binary | ios::ate };
	size_t size = input.tellg();
	input.seekg(0);

	MemHandle &memHandle = memHandles.emplace_back();
	memHandle.id = memHandles.size() - 1;
	memHandle.name = "english.txt";
	memHandle.size = size;
	memHandle.flags = 0;
	memHandle.loaded = true;
	memHandle.data.resize(size);

	input.read((char*)memHandle.data.data(), size);

	load_chunks(memHandle.id);

	stringsId = memHandle.id;
}

string Tinsel::get_string(u32 id)
{
	MemHandle &memHandle = memHandles[stringsId];
	u8* data = memHandle.data.data();

	u32 chunkSkip = id / 64;
	u32 strSkip = id % 64;
	u32 index = 0;

	while (chunkSkip-- != 0)
	{
		u32 nextIndex = *(u32*)(data + index + 4); // skip chunk type
		if (nextIndex == 0)
		{
			return "";
		}
		index = nextIndex;
	}

	data += index + 8; // skip chunk type and size

	while (strSkip-- != 0)
	{
		if ((*data & 0x80) == 0)
		{
			data += *data + 1;
		}
		else if (*data == 0x80)
		{
			data++;
			data += *data + 1;
		}
		else if (*data == 0x90)
		{
			data++;
			data += *data + 1 + 256;
		}
		else
		{
			int subCount;

			subCount = *data & ~0x80;
			data++;

			while (subCount--)
			{
				if (*data == 0x80)
				{
					data++;
					data += *data + 1;
				}
				else if (*data == 0x90)
				{
					data++;
					data += *data + 1 + 256;
				}
				else
				{
					data += *data + 1;
				}
			}
		}
	}

	u32 len = *data;
	if (len == 0x80)
	{
		data++;
		len = *data + 1;
	}
	else if (len == 0x90)
	{
		data++;
		len += *data + 1 + 256;
	}
	return string ((char*)(data + 1), len);
}
