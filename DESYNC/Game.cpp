#include "Game.h"
#include<iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Game::Game() : mIsRunning(true), mWindow(nullptr), mRenderer(nullptr) {}
Game:: ~Game() {}

SDL_Texture* Game::LoadTexture(const char* fileName) {
	int width, height, channels;
	unsigned char* pixels = stbi_load(fileName, &width, &height, &channels, 4);

	if (!pixels)
		return nullptr;

	SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * 4);

	if (!surface) {
		stbi_image_free(pixels);
		return nullptr;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(mRenderer, surface);
	SDL_DestroySurface(surface);
	stbi_image_free(pixels);

	return texture;
}

bool Game::Initialize() {

	srand((unsigned int)time(NULL));

	mLastTick = SDL_GetTicks();

	mPlayer.x = 0.0f;
	mPlayer.z = 0.0f;
	mPlayer.attackCooldown = .5f;
	mPlayer.hp = 100;
	mPlayer.angle = 0.0f;
	mPlayer.flashTimer = 0.0f;

	mSyncLevel = 0.0f;

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		return false;
	}

	if (!SDL_CreateWindowAndRenderer("DESYNC", 1920, 1080, SDL_WINDOW_FULLSCREEN, &mWindow, &mRenderer)) {
		return false;
	}

	SDL_SetRenderLogicalPresentation(mRenderer, 1920, 1080, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	mSpawnTimer = 0.0f;
	mHitStopTimer = 0.0f;
	mScreenShakeTimer = 0.0f;

	//load assets
	mInvaderTexture[0] = LoadTexture("Invaderwalk1.png");
	mInvaderTexture[1] = LoadTexture("Invaderwalk2.png");
	mSyncerTexture = LoadTexture("sync1.png");
	mSwordTexture = LoadTexture("sword.png");
	mMedkitTexture = LoadTexture("medkit.png");


	//object pooling
	for (int i = 0; i < 10; i++) {
		mEnemies.push_back({ 0.0f, 0.0f, 0.0f ,EnemyType::INVADER, 0, false, 0.0f });
	}

	//audio
	SDL_AudioSpec targetSpec;
	targetSpec.format = SDL_AUDIO_F32;
	targetSpec.channels = 1;
	targetSpec.freq = 44100;

	if (!SDL_LoadWAV("background.wav", &mWaveSpec, &mWaveBuffer, &mWaveLen)) {
		std::cout << "wav could not be uploaded: " << SDL_GetError() << std::endl;
	}

	mAudioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &mWaveSpec, nullptr, nullptr);

	if (mAudioStream) {
		SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(mAudioStream));
	}

	mWavePos = 0;

	SDL_SetWindowRelativeMouseMode(mWindow, true);

	return true;
}

void Game::ProccesInput() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) mIsRunning = false;

		if (event.type == SDL_EVENT_KEY_DOWN) {
			if (event.key.key == SDLK_ESCAPE) {
				mIsRunning = false;
			}

			if (mCurrentState == GameState::START && event.key.key == SDLK_SPACE) {
				mCurrentState = GameState::PLAYING;
				SDL_SetWindowRelativeMouseMode(mWindow, true);
			}

			if (event.key.key == SDLK_P) {
				if (mCurrentState == GameState::PLAYING) {
					mCurrentState = GameState::PAUSED;
					SDL_SetWindowRelativeMouseMode(mWindow, false);
				}
				else if (mCurrentState == GameState::PAUSED) {
					mCurrentState = GameState::PLAYING;
					SDL_SetWindowRelativeMouseMode(mWindow, true);
				}
			}

			if (mCurrentState == GameState::GAMEOVER && event.key.key == SDLK_R) {
				mPlayer.hp = 100;
				mSyncLevel = 0;
				mPlayer.x = 0; mPlayer.z = 0;

				for (auto& enemy : mEnemies) enemy.isActive = false;
				mMedkits.clear();

				mCurrentState = GameState::PLAYING;
				SDL_SetWindowRelativeMouseMode(mWindow, true);
			}
		}
	}
	mKeyboardState = SDL_GetKeyboardState(NULL);
}

