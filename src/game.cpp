#include <cassert>
#include <cmath>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <vector>
#include "game.h"

LineDirs findDir(Vector2 tail, Vector2 middle, Vector2 head) {
	//<Up> <Down>
	if(
		DIRCOMP(middle, 0, 1, 0, -1, head, tail)
	) return LineDirs::VERT;

	//<Left> <Right>
	if(
		DIRCOMP(middle, -1, 0, 1, 0, head, tail)	
	) return LineDirs::HORZ;

	// <Up> <Right>
	if(
		DIRCOMP(middle, 0, 1, 1, 0, head, tail)
	) return LineDirs::L_SHAPE;

	if(
		DIRCOMP(middle, 0, -1, 1, 0, head, tail)
	) return LineDirs::R_SHAPE;

	if(
		DIRCOMP(middle, 0, 1, -1, 0, head, tail)
	) return LineDirs::RL_SHAPE;

	if(
		DIRCOMP(middle, 0, -1, -1, 0, head, tail)
	) return LineDirs::RR_SHAPE;

	//UNREACHABLE
	return LineDirs::DEBUG_SHAPE;
}

Assets::Assets() {
	coin = LoadSound("res/web/coin.wav");
}

void OnInGame(Game &game) {
	ClearBackground(JAM_BLACK);
	if(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
		game.grid.cam.target = 
			Vector2Add(
					game.grid.cam.target, 
					Vector2Scale(
						Vector2Rotate( 
						GetMouseDelta(),
							180*DEG2RAD
						),
						1/game.grid.cam.zoom
					)
			);
	}
	Vector2 mouseGridPos = GetScreenToWorld2D(GetMousePosition(), game.grid.cam);
	if(GetMouseWheelMove() != 0) {
		game.grid.cam.zoom += GetMouseWheelMove();
	}
	Vector2 hoveredCell = {
		floorf(Clamp(mouseGridPos.x, 0, (game.grid.size.x-1)*10)/10),
		floorf(Clamp(mouseGridPos.y, 0, (game.grid.size.y-1)*10)/10),
	};
	if(!game.proposingLine) {
		if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !game.lineAccepted) {
			if(game.grid.elements[(int)hoveredCell.y][(int)hoveredCell.x].type
					== GridElementType::STATION) {
				if(game.grid.elements V2IDX(hoveredCell).data.station.connectedToPair) 
					goto skip;
				game.proposingLine = true;
				game.proposedLineStart = hoveredCell;
				game.currentHeldCell = hoveredCell;
				game.previousHeldCell = hoveredCell;
			}
			game.proposedLine.clear();
		} else {
			if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) game.lineAccepted = false;
			game.proposedLine.clear();
		}
	} else {
		if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {

			if(!Vector2Equals(hoveredCell, game.currentHeldCell)) {
				if(Vector2Equals(hoveredCell, game.previousHeldCell)) {
					game.proposingLine = false;
					game.grid.eraseProposedLines(game.proposedLine);
					goto skip;
				}
				if(game.grid.elements V2IDX(hoveredCell).type != GridElementType::UNOCCUPIED) {
					// check if station, or line
					if(game.grid.elements V2IDX(hoveredCell).type == GridElementType::LINE) {
						game.proposingLine = false;
						game.grid.eraseProposedLines(game.proposedLine);
						goto skip;
					}
					if(game.grid.elements V2IDX(hoveredCell).type == GridElementType::STATION) {
						if(Vector2Equals(game.proposedLineStart, hoveredCell)) {
							game.proposingLine = false;
							game.grid.eraseProposedLines(game.proposedLine);
							goto skip;
						}
						if( game.grid.elements 
								V2IDX(hoveredCell).data.station.pairID ==
							game.grid.elements 
								V2IDX(game.proposedLineStart).data.station.pairID) {
							 game.grid.elements 
								V2IDX(hoveredCell).data.station.connectedToPair = true;
							 game.grid.elements 
								V2IDX(game.proposedLineStart).data.station.connectedToPair = true;
							 game.proposingLine = false;
							 game.lineAccepted = true;
							 game.proposedLine.clear();
						} else {
							game.proposingLine = false;
							game.grid.eraseProposedLines(game.proposedLine);
							goto skip;
						}
					}
				}
				if(game.grid.elements V2IDX(game.currentHeldCell).type != GridElementType::STATION) {
					game.proposedLine.push_back(game.currentHeldCell);
					game.grid.elements V2IDX(game.currentHeldCell).type = GridElementType::LINE;
					if(findDir(game.previousHeldCell, game.currentHeldCell, hoveredCell) == 
							LineDirs::DEBUG_SHAPE) {
						game.proposingLine = false;
						game.grid.eraseProposedLines(game.proposedLine);
						goto skip;
					}
					game.grid.elements V2IDX(game.currentHeldCell).data = {
						.line = {
							.dir = findDir(game.previousHeldCell, game.currentHeldCell, hoveredCell),
							.col = game.grid.elements V2IDX(game.proposedLineStart).data.station.col
						}
					};
				}
					game.previousHeldCell = game.currentHeldCell;
					game.currentHeldCell = hoveredCell;
			}
		} else {
			game.proposingLine = false;
			game.grid.eraseProposedLines(game.proposedLine);
			goto skip;
		}
	}
