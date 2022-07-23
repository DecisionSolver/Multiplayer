#pragma once
#ifndef __GAME_OBJECTS_H__
#define __GAME_OBJECTS_H__

#include "Tools.h"

class Models;
class SimpleLogic;
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

		Vector3 PosCoords = { 0, 0, 0 };

		const Vector3 ResetPos, ResetRot, ResetScl;
		
		bool HasScale = false;
		Vector3 ScaleCoords = { 0, 0, 0 };

		bool HasRotation = false;
		Vector3 RotationCoords = { 0, 0, 0 };
		
		float Test1 = 1.0f, Test2 = 1.5f;

		std::shared_ptr<Models> model;
		std::shared_ptr<SimpleLogic> Logic;
	public:
		Object() {}
		Object(std::string ID, std::string ModelFileName, std::shared_ptr<SimpleLogic> Logic,
			TYPE type, Vector3 Position = { 0, 0, 0 }, Vector3 Scale = { 1, 1, 1 }, Vector3 Rotation = { 0, 0, 0 });

		void SetID_TEXT(const std::string &_ID_TEXT) { ID_TEXT = _ID_TEXT; }

		void SetHasScale(bool _HasScale) { HasScale = _HasScale; }
		void SetScaleCoords(Vector3 _ScaleCoords)
		{
			ScaleCoords = _ScaleCoords;
		}

		void SetHasRotation(bool _HasRotation)
		{
			HasRotation = _HasRotation;
		}
		void SetRotationCoords(Vector3 _RotationCoords)
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

		auto GetType() { return type; }

		bool GetScale() { return HasScale; }
		bool GetRotation() { return HasRotation; }
		std::string GetIdText() { return ID_TEXT; }
		std::string GetModelNameFile() { return ModelNameFile; }

		Vector3 GetRotCord() { return RotationCoords; }
		Vector3 GetScaleCord() { return ScaleCoords; }
		Vector3 GetPositionCord() { return PosCoords; }
		
		// R means "Reset"
		Vector3 GetRRot() { return ResetRot; }
		// R means "Reset"
		Vector3 GetRScale() { return ResetScl; }
		// R means "Reset"
		Vector3 GetRPos() { return ResetPos; }

		std::shared_ptr<Models> GetModel() { return model; }
		std::shared_ptr<SimpleLogic> GetLogic() { return Logic; }

		void UpdateLogic(float Time);

		void Destroy();

		bool RenderIt = true;
	};

	auto getObjects() { return Objects; }
	void setObject(std::shared_ptr<Object> Obj) { Objects.push_back(Obj); }
protected:
	//********
	std::vector<std::shared_ptr<Object>> Objects;
};
#endif // !__GAME_OBJECTS_H__
