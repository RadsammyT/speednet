#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
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
	timeWarning = LoadSound("res/web/timer.wav");
	lineStep = LoadSound("res/web/lineStep.wav");
	lineConnect = LoadSound("res/web/lineConnect.wav");
	failLaser = LoadSound("res/web/fail.wav");
	failBoom = LoadSound("res/web/fail2.wav");
	success = LoadSound("res/web/success.wav");
	bulldozerSound = LoadSound("res/web/bulldozer.wav");

	portalPair = LoadTexture("res/web/portal-icon.png");
	bulldozer = LoadTexture("res/web/bulldozer-icon.png");
	magicWatch = LoadTexture("res/web/magic-watch-icon.png");
	shopIcon = LoadTexture("res/web/shop-cart-icon.png");
}

void OnInGame(Game &game) {
	ClearBackground(JAM_BLACK);
	if(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && !game.inShop) {
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
	bool previousInDanger = game.grid.inDanger();
	Vector2 mouseGridPos = GetScreenToWorld2D(GetMousePosition(), game.grid.cam);
	if(GetMouseWheelMove() != 0) {
		game.grid.cam.zoom += GetMouseWheelMove();
	}
	game.grid.cam.zoom = Clamp(game.grid.cam.zoom, 1.5, 6);
	Vector2 hoveredCell = {
		floorf(Clamp(mouseGridPos.x, 0, (game.grid.size.x-1)*10)/10),
		floorf(Clamp(mouseGridPos.y, 0, (game.grid.size.y-1)*10)/10),
	};
	game.smoothHoveredCell.x = Clamp(game.smoothHoveredCell.x, 0, game.grid.size.x-1);
	game.smoothHoveredCell.y = Clamp(game.smoothHoveredCell.y, 0, game.grid.size.y-1);
	if(abs(game.smoothHoveredCell.x - hoveredCell.x) > abs(game.smoothHoveredCell.y - hoveredCell.y))
		game.smoothHoveredCell.x -= floorf(Clamp(game.smoothHoveredCell.x - hoveredCell.x, -1, 1));
	else
		game.smoothHoveredCell.y -= floorf(Clamp(game.smoothHoveredCell.y - hoveredCell.y, -1, 1));
	if(game.usingItem != ItemType::NONE && !game.inShop) {
		switch(game.usingItem) {
                case ItemType::PORTAL_PAIR:
					if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
							&& game.grid.elements V2IDX(game.smoothHoveredCell)
							.type == GridElementType::UNOCCUPIED) {
						game.grid.elements V2IDX(game.smoothHoveredCell)
							.type = GridElementType::STATION;
						game.grid.elements V2IDX(game.smoothHoveredCell)
							.data.station = {
								.col = JAM_PURPLE,
								.pairID = UINT32_MAX,
								.portalID = game.grid.pairIDCounter,
								.connectsToAll = true,
							};
						PlaySound(game.assets.lineStep);
						game.proposedLine.push_back(game.smoothHoveredCell);
						if(game.proposedLine.size() >= 2) {
							game.proposedLine.clear();
							game.grid.pairIDCounter++;
							game.consume(ItemType::PORTAL_PAIR);
							game.usingItem = ItemType::NONE;
							PlaySound(game.assets.lineConnect);
						}
					}
					if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
						game.grid.eraseProposedLines(game.proposedLine);
						game.usingItem = ItemType::NONE;
					}
					break;
                case ItemType::BULLDOZER:
					if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
						game.usingItem = ItemType::NONE;
					}
					if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
						int id = 0;
						auto& cell = game.grid.elements V2IDX(game.smoothHoveredCell);
						if(cell.type == GridElementType::UNOCCUPIED) {
							game.usingItem = ItemType::NONE;
							break;
						}
						if(cell.type == GridElementType::STATION) {
							if(cell.data.station.connectsToAll) {
								break;
							}
							id = cell.data.station.pairID;

						} 
						if(cell.type == GridElementType::LINE) id = cell.data.line.pairID;
						PlaySound(game.assets.bulldozerSound);
						game.grid.eraseAllOfPair(id);
						game.usingItem = ItemType::NONE;
						game.consume(ItemType::BULLDOZER);
					}
					break;
                case ItemType::MAGIC_WATCH:
					game.currentTime += 10;
					game.usingItem = ItemType::NONE;
					game.consume(ItemType::MAGIC_WATCH);
					break;
                case ItemType::NONE:
					assert(game.usingItem != ItemType::NONE);
                	break;
		}
	}
	if(!game.inShop && !game.forbidLineProposal && game.usingItem == ItemType::NONE) {
		if(!game.proposingLine) {
			if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !game.lineAccepted &&
					CheckCollisionPointRec(mouseGridPos, {0,0,
						(float)game.grid.size.x*10,(float)game.grid.size.y*10})) {
				if(game.grid.elements[(int)game.smoothHoveredCell.y][(int)game.smoothHoveredCell.x].type
						== GridElementType::STATION) {
					if(game.grid.elements V2IDX(game.smoothHoveredCell).data.station.connectedToPair ||
							game.grid.elements V2IDX(game.smoothHoveredCell).data.station.connectsToAll) 
						goto skip;
					game.proposingLine = true;
					game.proposedLineStart = game.smoothHoveredCell;
					game.currentHeldCell = game.smoothHoveredCell;
					game.previousHeldCell = game.smoothHoveredCell;
				}
				game.proposedLine.clear();
			} else {
				if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) game.lineAccepted = false;
				game.proposedLine.clear();
			}
		} else {
			if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {

				if(!Vector2Equals(game.smoothHoveredCell, game.currentHeldCell)) {
					if(Vector2Equals(game.smoothHoveredCell, game.previousHeldCell)) {
						game.proposingLine = false;
						game.grid.eraseProposedLines(game.proposedLine);
						goto skip;
					}
					if(game.grid.elements 
							V2IDX(game.smoothHoveredCell).type != GridElementType::UNOCCUPIED) {
						// check if station, or line
						if(game.grid.elements 
								V2IDX(game.smoothHoveredCell).type == GridElementType::LINE) {
							game.proposingLine = false;
							game.grid.eraseProposedLines(game.proposedLine);
							goto skip;
						}
						if(game.grid.elements 
								V2IDX(game.smoothHoveredCell).type == GridElementType::STATION) {
							if(Vector2Equals(game.proposedLineStart, game.smoothHoveredCell)) {
								game.proposingLine = false;
								game.grid.eraseProposedLines(game.proposedLine);
								goto skip;
							}
							if((game.grid.elements 
									V2IDX(game.smoothHoveredCell).data.station.pairID ==
								game.grid.elements 
									V2IDX(game.proposedLineStart).data.station.pairID ||
									game.grid.elements V2IDX(game.smoothHoveredCell)
									.data.station.connectsToAll)
									&& !game.grid.elements V2IDX(game.smoothHoveredCell)
										.data.station.connectedToPair){
								 game.grid.elements 
									V2IDX(game.smoothHoveredCell).data.station.connectedToPair = true;
								if(game.grid.elements
										V2IDX(game.smoothHoveredCell).data.station.connectsToAll &&
											(
											game.grid.elements
											V2IDX(game.smoothHoveredCell).data.station.pairID 
											== UINT32_MAX
											||
											game.grid.elements
											V2IDX(game.smoothHoveredCell).data.station.pairID == 
											game.grid.elements
											V2IDX(game.proposedLineStart).data.station.pairID
											)
											&& !game.grid.elements
											V2IDX(game.proposedLineStart).data.station.connectedToPair
										) {
										game.grid.elements V2IDX(game.smoothHoveredCell)
											.data.station.pairID =
											game.grid.elements V2IDX(game.proposedLineStart)
												.data.station.pairID; 
										game.grid.bindPortalToPair(
												game.grid.elements V2IDX(game.smoothHoveredCell)
												.data.station.portalID, 
												game.grid.elements V2IDX(game.proposedLineStart)
												.data.station.pairID);

								} else if(game.grid.elements V2IDX(game.smoothHoveredCell)
										.data.station.connectsToAll) {
									game.proposingLine= false;
									game.grid.elements 
									V2IDX(game.smoothHoveredCell).data.station.connectedToPair
									= false;
									game.grid.eraseProposedLines(game.proposedLine);
									goto skip;
								}

								PlaySound(game.assets.lineConnect);
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
						if(findDir(game.previousHeldCell,
									game.currentHeldCell, game.smoothHoveredCell) == 
								LineDirs::DEBUG_SHAPE) {
							game.proposingLine = false;
							game.grid.eraseProposedLines(game.proposedLine);
							goto skip;
						}
						game.grid.elements V2IDX(game.currentHeldCell).data = {
							.line = {
								.dir =
									findDir(game.previousHeldCell, game.currentHeldCell,
											game.smoothHoveredCell),
								.col =
									game.grid.elements V2IDX(game.proposedLineStart).data.station.col,
								.pairID =
									game.grid.elements
									V2IDX(game.proposedLineStart).data.station.pairID
							}
						};
						SetSoundPitch(game.assets.lineStep, (RAND_FLOAT_BI));
						PlaySound(game.assets.lineStep);
					}
						game.previousHeldCell = game.currentHeldCell;
						game.currentHeldCell = game.smoothHoveredCell;
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
	}
	if(IsKeyPressed(KEY_P)) {
		game.grid.populate(1);
	}
#endif

	float oldTimer = game.currentTime;
	if(game.grid.inDanger()) {
		if(!game.inShop && game.usingItem == ItemType::NONE)
			game.currentTime -= GetFrameTime();
		else
			game.currentTime -= GetFrameTime()/10;
	} else {
		if(previousInDanger) {
			game.successAtTime = game.currentTime;
			int oldCoins = game.coins;
			game.coins += (int)((game.successAtTime / game.currentMaxTime) * 3);
			if(game.coins != oldCoins) {
				game.coinsHighlightTimer = 1.0f;
			}
			PlaySound(game.assets.success);
		}
		game.currentTime -= GetFrameTime() * game.successAtTime;
	}
	game.coinsHighlightTimer -= GetFrameTime();
	if(floorf(oldTimer) != floorf(game.currentTime) && game.currentTime <= 5 &&
			game.grid.inDanger()) {
		PlaySound(game.assets.timeWarning);
	}
	if(game.currentTime <= 0){
		game.proposingLine = false;
		game.usingItem = ItemType::NONE;
		game.grid.eraseProposedLines(game.proposedLine);

		if(game.grid.inDanger()) {
			game.mgs = MainGameState::GAMEOVER;
			PlaySound(game.assets.failBoom);
			PlaySound(game.assets.failLaser);
			return;
		} else {
			game.successAtTime = 0.000000f;
		}

		if(!game.grid.populate(game.pairsOnTimer)) {
			if(game.grid.size.x == game.grid.size.y) {
				if(game.grid.areaOfPopulation.y != 0) {
					game.grid.areaOfPopulation.height = game.grid.size.y;
				}
				game.grid.areaOfPopulation.x = game.grid.size.x;
				game.grid.areaOfPopulation.y = 0;
				game.grid.resizeGrid(game.grid.size.x * 2, game.grid.size.y);
			} else {
				game.grid.areaOfPopulation.y = game.grid.size.y;
				game.grid.areaOfPopulation.x = 0;
				game.grid.areaOfPopulation.width = game.grid.size.x;
				game.grid.areaOfPopulation.height = game.grid.size.y;
				game.grid.resizeGrid(game.grid.size.x, game.grid.size.y * 2);
			}
			game.currentMaxTime +=
				sqrtf(game.currentMaxTime) * ((float)game.resizeCycle/sqrtf((float)game.resizeCycle));
			game.resizeCycle++;
			game.pairsOnTimer++;
			game.coinsHighlightTimer = 2.0f;
			game.coins += (int)(game.grid.stationLineRatio() * 100)/2;
			PlaySound(game.assets.coin);
			game.grid.populate(game.pairsOnTimer);
			game.grid.ensureNoSingularPair();
		} else {
			game.grid.ensureNoSingularPair();
		}
		game.currentTime = game.currentMaxTime;
	}

	
	BeginMode2D(game.grid.cam);
		DrawRectangle(game.smoothHoveredCell.x*10, game.smoothHoveredCell.y*10, 10, 10, JAM_WHITE);
		game.grid.draw(game);
#if DEBUG
		if(game.vizAOP)
			DrawRectangle(game.grid.areaOfPopulation.x*10, game.grid.areaOfPopulation.y*10,
				game.grid.areaOfPopulation.width*10, game.grid.areaOfPopulation.height*10,
				{255, 0, 0, 128});
#endif
	EndMode2D();

	for(int i = 0; i < 3; i++) {
		bool hovered = CheckCollisionPointRec(GetMousePosition(), {
			(float)GetScreenWidth()-200-(i*100), 5, 90, 150
		});
		DrawRectangleRec({(float)GetScreenWidth()-200-(i*100),
				5, 90, 90}, JAM_WHITE);
		bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
		bool inUse = game.usingItem == game.items[i].type;
		switch(game.items[i].type) {
			case ItemType::PORTAL_PAIR:
				DrawTexturePro(game.assets.portalPair, ASSET_ICON_REC,
						{(float)GetScreenWidth()-200-(i*100), 5, 90, 90}, {0,0}, 0, WHITE);
				if(clicked && game.items[i].quantity > 0 && game.usingItem == ItemType::NONE)
					game.usingItem = ItemType::PORTAL_PAIR;
				break;
			case ItemType::BULLDOZER:
				DrawTexturePro(game.assets.bulldozer, ASSET_ICON_REC,
						{(float)GetScreenWidth()-200-(i*100), 5, 90, 90}, {0,0}, 0, WHITE);
				if(clicked && game.items[i].quantity > 0 && game.usingItem == ItemType::NONE)
					game.usingItem = ItemType::BULLDOZER;
				break;
			case ItemType::MAGIC_WATCH:
				DrawTexturePro(game.assets.magicWatch, ASSET_ICON_REC,
						{(float)GetScreenWidth()-200-(i*100), 5, 90, 90}, {0,0}, 0, WHITE);
				if(clicked && game.items[i].quantity > 0 && game.usingItem == ItemType::NONE)
					game.usingItem = ItemType::MAGIC_WATCH;
			case ItemType::NONE:
				break;
		}
		DrawRectangleRounded({(float)GetScreenWidth()-200-(i*100), 105, 90, 40}, 0.5, 6,
			hovered || inUse ? JAM_BLACK : JAM_WHITE 
		);
		if(hovered)
			DrawRectangleRoundedLinesEx({(float)GetScreenWidth()-200-(i*100), 105, 90, 40},
			0.5, 6, 1.0f, JAM_WHITE);
		DRAWTEXTCENTER(string_format("%d", game.items[i].quantity).c_str(), 
				((float)GetScreenWidth()-200-(i*100)+45), 
				105+1, 
				40, 
				hovered || inUse ? JAM_WHITE : JAM_BLACK);
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

	DrawRectangleRounded(SHOP_ICON_COINS, 0.5, 6, JAM_WHITE);
	DRAWTEXTCENTER(string_format("$%d", game.coins).c_str(),
			SHOP_ICON_COINS.x+(95/2), 200 + 95/4 - (45/2), 45, game.coinsHighlightTimer > 0 ? JAM_GREEN : JAM_BLACK);

	DrawCircle(GetScreenWidth()-50, 50, 45, JAM_RED);
	DrawCircleSector({(float)GetScreenWidth()-50, 50}, 45, 0,
			(360) * game.currentTime/game.currentMaxTime, 100, JAM_WHITE);
	DRAWTEXTCENTEREX(string_format("%.02f", game.currentTime).c_str(),
			(float)GetScreenWidth()-50, 50, 30, 3, FloatEquals(game.successAtTime, 0.00000f) ? JAM_BLACK : JAM_GREEN);

	if(game.inShop) {
		DrawRectangleRounded(SHOP_MENU_BACKGROUND_REC, 0.5, 6, JAM_WHITE);
		if(CheckCollisionPointRec(GetMousePosition(), SHOP_MENU_PORTALS_REC)) {
			DrawRectangleRounded(SHOP_MENU_PORTALS_REC, 0.5, 6, JAM_WHITE);
			DrawRectangleRoundedLinesEx(SHOP_MENU_PORTALS_REC, 0.5, 6, 3, JAM_PURPLE);
			if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && game.coins >= SHOP_PORTAL_COST) {
				SetSoundPitch(game.assets.coin, 1);
				PlaySound(game.assets.coin);
				game.coins -= SHOP_PORTAL_COST;
				game.add(ItemType::PORTAL_PAIR);
			}
		} else
			DrawRectangleRounded(SHOP_MENU_PORTALS_REC, 0.5, 6, JAM_PURPLE);
		//DrawRectangleRounded(SHOP_MENU_BULLDOZER_REC, 0.5, 6, JAM_ORANGE);
		if(CheckCollisionPointRec(GetMousePosition(), SHOP_MENU_BULLDOZER_REC)) {
			DrawRectangleRounded(SHOP_MENU_BULLDOZER_REC, 0.5, 6, JAM_WHITE);
			DrawRectangleRoundedLinesEx(SHOP_MENU_BULLDOZER_REC, 0.5, 6, 3, JAM_ORANGE);
			if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && game.coins >= SHOP_BULLDOZER_COST) {
				SetSoundPitch(game.assets.coin, 1);
				PlaySound(game.assets.coin);
				game.coins -= SHOP_BULLDOZER_COST;
				game.add(ItemType::BULLDOZER);
			}
		} else
			DrawRectangleRounded(SHOP_MENU_BULLDOZER_REC, 0.5, 6, JAM_ORANGE);

		if(CheckCollisionPointRec(GetMousePosition(), SHOP_MENU_MWATCH_REC)) {
			DrawRectangleRounded(SHOP_MENU_MWATCH_REC, 0.5, 6, JAM_WHITE);
			DrawRectangleRoundedLinesEx(SHOP_MENU_MWATCH_REC, 0.5, 6, 3, JAM_BLUE);
			if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && game.coins >= SHOP_MWATCH_COST) {
				SetSoundPitch(game.assets.coin, 1);
				PlaySound(game.assets.coin);
				game.coins -= SHOP_MWATCH_COST;
				game.add(ItemType::MAGIC_WATCH);
			}
		} else
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
		DRAWTEXTCENTER(string_format("Cost: $%d", SHOP_PORTAL_COST).c_str(), 300, 365, 25,
				game.coins >= SHOP_PORTAL_COST ? JAM_BLACK : JAM_RED);
		DrawTextEx(GetFontDefault(), "A pair of portals you can place to \"wirelessly\""
				"\nconnect stations. Portals MUST connect to the"
				"\nsame pair. On use, press LMB on an empty square to"
				"\nplace one portal of two. Cancel with RMB.", {180, 400}, 10, 1, JAM_BLACK);

		DRAWTEXTCENTER(string_format("Bulldozer").c_str(), 800, 220, 25, JAM_BLACK);
		DRAWTEXTCENTER(string_format("Cost: $%d", SHOP_BULLDOZER_COST).c_str(),
				800, 365, 25, game.coins >= SHOP_BULLDOZER_COST ? JAM_BLACK : JAM_RED);
		DrawTextEx(GetFontDefault(), "A literal Bulldozer to erase a station and its pair"
				"\nregardless if its connected or not, lines and all."
				"\nOn use, press LMB over a station/line to erase all"
				"\nassociated with it. Cannot be used on totally\n unconnected portals. Cancel with RMB.", {740-75, 400},
				10, 1, JAM_BLACK);

		DRAWTEXTCENTER(string_format("Magic Watch").c_str(), 1300, 220, 25, JAM_BLACK);
		DRAWTEXTCENTER(string_format("Cost: $%d", SHOP_MWATCH_COST).c_str(),
				1300, 365, 25, game.coins >= SHOP_MWATCH_COST ? JAM_BLACK : JAM_RED);
		DrawTextEx(GetFontDefault(), "A magic watch to give you extra time when you"
				"\nneed it the most. Adds 10 seconds on use.", {1300-125, 400},
				10, 1, JAM_BLACK);
	}
}

void OnMainMenu(Game& game) {
	ClearBackground(JAM_BLACK);
	DRAWTEXTCENTER("SpeedNet", GetScreenWidth()/2, 100, 50, JAM_WHITE);
	BeginMode2D({
		.offset = {800, 400},
		.target = {25, 25},
		.rotation = 0,
		.zoom = 6,
	});
		game.grid.draw(game);
	EndMode2D();

	DrawRectangle(-50, GetScreenHeight() - 150, 2000, 100, JAM_BLACK);
	DrawRectangleLinesEx({-50, (float)GetScreenHeight() - 150, 2000, 100}, 3, JAM_WHITE);

	Rectangle rec = 
		{(float)GetScreenWidth()/2+250-200, (float)GetScreenHeight()-140, 200, 80};
	if(CheckCollisionPointRec(GetMousePosition(), rec)) {
		DrawRectangleRoundedLinesEx(rec, 0.5, 6, 3, JAM_WHITE);
		DRAWTEXTCENTER("Play", rec.x + 100, (float)GetScreenHeight()-115, 40,  JAM_WHITE);
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			game.reset();
			game.mgs = MainGameState::INGAME;
		}
	} else {
		DrawRectangleRounded(rec, 0.5, 6, JAM_WHITE);
		DRAWTEXTCENTER("Play", rec.x + 100, (float)GetScreenHeight()-115, 40,  JAM_BLACK);
	}

	rec = {(float)GetScreenWidth()/2-250, (float)GetScreenHeight()-140, 200, 80};
	if(CheckCollisionPointRec(GetMousePosition(), rec)) {
		DrawRectangleRoundedLinesEx(rec, 0.5, 6, 3, JAM_WHITE);
		DRAWTEXTCENTER("Instructions", rec.x + 100, (float)GetScreenHeight()-115, 30,  JAM_WHITE);
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			game.mgs = MainGameState::INSTRUCTIONS;
		}
	} else {
		DrawRectangleRounded(rec, 0.5, 6,JAM_WHITE);
		DRAWTEXTCENTER("Instructions", rec.x + 100, (float)GetScreenHeight()-115, 30,  JAM_BLACK);
	}
}

