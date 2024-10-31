#include "game.h"
#include <cmath>
#include <cstdlib>
#include <raylib.h>

Rectangle GetCameraView(Camera2D cam) {
	Vector2 screenInWorldStart = GetScreenToWorld2D((Vector2) { 0, 0 }, cam);
	Vector2 screenInWorldEnd = GetScreenToWorld2D((Vector2) { (float)GetScreenWidth(), (float)GetScreenHeight() }, cam);
	return { screenInWorldStart.x, screenInWorldStart.y,
		screenInWorldEnd.x - screenInWorldStart.x,
		screenInWorldEnd.y - screenInWorldStart.y };
}

Grid::Grid() {
	elements.resize(5);
	for(auto& i: elements) i.resize(5);
	size = {5,5};

	cam = {
		.offset = {800, 400},
		.target = {25, 25},
		.rotation = 0,
		.zoom = 6,
	};
}

void Grid::resizeGrid(int x, int y) {
	elements.resize(y);
	for(auto& i: elements) {
		i.resize(x);
	}
	size = {x, y};
}

void Grid::reset() {
	size = {5, 5};
	elements.resize(5);
	for(auto& i: elements) {
		i.clear();
		i.resize(5);
	}
}

void Grid::eraseProposedLines(std::vector<Vector2>& lines) {
	int size = lines.size();
	for(int i = 0; i < size; i++) {
		elements V2IDX(lines[0]).type = GridElementType::UNOCCUPIED;
		lines.erase(lines.begin());
	}
}

bool Grid::populate(int pairs) {
	std::vector<std::pair<int, int>> freeSpaces;
	for(int y = 0; y < elements.size(); y++) {
		for(int x = 0; x < elements[y].size(); x++) {
			if(elements[y][x].type == GridElementType::UNOCCUPIED) {
				if(!CheckCollisionPointRec({(float)x,(float)y}, areaOfPopulation)) {
					continue;
				}

				int spaceNeeded = 1;
				int spaceFound = 0;
				if(x != 0) {
					if(elements[y][x-1].type == GridElementType::UNOCCUPIED) spaceFound++;
				}
				if(y != 0) {
					if(elements[y-1][x].type == GridElementType::UNOCCUPIED) spaceFound++;
				}
				if(y != size.y-1) {
					if(elements[y+1][x].type == GridElementType::UNOCCUPIED) spaceFound++;
				}
				if(x != size.x-1) {
					if(elements[y][x+1].type == GridElementType::UNOCCUPIED) spaceFound++;
				}
				if(spaceFound < spaceNeeded) continue;
				freeSpaces.push_back({x, y});
			}
		}
	}
	if(freeSpaces.size() < 4) return false;
	int colIndex = 2 + (rand()%6);
	for(int i = 0; i < pairs * 2; i++) {
		if(freeSpaces.size() == 0) return false;
		int index = rand() % freeSpaces.size();
		auto& pos = freeSpaces[index];
		elements[pos.second][pos.first].type = GridElementType::STATION;
		elements[pos.second][pos.first].data.station = {
			.col = palette[colIndex],
			.pairID = pairIDCounter,
			.connectedToPair = false,
			.connectsToAll = false,
		};
		freeSpaces.erase(freeSpaces.begin() + index);
		if(i % 2 == 1) {
			colIndex = 2 + (rand()%6);
			pairIDCounter++;
		}
	}
	return true;
}

