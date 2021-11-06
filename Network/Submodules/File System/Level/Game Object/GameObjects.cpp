#include "GameObjects.h"

#if !defined (DS_Engine)
	#include "File System/File_system.h"
#else
	#include "Project Manager/File System/File_system.h"
#endif
extern std::shared_ptr<File_system> FS;

#if defined (DS_Engine)
	#include "UI System/Console/Console.h"
	#include "Physics/Physics.h"
#endif
#include "../Model/Models.h"
#include "../Logic/SimpleLogic.h"

//void GameObjects::Update()
//{
//}

GameObject::Object::Object(std::string ID_TEXT, std::string ModelNameFile, std::shared_ptr<SimpleLogic> Logic,
	TYPE type, DirectX::SimpleMath::Vector3 PosCoords, DirectX::SimpleMath::Vector3 ScaleCoords,
	DirectX::SimpleMath::Vector3 RotationCoords)
{
	// Set Up The Render Model
	auto File = FS->GetFile(ModelNameFile);
	if (File)
		model = std::make_shared<Models>(File->Path.string());
	if (!File || !model
#if defined (DS_Engine)
		|| model->getMeshes().empty()
#endif
		)
	{
#if __has_include("logger.h")
		Logger_Error("Creating a New Object is failed");
#endif
		return;
	}
	
	// Set Up The IDs
	this->ID_TEXT = ID_TEXT;
	this->ModelNameFile = path(ModelNameFile).filename().string();
	// Set Up The Logic (By Default Not To Set Up It)
	//SetLogic(Logic);
	// Set Up The Physic
	//PhysX->_createTriMesh(model, false);
	//SetPH(PhysX->GetPhysDynamicObject().back());
	this->type = type;

	model->setPosition(PosCoords);
	this->PosCoords = PosCoords;

	const_cast<DirectX::SimpleMath::Vector3 &>(ResetPos) = PosCoords;
	const_cast<DirectX::SimpleMath::Vector3 &>(ResetRot) = RotationCoords;
	const_cast<DirectX::SimpleMath::Vector3 &>(ResetScl) = ScaleCoords;

	model->setRotation(RotationCoords);
	this->RotationCoords = RotationCoords;
	HasRotation = true;

	model->setScale(ScaleCoords);
	this->ScaleCoords = ScaleCoords;
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
	// Set Up The Render Model
	auto File = FS->GetFile(Name);
	if (File)
	{
		Cache = RenderIt;

		if (model)
			model->Release();

		model = std::make_shared<Models>(File->Path.string());

		// Set Up The IDs
		ModelNameFile = path(Name).filename().string();

		RenderIt = Cache;
	}
	if (!File || !model
#if defined (DS_Engine)
		|| model->getMeshes().empty()
#endif
		)
	{
#if __has_include("logger.h")
		Logger_Error("Creating a New Object is failed");
#endif
		return;
	}
}

void GameObject::Object::UpdateLogic(float Time)
{
	//if (GetAsyncKeyState(VK_NUMPAD5))
	//	Obj->GetLogic()->follow(camera->GetEyePt());

	//Vector3 newPos = ConstrainToBoundary(PosCoords,
	//	Vector3(-100.f, 0.f, -100.f), Vector3(100.f, 50.f, 100.f)),
	//	newRot = Vector3::Zero;
	if (Logic && !Logic->GetPoints().empty())
		Logic->Update(PosCoords, RotationCoords, Time);
	//Obj->GetPH()->setGlobalPose(PxTransform(ToPxVec3(newPos)));
}

void GameObject::Object::Destroy()
{
	if (model)
		model->Release();
	//if (PH)
	//	SAFE_release(PH);
}