skip:
#if DEBUG
	if(IsKeyPressed(KEY_R)) {
		if(game.grid.size.x == game.grid.size.y) {
			game.grid.resizeGrid(game.grid.size.x * 2, game.grid.size.y);
		} else {
			game.grid.resizeGrid(game.grid.size.x, game.grid.size.y * 2);
		}
	}
	if(IsKeyPressed(KEY_P)) {
		game.grid.populate(1);
	}
#endif

	game.currentTime -= GetFrameTime();
	if(game.currentTime <= 0){
		if(!game.grid.populate(game.pairsOnTimer)) {
			if(game.grid.size.x == game.grid.size.y) {
				game.grid.resizeGrid(game.grid.size.x * 2, game.grid.size.y);
			} else {
				game.grid.resizeGrid(game.grid.size.x, game.grid.size.y * 2);
			}
			game.currentMaxTime +=
				sqrtf(game.currentMaxTime) * ((float)game.resizeCycle/sqrtf((float)game.resizeCycle));
			game.resizeCycle++;
			game.pairsOnTimer++;
		}
		game.currentTime = game.currentMaxTime;
	}
	DrawCircleSector({(float)GetScreenWidth()-50, 50}, 45, 0-90,
			(360-90) * game.currentTime/game.currentMaxTime, 100, JAM_WHITE);
	DRAWTEXTCENTER(std::to_string(game.currentTime).c_str(),
			GetScreenWidth()-50, 50, 10, JAM_BLACK);
	BeginMode2D(game.grid.cam);
		DrawRectangle(hoveredCell.x*10, hoveredCell.y*10, 10, 10, JAM_WHITE);
		game.grid.draw(game);
	EndMode2D();
}

void OnMainMenu(Game& game) {
	if(IsKeyPressed(KEY_ENTER)) {
		game.mgs = MainGameState::INGAME;
		game.currentMaxTime = 10;
		game.currentTime = 10;
		game.grid.reset();
		game.grid.populate(2);
	}
	ClearBackground(JAM_BLACK);
	DRAWTEXTCENTER("SpeedNet", GetScreenWidth()/2, 100, 30, JAM_WHITE);
	DRAWTEXTCENTER("press Enter to begin", GetScreenWidth()/2, 800-150, 30, JAM_WHITE);
	BeginMode2D({
		.offset = {800, 400},
		.target = {25, 25},
		.rotation = 0,
		.zoom = 6,
	});
		game.grid.draw(game);
	EndMode2D();
}

void OnPaletteTest(Game& game, Color* palette) {
#if SKIP_PALETTE
	game.mgs = MainGameState::MAINMENU;
#endif
	if(GetTime() >= 5) {
		game.mgs = MainGameState::MAINMENU;
		game.grid.elements[2][2].type = GridElementType::STATION;
		game.grid.elements[2][2].data.station = {
			.col = JAM_RED,
			.pairID = 0,
			.survivedTimer = false
		};
		game.grid.elements[2][3].type = GridElementType::LINE;
		game.grid.elements[2][3].data.line = {
			.dir = LineDirs::DEBUG_SHAPE,
			.col = JAM_RED,
			.pairID = 0,
		};
	}
		if(GetTime() >= 4)
			ClearBackground(JAM_BLACK);
		else
			ClearBackground(palette[(int)Clamp((GetTime()-1)/0.3f, 0, 7)]);
		for(int i = 0; i < 8; i++) {
			DrawRectangle(i * 80 + (800 - 80*4), 360, 80, 80, palette[i]);
		}
		DRAWTEXTCENTER("made in raylib", GetScreenWidth()/2, 380, 30, JAM_WHITE);
	
}