void OnPaletteTest(Game& game, Color* palette) {
#if SKIP_PALETTE
	game.mgs = MainGameState::MAINMENU;
#endif
#if DELAY_STARTUP
	if(IsKeyPressed(KEY_ENTER)) game.beginStartup = true;
	if(game.beginStartup) game.currentTime += GetFrameTime();
	if(game.currentTime >= 5) {
#else
	game.currentTime += GetFrameTime();
	if(game.currentTime >= 5) {
#endif
		PlaySound(game.assets.bulldozerSound);
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
		SetSoundPitch(game.assets.coin, 1.0f);
	}
	if(game.currentTime >= 4)
		ClearBackground(JAM_BLACK);
	else {
		if(game.paletteNum != (int)Clamp((game.currentTime-1)/0.3f, 0, 7)) {
			SetSoundPitch(game.assets.coin, 0.5 + (game.paletteNum * 0.25));
			PlaySound(game.assets.coin);
			game.paletteNum = (int)Clamp((game.currentTime-1)/0.3f, 0, 7);
		}
		ClearBackground(palette[(int)Clamp((game.currentTime-1)/0.3f, 0, 7)]);
	}
	for(int i = 0; i < 8; i++) {
		DrawRectangle(i * 80 + (800 - 80*4), 360, 80, 80, palette[i]);
	}
	DRAWTEXTCENTER("made in raylib", GetScreenWidth()/2, 380, 30, JAM_WHITE);
	
#if SKIP_PALETTE
	SetSoundPitch(game.assets.coin, 1.0f);
#endif
}

void OnGameOver(Game &game) {
	ClearBackground(JAM_BLACK);
	BeginMode2D(game.grid.cam);
		game.grid.draw(game);
#if DEBUG
		if(game.vizAOP)
			DrawRectangle(game.grid.areaOfPopulation.x*10, game.grid.areaOfPopulation.y*10,
					game.grid.areaOfPopulation.width*10, game.grid.areaOfPopulation.height*10, {255,0,0,128});
#endif
	EndMode2D();

	DrawRectangleRounded({(float)GetScreenWidth()/2-250, 100, 500, 200}, 0.5, 6, JAM_RED);
	DrawRectangleRoundedLinesEx({(float)GetScreenWidth()/2-250, 100, 500, 200}, 0.5, 6, 5, JAM_WHITE);

	DRAWTEXTCENTEREX("GAME OVER", (float)GetScreenWidth()/2, 150, 50, 3, JAM_BLACK);
	DRAWTEXTCENTEREX(
			string_format(
				"Level %d\nPairs Connected: %d\nEfficiency: %.00f%%", 
				game.resizeCycle,
				game.grid.numOfStations(true) / 2,
				game.grid.stationLineRatio() * 100).c_str(),
			(float)GetScreenWidth()/2, 200, 25, 3, JAM_BLACK);

	DrawRectangle(-50, GetScreenHeight() - 150, 2000, 100, JAM_BLACK);
	DrawRectangleLinesEx({-50, (float)GetScreenHeight() - 150, 2000, 100}, 3, JAM_WHITE);

	Rectangle rec = 
		{(float)GetScreenWidth()/2+250-200, (float)GetScreenHeight()-140, 200, 80};
	if(CheckCollisionPointRec(GetMousePosition(), rec)) {
		DrawRectangleRoundedLinesEx(rec, 0.5, 6, 3, JAM_WHITE);
		DRAWTEXTCENTER("Restart", rec.x + 100, (float)GetScreenHeight()-115, 40,  JAM_WHITE);
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			game.mgs = MainGameState::INGAME;
			game.reset();
		}
	} else {
		DrawRectangleRounded(rec, 0.5, 6, JAM_WHITE);
		DRAWTEXTCENTER("Restart", rec.x + 100, (float)GetScreenHeight()-115, 40,  JAM_BLACK);
	}

	rec = {(float)GetScreenWidth()/2-250, (float)GetScreenHeight()-140, 200, 80};
	if(CheckCollisionPointRec(GetMousePosition(), rec)) {
		DrawRectangleRoundedLinesEx(rec, 0.5, 6, 3, JAM_WHITE);
		DRAWTEXTCENTER("Title", rec.x + 100, (float)GetScreenHeight()-115, 40,  JAM_WHITE);
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			game.mgs = MainGameState::MAINMENU;
			game.reset();
		}
	} else {
		DrawRectangleRounded(rec, 0.5, 6,JAM_WHITE);
		DRAWTEXTCENTER("Title", rec.x + 100, (float)GetScreenHeight()-115, 40,  JAM_BLACK);
	}
}

