/////////////////////////////////////////////////////////////////////////////////////////////////
// Autor: Szymon Olkowski 203473, Niektóre funkcje wzięte z "Szablon SDL do drugiego projektu" //
/////////////////////////////////////////////////////////////////////////////////////////////////

#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include <iostream>
extern "C"
{
#include "./SDL2-2.0.10/include/SDL.h"
#include "./SDL2-2.0.10/include/SDL_main.h"
}

//////////////////////////////Stałe//////////////////////////////////////

#define SCREEN_WIDTH 3200 //SCreen width
#define SCREEN_HEIGHT 2000 //Screen height
#define CELL_SIZE 20 //Cell Size
#define MAX_SNAKE_LENGTH 1000 // max snake length
#define PLAY_AREA_X 50*20 //left side of playing area
#define PLAY_AREA_Y 20*20 //top side of playing area
#define PLAY_AREA_WIDTH 1500 //playing area width
#define PLAY_AREA_HEIGHT 1000 //playing area height

//////////////////////////////Struktury//////////////////////////////////

struct Snake
{
	int headX, headY;					 
	SDL_Point segments[MAX_SNAKE_LENGTH];
	int length;							
	int dx, dy;							
	bool isAlive;						 
	bool isGrowing;			
	double speed = 1;
	int score = 0;
};

struct RedDot {
	SDL_Point position;
	bool isVisible;
	double remainingTime;
	int bonusType; // 0 - shortening, 1 - slowing
};

//////////////////////////////Definicje Funkcji///////////////////////////


void DrawString(SDL_Surface *screen, int x, int y, char *text, SDL_Surface *charset);
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y);
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color);
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color);
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor);
void DrawSnake(SDL_Surface* screen, Snake* snake);
void Cleanup(SDL_Surface *charset, SDL_Surface *screen, SDL_Texture *scrtex, SDL_Renderer *renderer, SDL_Window *window);
int InitializeSDL(SDL_Window **window, SDL_Renderer **renderer, SDL_Surface **screen, SDL_Texture **scrtex);
void InitializeSnake(Snake* snake);
int LoadResources(SDL_Surface **charset, SDL_Surface *screen);
void MainLoop(SDL_Surface *screen, SDL_Texture *scrtex, SDL_Renderer *renderer, SDL_Surface *charset);
bool CheckSelfCollision(Snake *snake);
bool IsFoodUnderSnake(SDL_Point food, Snake* snake);
void DisplayImplementedRequirements(SDL_Surface* screen, SDL_Surface* charset);
void updateWorldTimeAndSpeedUp(bool showRestartPrompt, double* worldTime, double delta, 
	double* last_speedup_time, double* speedup_interval, int* interval_time, double* speedup_factor);
void ProcessGameInput(int* quit, bool* inGameOverScreen, Snake* snake, bool* directionChanged,
	double* worldTime, bool* showRestartPrompt, SDL_Point* food);
void HandleGameOver(int* quit, bool* inGameOverScreen, Snake* snake,
	double* worldTime, SDL_Point* food);
void DisplayRestartMessage(SDL_Surface* screen, SDL_Surface* charset);
void HandleAutomaticTurn(Snake* snake);
void HandleRedDot(RedDot& redDot, Snake* snake, double delta);
void GrowSnakeIfNeeded(Snake& snake);
void RenderAndHandleGameStep(
	SDL_Surface* screen,
	SDL_Texture* scrtex,
	SDL_Renderer* renderer,
	SDL_Surface* charset,
	bool& showRestartPrompt,
	bool& inGameOverScreen,
	Snake& snake,
	RedDot& redDot,
	SDL_Point& food,
	double& worldTime,
	int& quit,
	bool& directionChanged,
	double fps,
	int& frames,
	int bialy,
	int czarny,
	int niebieski,
	int czerwony
);
void MoveSnake(Snake& snake);
void CheckAndEatFood(Snake& snake, SDL_Point& food);
void CheckAndEatRedDot(Snake& snake, RedDot& redDot, int& interval_time);
void CheckCollisions(Snake& snake, bool& inGameOverScreen);

