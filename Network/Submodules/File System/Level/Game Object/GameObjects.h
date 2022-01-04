#pragma once
#ifndef __GAME_OBJECTS_H__
#define __GAME_OBJECTS_H__

#if __has_include("Core/pch.h")
	#include "Core/pch.h"
#else
	#include "../../Tools.h"
#endif

class Models;
class SimpleLogic;
#if defined (DS_Engine)
class Timer;
#endif
class GameObject
{
public:
	enum TYPE { Model = 1, Sound_Obj, NONE };
	struct Object
	{
	private:
		TYPE type = (TYPE)NONE;

		// Use To Load File (e.g. file name of model)
		std::string ID_TEXT, ModelNameFile;

		DirectX::SimpleMath::Vector3 PosCoords = DirectX::SimpleMath::Vector3::Zero;

		const DirectX::SimpleMath::Vector3 ResetPos, ResetRot, ResetScl;
		
		bool HasScale = false;
		DirectX::SimpleMath::Vector3 ScaleCoords = DirectX::SimpleMath::Vector3::Zero;

		bool HasRotation = false;
		DirectX::SimpleMath::Vector3 RotationCoords = DirectX::SimpleMath::Vector3::Zero;
		
		float Test1 = 1.0f, Test2 = 1.5f;

		std::shared_ptr<Models> model;
		std::shared_ptr<SimpleLogic> Logic;
		//PxRigidDynamic *PH = nullptr;
#if defined (DS_Engine)
		std::shared_ptr<Timer> time;
#endif
	public:
		Object() {}
		Object(std::string ID_TEXT, std::string ModelNameFile, std::shared_ptr<SimpleLogic> Logic,
			TYPE type, DirectX::SimpleMath::Vector3 PosCoords = DirectX::SimpleMath::Vector3::Zero,
			DirectX::SimpleMath::Vector3 ScaleCoords = Vector3::One,
			DirectX::SimpleMath::Vector3 RotationCoords = DirectX::SimpleMath::Vector3::Zero);

		void SetID_TEXT(LPCSTR _ID_TEXT) { ID_TEXT = _ID_TEXT; }

		void SetHasScale(bool _HasScale) { HasScale = _HasScale; }
		void SetScaleCoords(DirectX::SimpleMath::Vector3 _ScaleCoords)
		{
			ScaleCoords = _ScaleCoords;
		}

		void SetHasRotation(bool _HasRotation)
		{
			HasRotation = _HasRotation;
		}
		void SetRotationCoords(DirectX::SimpleMath::Vector3 _RotationCoords)
		{
			RotationCoords = _RotationCoords;
		}

		void SetPositionCoords(Vector3 _PosCoords)
		{
			PosCoords = _PosCoords;
		}

		void SetModel(std::shared_ptr<Models> Model) { model = Model; }
		void SetType(TYPE Type) { type = Type; }
		void SetLogic(std::shared_ptr<SimpleLogic> Logic);
		void RemoveLogic();

		// WARNING!
		// It Will Release "model" Variable And Load It Again With New File (It Means That ASSIMP Requires The Same)
		void SetModelName(const std::string &Name);

		//void SetPH(PxRigidDynamic *PH) { this->PH = PH; }

		//int GetID() { return ID; }
		auto GetType() { return type; }

		bool GetScale() { return HasScale; }
		bool GetRotation() { return HasRotation; }
		std::string GetIdText() { return ID_TEXT; }
		std::string GetModelNameFile() { return ModelNameFile; }

		DirectX::SimpleMath::Vector3 GetRotCord() { return RotationCoords; }
		DirectX::SimpleMath::Vector3 GetScaleCord() { return ScaleCoords; }
		DirectX::SimpleMath::Vector3 GetPositionCord() { return PosCoords; }
		
		// R means "Reset"
		DirectX::SimpleMath::Vector3 GetRRot() { return ResetRot; }
		// R means "Reset"
		DirectX::SimpleMath::Vector3 GetRScale() { return ResetScl; }
		// R means "Reset"
		DirectX::SimpleMath::Vector3 GetRPos() { return ResetPos; }

		std::shared_ptr<Models> GetModel() { return model; }
		std::shared_ptr<SimpleLogic> GetLogic() { return Logic; }
		//PxRigidDynamic *GetPH() { return PH; }

		void UpdateLogic(float Time);

		void Destroy();

		bool RenderIt = true;
	};

	//void Update();

	auto getObjects() { return Objects; }
	void setObject(std::shared_ptr<Object> Obj) { Objects.push_back(Obj); }
protected:
	//********
	HRESULT hr;

	//********
	std::vector<std::shared_ptr<Object>> Objects;
};
#endif // !__GAME_OBJECTS_H__