void OnInstructions(Game &game) {
	ClearBackground(JAM_BLACK);
	DrawRectangle(-50, GetScreenHeight() - 150, 2000, 100, JAM_BLACK);
	DrawRectangleLinesEx({-50, (float)GetScreenHeight() - 150, 2000, 100}, 3, JAM_WHITE);

	DrawText(
			"SpeedNet is a simple game about connections. It is not a relaxing one."
			"\n\nYou connect multiple pairs of stations within a grid while constantly "
			"\nfighting a time limit. If the timer reaches 0 and a single station is left"
			"\nunconnected, the game is over.\n\n"
			"Once the grid is sufficiently filled enough, it will expand to double its area"
			"\nwith more pairs to connect at a time."
			"\n\nWhile hovering over an unconnected station, hold LMB and move the mouse"
			"\nto navigate the empty space to connect to that station's pair. A thin"
			"\ncircle will appear over both stations while doing this."
			"\n\nAfter the grid resizes, or if you connect stations fast enough,"
			"\nyou will get coins based on your performance which you can then use"
			"\nto spend on 3 items in the shop that can be accessed at the top-right."
			"\nThe items will be described in-game.", 
			50, 50, 25, JAM_WHITE);

	Rectangle rec = 
		{(float)GetScreenWidth()/2-200, (float)GetScreenHeight()-140, 400, 80};
	if(CheckCollisionPointRec(GetMousePosition(), rec)) {
		DrawRectangleRoundedLinesEx(rec, 0.5, 6, 3, JAM_WHITE);
		DRAWTEXTCENTER("To Title", rec.x + 200, (float)GetScreenHeight()-115, 40,  JAM_WHITE);
		if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			game.mgs = MainGameState::MAINMENU;
		}
	} else {
		DrawRectangleRounded(rec, 0.5, 6, JAM_WHITE);
		DRAWTEXTCENTER("To Title", rec.x + 200, (float)GetScreenHeight()-115, 40,  JAM_BLACK);
	}
}

void Game::reset() {
	grid.cam = {
		.offset = {800, 400},
		.target = {25, 25},
		.rotation = 0,
		.zoom = 6,
	};
	currentMaxTime = 5;
	currentTime = 10;
	coins = 0;
	grid.areaOfPopulation = {0, 0, 5, 5};
	grid.reset();
	grid.populate(2);
	resizeCycle = 1;
	pairsOnTimer = 1;
	usingItem = ItemType::NONE;
	inShop = false;

	items[0] = {
		.type = ItemType::PORTAL_PAIR,
		.quantity = 0,
	};
	items[1] = {
		.type = ItemType::BULLDOZER,
		.quantity = 0,
	};
	items[2] = {
		.type = ItemType::MAGIC_WATCH,
		.quantity = 0,
	};
}

void Game::consume(ItemType item) {
	for(auto& i: items) {
		if(i.type == item) i.quantity--;
	}
}
void Game::add(ItemType item) {
	for(auto& i: items) {
		if(i.type == item) i.quantity++;
	}
}