//////////////////////////////Main Funkcje////////////////////////////////

#ifdef __cplusplus
extern "C"
#endif
int	main(int argc, char **argv)
{
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	SDL_Surface *screen = NULL, *charset = NULL;
	SDL_Texture *scrtex = NULL;

	if (PLAY_AREA_WIDTH % CELL_SIZE != 0 || PLAY_AREA_HEIGHT % CELL_SIZE != 0) {
		printf("Error: Play area dimensions must be divisible by cell size.\n");
		return 1;
	}

	if (InitializeSDL(&window, &renderer, &screen, &scrtex) != 0)
	{
		return 1;
	}

	if (LoadResources(&charset, screen) != 0)
	{
		Cleanup(charset, screen, scrtex, renderer, window);
		return 1;
	}

	MainLoop(screen, scrtex, renderer, charset);

	Cleanup(charset, screen, scrtex, renderer, window);
	return 0;
}

void MainLoop(SDL_Surface* screen, SDL_Texture* scrtex, SDL_Renderer* renderer, SDL_Surface* charset)
{
	bool directionChanged = false, showRestartPrompt = false, inGameOverScreen = false;
	int quit = 0, frames = 0;
	double delta, worldTime = 0, fpsTimer = 0, fps = 0;
	int t1 = SDL_GetTicks();
	int lastMoveTime = t1;
	int interval_time = 100; 
	double speedup_factor = 0.95, speedup_interval = 10.0, last_speedup_time = 0.0; 

	Snake snake;
	InitializeSnake(&snake);

	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int bialy = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);


	SDL_Point food = {
		(PLAY_AREA_X + CELL_SIZE) + (rand() % ((PLAY_AREA_WIDTH - 2 * CELL_SIZE) / CELL_SIZE)) * CELL_SIZE,
		(PLAY_AREA_Y + CELL_SIZE) + (rand() % ((PLAY_AREA_HEIGHT - 2 * CELL_SIZE) / CELL_SIZE)) * CELL_SIZE
	};

	SDL_Rect playArea = {
		PLAY_AREA_X,
		PLAY_AREA_Y,
		PLAY_AREA_WIDTH,
		PLAY_AREA_HEIGHT
	};

	RedDot redDot = { {0, 0}, false, 0.0, 0 };

	char text[128];

	while (!quit)
	{
		int t2 = SDL_GetTicks();
		delta = (t2 - t1) * 0.001;
		t1 = t2;

		updateWorldTimeAndSpeedUp(showRestartPrompt, &worldTime, delta, &last_speedup_time, 
			&speedup_interval, &interval_time, &speedup_factor);

		HandleRedDot(redDot, &snake, delta);

		if (!showRestartPrompt && !inGameOverScreen && snake.isAlive && t2 - lastMoveTime > interval_time) {
			lastMoveTime = t2;
			directionChanged = false; 

			MoveSnake(snake);

			CheckAndEatFood(snake, food);

			CheckAndEatRedDot(snake, redDot, interval_time);

			CheckCollisions(snake, inGameOverScreen);

			if (!directionChanged) {
				HandleAutomaticTurn(&snake);
			}

			GrowSnakeIfNeeded(snake);
		}
		SDL_FillRect(screen, NULL, czarny);

		fpsTimer += delta;
		if (fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		}

		RenderAndHandleGameStep(
			screen,
			scrtex,
			renderer,
			charset,
			showRestartPrompt,
			inGameOverScreen,
			snake,
			redDot,
			food,
			worldTime,
			quit,
			directionChanged,
			fps,
			frames,
			bialy,
			czarny,
			niebieski,
			czerwony
		);
	}
}

