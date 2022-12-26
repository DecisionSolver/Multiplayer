#include "GameObjects.h"

#include "File_system.h"
extern std::shared_ptr<File_system> FS;

#include "SimpleLogic.h"

GameObject::Object::Object(std::string ID, std::string ModelFileName, std::shared_ptr<SimpleLogic> Logic,
	TYPE Type, Vector3 Position, Vector3 Scale, Vector3 Rotation)
{
	// Set Up The IDs
	ID_TEXT = ID;
	ModelNameFile = std::filesystem::path(ModelFileName).filename().string();

	type = Type;

	PosCoords = Position;

	const_cast<Vector3 &>(ResetPos) = Position;
	const_cast<Vector3 &>(ResetRot) = Rotation;
	const_cast<Vector3 &>(ResetScl) = Scale;

	RotationCoords = Rotation;
	HasRotation = true;

	ScaleCoords = Scale;
	HasScale = true;
}

void GameObject::Object::SetLogic(std::shared_ptr<SimpleLogic> _Logic)
{
	if (!this->Logic)
		this->Logic = _Logic;
}

void GameObject::Object::RemoveLogic()
{
	if (Logic)
		this->Logic = nullptr;
}

bool Cache;
void GameObject::Object::SetModelName(const std::string &Name)
{
	Cache = RenderIt;

	// Set Up The IDs
	ModelNameFile = std::filesystem::path(Name).filename().string();

	RenderIt = Cache;
}

void GameObject::Object::UpdateLogic(float Time)
{
	if (Logic && !Logic->GetPoints().empty())
		Logic->Update(PosCoords, RotationCoords, Time);
}

void GameObject::Object::Destroy()
{
}
