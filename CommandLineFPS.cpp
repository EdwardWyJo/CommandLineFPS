/*
OneLoneCoder.com - Command Line First Person Shooter (FPS) Engine
Linux port using ncurses
Original by Javidx9: https://github.com/onelonecoder

```
GNU GPLv3 - See original license header for full details.

Build:
	g++ -O2 -o fps fps_linux.cpp -lncurses
Run:
	./fps

Controls: A = Turn Left, D = Turn Right, W = Walk Forwards, S = Walk Backwards
Quit:     Q or Ctrl+C
```

*/

#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <ncurses.h>

using namespace std;

int nScreenWidth = 120;         // Console Screen Size X (columns)
int nScreenHeight = 40;         // Console Screen Size Y (rows)
int nMapWidth = 16;             // World Dimensions
int nMapHeight = 16;

float fPlayerX = 14.7f;         // Player Start Position
float fPlayerY = 5.09f;
float fPlayerA = 0.0f;          // Player Start Rotation
float fFOV = 3.14159f / 4.0f;  // Field of View
float fDepth = 16.0f;           // Maximum rendering distance
float fSpeed = 5.0f;            // Walking Speed

int main()
{
// Initialise ncurses
initscr();
noecho();
cbreak();
keypad(stdscr, TRUE);
nodelay(stdscr, TRUE);   // Non-blocking input (replaces GetAsyncKeyState)
curs_set(0);             // Hide cursor

// Resize terminal check
resizeterm(nScreenHeight, nScreenWidth);

// Screen buffer
char *screen = new char[nScreenWidth * nScreenHeight + 1];

// Create Map of world space: # = wall block, . = space
string map;
map += "#########.......";
map += "#...............";
map += "#.......########";
map += "#..............#";
map += "#......##......#";
map += "#......##......#";
map += "#..............#";
map += "###............#";
map += "##.............#";
map += "#......####..###";
map += "#......#.......#";
map += "#......#.......#";
map += "#..............#";
map += "#......#########";
map += "#..............#";
map += "################";

auto tp1 = chrono::system_clock::now();
auto tp2 = chrono::system_clock::now();

while (true)
{
	// Time differential per frame for consistent movement speed
	tp2 = chrono::system_clock::now();
	chrono::duration<float> elapsedTime = tp2 - tp1;
	tp1 = tp2;
	float fElapsedTime = elapsedTime.count();

	// Handle input (non-blocking via nodelay)
	int ch = getch();
	if (ch == 'q' || ch == 'Q') break;

	// Handle CCW Rotation
	if (ch == 'a' || ch == 'A')
		fPlayerA -= (fSpeed * 0.75f) * fElapsedTime;

	// Handle CW Rotation
	if (ch == 'd' || ch == 'D')
		fPlayerA += (fSpeed * 0.75f) * fElapsedTime;

	// Handle Forwards movement & collision
	if (ch == 'w' || ch == 'W')
	{
		fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
		fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
		if (map[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
		{
			fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
			fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
		}
	}

	// Handle backwards movement & collision
	if (ch == 's' || ch == 'S')
	{
		fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
		fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
		if (map[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
		{
			fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
			fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
		}
	}

	for (int x = 0; x < nScreenWidth; x++)
	{
		// For each column, calculate the projected ray angle into world space
		float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

		float fStepSize = 0.1f;
		float fDistanceToWall = 0.0f;

		bool bHitWall = false;
		bool bBoundary = false;

		float fEyeX = sinf(fRayAngle);
		float fEyeY = cosf(fRayAngle);

		while (!bHitWall && fDistanceToWall < fDepth)
		{
			fDistanceToWall += fStepSize;
			int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
			int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

			// Test if ray is out of bounds
			if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
			{
				bHitWall = true;
				fDistanceToWall = fDepth;
			}
			else
			{
				if (map[nTestX * nMapWidth + nTestY] == '#')
				{
					bHitWall = true;

					vector<pair<float, float>> p;

					for (int tx = 0; tx < 2; tx++)
						for (int ty = 0; ty < 2; ty++)
						{
							float vy = (float)nTestY + ty - fPlayerY;
							float vx = (float)nTestX + tx - fPlayerX;
							float d = sqrt(vx * vx + vy * vy);
							float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
							p.push_back(make_pair(d, dot));
						}

					sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right)
						{ return left.first < right.first; });

					float fBound = 0.01f;
					if (acos(p.at(0).second) < fBound) bBoundary = true;
					if (acos(p.at(1).second) < fBound) bBoundary = true;
					if (acos(p.at(2).second) < fBound) bBoundary = true;
				}
			}
		}

		// Calculate distance to ceiling and floor
		int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
		int nFloor = nScreenHeight - nCeiling;

		// Shade walls based on distance using ASCII characters
		char nShade = ' ';
		if      (fDistanceToWall <= fDepth / 4.0f) nShade = '@';  // Very close
		else if (fDistanceToWall <  fDepth / 3.0f) nShade = 'O';
		else if (fDistanceToWall <  fDepth / 2.0f) nShade = 'o';
		else if (fDistanceToWall <  fDepth)         nShade = '.';
		else                                         nShade = ' '; // Too far away

		if (bBoundary) nShade = ' '; // Darken tile boundaries

		for (int y = 0; y < nScreenHeight; y++)
		{
			if (y <= nCeiling)
				screen[y * nScreenWidth + x] = ' ';
			else if (y > nCeiling && y <= nFloor)
				screen[y * nScreenWidth + x] = nShade;
			else // Floor
			{
				float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
				char fShade;
				if      (b < 0.25f) fShade = '#';
				else if (b < 0.5f)  fShade = 'x';
				else if (b < 0.75f) fShade = '.';
				else if (b < 0.9f)  fShade = '-';
				else                fShade = ' ';
				screen[y * nScreenWidth + x] = fShade;
			}
		}
	}

	// Write stats line into buffer
	char stats[120];
	snprintf(stats, sizeof(stats), "X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f  (WASD=move, Q=quit)",
		fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);
	for (int i = 0; i < nScreenWidth && stats[i]; i++)
		screen[i] = stats[i];

	// Draw minimap into buffer
	for (int nx = 0; nx < nMapWidth; nx++)
		for (int ny = 0; ny < nMapWidth; ny++)
			screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
	screen[((int)fPlayerX + 1) * nScreenWidth + (int)fPlayerY] = 'P';

	// Render buffer to ncurses window
	for (int y = 0; y < nScreenHeight; y++)
	{
		for (int x = 0; x < nScreenWidth; x++)
		{
			mvaddch(y, x, screen[y * nScreenWidth + x]);
		}
	}
	refresh();
}

// Clean up ncurses
endwin();
delete[] screen;
return 0;

}