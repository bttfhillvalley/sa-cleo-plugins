#define _USE_MATH_DEFINES
#include <cmath>
#include <map>
#include <set>
#include <string>
#include "irrKlang.h"
#include "plugin.h"
#include "CLEO.h"
#include "CCamera.h"
#include "CMenuManager.h"
#include "CMessages.h"
#include "CTheScripts.h"
#include "CTimer.h"
#include "CTxdStore.h"
#include "CWorld.h"
#include "eEntityStatus.h"
#include "extensions\ScriptCommands.h"

#define GRAVITY (0.008f)
#define Clamp(v, low, high) ((v) < (low) ? (low) : (v) > (high) ? (high) : (v))
#define SQR(x) ((x) * (x))

inline float DotProduct(const CVector& v1, const CVector& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

using namespace irrklang;
using namespace plugin;
using namespace std;

ofstream of("DEBUG", std::ofstream::app);

ISoundEngine* m_soundEngine;
char volume = 0;
int visibility;  // Hack for now, not sure how to pass this in as a (void*) without messing up the value
boolean paused = false;

struct GameSound {
	ISound* sound;
	CVehicle* vehicle;
	CVector offset{ 0.0,0.0,0.0 };
	bool spatial;
};

map<string, GameSound> soundMap;

class bttfsa {
public:


    bttfsa() {
		CPed *playa;
		CVector *pos, dir;
		vec3df cameraPos, soundPos, cameraDir;
		string key;
		float distance;

        // Initialise your plugin here
		if (CLEO_GetVersion() >= CLEO_VERSION) {
			//register opcodes
			CLEO_RegisterOpcode(0x3F02, getEngineStatus);
			CLEO_RegisterOpcode(0x3F03, turnOnEngine);
			CLEO_RegisterOpcode(0x3F04, getCurrentGear);
			CLEO_RegisterOpcode(0x3F05, setHover);
			//CLEO_RegisterOpcode(0x3F0A, replaceTex);

			CLEO_RegisterOpcode(0x3F0E, getWheelAngle);
			CLEO_RegisterOpcode(0x3F10, setCarComponentVisibility);
			CLEO_RegisterOpcode(0x3F11, setCarComponentIndexVisibility);
			CLEO_RegisterOpcode(0x3F12, setCarComponentAlpha);
			CLEO_RegisterOpcode(0x3F13, setCarComponentIndexAlpha);
			CLEO_RegisterOpcode(0x3F14, moveCarComponent);
			CLEO_RegisterOpcode(0x3F15, moveCarComponentIndex);
			CLEO_RegisterOpcode(0x3F16, rotateCarComponent);
			CLEO_RegisterOpcode(0x3F17, rotateCarComponentIndex);
			CLEO_RegisterOpcode(0x3F18, setCarComponentGlow);
			CLEO_RegisterOpcode(0x3F19, setCarComponentGlowIndex);
			CLEO_RegisterOpcode(0x3F1A, getGasPedalAudio);
			CLEO_RegisterOpcode(0x3F1B, getCarComponentPosition);
			CLEO_RegisterOpcode(0x3F1C, getCarComponentRotation);
			CLEO_RegisterOpcode(0x3F25, applyForwardForce);
			CLEO_RegisterOpcode(0x3F26, applyUpwardForce);
			CLEO_RegisterOpcode(0x3F27, getWheelStatus);
			CLEO_RegisterOpcode(0x3F28, setWheelStatus);
			CLEO_RegisterOpcode(0x3F2D, setCarEngineSound);
			CLEO_RegisterOpcode(0x3F2A, inRemote);
			CLEO_RegisterOpcode(0x3F31, getRotationMatrix);
			CLEO_RegisterOpcode(0x3F32, setRotationMatrix);
			CLEO_RegisterOpcode(0x3F33, getVelocity);
			CLEO_RegisterOpcode(0x3F34, getVelocityVector);
			CLEO_RegisterOpcode(0x3F35, setVelocityVector);
			CLEO_RegisterOpcode(0x3F36, getSteeringAngle);
			CLEO_RegisterOpcode(0x3F3E, isWheelsOnGround);
			CLEO_RegisterOpcode(0x3F3F, isWheelsNotOnGround);
			CLEO_RegisterOpcode(0x3F40, isCarComponentVisible);
			CLEO_RegisterOpcode(0x3F41, isCarComponentIndexVisible);
			CLEO_RegisterOpcode(0x3F47, getCarComponentAlpha);
			CLEO_RegisterOpcode(0x3F48, getCarComponentIndexAlpha);
			CLEO_RegisterOpcode(0x3F50, isCarComponentNotVisible);
			CLEO_RegisterOpcode(0x3F51, isCarComponentIndexNotVisible);
			CLEO_RegisterOpcode(0x3F52, fadeCarComponentAlpha);
			CLEO_RegisterOpcode(0x3F53, fadeCarComponentIndexAlpha);
			CLEO_RegisterOpcode(0x3F54, carComponentDigitOff);
			CLEO_RegisterOpcode(0x3F55, carComponentDigitOn);
			CLEO_RegisterOpcode(0x3F80, stopAllSounds);
			CLEO_RegisterOpcode(0x3F81, stopSound);
			CLEO_RegisterOpcode(0x3F82, isSoundPlaying);
			CLEO_RegisterOpcode(0x3F83, isSoundStopped);
			CLEO_RegisterOpcode(0x3F84, playSound);
			CLEO_RegisterOpcode(0x3F85, playSoundAtLocation);
			CLEO_RegisterOpcode(0x3F86, attachSoundToVehicle);
			CLEO_RegisterOpcode(0x3F90, playKeypad);
			CLEO_RegisterOpcode(0x3F91, stopSoundIndex);
			CLEO_RegisterOpcode(0x3F92, isSoundPlayingIndex);
			CLEO_RegisterOpcode(0x3F93, isSoundStoppedIndex);
			CLEO_RegisterOpcode(0x3F94, playSoundIndex);
			CLEO_RegisterOpcode(0x3F95, playSoundAtLocationIndex);
			CLEO_RegisterOpcode(0x3F96, attachSoundToVehicleIndex);
			CLEO_RegisterOpcode(0x3F97, setFrequency);
			CLEO_RegisterOpcode(0x3F98, setVolume);
			CLEO_RegisterOpcode(0x3F99, setDoppler);
			CLEO_RegisterOpcode(0x3F9A, setCarStatus);
			CLEO_RegisterOpcode(0x3F9B, getCarStatus);
			CLEO_RegisterOpcode(0x3F9C, setCarLights);
			CLEO_RegisterOpcode(0x3F9D, getCarLights);
		}
		else {
			MessageBox(HWND_DESKTOP, "An incorrect version of CLEO was loaded.", "bttfsa.SA.cleo", MB_ICONERROR);
		}
		Events::drawingEvent += [] {
			CarLights();
		};
		Events::initGameEvent += [&] {
			m_soundEngine = createIrrKlangDevice();
			m_soundEngine->setRolloffFactor(1.5f);
			Events::gameProcessEvent += [&] {
				// Set volume of sound engine to match game
				if (volume != FrontEndMenuManager.m_nSfxVolume) {
					volume = FrontEndMenuManager.m_nSfxVolume;

					m_soundEngine->setSoundVolume(volume / 127.0f);
				}
				playa = FindPlayerPed();
				if (playa)
				{
					pos = TheCamera.GetGameCamPosition();
					cameraPos.X = pos->x;
					cameraPos.Y = -1.0f * pos->y;
					cameraPos.Z = pos->z;

					dir = TheCamera.m_mCameraMatrix.up;
					cameraDir.X = dir.x;
					cameraDir.Y = -1.0f * dir.y;
					cameraDir.Z = dir.z;


					m_soundEngine->setListenerPosition(cameraPos, cameraDir, vec3df(0, 0, 0), vec3df(0, 0, 1));
				}
				if (!soundMap.empty()) {
					if (FrontEndMenuManager.m_bMenuActive && !paused) {

						for (auto const& [key, gamesound] : soundMap) {
							gamesound.sound->setIsPaused();
						}
						paused = true;
					}

					else if (!FrontEndMenuManager.m_bMenuActive) {
						auto itr = soundMap.begin();
						while (itr != soundMap.end()) {
							// Delete sound if its finished playing
							if (soundMap[itr->first].sound->isFinished()) {
								itr = soundMap.erase(itr);
								continue;
							}
							// Unpause sound if we're paused
							if (paused) {
								soundMap[itr->first].sound->setIsPaused(false);
							}
							if (soundMap[itr->first].vehicle) {
								// Attach sound to vehicle
								Command<Commands::GET_OFFSET_FROM_CAR_IN_WORLD_COORDS>(soundMap[itr->first].vehicle, soundMap[itr->first].offset.x, soundMap[itr->first].offset.y, soundMap[itr->first].offset.z, &soundPos.X, &soundPos.Y, &soundPos.Z);
								soundPos.Y *= -1.0;
								of << "Car Sound: " << soundPos.X << " " << soundPos.Y << " " << soundPos.Z << endl;
								soundMap[itr->first].sound->setPosition(soundPos);
							}
							else {
								// Set sound to specified location
								soundPos.X = soundMap[itr->first].offset.x;
								soundPos.Y = soundMap[itr->first].offset.y;
								soundPos.Z = soundMap[itr->first].offset.z;
								soundMap[itr->first].sound->setPosition(soundPos);
							}

							// Mute sound if > 150 units away, otherwise play at full volume
							/*distance = (float)cameraPos.getDistanceFrom(soundPos);
							if (distance < 150.0f || !soundMap[itr->first].spatial) {
								soundMap[itr->first].sound->setVolume(1.0f);
							}
							else {
								soundMap[itr->first].sound->setVolume(0.0f);
							}*/
							++itr;
						}
						paused = false;
					}
				}

				// Removes dynamically created objects.  Has to be here because game script tick causes them to flash briefly
				/*for (auto item : removeObjectQueue) {
					auto models = item.second;
					for (auto object : CPools::ms_pObjectPool) {
						if (models->find(object->m_nModelIndex) != models->end()) {
							CWorld::Remove(object);
						}
					}
					for (auto object : CPools::ms_pDummyPool) {
						if (models->find(object->m_nModelIndex) != models->end()) {
							CWorld::Remove(object);
						}
					}
				}*/
				/*animEntry* i = &anims[0];

				if (i->timeremain != 0) {
					of << i->timeremain << " " << i->dp.x << " " << i->dp.y << " " << i->dp.z << std::endl;
					CMatrix cmatrix(&i->frame->modelling, false);
					CVector cpos(cmatrix.pos);
					cmatrix.Rotate(i->dr.x, i->dr.y, i->dr.z);
					cmatrix.pos = cpos;
					//cmatrix.SetTranslateOnly(i->dp.x, i->dp.y, i->dp.z);


					cmatrix.UpdateRW();
					i->timeremain -= 1;
				}*/
			};
		};


    }

	static int16_t GetCarGunLeftRight(CPad* pad)
	{
		return plugin::CallMethodAndReturn<int16_t, 0x53FC50, CPad*>(pad);
	}

	static int16_t GetCarGunUpDown(CPad* pad)
	{
		return plugin::CallMethodAndReturn<int16_t, 0x53FC10, CPad*>(pad);
	}

	static void CarLights() {
		if (CPools::ms_pVehiclePool) {
			for (CVehicle* i : CPools::ms_pVehiclePool) {
				if (i != nullptr) {
					CDamageManager* dam = (CDamageManager*)i;
					if (i->m_nVehicleFlags.bLightsOn) {
						setVisibility(i, (char*)"intLight", 1);
						setGlow(i, (char*)"intLight", 1);
						if (dam->GetLightStatus(LIGHT_FRONT_LEFT) == DAMSTATE_OK)
						{
							setVisibility(i, (char*)"lightFl", 1);
							setGlow(i, (char*)"lightFl", 1);
						}
						else {
							setVisibility(i, (char*)"lightFl", 0);
							setGlow(i, (char*)"lightFl", 0);
						}
						if (dam->GetLightStatus(LIGHT_FRONT_RIGHT) == DAMSTATE_OK)
						{
							setVisibility(i, (char*)"lightFr", 1);
							setGlow(i, (char*)"lightFr", 1);
						}
						else {
							setVisibility(i, (char*)"lightFr", 0);
							setGlow(i, (char*)"lightFr", 0);
						}
						if (dam->GetLightStatus(LIGHT_REAR_RIGHT) == DAMSTATE_OK)
						{
							setVisibility(i, (char*)"lightRr", 1);
							setGlow(i, (char*)"lightRr", 1);
						}
						else {
							setVisibility(i, (char*)"lightRr", 0);
							setGlow(i, (char*)"lightRr", 0);
						}
						if (dam->GetLightStatus(LIGHT_REAR_LEFT) == DAMSTATE_OK)
						{
							setVisibility(i, (char*)"lightRl", 1);
							setGlow(i, (char*)"lightRl", 1);
						}
						else {
							setVisibility(i, (char*)"lightRl", 0);
							setGlow(i, (char*)"lightRl", 0);
						}
						if (dam->GetDoorStatus(BOOT) == DAMSTATE_OK) {
							setVisibility(i, (char*)"lightBt", 1);
							setGlow(i, (char*)"lightBt", 1);
						}

						else {
							setVisibility(i, (char*)"lightBt", 0);
							setGlow(i, (char*)"lightBt", 0);
						}
					}
					else {
						setVisibility(i, (char*)"intLight", 0);
						setGlow(i, (char*)"intLight", 0);
						setVisibility(i, (char*)"lightFl", 0);
						setGlow(i, (char*)"lightFl", 0);
						setVisibility(i, (char*)"lightFr", 0);
						setGlow(i, (char*)"lightFr", 0);
						setVisibility(i, (char*)"lightRr", 0);
						setGlow(i, (char*)"lightRr", 0);
						setVisibility(i, (char*)"lightRl", 0);
						setGlow(i, (char*)"lightRl", 0);
						setVisibility(i, (char*)"lightBt", 0);
						setGlow(i, (char*)"lightBt", 0);
					}
					if (dam->GetLightStatus(LIGHT_REAR_RIGHT) == DAMSTATE_OK)
					{
						if (i->m_fBreakPedal != 0) {
							setVisibility(i, (char*)"lightSr", 1);
							setGlow(i, (char*)"lightSr", 1);
						}
						else {
							setVisibility(i, (char*)"lightSr", 0);
							setGlow(i, (char*)"lightSr", 0);
						}
						if (i->m_fGasPedal < 0.0) {
							setVisibility(i, (char*)"lightRVr", 1);
							setGlow(i, (char*)"lightRVr", 1);
						}
						else {
							setVisibility(i, (char*)"lightRVr", 0);
							setGlow(i, (char*)"lightRVr", 0);
						}

					}
					else {
						setVisibility(i, (char*)"lightSr", 0);
						setGlow(i, (char*)"lightSr", 0);
						setVisibility(i, (char*)"lightRVr", 0);
						setGlow(i, (char*)"lightRVr", 0);
					}
					if (dam->GetLightStatus(LIGHT_REAR_LEFT) == DAMSTATE_OK)
					{
						if (i->m_fBreakPedal != 0) {
							setVisibility(i, (char*)"lightSl", 1);
							setGlow(i, (char*)"lightSl", 1);
						}
						else {
							setVisibility(i, (char*)"lightSl", 0);
							setGlow(i, (char*)"lightSl", 0);
						}
						if (i->m_fGasPedal < 0.0) {
							setVisibility(i, (char*)"lightRVl", 1);
							setGlow(i, (char*)"lightRVl", 1);
						}
						else {
							setVisibility(i, (char*)"lightRVl", 0);
							setGlow(i, (char*)"lightRVl", 0);
						}
					}
					else {
						setVisibility(i, (char*)"lightSl", 0);
						setGlow(i, (char*)"lightSl", 0);
						setVisibility(i, (char*)"lightRVl", 0);
						setGlow(i, (char*)"lightRVl", 0);
					}
					if (dam->GetDoorStatus(BOOT) == DAMSTATE_OK) {
						if (i->m_fBreakPedal != 0) {
							setVisibility(i, (char*)"lightR3", 1);
							setGlow(i, (char*)"lightR3", 1);
						}
						else {
							setVisibility(i, (char*)"lightR3", 0);
							setGlow(i, (char*)"lightR3", 0);
						}
					}
				}
			}
		}
	}

	static void HoverControl(CVehicle* vehicle)
	{
		if (vehicle->m_pFlyingHandlingData == nullptr)
			return;
		float fThrust = 0.0f;
		float fPitch = 0.0f;
		float fRoll = 0.0f;
		float fYaw = 0.0f;
		float fUp = 1.0f;
		tFlyingHandlingData* flyingHandling = vehicle->m_pFlyingHandlingData;
		float rm = pow(flyingHandling->m_fMoveRes, CTimer::ms_fTimeStep);
		/*if (vehicle->m_nStatus != 0 && vehicle->m_nStatus != 10) {
			rm *= 0.97f;
		}*/
		vehicle->m_vecMoveSpeed *= rm;
		float fUpSpeed = DotProduct(vehicle->m_vecMoveSpeed, vehicle->m_matrix->at);
		float fAttitude = asin(vehicle->m_matrix->up.z);
		float fAttitudeUp = fAttitude + radians(90.0f);
		float fHeading = atan2(vehicle->m_matrix->up.y, vehicle->m_matrix->up.x);
		if (vehicle->m_nStatus == 0 || vehicle->m_nStatus == 10) {
			fThrust = (CPad::GetPad(0)->GetAccelerate() - CPad::GetPad(0)->GetBrake()) / 255.0f;
			if (abs(GetCarGunUpDown(CPad::GetPad(0))) > 1.0f)
				fThrust = GetCarGunUpDown(CPad::GetPad(0)) / 128.0f;
			fRoll = -CPad::GetPad(0)->GetSteeringLeftRight() / 128.0f;
			fPitch = CPad::GetPad(0)->GetSteeringUpDown() / 128.0f;
			fYaw = GetCarGunLeftRight(CPad::GetPad(0)) / 128.0f;
			/*if (CPad::GetPad(0)->GetCarGunFired()) {
				fYaw = 0.0f;
				fPitch = Clamp(0.5f * DotProduct(vehicle->m_vecMoveSpeed, vehicle->m_matrix->up), -0.1f, 0.1f);
				fRoll = Clamp(0.5f * DotProduct(vehicle->m_vecMoveSpeed, vehicle->m_matrix->right), -0.1f, 0.1f);
			}*/
		}
		else {
			fThrust = -0.2f;
			fYaw = 0.0f;
			fPitch = Clamp(0.5f * DotProduct(vehicle->m_vecMoveSpeed, vehicle->m_matrix->up), -0.1f, 0.1f);
			fRoll = Clamp(0.5f * -vehicle->m_matrix->right.z, -0.1f, 0.1f);
		}
		fThrust = flyingHandling->m_fThrust * fThrust;
		if (vehicle->GetPosition().z > 1000.0f)
			fThrust *= 10.0f / (vehicle->GetPosition().z - 70.0f);
		vehicle->ApplyMoveForce(GRAVITY * vehicle->m_matrix->at * fThrust * vehicle->m_fMass * CTimer::ms_fTimeStep);

		CVector upVector(cos(fAttitudeUp) * cos(fHeading), cos(fAttitudeUp) * sin(fHeading), sin(fAttitudeUp));
		upVector.Normalise();

		float fLiftSpeed = DotProduct(vehicle->m_vecMoveSpeed, upVector);
		fUp -= flyingHandling->m_fThrustFallOff * fLiftSpeed;
		fUp *= cos(fAttitude);

		vehicle->ApplyMoveForce(GRAVITY * upVector * fUp * vehicle->m_fMass * CTimer::ms_fTimeStep);

		if (vehicle->m_matrix->at.z > 0.0f) {
			float upRight = Clamp(vehicle->m_matrix->right.z, -flyingHandling->m_fFormLift, flyingHandling->m_fFormLift);
			float upImpulseRight = -upRight * flyingHandling->m_fAttackLift * vehicle->m_fTurnMass * CTimer::ms_fTimeStep;
			vehicle->ApplyTurnForce(upImpulseRight * vehicle->m_matrix->at, vehicle->m_matrix->right);

			float upFwd = Clamp(vehicle->m_matrix->up.z, -flyingHandling->m_fFormLift, flyingHandling->m_fFormLift);
			float upImpulseFwd = -upFwd * flyingHandling->m_fAttackLift * vehicle->m_fTurnMass * CTimer::ms_fTimeStep;
			vehicle->ApplyTurnForce(upImpulseFwd * vehicle->m_matrix->at, vehicle->m_matrix->up);
		}
		else {
			float upRight = vehicle->m_matrix->right.z < 0.0f ? -flyingHandling->m_fFormLift : flyingHandling->m_fFormLift;
			float upImpulseRight = -upRight * flyingHandling->m_fAttackLift * vehicle->m_fTurnMass * CTimer::ms_fTimeStep;
			vehicle->ApplyTurnForce(upImpulseRight * vehicle->m_matrix->at, vehicle->m_matrix->right);

			float upFwd = vehicle->m_matrix->up.z < 0.0f ? -flyingHandling->m_fFormLift : flyingHandling->m_fFormLift;
			float upImpulseFwd = -upFwd * flyingHandling->m_fAttackLift * vehicle->m_fTurnMass * CTimer::ms_fTimeStep;
			vehicle->ApplyTurnForce(upImpulseFwd * vehicle->m_matrix->at, vehicle->m_matrix->up);
		}

		vehicle->ApplyTurnForce(fPitch * vehicle->m_matrix->at * flyingHandling->m_fPitch * vehicle->m_fTurnMass * CTimer::ms_fTimeStep, vehicle->m_matrix->up);
		vehicle->ApplyTurnForce(fRoll * vehicle->m_matrix->at * flyingHandling->m_fRoll * vehicle->m_fTurnMass * CTimer::ms_fTimeStep, vehicle->m_matrix->right);

		float fSideSpeed = -DotProduct(vehicle->m_vecMoveSpeed, vehicle->m_matrix->right);
		float fSideSlipAccel = flyingHandling->m_fSideSlip * fSideSpeed * abs(fSideSpeed);
		vehicle->ApplyMoveForce(vehicle->m_fMass * vehicle->m_matrix->right * fSideSlipAccel * CTimer::ms_fTimeStep);

		fSideSpeed = -DotProduct(vehicle->m_vecMoveSpeed, vehicle->m_matrix->at);
		fSideSlipAccel = flyingHandling->m_fSideSlip * fSideSpeed * abs(fSideSpeed);
		vehicle->ApplyMoveForce(vehicle->m_fMass * vehicle->m_matrix->at * fSideSlipAccel * CTimer::ms_fTimeStep);

		float fYawAccel = flyingHandling->m_fYawStab * fSideSpeed * abs(fSideSpeed) + flyingHandling->m_fYaw * fYaw;
		vehicle->ApplyTurnForce(fYawAccel * vehicle->m_matrix->right * vehicle->m_fTurnMass * CTimer::ms_fTimeStep, -vehicle->m_matrix->up);
		vehicle->ApplyTurnForce(fYaw * vehicle->m_matrix->up * flyingHandling->m_fYaw * vehicle->m_fTurnMass * CTimer::ms_fTimeStep, vehicle->m_matrix->right);

		float rX = pow(flyingHandling->m_vecTurnRes.x, CTimer::ms_fTimeStep);
		float rY = pow(flyingHandling->m_vecTurnRes.y, CTimer::ms_fTimeStep);
		float rZ = pow(flyingHandling->m_vecTurnRes.z, CTimer::ms_fTimeStep);
		CVector vecTurnSpeed = CustomMultiply3x3(vehicle->m_vecTurnSpeed, vehicle->GetMatrix());
		float fResistanceMultiplier = powf(1.0f / (flyingHandling->m_vecSpeedRes.z * SQR(vecTurnSpeed.z) + 1.0f) * rZ, CTimer::ms_fTimeStep);
		float fResistance = vecTurnSpeed.z * fResistanceMultiplier - vecTurnSpeed.z;
		vecTurnSpeed.x *= rX;
		vecTurnSpeed.y *= rY;
		vecTurnSpeed.z *= fResistanceMultiplier;
		vehicle->m_vecTurnSpeed = CustomMultiply3x3(vehicle->GetMatrix(), vecTurnSpeed);
		vehicle->ApplyTurnForce(-vehicle->m_matrix->right * fResistance * vehicle->m_fTurnMass, vehicle->m_matrix->up + CustomMultiply3x3(vehicle->GetMatrix(), vehicle->m_vecCentreOfMass));
	}

    static RwObject* __cdecl SetVehicleAtomicVisibilityCB(RwObject* rwObject, void* data) {
		if (data == (void*)(0))
			rwObject->flags = 0;
		else
			rwObject->flags = 4;
		return rwObject;
	}

	static RwObject* __cdecl GetAtomicVisibilityCB(RwObject* rwObject, void* data) {
		visibility = (int)rwObject->flags;
		return rwObject;
	}

	static RwObject* __cdecl GetVehicleAtomicObjectCB(RwObject* object, void* data)
	{
		*(RpAtomic**)data = (RpAtomic*)object;
		return object;
	}

	static float radians(float degrees) {
		return (float)(degrees * M_PI / 180.0);
	}

	static float degrees(float radians) {
		return (float)(radians * 180.0 / M_PI);
	}

	static bool isPlayerInCar(CVehicle* vehicle) {
		CPlayerInfo player = CWorld::Players[CWorld::PlayerInFocus];
		return player.m_pPed->m_nPedFlags.bInVehicle && player.m_pPed->m_pVehicle == vehicle;
	}

	static bool isPlayerInModel(int model) {
		CPlayerInfo player = CWorld::Players[CWorld::PlayerInFocus];
		return player.m_pPed->m_nPedFlags.bInVehicle && player.m_pPed->m_pVehicle->m_nModelIndex == model;
	}

	static void moveComponent(CVehicle* vehicle, char* component, float z, float y, float x) {
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			CMatrix cmmatrix(&frame->modelling, false);
			cmmatrix.SetTranslateOnly(x, y, z);
			cmmatrix.UpdateRW();
		}
	}

	// Reversed from VC
	static void rotateComponent(CVehicle* vehicle, char* component, float rz, float ry, float rx) {
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			CMatrix cmatrix(&frame->modelling, false);
			CVector cpos(cmatrix.pos);
			cmatrix.SetRotate(radians(rx), radians(ry), radians(rz));
			cmatrix.pos = cpos;
			cmatrix.UpdateRW();
		}
	}

	static void setVisibility(CVehicle* vehicle, char* component, int visible) {
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			RwFrameForAllObjects(frame, GetAtomicVisibilityCB, NULL);
			if (visible != visibility) {
				RwFrameForAllObjects(frame, SetVehicleAtomicVisibilityCB, (void*)visible);
			}
		}
	}

	static void getVisibility(CEntity* model, char* component) {
		visibility = 0;
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(model->m_pRwClump, component);
		if (frame) {
			RwFrameForAllObjects(frame, GetAtomicVisibilityCB, NULL);
		}
	}

	static void setAlpha(CVehicle* vehicle, char* component, int alpha) {
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			RpAtomic* atomic;
			RwFrameForAllObjects(frame, GetVehicleAtomicObjectCB, &atomic);
			vehicle->SetComponentAtomicAlpha(atomic, alpha);
			RwFrameForAllObjects(frame, SetVehicleAtomicVisibilityCB, (void*)alpha);
		}
	}

	static RwUInt8 getAlpha(CVehicle* vehicle, char* component) {
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		RwUInt8 alpha = 0;
		if (frame) {
			RpAtomic* atomic;
			RpGeometry* geometry;
			RwFrameForAllObjects(frame, GetVehicleAtomicObjectCB, &atomic);
			geometry = atomic->geometry;
			alpha = atomic->geometry->matList.materials[0]->color.alpha;
		}
		return alpha;
	}

	static void fadeAlpha(CVehicle* vehicle, char* component, int target, int fade) {
		int alpha = getAlpha(vehicle, component);
		target = max(0, target);
		target = min(target, 255);
		if (alpha > target) {
			alpha -= fade;
			alpha = max(alpha, target);
		}
		else if (alpha < target) {
			alpha += fade;
			alpha = min(alpha, target);
		}
		setAlpha(vehicle, component, alpha);
	}

	static RpMaterial* __cdecl SetAmbientCB(RpMaterial* material, void* data)
	{
		RwSurfaceProperties* properties = (RwSurfaceProperties*)RpMaterialGetSurfaceProperties(material);
		if (data == (void*)(0))
			properties->ambient = 0.5f;
		else
			properties->ambient = 5.0;
		return material;
	}

	static void setGlow(CVehicle* vehicle, char* component, int glow) {
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			RpAtomic* atomic;
			RpGeometry* geometry;
			RwFrameForAllObjects(frame, GetVehicleAtomicObjectCB, &atomic);
			geometry = atomic->geometry;
			RpGeometryForAllMaterials(geometry, SetAmbientCB, (void*)glow);
		}
	}

	static CVector CustomMultiply3x3(CMatrixLink *m, CVector v) {
		return CVector{
			m->right.x * v.x + m->up.x * v.y + m->at.x * v.z,
			m->right.y * v.x + m->up.y * v.y + m->at.y * v.z,
			m->right.z * v.x + m->up.z * v.y + m->at.z * v.z,
		};
	}

	// vector by matrix mult, resulting in a vector where each component is the dot product of the in vector and a matrix direction
	static CVector CustomMultiply3x3(CVector v, CMatrixLink *m) {
		return CVector(DotProduct(m->right, v),
			DotProduct(m->up, v),
			DotProduct(m->at, v));
	}

	static OpcodeResult WINAPI setCarComponentVisibility(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		setVisibility(vehicle, component, CLEO_GetIntOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setCarComponentIndexVisibility(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		sprintf(component, "%s%d", component, CLEO_GetIntOpcodeParam(thread));
		setVisibility(vehicle, component, CLEO_GetIntOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI isCarComponentVisible(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		getVisibility(vehicle, component);
		CLEO_SetThreadCondResult(thread, visibility != 0);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI isCarComponentNotVisible(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		getVisibility(vehicle, component);
		CLEO_SetThreadCondResult(thread, visibility == 0);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI isCarComponentIndexVisible(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		sprintf(component, "%s%d", component, CLEO_GetIntOpcodeParam(thread));
		getVisibility(vehicle, component);
		CLEO_SetThreadCondResult(thread, visibility != 0);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI isCarComponentIndexNotVisible(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		sprintf(component, "%s%d", component, CLEO_GetIntOpcodeParam(thread));
		getVisibility(vehicle, component);
		CLEO_SetThreadCondResult(thread, visibility == 0);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setCarComponentAlpha(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		setAlpha(vehicle, component, CLEO_GetIntOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setCarComponentIndexAlpha(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		sprintf(component, "%s%d", component, CLEO_GetIntOpcodeParam(thread));
		setAlpha(vehicle, component, CLEO_GetIntOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI fadeCarComponentAlpha(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		int alpha = CLEO_GetIntOpcodeParam(thread);
		int fade = CLEO_GetIntOpcodeParam(thread);
		if (isPlayerInCar(vehicle) || !isPlayerInModel(vehicle->m_nModelIndex)) {
			fadeAlpha(vehicle, component, alpha, fade);
		}
		else {
			setVisibility(vehicle, component, alpha);
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI fadeCarComponentIndexAlpha(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		sprintf(component, "%s%d", component, CLEO_GetIntOpcodeParam(thread));
		int alpha = CLEO_GetIntOpcodeParam(thread);
		int fade = CLEO_GetIntOpcodeParam(thread);
		if (isPlayerInCar(vehicle) || !isPlayerInModel(vehicle->m_nModelIndex)) {
			fadeAlpha(vehicle, component, alpha, fade);
		}
		else {
			setVisibility(vehicle, component, alpha);
		}
		return OR_CONTINUE;
	}

	static int getCurrentDigit(CVehicle* vehicle, char* component) {
		char digitComponent[128];
		for (int digit = 0; digit < 20; digit++) {
			sprintf(digitComponent, "%s%d", component, digit);
			getVisibility(vehicle, digitComponent);
			if (visibility > 0) {
				return digit;
			}
		}
		return -1;
	}

	static void digitOff(CVehicle* vehicle, char* component) {
		char digitComponent[128];
		int digit = getCurrentDigit(vehicle, component);
		if (digit != -1) {
			sprintf(digitComponent, "%s%d", component, digit);
			setVisibility(vehicle, digitComponent, 0);
		}
	}

	static void digitOn(CVehicle* vehicle, char* component, int digit) {
		if (digit == -1) {
			digitOff(vehicle, component);
			return;
		}
		char digitComponent[128];
		sprintf(digitComponent, "%s%d", component, digit);
		getVisibility(vehicle, digitComponent);
		if (visibility == 0) {
			digitOff(vehicle, component);
			setVisibility(vehicle, digitComponent, 1);
		}
	}

	static OpcodeResult WINAPI carComponentDigitOff(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		digitOff(vehicle, component);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI carComponentDigitOn(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		int digit = CLEO_GetIntOpcodeParam(thread);
		digitOn(vehicle, component, digit);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getCarComponentAlpha(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		CLEO_SetIntOpcodeParam(thread, getAlpha(vehicle, component));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getCarComponentIndexAlpha(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		sprintf(component, "%s%d", component, CLEO_GetIntOpcodeParam(thread));
		CLEO_SetIntOpcodeParam(thread, getAlpha(vehicle, component));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI moveCarComponent(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		moveComponent(vehicle, component, CLEO_GetFloatOpcodeParam(thread), CLEO_GetFloatOpcodeParam(thread), CLEO_GetFloatOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI moveCarComponentIndex(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		sprintf(component, "%s%d", component, CLEO_GetIntOpcodeParam(thread));
		moveComponent(vehicle, component, CLEO_GetFloatOpcodeParam(thread), CLEO_GetFloatOpcodeParam(thread), CLEO_GetFloatOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getCarComponentPosition(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			CMatrix cmatrix(&frame->modelling, false);
			CLEO_SetFloatOpcodeParam(thread, cmatrix.pos.x);
			CLEO_SetFloatOpcodeParam(thread, cmatrix.pos.y);
			CLEO_SetFloatOpcodeParam(thread, cmatrix.pos.z);
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI rotateCarComponent(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		rotateComponent(vehicle, component, CLEO_GetFloatOpcodeParam(thread), CLEO_GetFloatOpcodeParam(thread), CLEO_GetFloatOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI rotateCarComponentIndex(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		sprintf(component, "%s%d", component, CLEO_GetIntOpcodeParam(thread));
		rotateComponent(vehicle, component, CLEO_GetFloatOpcodeParam(thread), CLEO_GetFloatOpcodeParam(thread), CLEO_GetFloatOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getCarComponentRotation(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			CMatrix cmatrix(&frame->modelling, false);
			float z = atan2f(cmatrix.right.y, cmatrix.up.y);
			if (z < 0.0f)
				z += (float)(2.0f * M_PI);
			float s = sinf(z);
			float c = cosf(z);
			float x = atan2f(-cmatrix.at.y, s * cmatrix.right.y + c * cmatrix.up.y);
			if (x < 0.0f)
				x += (float)(2.0f * M_PI);
			float y = atan2f(-(cmatrix.right.z * c - cmatrix.up.z * s), cmatrix.right.x * c - cmatrix.up.x * s);
			if (y < 0.0f)
				y += (float)(2.0f * M_PI);

			CLEO_SetFloatOpcodeParam(thread, degrees(x));
			CLEO_SetFloatOpcodeParam(thread, degrees(y));
			CLEO_SetFloatOpcodeParam(thread, degrees(z));
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setCarComponentGlow(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		setGlow(vehicle, component, CLEO_GetIntOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setCarComponentGlowIndex(CScriptThread* thread)
	{
		char component[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, component, sizeof(component));
		sprintf(component, "%s%d", component, CLEO_GetIntOpcodeParam(thread));
		setGlow(vehicle, component, CLEO_GetIntOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getWheelStatus(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CAutomobile* automobile;
		int status = -1;
		if (vehicle) {
			automobile = reinterpret_cast<CAutomobile*>(vehicle);
			status = automobile->m_damageManager.GetWheelStatus(0);
		}
		CLEO_SetIntOpcodeParam(thread, status);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setWheelStatus(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CAutomobile* automobile;
		if (vehicle) {
			automobile = reinterpret_cast<CAutomobile*>(vehicle);
			int status = CLEO_GetIntOpcodeParam(thread);
			automobile->m_damageManager.SetWheelStatus(0, status);
			automobile->m_damageManager.SetWheelStatus(1, status);
			automobile->m_damageManager.SetWheelStatus(2, status);
			automobile->m_damageManager.SetWheelStatus(3, status);
			if (status == 0) {
				automobile->m_damageManager.m_fWheelDamageEffect = 0.0f;
			}
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getGasPedalAudio(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			CAutomobile* automobile = reinterpret_cast<CAutomobile*>(vehicle);;
			CLEO_SetIntOpcodeParam(thread, automobile->m_aWheelState[0]);
			CLEO_SetIntOpcodeParam(thread, automobile->m_aWheelState[1]);
			CLEO_SetIntOpcodeParam(thread, automobile->m_aWheelState[2]);
			CLEO_SetIntOpcodeParam(thread, automobile->m_aWheelState[3]);
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI applyForwardForce(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));

		if (vehicle) {
			float speed = CLEO_GetFloatOpcodeParam(thread);
			CVector force = vehicle->m_matrix->up * speed;
			vehicle->ApplyMoveForce(force);
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI applyUpwardForce(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));

		if (vehicle) {
			float speed = CLEO_GetFloatOpcodeParam(thread);
			CVector force = vehicle->m_matrix->at * speed;
			vehicle->ApplyMoveForce(force);
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setHover(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			if (CLEO_GetIntOpcodeParam(thread) == 1) {
				// TODO: Load per vehicle configuration from a file.
				//vehicle->FlyingControl(6, 1.0f, 1.0f, 1.0f, 1.0f);
				//vehicle->FlyingControl(3, 0.0f, 0.0f, 0.0f, 0.0f);
				vehicle->m_pFlyingHandlingData->m_fThrust = 0.50f;
				vehicle->m_pFlyingHandlingData->m_fThrustFallOff = 5.0f;
				vehicle->m_pFlyingHandlingData->m_fYaw = -0.001f;
				vehicle->m_pFlyingHandlingData->m_fYawStab = 0.0f;
				vehicle->m_pFlyingHandlingData->m_fSideSlip = 0.1f;
				vehicle->m_pFlyingHandlingData->m_fRoll = 0.0065f;
				vehicle->m_pFlyingHandlingData->m_fRollStab = 0.0f;
				vehicle->m_pFlyingHandlingData->m_fPitch = 0.0035f;
				vehicle->m_pFlyingHandlingData->m_fPitchStab = 0.0f;
				vehicle->m_pFlyingHandlingData->m_fFormLift = 0.0f;
				vehicle->m_pFlyingHandlingData->m_fAttackLift = 0.0f;
				vehicle->m_pFlyingHandlingData->m_fGearUpR = 0.1f;
				vehicle->m_pFlyingHandlingData->m_fGearDownL = 1.0f;
				vehicle->m_pFlyingHandlingData->m_fWindMult = 0.1f;
				vehicle->m_pFlyingHandlingData->m_fMoveRes = 0.999f;
				vehicle->m_pFlyingHandlingData->m_vecTurnRes.x = 0.9f;
				vehicle->m_pFlyingHandlingData->m_vecTurnRes.y = 0.9f;
				vehicle->m_pFlyingHandlingData->m_vecTurnRes.z = 0.99f;
				vehicle->m_pFlyingHandlingData->m_vecSpeedRes.x = 0.0f;
				vehicle->m_pFlyingHandlingData->m_vecSpeedRes.y = 0.0f;
				vehicle->m_pFlyingHandlingData->m_vecSpeedRes.z = 0.0f;
				HoverControl(vehicle);
				//vehicle->m_nVehicleWeaponInUse = CAR_WEAPON_LOCK_ON_ROCKET;
				//vehicle->m_nVehicleSubClass = VEHICLE_PLANE;

				//CMatrix mat;
				//CVector pos;

				/*//automobile->DoHoverSuspensionRatios();
				mat.Attach(RwFrameGetMatrix(automobile->m_aCarNodes[CAR_WHEEL_RB]), false);
				//mat.SetTranslate(mat.pos.x, mat.pos.y, mat.pos.z);
				mat.SetRotateY((float)(-M_PI / 2.0f));
				mat.Update();

				mat.Attach(RwFrameGetMatrix(automobile->m_aCarNodes[CAR_WHEEL_LB]), false);
				//mat.SetTranslate(mat.pos.x, mat.pos.y, mat.pos.z);
				mat.SetRotateY((float)(M_PI / 2.0f));
				mat.Update();

				mat.Attach(RwFrameGetMatrix(automobile->m_aCarNodes[CAR_WHEEL_RF]), false);
				//mat.SetTranslate(mat.pos.x, mat.pos.y, mat.pos.z);
				mat.SetRotateY((float)(-M_PI / 2.0f));
				mat.Update();

				mat.Attach(RwFrameGetMatrix(automobile->m_aCarNodes[CAR_WHEEL_LF]), false);
				//mat.SetTranslate(mat.pos.x, mat.pos.y, mat.pos.z);
				mat.SetRotateY((float)(M_PI / 2.0f));
				mat.Update();*/
				//testcar->Teleport(CVector(0.0, 0.0, 0.0));
			}
			else {
				vehicle->m_nVehicleFlags.bVehicleCanBeTargettedByHS = 0;
			}
		}
		return OR_CONTINUE;
	}

	static int convertMatrixToInt(CVector vector) {
		return ((int)((vector.x + 4.0) * 100) * 1000000) + ((int)((vector.y + 4.0) * 100) * 1000) + ((int)((vector.z + 4.0) * 100));
	}

	static void convertIntToMatrix(CVector& vector, int value) {
		int x, y, z;
		x = value / 1000000;
		y = (value - (x * 1000000)) / 1000;
		z = value - (x * 1000000) - (y * 1000);
		vector.x = x / 100.f - 4.0f;
		vector.y = y / 100.f - 4.0f;
		vector.z = z / 100.f - 4.0f;
	}

	static OpcodeResult WINAPI getRotationMatrix(CScriptThread* thread)
	{
		int forward = 0;
		int right = 0;
		int up = 0;
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			forward = convertMatrixToInt(vehicle->m_matrix->up);
			right = convertMatrixToInt(vehicle->m_matrix->right);
			up = convertMatrixToInt(vehicle->m_matrix->at);
		}
		CLEO_SetIntOpcodeParam(thread, forward);
		CLEO_SetIntOpcodeParam(thread, right);
		CLEO_SetIntOpcodeParam(thread, up);
		return OR_CONTINUE;
	}
	static OpcodeResult WINAPI setRotationMatrix(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			convertIntToMatrix(vehicle->m_matrix->up, CLEO_GetIntOpcodeParam(thread));
			convertIntToMatrix(vehicle->m_matrix->right, CLEO_GetIntOpcodeParam(thread));
			convertIntToMatrix(vehicle->m_matrix->at, CLEO_GetIntOpcodeParam(thread));
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getVelocityVector(CScriptThread* thread)
	{
		int velocity = 0;
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			velocity = convertMatrixToInt(vehicle->m_vecMoveSpeed);
		}
		CLEO_SetIntOpcodeParam(thread, velocity);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setVelocityVector(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			convertIntToMatrix(vehicle->m_vecMoveSpeed, CLEO_GetIntOpcodeParam(thread));
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getVelocity(CScriptThread* thread)
	{
		int velocity = 0;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			x = vehicle->m_vecMoveSpeed.x / vehicle->m_vecMoveSpeed.Magnitude();
			y = vehicle->m_vecMoveSpeed.y / vehicle->m_vecMoveSpeed.Magnitude();
			z = vehicle->m_vecMoveSpeed.z / vehicle->m_vecMoveSpeed.Magnitude();
		}
		CLEO_SetFloatOpcodeParam(thread, x);
		CLEO_SetFloatOpcodeParam(thread, y);
		CLEO_SetFloatOpcodeParam(thread, z);
		return OR_CONTINUE;
	}

	static bool wheelsOnGround(int vehiclePointer) {
		CVehicle* vehicle = CPools::GetVehicle(vehiclePointer);
		if (vehicle) {
			CAutomobile* automobile = reinterpret_cast<CAutomobile*>(vehicle);
			return automobile->m_nWheelsOnGround > 0;
		}
		return false;
	}

	static OpcodeResult WINAPI isWheelsOnGround(CScriptThread* thread) {
		CLEO_SetThreadCondResult(thread, wheelsOnGround(CLEO_GetIntOpcodeParam(thread)));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI isWheelsNotOnGround(CScriptThread* thread) {
		CLEO_SetThreadCondResult(thread, !wheelsOnGround(CLEO_GetIntOpcodeParam(thread)));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getWheelAngle(CScriptThread* thread) {
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			CAutomobile* automobile = reinterpret_cast<CAutomobile*>(vehicle);
			for (int n = 0; n < 4; n++) {
				CLEO_SetFloatOpcodeParam(thread, degrees(automobile->m_fWheelRotation[n]));
			}
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getSteeringAngle(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CAutomobile* automobile;
		float angle = 0.0;
		if (vehicle) {
			automobile = reinterpret_cast<CAutomobile*>(vehicle);
			angle = degrees(automobile->m_fSteerAngle);
		}
		CLEO_SetFloatOpcodeParam(thread, angle);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setCarStatus(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		vehicle->m_nStatus = CLEO_GetIntOpcodeParam(thread) & 0xFF;
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getCarStatus(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_SetIntOpcodeParam(thread, vehicle->m_nStatus);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setCarLights(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		vehicle->m_nVehicleFlags.bLightsOn = CLEO_GetIntOpcodeParam(thread) != 0;
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getCarLights(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_SetIntOpcodeParam(thread, vehicle->m_nVehicleFlags.bLightsOn);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI turnOnEngine(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			vehicle->m_nStatus = 0;
			vehicle->m_nVehicleFlags.bEngineOn = 1;
		}

		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getEngineStatus(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		int status = 0;
		if (vehicle) {
			status = vehicle->m_nStatus == 0 || vehicle->m_nStatus == 10;
		}
		CLEO_SetIntOpcodeParam(thread, status);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI getCurrentGear(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			CLEO_SetIntOpcodeParam(thread, (int)vehicle->m_nCurrentGear);
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setCarEngineSound(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		if (vehicle) {
			vehicle->m_nVehicleFlags.bEngineOn = CLEO_GetIntOpcodeParam(thread);
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI inRemote(CScriptThread* thread)
	{
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CPlayerInfo player = CWorld::Players[CWorld::PlayerInFocus];


		if (vehicle) {
			CAutomobile* automobile = reinterpret_cast<CAutomobile*>(vehicle);
			//CMessages::AddMessageJumpQ(message, 150, 0);

			if (player.m_pRemoteVehicle && automobile->m_nStatus == STATUS_REMOTE_CONTROLLED) {
				CLEO_SetThreadCondResult(thread, true);
				return OR_CONTINUE;
			}
		}
		CLEO_SetThreadCondResult(thread, false);
		return OR_CONTINUE;
	}

	/*static OpcodeResult WINAPI oldReplaceTex(CScriptThread* thread)
	{
		CTxdStore::PushCurrentTxd();
		RwTexture* replacement, * original;
		CTxdStore::SetCurrentTxd(CTxdStore::FindTxdSlot(Params[1].cVar));

		char texture[256];
		sprintf(texture, "%s%d", Params[3].cVar, Params[4].nVar);
		replacement = RwTexDictionaryFindNamedTexture(texture, NULL);
		if (replacement) {
			original = RwTextureRead(Params[2].cVar, NULL);
			if (original) {
				memcpy(original, replacement, sizeof(replacement));
			}
			RwTextureDestroy(replacement);
		}
		CTxdStore::PopCurrentTxd();
		return OR_CONTINUE;
	}*/

	// Sound helper methods
	static void cleanupSound(string key) {
		if (soundMap.contains(key)) {
			soundMap[key].sound->stop();
			soundMap[key].sound->drop();
			soundMap.erase(key);
		}
	}

	static string getKeyIndex(char* name, int index) {
		string key(name);
		return key + "_" + to_string(index);
	}

	static int findEmptyIndex(char* name) {
		string key;
		int index = 0;
		do {
			key = getKeyIndex(name, ++index);
		} while (soundMap.contains(key));
		return index;
	}

	// Sound opcodes
	static OpcodeResult WINAPI stopAllSounds(CScriptThread* thread)
	{
		m_soundEngine->stopAllSounds();
		CLEO_SetThreadCondResult(thread, true);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI stopSound(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		string key(name);
		cleanupSound(key);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI stopSoundIndex(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		string key = getKeyIndex(name, CLEO_GetIntOpcodeParam(thread));
		cleanupSound(key);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI isSoundPlaying(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		string key(name);
		CLEO_SetThreadCondResult(thread, soundMap.contains(key) && !soundMap[key].sound->isFinished());
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI isSoundStopped(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		string key(name);
		CLEO_SetThreadCondResult(thread, !(soundMap.contains(key) && !soundMap[key].sound->isFinished()));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI isSoundPlayingIndex(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		string key = getKeyIndex(name, CLEO_GetIntOpcodeParam(thread));
		CLEO_SetThreadCondResult(thread, soundMap.contains(key) && !soundMap[key].sound->isFinished());
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI isSoundStoppedIndex(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		string key = getKeyIndex(name, CLEO_GetIntOpcodeParam(thread));
		CLEO_SetThreadCondResult(thread, !(soundMap.contains(key) && !soundMap[key].sound->isFinished()));
		return OR_CONTINUE;
	}

	static void __playSound(char *name, int index, int loop) {
		char fullpath[128];
		string key(name);
		if (index >= 0) {
			key = getKeyIndex(name, index);
		}
		snprintf(fullpath, 128, ".\\sound\\%s", name);
		cleanupSound(key);
		soundMap[key].sound = m_soundEngine->play2D(fullpath, loop, false, true);
		soundMap[key].spatial = false;
	}

	static OpcodeResult WINAPI playSound(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		__playSound(name, -1, CLEO_GetIntOpcodeParam(thread));
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI playSoundIndex(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		int index = findEmptyIndex(name);
		string key = getKeyIndex(name, index);
		__playSound(name, index, CLEO_GetIntOpcodeParam(thread));
		CLEO_SetIntOpcodeParam(thread, index);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI playKeypad(CScriptThread* thread)
	{
		char fullpath[128];
		snprintf(fullpath, 128, ".\\sound\\%d.wav", CLEO_GetIntOpcodeParam(thread));
		m_soundEngine->play2D(fullpath);
		return OR_CONTINUE;
	}

	static void __playSoundLocation(char *name, int index, float x, float y, float z, int loop, float minDistance) {
		char fullpath[128];
		string key(name);
		if (index >= 0) {
			key = getKeyIndex(name, index);
		}
		snprintf(fullpath, 128, ".\\sound\\%s", name);
		cleanupSound(key);
		vec3df pos;
		pos.X = x;
		pos.Y = -1.0f * y;
		pos.Z = z;
		soundMap[key].offset = CVector(pos.X, pos.Y, pos.Z);
		soundMap[key].sound = m_soundEngine->play3D(fullpath, pos, loop, false, true);
		soundMap[key].sound->setMinDistance(minDistance);
		soundMap[key].spatial = true;
	}

	static OpcodeResult WINAPI playSoundAtLocation(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		float x = CLEO_GetFloatOpcodeParam(thread);
		float y = CLEO_GetFloatOpcodeParam(thread);
		float z = CLEO_GetFloatOpcodeParam(thread);
		int loop = CLEO_GetIntOpcodeParam(thread);
		float minDistance = CLEO_GetFloatOpcodeParam(thread);
		__playSoundLocation(name, -1, x, y, z, loop, minDistance);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI playSoundAtLocationIndex(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		float x = CLEO_GetFloatOpcodeParam(thread);
		float y = CLEO_GetFloatOpcodeParam(thread);
		float z = CLEO_GetFloatOpcodeParam(thread);
		int loop = CLEO_GetIntOpcodeParam(thread);
		float minDistance = CLEO_GetFloatOpcodeParam(thread);
		int index = findEmptyIndex(name);
		__playSoundLocation(name, index, x, y, z, loop, minDistance);
		CLEO_SetIntOpcodeParam(thread, index);
		return OR_CONTINUE;
	}

	static void __attachSoundToVehicle(CVehicle* vehicle, char* name, int index, float x, float y, float z, int loop, float minDistance) {
		char fullpath[128];
		string key(name);
		if (index >= 0) {
			key = getKeyIndex(name, index);
		}
		snprintf(fullpath, 128, ".\\sound\\%s", name);
		cleanupSound(key);
		vec3df pos;
		soundMap[key].vehicle = vehicle;
		soundMap[key].offset = CVector(x, y, z);
		Command<Commands::GET_OFFSET_FROM_CAR_IN_WORLD_COORDS>(vehicle, soundMap[key].offset.x, soundMap[key].offset.y, soundMap[key].offset.z, &pos.X, &pos.Y, &pos.Z);
		pos.Y *= -1.0;
		soundMap[key].sound = m_soundEngine->play3D(fullpath, pos, loop, false, true);
		soundMap[key].sound->setMinDistance(minDistance);
		soundMap[key].spatial = true;
	}

	static OpcodeResult WINAPI attachSoundToVehicle(CScriptThread* thread)
	{
		int index = CLEO_GetIntOpcodeParam(thread);
		char name[128];
		CVehicle* vehicle = CPools::GetVehicle(index);
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		float x = CLEO_GetFloatOpcodeParam(thread);
		float y = CLEO_GetFloatOpcodeParam(thread);
		float z = CLEO_GetFloatOpcodeParam(thread);
		int loop = CLEO_GetIntOpcodeParam(thread);
		float minDistance = CLEO_GetFloatOpcodeParam(thread);

		if (vehicle) {
			__attachSoundToVehicle(vehicle, name, index, x, y, z, loop, minDistance);
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI attachSoundToVehicleIndex(CScriptThread* thread)
	{
		int index = 0;
		char name[128];
		CVehicle* vehicle = CPools::GetVehicle(CLEO_GetIntOpcodeParam(thread));
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		float x = CLEO_GetFloatOpcodeParam(thread);
		float y = CLEO_GetFloatOpcodeParam(thread);
		float z = CLEO_GetFloatOpcodeParam(thread);
		int loop = CLEO_GetIntOpcodeParam(thread);
		float minDistance = CLEO_GetFloatOpcodeParam(thread);
		if (vehicle) {
			index = findEmptyIndex(name);
			__attachSoundToVehicle(vehicle, name, index, x, y, z, loop, minDistance);
		}
		CLEO_SetIntOpcodeParam(thread, index);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setFrequency(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		string key = getKeyIndex(name, CLEO_GetIntOpcodeParam(thread));
		float frequency = CLEO_GetFloatOpcodeParam(thread);
		if (soundMap.contains(key)) {
			soundMap[key].sound->setPlaybackSpeed(frequency);
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setVolume(CScriptThread* thread)
	{
		char name[128];
		CLEO_ReadStringOpcodeParam(thread, name, sizeof(name));
		string key = getKeyIndex(name, CLEO_GetIntOpcodeParam(thread));
		float volume = CLEO_GetFloatOpcodeParam(thread);
		if (soundMap.contains(key)) {
			soundMap[key].sound->setVolume(volume);
		}
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI setDoppler(CScriptThread* thread)
	{
		m_soundEngine->setDopplerEffectParameters(CLEO_GetFloatOpcodeParam(thread), CLEO_GetFloatOpcodeParam(thread));
		return OR_CONTINUE;
	}
} _bttfsa;