void RenderAndHandleGameStep(
	SDL_Surface* screen,
	SDL_Texture* scrtex,
	SDL_Renderer* renderer,
	SDL_Surface* charset,
	bool& showRestartPrompt,
	bool& inGameOverScreen,
	Snake& snake,
	RedDot& redDot,
	SDL_Point& food,
	double& worldTime,
	int& quit,
	bool& directionChanged,
	double fps,
	int& frames,
	int bialy,
	int czarny,
	int niebieski,
	int czerwony
)
{
	char text[128];

	if (showRestartPrompt) {
		DisplayRestartMessage(screen, charset);
	}
	else {
		if (inGameOverScreen) {
			snake.isAlive = false;
		}
		if (snake.isAlive || !inGameOverScreen) {
			int x = PLAY_AREA_X + PLAY_AREA_WIDTH + 20;
			int y = PLAY_AREA_Y + 13 * 16;

			DisplayImplementedRequirements(screen, charset);
			DrawRectangle(screen, PLAY_AREA_X, PLAY_AREA_Y, PLAY_AREA_WIDTH, PLAY_AREA_HEIGHT, bialy, czarny);
			DrawSnake(screen, &snake);

			SDL_Rect foodRect = { food.x, food.y, CELL_SIZE, CELL_SIZE };
			SDL_FillRect(screen, &foodRect, niebieski);

			sprintf(text, "Time: %.1lf s  FPS: %.0lf  Score: %d", worldTime, fps, snake.score);
			DrawString(screen, x, y, text, charset);
			y += 32;

			if (redDot.isVisible) {
				SDL_Rect redDotRect = { redDot.position.x, redDot.position.y, CELL_SIZE, CELL_SIZE };
				SDL_FillRect(screen, &redDotRect, czerwony);

				int progressBarWidth = (int)((redDot.remainingTime / 4.0) * 200);
				DrawRectangle(screen, x, y, 200, 20, bialy, czarny);
				DrawRectangle(screen, x, y, progressBarWidth, 20, czerwony, czerwony);
			}
			y += 32;
			y += 16;

			sprintf(text, "ESC: End Game");
			DrawString(screen, x, y, text, charset);
			y += 16;

			sprintf(text, "N: New Game");
			DrawString(screen, x, y, text, charset);
		}
		else {
			sprintf(text, "Game Over! Final Score: %d", snake.score);
			DrawString(screen, SCREEN_WIDTH / 2 - (int)strlen(text) * 8 / 2,
				SCREEN_HEIGHT / 2 - 20, text, charset);

			sprintf(text, "Press N for new game or E to Exit");
			DrawString(screen, SCREEN_WIDTH / 2 - (int)strlen(text) * 8 / 2,
				SCREEN_HEIGHT / 2 + 20, text, charset);
		}
	}

	SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
	SDL_RenderCopy(renderer, scrtex, NULL, NULL);
	SDL_RenderPresent(renderer);

	ProcessGameInput(&quit, &inGameOverScreen, &snake, &directionChanged,
		&worldTime, &showRestartPrompt, &food);
	HandleGameOver(&quit, &inGameOverScreen, &snake,
		&worldTime, &food);

	frames++;
}

void DisplayRestartMessage(SDL_Surface* screen, SDL_Surface* charset) {
	char text[256];

	sprintf(text, "Do you want to restart the game?");
	DrawString(screen, SCREEN_WIDTH / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 - 40, text, charset);
	sprintf(text, "Press Y for Yes or N for No");
	DrawString(screen, SCREEN_WIDTH / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2, text, charset);
}