void Game::Update(float deltaTime) {

	if (mCurrentState != GameState::PLAYING) return;

	if (mScreenShakeTimer > 0.0f) mScreenShakeTimer -= deltaTime;
	if (mHitStopTimer > 0.0f) { mHitStopTimer -= deltaTime; return; }


	float mouseDeltaX, mouseDeltaY;
	SDL_GetRelativeMouseState(&mouseDeltaX, &mouseDeltaY);
	float sensitivity = .001f;
	mPlayer.angle += mouseDeltaX * sensitivity;

	if (mPlayer.flashTimer > 0.0f) mPlayer.flashTimer -= deltaTime;


	//forward vector with trigonometry
	float directionX = std::sin(mPlayer.angle);
	float directionZ = std::cos(mPlayer.angle);
	//right vector with trigonometry
	float rightX = std::cos(mPlayer.angle);
	float rightZ = -std::sin(mPlayer.angle);

	float moveSpeed = 5.0f;

	//forward movement
	if (mKeyboardState[SDL_SCANCODE_W]) {
		mPlayer.x += directionX * moveSpeed * deltaTime;
		mPlayer.z += directionZ * moveSpeed * deltaTime;
	}
	//backward movement
	if (mKeyboardState[SDL_SCANCODE_S]) {
		mPlayer.x -= directionX * moveSpeed * deltaTime;
		mPlayer.z -= directionZ * moveSpeed * deltaTime;
	}
	//left movement
	if (mKeyboardState[SDL_SCANCODE_A]) {
		mPlayer.x -= rightX * moveSpeed * deltaTime;
		mPlayer.z -= rightZ * moveSpeed * deltaTime;
	}
	//right movement
	if (mKeyboardState[SDL_SCANCODE_D]) {
		mPlayer.x += rightX * moveSpeed * deltaTime;
		mPlayer.z += rightZ * moveSpeed * deltaTime;
	}

	if (mPlayer.attackCooldown > 0.0f) mPlayer.attackCooldown -= deltaTime;
	for (auto& enemy : mEnemies) {
		if (enemy.flashTimer > 0.0f) enemy.flashTimer -= deltaTime;
	}


	Uint32 mouseState = SDL_GetMouseState(nullptr, nullptr);

	if ((mouseState & SDL_BUTTON_LMASK) && mPlayer.attackCooldown <= 0.0f) {
		mPlayer.attackCooldown = 0.3;

		//calculate range
		for (auto& enemy : mEnemies) {

			float relativeX = enemy.x - mPlayer.x;
			float relativeZ = enemy.z - mPlayer.z;

			float cosAngle = std::cos(mPlayer.angle);
			float sinAngle = std::sin(mPlayer.angle);
			float rotationX = (relativeX * cosAngle) - (relativeZ * sinAngle);
			float rotationZ = (relativeX * sinAngle) + (relativeZ * cosAngle);

			//hit detection
			if (rotationZ >= 0.0f && rotationZ <= 3.0f && std::abs(rotationX) < 1.0f) {
				enemy.hp -= 35;
				enemy.flashTimer = 0.1f;
				mHitStopTimer = 0.04f;
				mScreenShakeTimer = 0.05f;
			}
		}

		for (auto& enemy : mEnemies) {
			if (!enemy.isActive) continue;

			if (enemy.isActive && enemy.hp <= 0) {
				enemy.isActive = false;

				if (enemy.type == EnemyType::SYNCER) {
					if (rand() % 100 < 80) //drop medkit with %80 chance
						mMedkits.push_back({ enemy.x,enemy.z,true });
				}
			}
		}

	}
	for (auto& enemy : mEnemies) {
		if (!enemy.isActive) continue;

		if (enemy.type == EnemyType::INVADER) {
			float distanceX = mPlayer.x - enemy.x;
			float distanceZ = mPlayer.z - enemy.z;

			float distance = std::sqrt((distanceX * distanceX) + (distanceZ * distanceZ)); //finding the hypotenuse using pythagorean theorem.

			if (enemy.attackTimer > 0.0f) enemy.attackTimer -= deltaTime;

			if (distance > 1.5f) {
				float enemySpeed = 2.0f;

				enemy.x += (distanceX / distance) * enemySpeed * deltaTime;
				enemy.z += (distanceZ / distance) * enemySpeed * deltaTime;
			}
			else
			{
				if (enemy.attackTimer <= 0.0f && mPlayer.flashTimer <= 0.0f)
				{
					mPlayer.hp -= 5;
					mPlayer.flashTimer = 0.2f;
					enemy.attackTimer = 1.0f;
					mScreenShakeTimer = 0.4f;
				}
			}
		}
		else if (enemy.type == EnemyType::SYNCER) {
			mSyncLevel += 1.5f * deltaTime;
		}
	}
	//enemy spawner with object pooling
	mSpawnTimer -= deltaTime;
	if (mSpawnTimer <= 0.0f) {
		mSpawnTimer = 2.0f; //new enemies spawn every 2 seconds

		for (auto& enemy : mEnemies) {
			if (!enemy.isActive) {                     //pi↓    
				float randomAngle = (rand() % 360) * (3.14159f / 180.0f);
				float randomDist = 5.0f + (rand() % 11); //spawn at distance of 5-15 units

				enemy.x = mPlayer.x + (std::cos(randomAngle) * randomDist);
				enemy.z = mPlayer.z + (std::sin(randomAngle) * randomDist);

				enemy.hp = 100;
				enemy.flashTimer = 0.0f;

				//%30 -> syncer %70 -> invader
				if (rand() % 100 < 30) {
					enemy.type = EnemyType::SYNCER;
				}
				else {
					enemy.type = EnemyType::INVADER;
				}

				enemy.isActive = true;
				break;
			}
		}
	}

	mSyncLevel -= 1.0f * deltaTime;

	if (mSyncLevel > 100)
	{
		mSyncLevel = 100;
	}
	if (mSyncLevel < 0)
	{
		mSyncLevel = 0;
	}

	//taking medkit
	if (mKeyboardState[SDL_SCANCODE_E]) {
		for (auto& kit : mMedkits) {
			if (!kit.isActive) continue;

			float distanceX = mPlayer.x - kit.x;
			float distanceZ = mPlayer.z - kit.z;
			float distance = std::sqrt(distanceX * distanceX + distanceZ * distanceZ);

			if (distance < 1.5f) {
				mPlayer.hp += 25;
				if (mPlayer.hp > 100) mPlayer.hp = 100;
				kit.isActive = false;
			}
		}
	}

	//audio glitch
	if (mAudioStream && mWaveBuffer) {
		if (SDL_GetAudioStreamQueued(mAudioStream) < (mWaveSpec.freq / 10)) {

			int bytesToWrite = 1024;
			if (mWavePos + bytesToWrite > mWaveLen) mWavePos = 0;

			int glitchChance = 0;
			if (mSyncLevel < 30.0f)      glitchChance = 20;
			else if (mSyncLevel < 60.0f) glitchChance = 15;
			else if (mSyncLevel < 90.0f) glitchChance = 10;

			if (glitchChance > 0 && (rand() % 100 < glitchChance)) {
				mWavePos -= (mWavePos > 2048) ? 2048 : 0;
			}

			SDL_PutAudioStreamData(mAudioStream, mWaveBuffer + mWavePos, bytesToWrite);

			mWavePos += bytesToWrite;
		}
	}

	//gameover controller
	if (mPlayer.hp <= 0 || mSyncLevel >= 100.0f) {
		mCurrentState = GameState::GAMEOVER;
		SDL_SetWindowRelativeMouseMode(mWindow, false);
	}
}

