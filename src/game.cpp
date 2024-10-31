#include <cassert>
#include <cmath>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
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

	portalPair = LoadTexture("res/web/portal-icon.png");
	bulldozer = LoadTexture("res/web/bulldozer-icon.png");
	magicWatch = LoadTexture("res/web/magic-watch-icon.png");
	shopIcon = LoadTexture("res/web/shop-cart-icon.png");
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
	game.grid.cam.zoom = Clamp(game.grid.cam.zoom, 1.5, 6);
	Vector2 hoveredCell = {
		floorf(Clamp(mouseGridPos.x, 0, (game.grid.size.x-1)*10)/10),
		floorf(Clamp(mouseGridPos.y, 0, (game.grid.size.y-1)*10)/10),
	};
	if(game.usingItem != ItemType::NONE) {

	}
	if(!game.inShop && !game.forbidLineProposal && game.usingItem == ItemType::NONE) {
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
									V2IDX(game.proposedLineStart).data.station.pairID ||
									game.grid.elements V2IDX(hoveredCell)
									.data.station.connectsToAll){
								 game.grid.elements 
									V2IDX(hoveredCell).data.station.connectedToPair = true;
								 if(game.grid.elements
										 V2IDX(hoveredCell).data.station.connectsToAll) {
										 game.grid.elements V2IDX(hoveredCell)
											 .data.station.pairID =
											 game.grid.elements V2IDX(game.proposedLineStart)
											 .data.station.pairID; 
								 }
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
					if(game.grid.elements V2IDX(game.currentHeldCell).type !=
							GridElementType::STATION) {
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
								.dir =
									findDir(game.previousHeldCell, game.currentHeldCell, hoveredCell),
								.col =
									game.grid.elements V2IDX(game.proposedLineStart).data.station.col,
								.pairID =
									game.grid.elements
									V2IDX(game.proposedLineStart).data.station.pairID
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

	if(!game.inShop)
		game.currentTime -= GetFrameTime();
	else
		game.currentTime -= GetFrameTime()/10;
	if(game.currentTime <= 0){
		game.proposingLine = false;
		game.grid.eraseProposedLines(game.proposedLine);

		if(!game.grid.populate(game.pairsOnTimer)) {
			if(game.grid.size.x == game.grid.size.y) {
				game.grid.areaOfPopulation.x = game.grid.size.x;
				game.grid.areaOfPopulation.y = 0;
				game.grid.areaOfPopulation.height = game.grid.size.y * 2;
				game.grid.resizeGrid(game.grid.size.x * 2, game.grid.size.y);
			} else {
				game.grid.areaOfPopulation.y = game.grid.size.y;
				game.grid.areaOfPopulation.x = 0;
				game.grid.areaOfPopulation.width = game.grid.size.x * 2;
				game.grid.resizeGrid(game.grid.size.x, game.grid.size.y * 2);
			}
			game.currentMaxTime +=
				sqrtf(game.currentMaxTime) * ((float)game.resizeCycle/sqrtf((float)game.resizeCycle));
			game.resizeCycle++;
			game.pairsOnTimer++;
			game.coins += (int)(game.grid.stationLineRatio() * 100)/2;
			game.grid.populate(game.pairsOnTimer);
		}
		game.currentTime = game.currentMaxTime;
	}

	
	BeginMode2D(game.grid.cam);
		DrawRectangle(hoveredCell.x*10, hoveredCell.y*10, 10, 10, JAM_WHITE);
		game.grid.draw(game);
	EndMode2D();

	for(int i = 0; i < 3; i++) {
		bool hovered = CheckCollisionPointRec(GetMousePosition(), {
			(float)GetScreenWidth()-200-(i*100), 5, 90, 150
		});
		DrawRectangleRec({(float)GetScreenWidth()-200-(i*100),
				5, 90, 90}, JAM_WHITE);
		bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
		switch(game.items[i].type) {
			case ItemType::PORTAL_PAIR:
				DrawTexturePro(game.assets.portalPair, ASSET_ICON_REC,
						{(float)GetScreenWidth()-200-(i*100), 5, 90, 90}, {0,0}, 0, WHITE);
				if(clicked && game.items[i].quantity > 0)
					game.usingItem = ItemType::PORTAL_PAIR;
				break;
			case ItemType::BULLDOZER:
				DrawTexturePro(game.assets.bulldozer, ASSET_ICON_REC,
						{(float)GetScreenWidth()-200-(i*100), 5, 90, 90}, {0,0}, 0, WHITE);
				if(clicked && game.items[i].quantity > 0)
					game.usingItem = ItemType::BULLDOZER;
				break;
			case ItemType::MAGIC_WATCH:
				DrawTexturePro(game.assets.magicWatch, ASSET_ICON_REC,
						{(float)GetScreenWidth()-200-(i*100), 5, 90, 90}, {0,0}, 0, WHITE);
				if(clicked && game.items[i].quantity > 0)
					game.usingItem = ItemType::MAGIC_WATCH;
			case ItemType::NONE:
				break;
		}
		DrawRectangleRounded({(float)GetScreenWidth()-200-(i*100), 105, 90, 40}, 0.5, 6,
			hovered ? JAM_BLACK : JAM_WHITE 
		);
		if(hovered)
			DrawRectangleRoundedLinesEx({(float)GetScreenWidth()-200-(i*100), 105, 90, 40},
			0.5, 6, 1.0f, JAM_WHITE);
		DRAWTEXTCENTER(string_format("%d", game.items[i].quantity).c_str(), 
				((float)GetScreenWidth()-200-(i*100)+45), 
				105+1, 
				40, 
				hovered ? JAM_WHITE : JAM_BLACK);
	}

	if(CheckCollisionPointRec(GetMousePosition(), SHOP_ICON_REC) || game.inShop) {
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
				&& CheckCollisionPointRec(GetMousePosition(), SHOP_ICON_REC))
			game.inShop = !game.inShop;
		DrawRectangleRounded(SHOP_ICON_REC, 0.5, 6, JAM_BLACK);
		DrawRectangleRoundedLinesEx(SHOP_ICON_REC, 0.5, 6, 2, JAM_WHITE);
	} else {
		DrawRectangleRounded(SHOP_ICON_REC, 0.5, 6, JAM_WHITE);
		DrawRectangleRoundedLinesEx(SHOP_ICON_REC, 0.5, 6, 2, JAM_BLACK);
	}
	Rectangle shopIconPos = SHOP_ICON_REC;
	shopIconPos.y -= 7;
	DrawTexturePro(game.assets.shopIcon, ASSET_ICON_REC, shopIconPos, {0,0}, 0, WHITE);

	DrawCircle(GetScreenWidth()-50, 50, 45, JAM_RED);
	DrawCircleSector({(float)GetScreenWidth()-50, 50}, 45, 0,
			(360) * game.currentTime/game.currentMaxTime, 100, JAM_WHITE);
	DRAWTEXTCENTER(string_format("%.02f", game.currentTime).c_str(),
			GetScreenWidth()-50, 50, 30, JAM_BLACK);

	if(game.inShop) {
		DrawRectangleRounded(SHOP_MENU_BACKGROUND_REC, 0.5, 6, JAM_WHITE);
		if(CheckCollisionPointRec(GetMousePosition(), SHOP_MENU_PORTALS_REC)) {
			DrawRectangleRounded(SHOP_MENU_PORTALS_REC, 0.5, 6, JAM_WHITE);
			DrawRectangleRoundedLinesEx(SHOP_MENU_PORTALS_REC, 0.5, 6, 3, JAM_PURPLE);
		} else
			DrawRectangleRounded(SHOP_MENU_PORTALS_REC, 0.5, 6, JAM_PURPLE);
		//DrawRectangleRounded(SHOP_MENU_BULLDOZER_REC, 0.5, 6, JAM_ORANGE);
		if(CheckCollisionPointRec(GetMousePosition(), SHOP_MENU_BULLDOZER_REC)) {
			DrawRectangleRounded(SHOP_MENU_BULLDOZER_REC, 0.5, 6, JAM_WHITE);
			DrawRectangleRoundedLinesEx(SHOP_MENU_BULLDOZER_REC, 0.5, 6, 3, JAM_ORANGE);
		} else
			DrawRectangleRounded(SHOP_MENU_BULLDOZER_REC, 0.5, 6, JAM_ORANGE);
		DrawRectangleRounded(SHOP_MENU_MWATCH_REC, 0.5, 6, JAM_BLUE);

		DrawRectangleRec(SHOP_MENU_PORTALS_ICON, JAM_BLACK);
		DrawTexturePro(game.assets.portalPair, ASSET_ICON_REC, SHOP_MENU_PORTALS_ICON, {0,0},
				0.0f, WHITE);
		DrawTexturePro(game.assets.bulldozer, ASSET_ICON_REC, SHOP_MENU_BULLDOZER_ICON, {0,0},
				0.0f, WHITE);
		DrawTexturePro(game.assets.magicWatch, ASSET_ICON_REC, SHOP_MENU_MWATCH_ICON, {0,0},
				0.0f, WHITE);

		//portal text
		DRAWTEXTCENTER(string_format("Portals").c_str(), 300, 220, 25, JAM_BLACK);
		DRAWTEXTCENTER(string_format("Cost: %d", SHOP_PORTAL_COST).c_str(), 300, 365, 25, JAM_BLACK);
		DrawTextEx(GetFontDefault(), "A pair of portals you can place to \"wirelessly\""
				"\nconnect stations. Portals MUST connect to the\nsame pair.", {180, 400}, 10, 1, JAM_BLACK);

		DRAWTEXTCENTER(string_format("Bulldozer").c_str(), 800, 220, 25, JAM_BLACK);
		DRAWTEXTCENTER(string_format("Cost: %d", SHOP_BULLDOZER_COST).c_str(),
				800, 365, 25, JAM_BLACK);
		DrawTextEx(GetFontDefault(), "A literal Bulldozer to erase a station and its pair"
				"\nregardless if its connected or not, lines and all.", {740-75, 400},
				10, 1, JAM_BLACK);

		DRAWTEXTCENTER(string_format("Magic Watch").c_str(), 1300, 220, 25, JAM_BLACK);
		DRAWTEXTCENTER(string_format("Cost: %d", SHOP_MWATCH_COST).c_str(),
				1300, 365, 25, JAM_BLACK);
		DrawTextEx(GetFontDefault(), "A magic watch to give you extra time when you"
				"\nneed it the most. Adds 10 seconds on use.", {1300-125, 400},
				10, 1, JAM_BLACK);
	}
}

