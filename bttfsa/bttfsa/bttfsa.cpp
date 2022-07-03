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
#include "extensions\ScriptCommands.h"

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
			CLEO_RegisterOpcode(0x3F05, setHover);
			CLEO_RegisterOpcode(0x3F10, setCarComponentVisibility);
			CLEO_RegisterOpcode(0x3F11, setCarComponentIndexVisibility);
			CLEO_RegisterOpcode(0x3F14, moveCarComponent);
			CLEO_RegisterOpcode(0x3F15, moveCarComponentIndex);
			CLEO_RegisterOpcode(0x3F16, rotateCarComponent);
			CLEO_RegisterOpcode(0x3F17, rotateCarComponentIndex);
			CLEO_RegisterOpcode(0x3F25, applyForwardForce);
			CLEO_RegisterOpcode(0x3F26, applyUpwardForce);
			CLEO_RegisterOpcode(0x3F28, setWheelStatus);
			CLEO_RegisterOpcode(0x3F80, stopAllSounds);
			CLEO_RegisterOpcode(0x3F81, stopSound);
			CLEO_RegisterOpcode(0x3F82, isSoundPlaying);
			CLEO_RegisterOpcode(0x3F83, isSoundStopped);
			CLEO_RegisterOpcode(0x3F84, playSound);
			//CLEO_RegisterOpcode(0x3F85, playSoundAtLocation);
			CLEO_RegisterOpcode(0x3F86, attachSoundToVehicle);
			CLEO_RegisterOpcode(0x3F90, playKeypad);
			CLEO_RegisterOpcode(0x3F91, stopSoundIndex);
			CLEO_RegisterOpcode(0x3F92, isSoundPlayingIndex);
			CLEO_RegisterOpcode(0x3F93, isSoundStoppedIndex);
			CLEO_RegisterOpcode(0x3F94, playSoundIndex);
			//CLEO_RegisterOpcode(0x3F95, playSoundAtLocationIndex);
			CLEO_RegisterOpcode(0x3F96, attachSoundToVehicleIndex);
		}
		else {
			MessageBox(HWND_DESKTOP, "An incorrect version of CLEO was loaded.", "bttfsa.SA.cleo", MB_ICONERROR);
		}	
		Events::initGameEvent += [&] {
			m_soundEngine = createIrrKlangDevice();
			m_soundEngine->setRolloffFactor(1.5f);
		};

		Events::processScriptsEvent += [&] {
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
						distance = (float)cameraPos.getDistanceFrom(soundPos);
						if (distance < 150.0f || !soundMap[itr->first].spatial) {
							soundMap[itr->first].sound->setVolume(1.0f);
						}
						else {
							soundMap[itr->first].sound->setVolume(0.0f);
						}
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
    }

    static RwObject* __cdecl SetVehicleAtomicVisibilityCB(RwObject* rwObject, void* data) {
		if (data == (void*)(0))
			rwObject->flags = 0;
		else
			rwObject->flags = 4;
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

	static void moveComponent(CVehicle* vehicle, char* component, float x, float y, float z) {
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			CMatrix cmmatrix(&frame->modelling, false);
			cmmatrix.SetTranslateOnly(x, y, z);
			cmmatrix.UpdateRW();
		}
	}

	static void rotateComponent(CVehicle* vehicle, char* component, float rx, float ry, float rz) {
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			CMatrix cmatrix(&frame->modelling, false);
			CVector cpos(cmatrix.pos);
			cmatrix.SetRotate(radians(rx), radians(ry), radians(rz));
			cmatrix.pos = cpos;
			cmatrix.UpdateRW();
		}
	}

	static void setVisibility(CVehicle* vehicle, char* component, int visibility) {
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			RwFrameForAllObjects(frame, SetVehicleAtomicVisibilityCB, (void*)visibility);
		}
	}

	static void setGlow(CVehicle* vehicle, char* component, int glow) {
		RwFrame* frame = CClumpModelInfo::GetFrameFromName(vehicle->m_pRwClump, component);
		if (frame) {
			RpAtomic* atomic;
			RpGeometry* geometry;
			RwFrameForAllObjects(frame, GetVehicleAtomicObjectCB, &atomic);
			geometry = atomic->geometry;
			//geometry->matList.materials[0]->texture->name
			if (glow == 0) {
				geometry->flags &= 0xF7;  // Turn off Prelit
				geometry->flags |= 0x20;  // Turn on Light
			}
			else {
				geometry->flags |= 0x8;  // Turn on Prelit
				geometry->flags &= 0xDF;  // Turn off Light
			}
		}
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
				CAutomobile* automobile = reinterpret_cast<CAutomobile*>(vehicle);
				// TODO: Load per vehicle configuration from a file.
				vehicle->FlyingControl(6, 0.0f, 0.0f, 0.0f, 0.0f);
				vehicle->m_pFlyingHandlingData->m_fThrust = 0.50f;
				vehicle->m_pFlyingHandlingData->m_fThrustFallOff = 1.0f;
				vehicle->m_pFlyingHandlingData->m_fYaw = -0.02f;
				vehicle->m_pFlyingHandlingData->m_fYawStab = 0.0002f;
				vehicle->m_pFlyingHandlingData->m_fSideSlip = 0.1f;
				vehicle->m_pFlyingHandlingData->m_fRoll = 0.02f;
				vehicle->m_pFlyingHandlingData->m_fRollStab = 0.0002f;
				vehicle->m_pFlyingHandlingData->m_fPitch = 0.02f;
				vehicle->m_pFlyingHandlingData->m_fPitchStab = 0.0002f;
				vehicle->m_pFlyingHandlingData->m_fFormLift = 0.0f;
				vehicle->m_pFlyingHandlingData->m_fAttackLift = 0.0f;
				vehicle->m_pFlyingHandlingData->m_fGearUpR = 0.1f;
				vehicle->m_pFlyingHandlingData->m_fGearDownL = 1.0f;
				vehicle->m_pFlyingHandlingData->m_fWindMult = 0.2f;
				vehicle->m_pFlyingHandlingData->m_fMoveRes = 0.997f;
				vehicle->m_pFlyingHandlingData->m_vecTurnRes.x = 0.998f;
				vehicle->m_pFlyingHandlingData->m_vecTurnRes.y = 0.998f;
				vehicle->m_pFlyingHandlingData->m_vecTurnRes.z = 0.998f;
				vehicle->m_pFlyingHandlingData->m_vecSpeedRes.x = 0.0f;
				vehicle->m_pFlyingHandlingData->m_vecSpeedRes.y = 0.0f;
				vehicle->m_pFlyingHandlingData->m_vecSpeedRes.z = 0.0f;

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
		}
		return OR_CONTINUE;
	}

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

	/*static void __playSoundLocation(char *name) {
		char fullpath[128];
		snprintf(fullpath, 128, ".\\sound\\%s", Params[0].cVar);
		cleanupSound(key);
		vec3df pos;
		pos.X = Params[1].fVar;
		pos.Y = -1.0f * Params[2].fVar;
		pos.Z = Params[3].fVar;
		soundMap[key].offset = CVector(pos.X, pos.Y, pos.Z);
		soundMap[key].sound = m_soundEngine->play3D(fullpath, pos, Params[4].nVar, false, true);
		soundMap[key].sound->setMinDistance(Params[5].fVar);
		soundMap[key].spatial = true;
	}

	static OpcodeResult WINAPI playSoundAtLocation(CScriptThread* thread)
	{
		string key(Params[0].cVar);
		__playSoundLocation(key);
		return OR_CONTINUE;
	}

	static OpcodeResult WINAPI playSoundAtLocationIndex(CScriptThread* thread)
	{
		int index = findEmptyIndex(Params[0].cVar);
		string key = getKeyIndex(Params[0].cVar, index);
		__playSoundLocation(key);
		Params[0].nVar = index;
		script->Store(1);
		return OR_CONTINUE;
	}*/

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
} _bttfsa;
