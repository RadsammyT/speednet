#include <raylib.h>

int main() {
	InitWindow(1600, 800, "RaylibJam");
	while(!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(BLACK);
		EndDrawing();
	}
	CloseWindow();
}
