#pragma once
#ifndef __LEVELS__H_
#define __LEVELS__H_

#if __has_include("Core/pch.h")
	#include "Core/pch.h"
#else
	#include "../Tools.h"
#endif
#include "Game Object/GameObjects.h"

///////////////////////////////////
// _XML_NODES_
// The Main ROOT
#define _SCENE_ "scene" // Can Be "root"
#define _SCENE_BEGIN "<" _SCENE_ ">"
#define _SCENE_END "</" _SCENE_ ">"

#define _MODELS_ "models"
#define _MODELS_BEGIN "<" _MODELS_ ">"
#define _MODELS_END "</" _MODELS_ ">"

#define _SOUND_OBJECTS_ "s_objs"
#define _SOUND_OBJECTS_BEGIN "<" _SOUND_OBJECTS_ ">"
#define _SOUND_OBJECTS_END "</" _SOUND_OBJECTS_ ">"

#define _LOGIC_ "logic"
#define _LOGIC_BEGIN "<" _LOGIC_ ">"
#define _LOGIC_END "</" _LOGIC_ ">"

#define _SETTINGS_ "settings"
#define _SETTINGS_BEGIN "<" _SETTINGS_ ">"
#define _SETTINGS_END "</" _SETTINGS_ ">"
////////////////////////////////////////////////////


////////////////////////////////////////////////////
// Each Node
#define _MODEL_ "model"
#define _MODEL_BEGIN "<" _MODEL_

#define _SOUND_OBJECT_ "s_obj"
#define _SOUND_OBJECT_BEGIN "<" _SOUND_OBJECT_

#define _KEY_ "key"
#define _KEY_BEGIN "<" _KEY_
////////////////////////////////////////////////////


////////////////////////////////////////////////////
// Attributes
#define _ATTR_ID_ "id"
#define _ATTR_NAME_ "name"
#define _ATTR_FILE_NAME_ "file_name"
#define _ATTR_POSITION_ "pos"
#define _ATTR_SCALE_ "scale"
#define _ATTR_ROTATE_ "rotate"
#define _ATTR_STATE_ "state"
#define _ATTR_CAM_POSITION_ "cam_pos"
#define _ATTR_CAM_LOOK_ "cam_look"

// New
#define _ATTR_MARK_ "mark"
#define _ATTR_MARK_REMOVED_ "removed"
#define _ATTR_MARK_ADDED_ "added"
////////////////////////////////////////////////////


///////////////////////////////////

class SimpleLogic;
class Level: public GameObject
{
public:
	struct Node
	{
	private:
		struct NewInfo
		{
			bool Pos = false, Scale = false, Rot = false, // If We Change These Params
				IsVisible = false, IsRemoved = false, IsAdded = false, IsAddLogic = false;
			Vector3 _Pos, _Scale, _Rot; // To Set It When Change From SDK
			TYPE T = TYPE::NONE;
		};

	public:
		Node() {}
		Node(std::shared_ptr<GameObject::Object> GObj) { GM = GObj; }

		std::shared_ptr<GameObject::Object> GM = std::make_shared<GameObject::Object>();

		bool IsItChanged = false; // Needed To Save Action
		std::string ID; // Only ID Of Node
		std::string RenderName;
		std::shared_ptr<NewInfo> SaveInfo = std::make_shared<NewInfo>();
	};
private:
	struct Child
	{
	private:
		std::vector<std::shared_ptr<Node>> Nodes;
		bool IsChangedSettings = false;

	public:
		std::shared_ptr<Node> AddNewNode(const std::shared_ptr<Node> &ND);
		bool DeleteNode(const std::string &ID);
		void Update();
		auto GetNodes() { return Nodes; }
		std::shared_ptr<Node> getNodeByID(const std::string &ID);

		void MarkChangeSettings() { IsChangedSettings = true; }
		bool GetChangedSettings() { return IsChangedSettings; }
	};
	std::shared_ptr<Child> MainChild = std::make_shared<Child>(); // It's a Main Scene
	std::string Save(const std::shared_ptr<tinyxml2::XMLDocument> &Doc, const std::shared_ptr<Node> &Node);
public:
	HRESULT Init();

	HRESULT Load(const std::string &FileBuff);
	HRESULT Load(const std::shared_ptr<tinyxml2::XMLDocument> &NewDoc);
	void Process();
	void Update();

	std::shared_ptr<Node> Add(const std::string &PathModel, const std::string &NodeName = {},
		const Vector3 &Pos = Vector3::Zero, const Vector3 &Scale = Vector3::One, const Vector3 &Rotate = Vector3::Zero);
	std::shared_ptr<Node> Add(const std::shared_ptr<GameObject::Object> &GM);
	void AddTo(const std::string &ID, const std::shared_ptr<SimpleLogic> &Logic);
	void AddTo(const std::shared_ptr<Node> &nd, const std::shared_ptr<SimpleLogic> &Logic);
	void Remove(const std::string &ID);
	void RemoveFrom(const std::shared_ptr<Node> &nd);

	auto getChild() { return MainChild; }
	void Destroy();

	std::shared_ptr<tinyxml2::XMLDocument> getDocXMLFile() { return doc; }

	bool Commit(const std::string &Author, const std::string &Description);

	void SetNotSaved(bool b) { NotSaved = b; }
	bool IsNotSaved() { return NotSaved; }

	// Check If This Level Loads Correctly And Valid!
	bool IsLoaded() { return Loaded; }

	void CheckOut(const std::vector<uint8_t> &Comm);

	void SetCB_OnAddNewNode(std::function<void(std::shared_ptr<Level::Node>)> cb) { clb_OnAddNewNode = cb; }
	void SetCB_OnDeleteNode(std::function<void(const std::string &)> cb) { clb_OnDeleteNode = cb; }
protected:
	// **********
	std::shared_ptr<tinyxml2::XMLDocument> doc = std::make_shared<tinyxml2::XMLDocument>();
	static void Spawn(/*std::vector3 pos, GameObjects::TYPE type*/);
	bool NotSaved = false, Loaded = false;

	nlohmann::json Decompress(const std::string &Buffer);
	nlohmann::json Decompress(const std::vector<uint8_t> &Buffer);

	std::function<void(std::shared_ptr<Level::Node>)> clb_OnAddNewNode;
	std::function<void(const std::string &)> clb_OnDeleteNode;
};
#endif // !__LEVELS__H_
