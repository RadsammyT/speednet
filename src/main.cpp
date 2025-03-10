#include <cstdlib>
#include <ctime>
#include <raylib.h>
#include <raymath.h>
#include "game.h"
#include "rayImGui/imgui.h"
#include "rayImGui/rlImGui.h"

int main() {
	srand(time(NULL));

	InitAudioDevice();

	InitWindow(1600, 800, "SpeedNet");
	SetTargetFPS(60);
	rlImGuiSetup(true);
	Game game;
	game.mgs = MainGameState::PALETTETEST;
#if DEBUG
	bool showDemoWindow = false;
	bool showDebugRectangle = false;
	Rectangle debugRectangle = {0,0,0,0};
#endif
	while(!WindowShouldClose()) {
		BeginDrawing();
			switch(game.mgs) {
				case MainGameState::PALETTETEST:
					OnPaletteTest(game, palette);
					break;
				case MainGameState::MAINMENU:
					OnMainMenu(game);
					break;
				case MainGameState::INGAME:
					OnInGame(game);
					break;
				case MainGameState::GAMEOVER:
					OnGameOver(game);
					break;
				case MainGameState::INSTRUCTIONS:
					OnInstructions(game);
					break;
			}
#if DEBUG
			rlImGuiBegin();
				ImGui::Begin("test");

					Vector2 mouseGridPos = GetScreenToWorld2D(GetMousePosition(), game.grid.cam);
					Vector2 hoveredCell = {
						floorf(Clamp(mouseGridPos.x, 0, (game.grid.size.x-1)*10)/10),
						floorf(Clamp(mouseGridPos.y, 0, (game.grid.size.y-1)*10)/10),
					};


					ImGui::Checkbox("demo", &showDemoWindow);
					ImGui::Checkbox("debug rect", &showDebugRectangle);
					ImGui::Checkbox("visualize areaOfPopulation", &game.vizAOP);
					if(showDebugRectangle) {
						ImGui::DragFloat4("XYWH", &debugRectangle.x);
					}
					ImGui::Text("FPS: %d", GetFPS());
					ImGui::Text("MouseWorldPos: %f %f", mouseGridPos.x, mouseGridPos.y);
					ImGui::Text("MouseWorldPos: %d %d", (int)hoveredCell.x, (int)hoveredCell.y);
					ImGui::Text("used item: %d", static_cast<int>(game.usingItem));
					ImGui::Text("camzoom: %f", game.grid.cam.zoom);
					ImGui::Text("pairsOnTimer: %d", game.pairsOnTimer);
					ImGui::Text("ratio: %f", game.grid.stationLineRatio());
					if(ImGui::TreeNode("proposedLines")) {
						for(auto& i: game.proposedLine) {
							ImGui::Text("%d %d", (int)i.x, (int)i.y);
						}
						ImGui::TreePop();
					}
					if(ImGui::TreeNode("game grid element stations")) {
						for(int y = 0; y < game.grid.elements.size(); y++) {
							for(int x = 0; x < game.grid.elements[y].size(); x++) {
								if(game.grid.elements[y][x].type == GridElementType::STATION)
									ImGui::Text("%d %d C:%d", x, y,
											game.grid.elements[y][x].data.station.connectedToPair);
							}
						}
						ImGui::TreePop();
					}
					if(ImGui::Button("reset, repopulate")) {
						game.grid.reset();
						game.grid.populate(2);
					}
					if(ImGui::Button("ACTUALLY resize")) {
						game.grid.flood();
						game.currentTime = 0.1f;
					}
					if(ImGui::Button("MORE TIME")) {
						game.currentTime += 100;
					}
					if(ImGui::Button("INFINITE WEALTH")) {
						game.coins += 100;
					}
				ImGui::End();
				if(showDemoWindow) ImGui::ShowDemoWindow();
			rlImGuiEnd();
			if(showDebugRectangle) {
				DrawRectangleRec(debugRectangle, RED);
			}
#endif // DEBUG
		EndDrawing();
	}
	rlImGuiShutdown();
	CloseAudioDevice();
	CloseWindow();
}
