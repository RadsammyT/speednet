#include "game.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <raylib.h>
#include <stdexcept>

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
		if(freeSpaces.size() == 0) break;;
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

void Grid::eraseAllOfPair(int pairID) {
	std::vector<Vector2> elementsOfPair; // excludes portals
	std::vector<Vector2> portalsOfPair;
	for(int y = 0; y < size.y; y++) {
		for(int x = 0; x < size.x; x++) {
			if(elements[y][x].type == GridElementType::STATION) {
				if(elements[y][x].data.station.connectsToAll &&
						elements[y][x].data.station.pairID == pairID) {
					portalsOfPair.push_back({(float)x,(float)y});
				} else {
					if(elements[y][x].data.station.pairID != pairID) continue;
					elementsOfPair.push_back({(float)x,(float)y});
				}
			}
			if(elements[y][x].type == GridElementType::LINE) {
				if(elements[y][x].data.line.pairID == pairID)
					elementsOfPair.push_back({(float)x,(float)y});
			}
		}
	}
	assert(portalsOfPair.size() <= 2);
	bool erasePortals;
	if(portalsOfPair.size() >= 2)
		erasePortals = elements V2IDX(portalsOfPair[0]).data.station.connectedToPair ||
			elements V2IDX(portalsOfPair[1]).data.station.connectedToPair;
	else erasePortals = false;
	if(erasePortals) eraseProposedLines(portalsOfPair);
	eraseProposedLines(elementsOfPair);
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
				if(elements[y][x].data.station.connectsToAll) {
					DrawTexturePro(game.assets.portalPair, ASSET_ICON_REC,
							{(float)x*10, (float)y*10, 10, 10}, {0,0}, 0, WHITE);
					if(!elements[y][x].data.station.connectedToPair)
						DrawRectangle((float)x*10+3, (float)y*10+3, 4, 4, JAM_BLACK);
					continue;
				}
				if(elements[y][x].data.station.connectedToPair)
					DrawRectangle(x*10, y*10, 10, 10, elements[y][x].data.station.col);
				else
					DrawRectangleLinesEx({(float)x*10, (float)y*10, 10, 10}, 3, 
							elements[y][x].data.station.col);
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
							&& !elements[y][x].data.station.connectedToPair
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

bool Grid::inDanger() {
	for(auto& i: elements) {
		for(auto& j: i) {
			if(j.type == GridElementType::STATION)
				if(!j.data.station.connectedToPair)
					return true;
		}
	}
	return false;
}

void Grid::flood() {
	for(auto& i: elements) {
		for(auto& j: i) {
			j.type = GridElementType::STATION;
			j.data.station = {
				.pairID = UINT32_MAX,
				.connectedToPair = true,
				.connectsToAll = false,
			};
		}
	}
}

int Grid::numOfStations(bool countConnected) {
	int stations = 0;
	for(auto& i: elements) {
		for(auto& j: i) {
			if(j.type == GridElementType::STATION) {
				if(countConnected) {
					if(j.data.station.connectedToPair)
						stations++;
				} else stations++;
			}
		}
	}
	return stations;
}

void Grid::bindPortalToPair(int portalID, int pairID) {
	Color pairColor = WHITE;
	for(auto& i: elements) {
		for(auto& j: i) {
			if(j.type == GridElementType::STATION) {
				if(j.data.station.connectsToAll && j.data.station.portalID == portalID) {
					j.data.station.pairID = pairID;
				} else {
					if(j.data.station.pairID == pairID) {
						pairColor = j.data.station.col;
					}
				}
			}
		}
	}
	for(auto& i: elements) {
		for(auto& j: i) {
			if(j.type == GridElementType::LINE) {
				if(j.data.line.pairID == pairID) {
					j.data.line.col = pairColor;
				}
			}
		}
	}
}

void Grid::ensureNoSingularPair() {
	std::map<int, std::vector<Vector2>> pairToObj;
	for(int y = 0; y < size.y; y++) {
		for(int x = 0; x < size.x; x++) {
			int pairID;
			if(elements[y][x].type == GridElementType::STATION) pairID = elements[y][x].data.station.pairID;
			if(elements[y][x].type == GridElementType::LINE) pairID = elements[y][x].data.line.pairID;
			if(pairToObj.find(pairID) != pairToObj.end()) {
				pairToObj.at(pairID).push_back({(float)x,(float)y});
			} else {
				pairToObj.insert({pairID, {}});
				pairToObj.at(pairID).push_back({(float)x,(float)y});
			}
		}
	}
	for(auto& [k, v]: pairToObj) {
		if(v.size() == 1) {
			eraseProposedLines(v);
		}
	}
}

float Grid::stationLineRatio() {
	int stations = 0, lines = 0;
	for(auto& i: elements) {
		for(auto& j: i) {
			if(j.type == GridElementType::LINE) lines++;
			if(j.type == GridElementType::STATION) {
				if(j.data.station.connectsToAll) continue;
				if(!j.data.station.connectedToPair) continue;
				stations++;
			} 
		}
	}
	if(lines == 0 || stations == 0) return 0;
	return (float)stations/(float)lines;
}

GridElement::GridElement() {
	type = GridElementType::UNOCCUPIED;
}