void Game::Render() {
	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
	SDL_RenderClear(mRenderer);

	// calculate screen shake
	float shakeOffsetX = 0.0f;
	float shakeOffsetY = 0.0f;
	if (mScreenShakeTimer > 0.0f) {
		shakeOffsetX = (rand() % 16 - 8) * (mScreenShakeTimer / 0.2f);
		shakeOffsetY = (rand() % 16 - 8) * (mScreenShakeTimer / 0.2f);
	}

	float centerX = 960.0f + shakeOffsetX;
	float centerY = 540.0f + shakeOffsetY;
	float currentHorizonY = 560.0f + shakeOffsetY;

	// --- BACKGROUND (SKY & FLOOR) ---
	SDL_FRect skyRect{ -200.0f, -200.0f + shakeOffsetY, 2300.0f, currentHorizonY + 200.0f };
	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
	SDL_RenderFillRect(mRenderer, &skyRect);

	//floor base
	SDL_FRect floorRect{ -200.0f, currentHorizonY, 2300.0f, 1200.0f };
	SDL_SetRenderDrawColor(mRenderer, 0, 40, 0, 255);
	SDL_RenderFillRect(mRenderer, &floorRect);

	//grid lines (horizontal)
	for (int z = 45; z > 1; z--) {
		float lineY = currentHorizonY + (2000.0f / (z + 2.0f));
		if (lineY < 1080.0f + shakeOffsetY) {
			SDL_SetRenderDrawColor(mRenderer, 0, 60, 0, 255);
			SDL_RenderLine(mRenderer, 0, lineY, 1920, lineY);
		}
	}

	//grid lines (vertical)
	float playerAngle = mPlayer.angle;
	float gridAngleShift = std::fmod(playerAngle * 250.0f, 100.0f);
	SDL_SetRenderDrawColor(mRenderer, 0, 80, 0, 255);

	for (int i = -30; i < 50; i++) {
		float startX_bottom = centerX + (i * 100.0f) - gridAngleShift;
		SDL_RenderLine(mRenderer, centerX, currentHorizonY, startX_bottom, 1080.0f + shakeOffsetY);
	}

	switch (mCurrentState) {
	case GameState::START:
		DrawUIOverlay("DESYNC - PRESS SPACE TO START", { 0, 255, 255, 255 });
		break;

	case GameState::PLAYING:
	{
		// --- SPRITE RENDERING (ENEMIES & MEDKITS) ---
		float sinAngle = std::sin(playerAngle);
		float cosAngle = std::cos(playerAngle);

		//unified render item for depth sorting
		struct RenderItem {
			float rotationX, rotationZ;
			float size;
			bool isMedkit;
			bool isFlash;
			EnemyType type;
		};
		std::vector<RenderItem> renderList;

		// collect enemies
		for (auto& enemy : mEnemies) {
			if (!enemy.isActive) continue;
			float relX = enemy.x - mPlayer.x;
			float relZ = enemy.z - mPlayer.z;
			float rotX = (relX * cosAngle) - (relZ * sinAngle);
			float rotZ = (relX * sinAngle) + (relZ * cosAngle);
			if (rotZ > 0.1f) {
				float scale = 960.0f / rotZ;
				renderList.push_back({ rotX, rotZ, 2.0f * scale, false, enemy.flashTimer > 0.0f, enemy.type });
			}
		}

		// collect medkits
		for (auto& kit : mMedkits) {
			if (!kit.isActive) continue;
			float relX = kit.x - mPlayer.x;
			float relZ = kit.z - mPlayer.z;
			float rotX = (relX * cosAngle) - (relZ * sinAngle);
			float rotZ = (relX * sinAngle) + (relZ * cosAngle);
			if (rotZ > 0.1f) {
				float scale = 960.0f / rotZ;
				renderList.push_back({ rotX, rotZ, 0.5f * scale, true, false, EnemyType::INVADER });
			}
		}

		// sort by depth
		std::sort(renderList.begin(), renderList.end(), [](const RenderItem& a, const RenderItem& b) {
			return a.rotationZ > b.rotationZ;
			});

		// draw sprites
		int currentFrame = (SDL_GetTicks() / 300) % 2;
		for (auto& item : renderList) {
			float scale = 960.0f / item.rotationZ;
			SDL_FRect rect;
			rect.w = item.size;
			rect.h = item.size;
			rect.x = centerX + (item.rotationX * scale) - (item.size / 2.0f);
			rect.y = (centerY + (1.0f * scale)) - item.size;

			if (item.isMedkit) {
				SDL_RenderTexture(mRenderer, mMedkitTexture, nullptr, &rect);
			}
			else {
				SDL_Texture* tex = (item.type == EnemyType::SYNCER) ? mSyncerTexture : mInvaderTexture[currentFrame];
				if (tex) {
					if (item.isFlash) {
						SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_ADD);
						SDL_SetTextureColorMod(tex, 255, 255, 255);

						for (int i = 0; i < 4; i++) {
							SDL_RenderTexture(mRenderer, tex, nullptr, &rect);
						}
						SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
					}
					else {
						SDL_SetTextureColorMod(tex, 255, 255, 255);
						SDL_RenderTexture(mRenderer, tex, nullptr, &rect);
					}
				}
			}
		}

		// --- SWORD ---
		float texWidth, texHeight;
		SDL_GetTextureSize(mSwordTexture, &texWidth, &texHeight);

		float finalSwordHeight = 900.0f;
		float finalSwordWidth = finalSwordHeight * (texWidth / texHeight);

		SDL_FRect swordRect;
		swordRect.w = finalSwordWidth;
		swordRect.h = finalSwordHeight;

		// position: shift to right side
		swordRect.x = 1920.0f - finalSwordWidth - 150.0f + shakeOffsetX;

		float rotationAngle = 0.0f;
		float yOffset = 0.0f;

		// procedural animation logic
		if (mPlayer.attackCooldown > 0.0f) {
			float t = 1.0f - (mPlayer.attackCooldown / 0.3f);
			if (t < 0.4f) { // fast swing
				rotationAngle = (t / 0.4f) * -90.0f;
				yOffset = (t / 0.4f) * 80.0f;
			}
			else { // recovery
				rotationAngle = -90.0f + (((t - 0.4f) / 0.6f) * 90.0f);
				yOffset = 80.0f - (((t - 0.4f) / 0.6f) * 80.0f);
			}
		}
		else { // idle
			yOffset = std::sin(SDL_GetTicks() * 0.005f) * 15.0f;
			rotationAngle = std::sin(SDL_GetTicks() * 0.002f) * 2.0f;
		}

		swordRect.y = 1080.0f - finalSwordHeight + 150.0f + shakeOffsetY + yOffset;

		SDL_FPoint pivot = { finalSwordWidth / 2.0f, finalSwordHeight * 0.9f };
		SDL_RenderTextureRotated(mRenderer, mSwordTexture, nullptr, &swordRect, rotationAngle, &pivot, SDL_FLIP_NONE);

		// --- UI (SYNC & HEALTH) ---
		// background
		SDL_FRect syncBg = { 40, 40, 400, 30 };
		SDL_SetRenderDrawColor(mRenderer, 50, 50, 50, 255);
		SDL_RenderFillRect(mRenderer, &syncBg);

		// movable part for sync bar
		SDL_FRect syncBar = { 40, 40, (mSyncLevel / 100.0f) * 400.0f, 30 };
		SDL_SetRenderDrawColor(mRenderer, 0, 255, 255, 255);
		SDL_RenderFillRect(mRenderer, &syncBar);

		// background
		SDL_FRect healthBg = { 40, 80, 400, 30 };
		SDL_SetRenderDrawColor(mRenderer, 50, 0, 0, 255);
		SDL_RenderFillRect(mRenderer, &healthBg);

		// movable part for health bar
		SDL_FRect healthBar = { 40, 80, (mPlayer.hp / 100.0f) * 400.0f, 30 };
		SDL_SetRenderDrawColor(mRenderer, 255, 0, 0, 255);
		SDL_RenderFillRect(mRenderer, &healthBar);

		SDL_SetRenderDrawColor(mRenderer, 200, 200, 200, 180);
		SDL_RenderDebugText(mRenderer, 1750, 40, "[P] PAUSE");
		SDL_RenderDebugText(mRenderer, 1750, 60, "[ESC] QUIT");

		if (mSyncLevel > 70.0f) {
			SDL_SetRenderDrawColor(mRenderer, 255, 50, 50, 255);
			SDL_RenderDebugText(mRenderer, 900, 150, "!!! SYSTEM CRITICAL !!!");
		}

		// Player damage flash
		if (mPlayer.flashTimer > 0.0f) {
			SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(mRenderer, 255, 0, 0, 100);
			SDL_FRect flash = { 0, 0, 1920, 1080 };
			SDL_RenderFillRect(mRenderer, &flash);
			SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_NONE);
		}
	}
	break;

	case GameState::PAUSED:
		DrawUIOverlay("PAUSED - PRESS P TO RESUME", { 255, 255, 0, 255 });
		break;

	case GameState::GAMEOVER:
		DrawUIOverlay("YOU ARE SYNCED - PRESS R TO RESTART", { 255, 0, 0, 255 });
		break;
	}

	SDL_RenderPresent(mRenderer);
}