void Grid::draw(Game& game) {

	Rectangle view = GetCameraView(cam);
	view.x -= 10;
	view.y -= 10;
	view.width += 20;
	view.height += 20;
	for(int y = 0; y < size.y; y++) {
		for(int x = 0; x < size.x; x++) {
			if(!CheckCollisionRecs(view, {(float)x*10, (float)y*10, 10, 10})) continue;
			if(elements[y][x].type == GridElementType::STATION) {
				if(elements[y][x].data.station.connectedToPair)
					DrawRectangle(x*10, y*10, 10, 10, elements[y][x].data.station.col);
				else
					DrawRectangleLinesEx({(float)x*10, (float)y*10, 10, 10}, 3, 
							elements[y][x].data.station.col);
				if(
						game.proposingLine &&
							elements V2IDX(game.proposedLineStart).data.station.pairID
							==
							elements[y][x].data.station.pairID
						) {
					DrawCircleLines(
							x*10+5, y*10+5,
							3 + ((sinf(GetTime()) + 1 / 2) * 2), 
							JAM_WHITE);
				}
			}
		}
	}
	for(int y = 0; y < size.y; y++) {
		for(int x = 0; x < size.x; x++) {
			DrawRectangleLinesEx({(float)x*10, (float)y*10, 10, 10}, 0.5, JAM_WHITE);
			if(elements[y][x].type == GridElementType::STATION) {
				if(
						game.proposingLine &&
							elements V2IDX(game.proposedLineStart).data.station.pairID
							==
							elements[y][x].data.station.pairID
						) {
					DrawCircleLines(
							x*10+5, y*10+5,
							3 + ((sinf(GetTime()) + 1 / 2) * 2), 
							JAM_WHITE);
				}
			}
			if(elements[y][x].type == GridElementType::LINE) {
				switch(elements[y][x].data.line.dir) {
					case LineDirs::DEBUG_SHAPE:
						//DrawRectangle(x*10 + 3, y*10 + 3, 4, 4, elements[y][x].data.line.col);
						break;
					case LineDirs::HORZ:
						DrawRectangle(x*10, y*10 + 3, 10, 4, elements[y][x].data.line.col);
						break;
					case LineDirs::VERT:
						DrawRectangle(x*10 + 3, y*10, 4, 10, elements[y][x].data.line.col);
						break;
					case LineDirs::R_SHAPE:
						DrawRectangle(x*10 + 3, y*10, 4, 6, elements[y][x].data.line.col);
						DrawRectangle(x*10 + 3, y*10 + 3, 8, 4, elements[y][x].data.line.col);
						break;
					case LineDirs::L_SHAPE:
						DrawRectangle(x*10 + 3, y*10 + 3, 4, 8, elements[y][x].data.line.col);
						DrawRectangle(x*10 + 3, y*10 + 3, 8, 4, elements[y][x].data.line.col);
						break;
					case LineDirs::RL_SHAPE:
						DrawRectangle(x*10 + 3, y*10 + 3, 4, 8, elements[y][x].data.line.col);
						DrawRectangle(x*10, y*10 + 3, 7, 4, elements[y][x].data.line.col);
						break;
					case LineDirs::RR_SHAPE:
						DrawRectangle(x*10 + 3, y*10 - 2, 4, 8, elements[y][x].data.line.col);
						DrawRectangle(x*10, y*10 + 3, 7, 4, elements[y][x].data.line.col);
						break;

					default:
						break;
				}
			}
#if DEBUG
			switch(elements[y][x].type) {

				case GridElementType::UNOCCUPIED:
					break;
				case GridElementType::STATION:
					//DrawText("S", x*10, y*10, 10, JAM_WHITE);
					break;
				case GridElementType::LINE:
#if 0
					DrawText("L", x*10, y*10, 10, JAM_WHITE);
#else
					switch(elements[y][x].data.line.dir) {
						case LineDirs::VERT:
							DrawTextEx(GetFontDefault(), "V",
									{(float)x*10, (float)y*10}, 5, 1, JAM_WHITE);
							break;
						case LineDirs::HORZ:
							DrawTextEx(GetFontDefault(), "H",
									{(float)x*10, (float)y*10}, 5, 1, JAM_WHITE);
							break;
						case LineDirs::L_SHAPE:
							DrawTextEx(GetFontDefault(), "L",
									{(float)x*10, (float)y*10}, 5, 1, JAM_WHITE);
							break;
						case LineDirs::R_SHAPE:
							DrawTextEx(GetFontDefault(), "R",
									{(float)x*10, (float)y*10}, 5, 1, JAM_WHITE);
							break;
						case LineDirs::RL_SHAPE:
							DrawTextEx(GetFontDefault(), "RL",
									{(float)x*10, (float)y*10}, 5, 1, JAM_WHITE);
							break;
						case LineDirs::RR_SHAPE:
							DrawTextEx(GetFontDefault(), "RR",
									{(float)x*10, (float)y*10}, 5, 1, JAM_WHITE);
							break;
						case LineDirs::DEBUG_SHAPE:
							break;
					}
#endif
					break;
            }
#endif
		}
	} 
}

float Grid::stationLineRatio() {
	int stations = 0, lines = 0;
	for(auto& i: elements) {
		for(auto& j: i) {
			if(j.type == GridElementType::LINE) lines++;
			if(j.type == GridElementType::STATION) stations++;
		}
	}
	if(lines == 0 || stations == 0) return 0;
	return (float)stations/(float)lines;
}

GridElement::GridElement() {
	type = GridElementType::UNOCCUPIED;
}