void Cleanup(SDL_Surface* charset, SDL_Surface* screen, SDL_Texture* scrtex, SDL_Renderer* renderer, SDL_Window* window)
{
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void updateWorldTimeAndSpeedUp(bool showRestartPrompt, double* worldTime, double delta, double* last_speedup_time, double* speedup_interval, int* interval_time, double* speedup_factor) {
	if (!showRestartPrompt) {
		*worldTime += delta;
	}

	if (*worldTime - *last_speedup_time >= *speedup_interval) {
		*interval_time = (int)(*interval_time * *speedup_factor);
		*last_speedup_time = *worldTime;
		*speedup_factor = 0.95;
	}
}

//////////////////////////////Input/Event/////////////////////////////////

void ProcessGameInput(int* quit, bool* inGameOverScreen, Snake* snake, bool* directionChanged,
	double* worldTime, bool* showRestartPrompt, SDL_Point* food) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			*quit = true;
		}
		else if (event.type == SDL_KEYDOWN && !(*directionChanged)) {
			switch (event.key.keysym.sym) {
			case SDLK_ESCAPE:
				*inGameOverScreen = true;
				break;
			case SDLK_n:
				if (snake->isAlive && !(*showRestartPrompt)) {
					*showRestartPrompt = true; 
				}
				else if (*showRestartPrompt) {
					*showRestartPrompt = false;
				}
				break;
			case SDLK_y:
				if (*showRestartPrompt) {
					*showRestartPrompt = false;
					InitializeSnake(snake);
					snake->score = 0;
					*worldTime = 0;
					snake->isAlive = true;
					do {
						food->x = (PLAY_AREA_X + CELL_SIZE) + (rand() % ((PLAY_AREA_WIDTH - 2 * CELL_SIZE) / CELL_SIZE)) * CELL_SIZE;
						food->y = (PLAY_AREA_Y + CELL_SIZE) + (rand() % ((PLAY_AREA_HEIGHT - 2 * CELL_SIZE) / CELL_SIZE)) * CELL_SIZE;
					} while (IsFoodUnderSnake(*food, snake));
				}
				break;
			case SDLK_w:
				if (snake->dy != 1) {
					snake->dx = 0;
					snake->dy = -1;
					*directionChanged = true;
				}
				break;
			case SDLK_s:
				if (snake->dy != -1) {
					snake->dx = 0;
					snake->dy = 1;
					*directionChanged = true;
				}
				break;
			case SDLK_a:
				if (snake->dx != 1) {
					snake->dx = -1;
					snake->dy = 0;
					*directionChanged = true;
				}
				break;
			case SDLK_d:
				if (snake->dx != -1) {
					snake->dx = 1;
					snake->dy = 0;
					*directionChanged = true;
				}
				break;
			}
		}
	}
}

void HandleGameOver(int* quit, bool* inGameOverScreen, Snake* snake,
	double* worldTime, SDL_Point* food) {
	if (!snake->isAlive || *inGameOverScreen) {
		const Uint8* state = SDL_GetKeyboardState(NULL);
		if (state[SDL_SCANCODE_N]) {
			InitializeSnake(snake);
			snake->score = 0;
			*worldTime = 0;
			snake->isAlive = true;
			*inGameOverScreen = false;
			do {
				food->x = (PLAY_AREA_X + CELL_SIZE) + (rand() % ((PLAY_AREA_WIDTH - 2 * CELL_SIZE) / CELL_SIZE)) * CELL_SIZE;
				food->y = (PLAY_AREA_Y + CELL_SIZE) + (rand() % ((PLAY_AREA_HEIGHT - 2 * CELL_SIZE) / CELL_SIZE)) * CELL_SIZE;
			} while (IsFoodUnderSnake(*food, snake));
		}
		else if (state[SDL_SCANCODE_E]) {
			*quit = true;
		}
	}
}

//////////////////////////////Inicjalizacja///////////////////////////////

int LoadResources(SDL_Surface** charset, SDL_Surface* screen)
{
	*charset = SDL_LoadBMP("./cs8x8.bmp");
	if (*charset == NULL)
	{
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		return 1;
	}
	SDL_SetColorKey(*charset, true, 0x000000);
	return 0;
}

