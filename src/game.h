#include <cstdint>
#include <raylib.h>
#include <vector>

#define JAM_BLACK CLITERAL(Color){27, 28, 51}
#define JAM_WHITE CLITERAL(Color){253, 253, 248}
#define JAM_PURPLE CLITERAL(Color){123, 83, 173}
#define JAM_BLUE CLITERAL(Color){45, 147, 221}
#define JAM_RED CLITERAL(Color){211, 59, 32}
#define JAM_ORANGE CLITERAL(Color){218, 125, 34}
#define JAM_YELLOW CLITERAL(Color){230, 218, 41}
#define JAM_GREEN CLITERAL(Color){40, 198 65}

enum class GridElementType {
	UNOCCUPIED,
	STATION,
	LINE
};

enum class LineDirs {
	VERT,
	HORZ,
	L_SHAPE,
	R_SHAPE,
	RL_SHAPE,
	RR_SHAPE,
};

struct GridElement {
	GridElementType type;
	union {
		struct {
			Color col;
			uint32_t pairID;
			bool survivedTimer;
		} station;
		struct {
			LineDirs dir;
			Color col;
			uint32_t pairID;
		} line;
	} data;
};

struct Grid {
	struct {
		int x, y;
	} size;
	std::vector<std::vector<GridElement>> elements;	
};

struct Game {
	Grid grid;


};