void OnMainMenu(Game& game) {
	if(IsKeyPressed(KEY_ENTER)) {
		game.mgs = MainGameState::INGAME;
		game.currentMaxTime = 5;
		game.currentTime = 10;
		game.grid.areaOfPopulation = {0, 0, 5, 5};
		game.grid.reset();
		game.grid.populate(2);

		game.items[0] = {
			.type = ItemType::PORTAL_PAIR,
			.quantity = 0,
		};
		game.items[1] = {
			.type = ItemType::BULLDOZER,
			.quantity = 0,
		};
		game.items[2] = {
			.type = ItemType::MAGIC_WATCH,
			.quantity = 0,
		};
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
		else {
			if(game.paletteNum != (int)Clamp((GetTime()-1)/0.3f, 0, 7)) {
				SetSoundPitch(game.assets.coin, 0.5 + (game.paletteNum * 0.25));
				PlaySound(game.assets.coin);
				game.paletteNum = (int)Clamp((GetTime()-1)/0.3f, 0, 7);
			}
			ClearBackground(palette[(int)Clamp((GetTime()-1)/0.3f, 0, 7)]);
		}
		for(int i = 0; i < 8; i++) {
			DrawRectangle(i * 80 + (800 - 80*4), 360, 80, 80, palette[i]);
		}
		DRAWTEXTCENTER("made in raylib", GetScreenWidth()/2, 380, 30, JAM_WHITE);
	
}
