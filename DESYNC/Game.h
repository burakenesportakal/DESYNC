#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

struct Player
{
	float x, z, attackCooldown;
	int hp;
	float angle;
	float flashTimer;
};

enum class EnemyType { INVADER, SYNCER };

enum class GameState { START, PLAYING, PAUSED, GAMEOVER };

struct Enemy {
	float x, z, flashTimer;
	EnemyType type;
	int hp;
	bool isActive;
	float attackTimer;
};

struct Medkit {
	float x, z;
	bool isActive;
};


class Game
{
public:
	Game();
	~Game();

	bool Initialize();
	void Shutdown();
	void RunLoop();

private:

	void ProccesInput();
	void Update(float deltaTime);
	void Render();
	SDL_Texture* LoadTexture(const char* fileName);

	bool mIsRunning;
	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	const bool* mKeyboardState;
	Uint64 mLastTick;
	float mSyncLevel;
	float mSpawnTimer;
	float mHitStopTimer;
	float mScreenShakeTimer;


	std::vector<Enemy> mEnemies;
	std::vector<Medkit> mMedkits;
	Player mPlayer;

	SDL_Texture* mInvaderTexture[2];
	SDL_Texture* mSyncerTexture;
	SDL_Texture* mSwordTexture;
	SDL_Texture* mMedkitTexture;


	//audio
	SDL_AudioStream* mAudioStream;
	Uint8* mWaveBuffer;
	Uint32 mWaveLen;
	Uint32 mWavePos;
	SDL_AudioSpec mWaveSpec;

	//UI
	GameState mCurrentState = GameState::START;
	void DrawUIOverlay(const char* text, SDL_Color color);
};