int InitializeSDL(SDL_Window **window, SDL_Renderer **renderer, SDL_Surface **screen, SDL_Texture **scrtex)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	if (SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, window, renderer) != 0)
	{
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(*renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(*renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(*window, "Szablon do zdania drugiego 2017");

	*screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	*scrtex = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_ShowCursor(SDL_DISABLE);

	return 0;
}

void InitializeSnake(Snake* snake)
{
	snake->headX = ((PLAY_AREA_X + PLAY_AREA_WIDTH / 2) / CELL_SIZE) * CELL_SIZE;
	snake->headY = ((PLAY_AREA_Y + PLAY_AREA_HEIGHT / 2)  / CELL_SIZE) * CELL_SIZE;
	snake->length = 3;
	for (int i = 0; i < snake->length; i++)
	{
		snake->segments[i] = { snake->headX - i * CELL_SIZE, snake->headY };
	}
	snake->dx = 1; 
	snake->dy = 0;
	snake->isAlive = true;
	snake->isGrowing = false;
}

/////////////////////////Drawing Funckje//////////////////////////////////

void DrawString(SDL_Surface *screen, int x, int y, char *text,
				SDL_Surface *charset)
{
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text)
	{
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y)
{
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};

void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color)
{
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
};

void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color)
{
	for (int i = 0; i < l; i++)
	{
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};

void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
				   Uint32 outlineColor, Uint32 fillColor)
{
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

void DrawSnake(SDL_Surface* screen, Snake* snake) {
	Uint32 green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	for (int i = 0; i < snake->length; i++) {
		SDL_Rect rect = {
			snake->segments[i].x,
			snake->segments[i].y,
			CELL_SIZE,
			CELL_SIZE
		};
		SDL_FillRect(screen, &rect, green);
	}
}

void DisplayImplementedRequirements(SDL_Surface* screen, SDL_Surface* charset) {
	char text[128];

	int x = PLAY_AREA_X + PLAY_AREA_WIDTH + 20;
	int y = PLAY_AREA_Y;
	y += 16;
	DrawString(screen, x, y, "Implemented Requirements:", charset);
	y += 16;
	DrawString(screen, x, y, "- 1. Graphic Design of Game", charset);
	y += 16;
	DrawString(screen, x, y, "- 2. Basic Snake Movement", charset);
	y += 16;
	DrawString(screen, x, y, "- 3. Collision Detection", charset);
	y += 16;
	DrawString(screen, x, y, "- 4. Time and Info Display", charset);
	y += 32;
	DrawString(screen, x, y, "Implemented Optional Requirements:", charset);
	y += 16;
	DrawString(screen, x, y, "- A. Lengthening of the snake.", charset);
	y += 16;
	DrawString(screen, x, y, "- B. Speedup", charset);
	y += 16;
	DrawString(screen, x, y, "- C. Red dot bonus", charset);
	y += 16;
	DrawString(screen, x, y, "- D. Points", charset);
}

///////////////////////////////////Snake//////////////////////////////////


bool CheckSelfCollision(Snake* snake)
{
	for (int i = 1; i < snake->length; i++)
	{
		if (snake->segments[0].x == snake->segments[i].x &&
			snake->segments[0].y == snake->segments[i].y)
		{
			return true;
		}
	}
	return false;
}

void GrowSnakeIfNeeded(Snake& snake) {
	if (snake.isGrowing) {
		if (snake.length < MAX_SNAKE_LENGTH) {
			snake.segments[snake.length] = snake.segments[snake.length - 1];
			snake.length++;
		}
		snake.isGrowing = false;
	}
}

void MoveSnake(Snake& snake) {
	for (int i = snake.length - 1; i > 0; i--) {
		snake.segments[i] = snake.segments[i - 1];
	}
	snake.segments[0].x += snake.dx * CELL_SIZE;
	snake.segments[0].y += snake.dy * CELL_SIZE;
}

void CheckCollisions(Snake& snake, bool& inGameOverScreen) {
	if (snake.segments[0].x < PLAY_AREA_X
		|| snake.segments[0].y < PLAY_AREA_Y
		|| snake.segments[0].x >= PLAY_AREA_X + PLAY_AREA_WIDTH
		|| snake.segments[0].y >= PLAY_AREA_Y + PLAY_AREA_HEIGHT
		|| CheckSelfCollision(&snake))
	{
		snake.isAlive = false;
		inGameOverScreen = true;
	}
}

void HandleAutomaticTurn(Snake* snake) {
	if (snake->segments[0].x == PLAY_AREA_X + PLAY_AREA_WIDTH - CELL_SIZE && snake->segments[0].y == PLAY_AREA_Y + PLAY_AREA_HEIGHT - CELL_SIZE) {
		snake->dx = -1;
		snake->dy = 0;
	}
	else if (snake->segments[0].x == PLAY_AREA_X && snake->segments[0].y == PLAY_AREA_Y + PLAY_AREA_HEIGHT - CELL_SIZE) {
		snake->dy = -1;
		snake->dx = 0;
	}
	else if (snake->segments[0].y == PLAY_AREA_Y && snake->segments[0].x == PLAY_AREA_X + PLAY_AREA_WIDTH - CELL_SIZE) {
		snake->dy = 1;
		snake->dx = 0;
	}
	else if (snake->segments[0].y == PLAY_AREA_Y && snake->segments[0].x == PLAY_AREA_X) {
		snake->dx = 1;
		snake->dy = 0;
	}
	else if (snake->segments[0].x <= PLAY_AREA_X) {
		snake->dy = -1;
		snake->dx = 0;
	}
	else if (snake->segments[0].x >= PLAY_AREA_X + PLAY_AREA_WIDTH - CELL_SIZE) {
		snake->dy = 1;
		snake->dx = 0;
	}
	else if (snake->segments[0].y <= PLAY_AREA_Y) {
		snake->dx = 1;
		snake->dy = 0;
	}
	else if (snake->segments[0].y >= PLAY_AREA_Y + PLAY_AREA_HEIGHT - CELL_SIZE) {
		snake->dx = -1;
		snake->dy = 0;
	}
}

/////////////////////////////////////////////Blue/Red Dot (food)//////////////////////

void HandleRedDot(RedDot& redDot, Snake* snake, double delta) {
	if (!redDot.isVisible && rand() % 10000 < 1) {
		do {
			redDot.position.x = (PLAY_AREA_X + CELL_SIZE) + (rand() % ((PLAY_AREA_WIDTH - 2 * CELL_SIZE) / CELL_SIZE)) * CELL_SIZE;
			redDot.position.y = (PLAY_AREA_Y + CELL_SIZE) + (rand() % ((PLAY_AREA_HEIGHT - 2 * CELL_SIZE) / CELL_SIZE)) * CELL_SIZE;
		} while (IsFoodUnderSnake(redDot.position, snake));

		redDot.isVisible = true;
		redDot.remainingTime = 4.0;
		redDot.bonusType = rand() % 2;
	}

	if (redDot.isVisible) {
		redDot.remainingTime -= delta;
		if (redDot.remainingTime <= 0) {
			redDot.isVisible = false;
		}
	}
}

void CheckAndEatRedDot(Snake& snake, RedDot& redDot, int& interval_time) {
	if (redDot.isVisible
		&& snake.segments[0].x == redDot.position.x
		&& snake.segments[0].y == redDot.position.y)
	{
		snake.score += 2;
		if (redDot.bonusType == 0) {
			snake.length = (snake.length - 3 >= 2) ? (snake.length - 3) : 2;
		}
		else if (redDot.bonusType == 1) {
			interval_time *= 1.25;
		}
		redDot.isVisible = false;
	}
}

bool IsFoodUnderSnake(SDL_Point food, Snake* snake) {
	for (int i = 0; i < snake->length; i++) {
		if (food.x == snake->segments[i].x && food.y == snake->segments[i].y) {
			return true;
		}
	}
	return false;
}

void CheckAndEatFood(Snake& snake, SDL_Point& food) {
	if (snake.segments[0].x == food.x && snake.segments[0].y == food.y) {
		snake.isGrowing = true;
		snake.score++;
		do {
			food.x = (PLAY_AREA_X + CELL_SIZE)
				+ (rand() % ((PLAY_AREA_WIDTH - 2 * CELL_SIZE) / CELL_SIZE)) * CELL_SIZE;
			food.y = (PLAY_AREA_Y + CELL_SIZE)
				+ (rand() % ((PLAY_AREA_HEIGHT - 2 * CELL_SIZE) / CELL_SIZE)) * CELL_SIZE;
		} while (IsFoodUnderSnake(food, &snake));
	}
}