#include <iostream>
#include <Windows.h>
#include <chrono>
#include <string.h>
#include "olcConsoleGameEngine.h"

using namespace std;

// Screen-related constants
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 70
#define FONT_WIDTH 4
#define FONT_HEIGHT 8

// Map-related constants
#define MAP_HEIGHT 16
#define MAP_WIDTH 16

// Angle-related constants
#define RAD_270 4.7123889f
#define RAD_135 2.3561944f
#define RAD_90 1.5707963f
#define RAD_45 0.7853981f
#define PI 3.1415926f

// Vizualization-related constants
#define VIEW_DEPTH 16
#define FIELD_VIEW 1.5707963f
#define BORDER_WIDTH .12f
#define PLAYER_HEIGHT 1.f

// Player-related constants
#define MOV_SPEED .75f
#define ROTATION_SPEED .5f

// Sprites
olcSprite* wallSprite = new olcSprite(L"wall.spr");
olcSprite* lampSprite = new olcSprite(L"lamp.spr");
olcSprite* fireballSprite = new olcSprite(L"fireball.spr");
olcSprite* grassSprite = new olcSprite(16, 16);

float PlayerX = 8,
	  PlayerY = 8,
	  PlayerA = PI;

int main()
{
	// Set up console font
	{
		CONSOLE_FONT_INFOEX font = {
			sizeof(CONSOLE_FONT_INFOEX),	// Size of the structure
			0,								// Depth location on system's font table
			{ FONT_WIDTH, FONT_HEIGHT },	// Width and Height
			FW_EXTRALIGHT,					// Font family
			FW_NORMAL,						// Font tickness
			L"Consolas"						// Font name
		};
		SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), false, &font);
	}

	// Creates the map
	wstring map = L"";
	map += L"################";
	map += L"#.#............#";
	map += L"#.#............#";
	map += L"#.#............#";
	map += L"#.#............#";
	map += L"#.#............#";
	map += L"#.#............#";
	map += L"#.#............#";
	map += L"#.#............#";
	map += L"#.........######";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#.#..#..#..#..##";
	map += L"#..............#";
	map += L"################";

	// Creates the sprite
	{
		const char* letters =
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  " \
			"   ^    ^    ^  ";
		
		WORD colors[6] = {
			FOREGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY
		};
		WORD color = FOREGROUND_RED | BACKGROUND_GREEN;

		for (int y = 0; y < 16; y++)
			for (int x = 0; x < 16; x++) {
				grassSprite->SetGlyph(x, y, letters[y * 16 + x]);
				grassSprite->SetColour(x, y, colors[0]);
			}
	}

	// Set up console
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CHAR_INFO screen[SCREEN_WIDTH * SCREEN_HEIGHT];
	SMALL_RECT screenRect = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 },
			   windowRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	COORD finalPoint = { SCREEN_WIDTH, SCREEN_HEIGHT },
		  initalPoint = { 0, 0 };

	SetConsoleWindowInfo(console, true, &screenRect);
	SetConsoleScreenBufferSize(console, { SCREEN_WIDTH, SCREEN_HEIGHT });
	SetConsoleTitle(L"Console FPS ;)");
	SetConsoleActiveScreenBuffer(console);
	
	// Initializes the game-loop
	chrono::system_clock::time_point
		tp1 = chrono::system_clock::now(),
		tp2;
	float dt;

	while (1)
	{
		// Calculates Delta-time
		tp2 = chrono::system_clock::now();
		dt = (tp2 - tp1).count() / 1000000.f;
		tp1 = chrono::system_clock::now();

		// Character controller
		// Rotation
		if (GetAsyncKeyState('A'))
			PlayerA -= ROTATION_SPEED * dt;
		if (GetAsyncKeyState('D'))
			PlayerA += ROTATION_SPEED * dt;

		// Movement
		int dirFront = (bool)(GetAsyncKeyState('W') || GetAsyncKeyState(VK_UP)) -
			((bool)(GetAsyncKeyState('S') || GetAsyncKeyState(VK_DOWN))),
			dirSide = (bool)GetAsyncKeyState(VK_RIGHT) - (bool)GetAsyncKeyState(VK_LEFT);

		float ForceX = 0.f,
			ForceY = 0.f;

		if (dirFront) {
			ForceX = dirFront * sinf(PlayerA) * MOV_SPEED * dt;
			ForceY = dirFront * cosf(PlayerA) * MOV_SPEED * dt;
		}
		if (dirSide) {
			ForceX += dirSide * sinf(RAD_90 + PlayerA) * MOV_SPEED * dt;
			ForceY += dirSide * cosf(RAD_90 + PlayerA) * MOV_SPEED * dt;
		}

		if (map[(int)(PlayerY += ForceY) * MAP_WIDTH + (int)PlayerX] == '#')
			PlayerY -= ForceY;
		if (map[(int)PlayerY * MAP_WIDTH + (int)(PlayerX += ForceX)] == '#')
			PlayerX -= ForceX;

		// Renderization
		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			float currentAngle = (PlayerA - FIELD_VIEW * .5f) + ((float)x / (float)SCREEN_WIDTH) * FIELD_VIEW,
				sin = sinf(currentAngle),
				cos = cosf(currentAngle),
				rayDistance = 0.f,
				sampleX = 0.f;
			char textureHitten = ' ';

			bool outOfRange = false;
			// Ray-casting until hit a wall or get out of range			
			do
			{
				float rayX = PlayerX + sin * (rayDistance += .02f),
				      rayY = PlayerY + cos * rayDistance;
				int posX = (int)rayX,
					posY = (int)rayY;

				// Out of range verification
				if (posX < 0 || posX >= MAP_WIDTH || posY < 0 || posY >= MAP_HEIGHT)
					break;				
				else
				{
					// Hitted a wall verification
					if (map[posY * MAP_WIDTH + posX] == '#')
					{
						float midPointX = posX + .5f,
							midPointY = posY + .5f,
							angle = atan2f(rayY - midPointY, rayX - midPointX);

						if ((angle >= -RAD_45 && angle < RAD_45) || (angle >= RAD_135 || angle < -RAD_135))
							sampleX = rayY - (float)posY;
						if ((angle >= RAD_45 && angle < RAD_135) || (angle < -RAD_45 && angle >= -RAD_135))
							sampleX = rayX - (float)posX;
						

						textureHitten = '#';
						break;
					}
				}
			} while (rayDistance < VIEW_DEPTH);

			// Column drawing
			float ceilingY = SCREEN_HEIGHT * .5f - SCREEN_HEIGHT * .5f / rayDistance,
				floorY = SCREEN_HEIGHT - ceilingY;

			float wallHeight = floorY - ceilingY;

			int fx = ((int)pow(x - (SCREEN_WIDTH >> 1), 2) / 600);
			for (int y = 0; y < SCREEN_HEIGHT; y++)
				if (y <= ceilingY)
					screen[y * SCREEN_WIDTH + x] = { { ' ' }, BACKGROUND_BLUE };
				else if (y < floorY)
				{
					switch (textureHitten) {
						case '#':
							{
								WCHAR letter;
								if (rayDistance < VIEW_DEPTH * .33f)		letter = 0x2588;
								else if (rayDistance < VIEW_DEPTH * .67f)	letter = 0x2593;
								else										letter = 0x2592;
								WORD color = BACKGROUND_BLUE | (WORD)wallSprite->SampleColour(sampleX, (float)(y - ceilingY) / (float)(floorY - ceilingY));
								screen[y * SCREEN_WIDTH + x] = { { letter }, color };
							}
							break;

						default:
							screen[y * SCREEN_WIDTH + x] = { { ' ' }, BACKGROUND_BLUE };
							break;
					}
				}
				else
				{
					int divs = (SCREEN_HEIGHT >> 1) - 1;
					float verticalAngle = (float)(y - divs) / (float)divs * RAD_90,
						verticalCos = cosf(verticalAngle),
						verticalSin = sinf(verticalAngle),
						verticalDistX = 0.f,
						verticalDistY = 0.f;
					do
						verticalDistX += verticalCos * .02f,
						verticalDistY += verticalSin * .02f;
					while (verticalDistY < PLAYER_HEIGHT);

					float textureX = PlayerX + sin * verticalDistX,
						textureY = PlayerY + cos * verticalDistX,
						sampletX = textureX - (int)textureX,
						sampletY = textureY - (int)textureY;

					screen[y * SCREEN_WIDTH + x] = { 
						{
							(WCHAR)grassSprite->SampleGlyph(sampletX, sampletY)
						},
						(WORD)grassSprite->SampleColour(sampletX, sampletY)
					};
				}
		}

		// Map drawing
		for (int my = 0; my < MAP_HEIGHT; my++)
			for (int mx = 0; mx < MAP_WIDTH; mx++)
				screen[my * SCREEN_WIDTH + SCREEN_WIDTH - mx - 1] = { { map[my * MAP_WIDTH + mx] }, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE};
		screen[(int)PlayerY * SCREEN_WIDTH + SCREEN_WIDTH - (int)PlayerX - 1].Char.UnicodeChar = 'Y';

		WriteConsoleOutput(console, screen, finalPoint, initalPoint, &windowRect);
	}
	return 0;
}
