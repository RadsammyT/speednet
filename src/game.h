#pragma once
#include <cstdint>
#include <memory>
#include <raylib.h>
#include <stdexcept>
#include <string>
#include <vector>

#define JAM_BLACK	CLITERAL(Color){27, 28, 51, 255}
#define JAM_WHITE	CLITERAL(Color){253, 253, 248, 255}
#define JAM_PURPLE	CLITERAL(Color){123, 83, 173, 255}
#define JAM_BLUE	CLITERAL(Color){45, 147, 221, 255}
#define JAM_RED		CLITERAL(Color){211, 59, 32, 255}
#define JAM_ORANGE	CLITERAL(Color){218, 125, 34, 255}
#define JAM_YELLOW	CLITERAL(Color){230, 218, 41, 255}
#define JAM_GREEN	CLITERAL(Color){40, 198, 65, 255}

#define DRAWTEXTCENTER(str, x, y, size, color) DrawText(str, x-(MeasureText(str,size)/2), y, size, color)
#define V2IDX(vec) [(int)vec.y][(int)vec.x]

#define DIRCOMP(vec, x1, y1, x2, y2, head, tail) (Vector2Equals({vec.x + x1, vec.y + y1}, tail) \
		&& Vector2Equals({vec.x + x2, vec.y + y2}, head)) \
	|| (Vector2Equals({vec.x + x1, vec.y + y1}, head) \
		&& Vector2Equals({vec.x + x2, vec.y + y2}, tail))

//BUILD CONFIGS
#define SKIP_PALETTE 1
#define DEBUG 1

static Color palette[8] = {
	JAM_BLACK,	
	JAM_WHITE,	
	JAM_PURPLE,	
	JAM_BLUE,	
	JAM_RED	,	
	JAM_ORANGE,	
	JAM_YELLOW,
	JAM_GREEN,
};

enum class ItemType {
	PORTAL_PAIR,
	BULLDOZER,
	MAGIC_WATCH
};

enum class GridElementType {
	UNOCCUPIED,
	STATION,
	LINE
};

enum class LineDirs {
	VERT, // <Up> TO <Down>
	HORZ, // <Left> TO <Right>
	L_SHAPE, // <Up> TO <Right>
	R_SHAPE, // <Down> TO <Right>
	RL_SHAPE,// <Up> TO <Left>
	RR_SHAPE,// <Down> TO <Left>
	DEBUG_SHAPE
};

enum class MainGameState {
	MAINMENU,
	INGAME,
	GAMEOVER,
	PALETTETEST
};

struct Assets {
	Texture 
		portalPair,
		bulldozer,
		magicWatch,
		timeUpgrade;

	Sound
		timeWarning,
		coin;

	Assets();
};

struct GridElement {
	GridElementType type;
	union {
		struct {
			Color col;
			uint32_t pairID;
			bool survivedTimer;
			bool connectedToPair;
			bool connectsToAll;
		} station;
		struct {
			LineDirs dir;
			Color col;
			uint32_t pairID;
		} line;
	} data;

	GridElement(); /* {
		type = GridElementType::UNOCCUPIED;
	}*/
};


struct Item {
	ItemType type;
	int quantity;
	bool modified;
};

struct Game;

struct Grid {
	struct {
		int x, y;
	} size;
	std::vector<std::vector<GridElement>> elements;	
	Camera2D cam;

	uint32_t pairIDCounter = 0;

	void resizeGrid(int x, int y);
	void eraseProposedLines(std::vector<Vector2>& lines);
	void reset();
	bool populate(int pairs);
	void draw(Game& game);

	float stationLineRatio();

	Grid(); /*{
		elements.resize(5);
		for(auto& i: elements) i.resize(5);
	}*/
};

struct Game {


	MainGameState mgs;

	Grid grid;

	Item items[3];

	Assets assets;

	float currentMaxTime;
	float currentTime;

	int pairsOnTimer = 1;
	int resizeCycle = 1;
	
	Vector2 previousHeldCell = {0, 0};
	Vector2 currentHeldCell = {0, 0};
	Vector2 proposedLineStart = {0, 0};
	bool proposingLine = false;
	bool lineAccepted = false;

	std::vector<Vector2> proposedLine;
};

void OnMainMenu(Game& game);
void OnInGame(Game& game);
void OnGameOver(Game& game);
void OnPaletteTest(Game& game, Color* palette);

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}