void Game::RunLoop() {
	while (mIsRunning)
	{
		ProccesInput();

		Uint64 currentTick = SDL_GetTicks();
		float deltaTime = (currentTick - mLastTick) / 1000.0f; //ms type

		if (deltaTime > 0.05f) deltaTime = 0.05f; //optional - for protection

		mLastTick = currentTick;

		Update(deltaTime);
		Render();
	}
}

void Game::DrawUIOverlay(const char* text, SDL_Color color) {
	SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 180);

	SDL_FRect overlay = { 0.0f, 0.0f, 1920.0f, 1080.0f };
	SDL_RenderFillRect(mRenderer, &overlay);

	SDL_SetRenderDrawColor(mRenderer, color.r, color.g, color.b, 255);

	SDL_SetRenderScale(mRenderer, 4.0f, 4.0f);

	int textLength = 0;
	while (text[textLength] != '\0') textLength++;

	float textX = 240.0f - (textLength * 4.0f);
	float textY = 135.0f - 4.0f;

	SDL_RenderDebugText(mRenderer, textX, textY, text);

	SDL_SetRenderScale(mRenderer, 1.0f, 1.0f);
	SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_NONE);
}

void Game::Shutdown() {
	for (int i = 0; i < 2; i++) {
		if (mInvaderTexture[i]) SDL_DestroyTexture(mInvaderTexture[i]);
	}

	if (mMedkitTexture) SDL_DestroyTexture(mMedkitTexture);
	if (mSyncerTexture) SDL_DestroyTexture(mSyncerTexture);
	if (mSwordTexture) SDL_DestroyTexture(mSwordTexture);

	if (mWaveBuffer) SDL_free(mWaveBuffer);
	if (mAudioStream) SDL_DestroyAudioStream(mAudioStream);

	if (mRenderer) SDL_DestroyRenderer(mRenderer);
	if (mWindow) SDL_DestroyWindow(mWindow);
	SDL_Quit();
}
