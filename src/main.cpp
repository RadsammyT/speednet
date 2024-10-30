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

	Game game;
	game.mgs = MainGameState::PALETTETEST;
	bool showDemoWindow = false;
	InitWindow(1600, 800, "RaylibJam");
	rlImGuiSetup(true);
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
					break;
			}
			rlImGuiBegin();
				ImGui::Begin("test");

					Vector2 mouseGridPos = GetScreenToWorld2D(GetMousePosition(), game.grid.cam);
					Vector2 hoveredCell = {
						floorf(Clamp(mouseGridPos.x, 0, (game.grid.size.x-1)*10)/10),
						floorf(Clamp(mouseGridPos.y, 0, (game.grid.size.y-1)*10)/10),
					};

					ImGui::Checkbox("demo", &showDemoWindow);
					ImGui::Text("MouseWorldPos: %f %f", mouseGridPos.x, mouseGridPos.y);
					ImGui::Text("MouseWorldPos: %d %d", (int)hoveredCell.x, (int)hoveredCell.y);
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
									ImGui::Text("%d %d", x, y);
							}
						}
						ImGui::TreePop();
					}
					if(ImGui::Button("reset, repopulate")) {
						game.grid.reset();
						game.grid.populate(2);
					}
				ImGui::End();
				if(showDemoWindow) ImGui::ShowDemoWindow();
			rlImGuiEnd();
		EndDrawing();
	}
	rlImGuiShutdown();
	CloseAudioDevice();
	CloseWindow();
}
