#include "Levels.h"

#if defined (DS_Engine)
	class Engine;
	extern shared_ptr<Engine> Application;
	#include "Engine.h"
	#include "Project Manager/File System/File_system.h"
	#include "Multiplayer/Include/pch.h"
#else
	#include "File System/File_system.h"
#endif
extern std::shared_ptr<File_system> FS;

#if defined (DS_Engine)
	#include "Project Manager/Project System/Project.h"
#else
	#include "../Project System/Project.h"
#endif

extern std::unique_ptr<ProjectFile> Project;

#if defined (DS_Engine)
		#include "Entity/Camera.h"
extern shared_ptr<Camera> camera;
#endif
#include "Model/Models.h"

#include "Logic/SimpleLogic.h"

#if defined (DS_Engine)
	// Sound Objects Type
	#include "Audio System/Audio.h"
	extern shared_ptr<Audio> Sound;
#endif


nlohmann::json Level::Decompress(const std::string &Buffer)
{
	auto Decomp = nlohmann::json::from_msgpack(std::vector<uint8_t>{Buffer.begin(),
		Buffer.end()}).front();
#if defined (DEBUG) || defined (_DEBUG)
	OutputDebugStringA(("\n" + Decomp.dump() + "\n").c_str());
#endif
	return Decomp;
}
nlohmann::json Level::Decompress(const std::vector<uint8_t> &Buffer)
{
	auto Decomp = nlohmann::json::from_msgpack(Buffer).front();
#if defined (DEBUG) || defined (_DEBUG)
	OutputDebugStringA(("\n" + Decomp.dump() + "\n").c_str());
#endif
	return Decomp;
}

HRESULT Level::Load(const std::shared_ptr<tinyxml2::XMLDocument> &NewDoc)
{
	if (!doc)
		doc = make_shared<tinyxml2::XMLDocument>(&*NewDoc);
	else
		doc = NewDoc;
	Process();
	
	Loaded = true;
	return S_OK;
}

HRESULT Level::Load(const std::string &FileBuff)
{
	if (FileBuff.empty())
	{
#if __has_include("logger.h")
		Logger_Error("File Buff Was Empty Here! Abort Loading Project!");
#endif
		return E_FAIL;
	}

	doc.reset(JSONtoXML(Decompress(FileBuff)));
	Process();

	Loaded = true;
	return S_OK;
}

void Level::Spawn(/*Vector3 pos, GameObjects::TYPE type*/)
{
//	switch (type)
//	{
	//case GameObjects::OBJECTS_Dyn:
		//Obj_other.push_back(make_shared<GameObjects::Object>(ID_TEXT, i, ModelName,
	//Logic, type, Pos, Scale, Rotate));
	//	break;
//	case GameObjects::NPC:
//		break;
//	case GameObjects::ACTOR:
//		break;
//	case GameObjects::OBJECTS_Stat:
//		break;
//	case GameObjects::ETC:
//		break;
//	case GameObjects::NONE:
//		break;

	//default:
	//	break;
//	}
}

void Level::Process()
{
	bool IsModels = true, IsSobjs = true; // If do not then abort create new nodes

	XMLNode *scene = doc->FirstChildElement(_SCENE_), // We're now at <scene>
		*models = nullptr,
		*s_objs = nullptr;

	if (scene && scene->FirstChildElement(_SETTINGS_))
	{
#if defined (DS_Engine)
		auto settings = scene->FirstChildElement(_SETTINGS_);
		XMLAttribute *FirstAttr = const_cast<XMLAttribute *>(settings->FirstAttribute());
		
		Vector3 Pos = Vector3::Zero;
		Vector3 Look = Vector3::Zero;

		for (;;)
		{
			if (!FirstAttr) break;
			std::string ID_TEXT = FirstAttr->Name();
			to_lower(ID_TEXT);
			std::vector<float> Result;
			if (ID_TEXT == _ATTR_CAM_POSITION_)
			{
				Result.clear();
				getFloat3Text(FirstAttr->Value(), ",", Result);
				Pos = Vector3(Result.data());
			}
			if (ID_TEXT == _ATTR_CAM_LOOK_)
			{
				Result.clear();
				getFloat3Text(FirstAttr->Value(), ",", Result);
				Look = Vector3(Result.data());
			}
			FirstAttr = const_cast<XMLAttribute *>(FirstAttr->Next());
			if (!FirstAttr)
				break;
			else
				continue;
		}
		camera->Teleport(Pos, Look);
#endif
	}

	// Does the matter between First or Last?
	if (scene && scene->FirstChildElement(_SOUND_OBJECTS_))
		s_objs = scene->FirstChildElement(_SOUND_OBJECTS_);
	else if (scene && scene->LastChildElement(_SOUND_OBJECTS_))
		s_objs = scene->LastChildElement(_SOUND_OBJECTS_);
	else
		IsSobjs = false;

	// Does the matter between First or Last?
	if (scene && scene->FirstChildElement(_MODELS_))
		models = scene->FirstChildElement(_MODELS_);
	else if (scene && scene->LastChildElement(_MODELS_))
		models = scene->LastChildElement(_MODELS_);
	else
		IsModels = false;

	std::vector<XMLElement *> Models, S_objs;
	if (IsModels)
	{
		for (;;)
		{
			//After Load We Need To Clear Commit's Data (When We Commit Project There's Write New Nodes)
	
			if (models->NoChildren()) break; // <models/>
			if (Models.empty())
				Models.push_back(models->FirstChildElement());

			if (Models.back() && Models.back()->NextSiblingElement())
				Models.push_back(Models.back()->NextSiblingElement());
			if (!Models.back()->NextSiblingElement())
				break;
		}
	}
	if (IsSobjs)
	{
		for (;;)
		{
			if (s_objs->NoChildren()) break; // <s_objs/>
			if (S_objs.empty())
				S_objs.push_back(s_objs->FirstChildElement());

			if (S_objs.back() && S_objs.back()->NextSiblingElement())
				S_objs.push_back(S_objs.back()->NextSiblingElement());

			if (!S_objs.back()->NextSiblingElement())
				break;
		}
	}

	int I = 0;
	for (auto It: Models)
	{
		Vector3 Pos = Vector3::Zero, Scale = Vector3::One,
			Rotate = Vector3::Zero;
		std::string ID_TEXT, ModelID, ModelFileName, NameOfNode;
		bool IsRemoved = false, IsAdded = false;

		XMLAttribute *FirstAttr = const_cast<XMLAttribute *>(It->FirstAttribute());
		for (;;)
		{
			std::vector<float> Result;
			if (FirstAttr)
			{
				ID_TEXT = FirstAttr->Name();
				to_lower(ID_TEXT);

				if (ID_TEXT == _ATTR_MARK_)
				{
					if (FirstAttr->Value() == _ATTR_MARK_REMOVED_)
						IsRemoved = true;
					if (FirstAttr->Value() == _ATTR_MARK_ADDED_)
						IsAdded = true;
				}
				if (ID_TEXT == _ATTR_ID_)
					ModelID = FirstAttr->Value();
				if (ID_TEXT == _ATTR_FILE_NAME_)
					ModelFileName = FirstAttr->Value();
				if (ID_TEXT == _ATTR_NAME_)
					NameOfNode = FirstAttr->Value();
				
				// No Need To Do Something When It Will Remove
				if (!IsRemoved)
				{
					if (ID_TEXT == _ATTR_SCALE_)
					{
						getFloat3Text(FirstAttr->Value(), ",", Result);
						Scale = Vector3(Result.data());
					}
					if (ID_TEXT == _ATTR_ROTATE_)
					{
						getFloat3Text(FirstAttr->Value(), ",", Result);
						Rotate = Vector3(Result.data());
					}
					if (ID_TEXT == _ATTR_POSITION_)
					{
						getFloat3Text(FirstAttr->Value(), ",", Result);
						Pos = Vector3(Result.data());
					}
				}

				FirstAttr = const_cast<XMLAttribute *>(FirstAttr->Next());
				if (!FirstAttr)
					break;
			}
		}

		if (IsRemoved)
		{
			if (!MainChild->GetNodes().empty())
			{
				auto ThisNode = MainChild->getNodeByID(NameOfNode);
				if (ThisNode && !ThisNode->ID.empty())
					Remove(ThisNode->ID);
			}
		}
		else
		{
			std::shared_ptr<Level::Node> NewNode;
			// Check if the model has in resources of engine then add it to level
			if (exists(FS->getPathFromType(_TypeOfFile::MODELS) + ModelFileName))
			{
				NewNode = Add(FS->getPathFromType(_TypeOfFile::MODELS) + ModelFileName, NameOfNode,
					Pos, Scale, Rotate);
				if (IsAdded)
					NewNode->SaveInfo->IsAdded = IsAdded;
			}
			else
			{
#if __has_include("logger.h")
				Logger_Error_F("Model: \"%s\" not found in resources Engine and be skiped",
					ModelFileName.c_str());
#endif
			}

			if (NewNode && !It->NoChildren()) // If Have <logic>
			{
				XMLElement *_Logic = nullptr;
				std::shared_ptr<SimpleLogic> NewLogic = make_shared<SimpleLogic>();
				_Logic = It->FirstChildElement();
				if (!_Logic->FirstChildElement())
					continue;
				else
					_Logic = _Logic->FirstChildElement();
				for (;;)
				{
					if (_Logic)
					{
						XMLAttribute *FirstAttr_Logic = const_cast<XMLAttribute *>(_Logic->FirstAttribute());
						Pos = Vector3::Zero;
						Vector3 Rot = Vector3::Zero;
						int State = 0;
						for (;;)
						{
							std::vector<float> Result;
							std::string Name = FirstAttr_Logic->Name();
							to_lower(Name);
							if (Name == _ATTR_POSITION_)
							{
								getFloat3Text(FirstAttr_Logic->Value(), ",", Result);
								Pos = Vector3(Result.data());
							}
							if (Name == _ATTR_ROTATE_)
							{
								getFloat3Text(FirstAttr_Logic->Value(), ",", Result);
								Rot = Vector3(Result.data());
							}
							if (Name == _ATTR_STATE_)
								State = atoi(FirstAttr_Logic->Value());
							FirstAttr_Logic = const_cast<XMLAttribute *>(FirstAttr_Logic->Next());
							if (!FirstAttr_Logic)
								break;
							else
								continue;
						}
						NewLogic->AddNewPoint(Pos, Rot, (SimpleLogic::LogicMode)State);

						if (!_Logic->NextSiblingElement())
							break;
						else
							_Logic = _Logic->NextSiblingElement();
					}
					AddTo(NewNode, NewLogic);
					NewNode->SaveInfo->IsAddLogic = false;
				}
			}
		
			I++;
		}
	}

	if (IsModels)
	{
		for (auto It: Models)
		{
			models->DeleteChild(It);
		}
	}
	if (IsSobjs)
	{
		for (auto It: S_objs)
		{
			s_objs->DeleteChild(It);
		}
	}
}

void Level::Update()
{
	if (MainChild)
		MainChild->Update();
}

#if defined (DS_Engine)
#if !defined(without_multiplayer)
	#include "Multiplayer/Include/pch.h"
#endif
#endif

std::shared_ptr<Level::Node> Level::Add(const std::string &PathModel, const std::string &NodeName,
	const Vector3 &Pos, const Vector3 &Scale, const Vector3 &Rotate)
{
	std::shared_ptr<Node> nd = make_shared<Node>();
	if (PathModel.empty()) return nd;

#if defined (DS_Engine)
#if defined(without_multiplayer)
	nd->ID = to_std::string(MainChild->GetNodes().size());
#else
	nd->ID = md5_from_buffer(PathModel);
#endif
#else
	nd->ID = md5_from_buffer(PathModel);
#endif
	nd->RenderName = NodeName.empty() ? path(PathModel).filename().string() : NodeName;
	nd->GM = make_shared<GameObject::Object>(nd->ID, nd->RenderName,
		nullptr, Model, Pos, Scale, Rotate);
	if (!nd->GM || (nd->GM && nd->GM->GetIdText().empty()))
		return make_shared<Level::Node>();

	// Need To Save It As New Object (or mark it)
	nd->SaveInfo->T = nd->GM->GetType();
	nd->IsItChanged = true;
	nd->SaveInfo->IsVisible = nd->GM->RenderIt;
	nd->SaveInfo->Pos = nd->SaveInfo->Rot =
		nd->SaveInfo->Scale = true;
 
	auto Obj = MainChild->AddNewNode(nd);

	if (clb_OnAddNewNode)
		clb_OnAddNewNode(Obj);

	return Obj;
}

std::shared_ptr<Level::Node> Level::Add(const std::shared_ptr<GameObject::Object> &GM)
{
	std::shared_ptr<Node> nd = make_shared<Node>();

	nd->ID = GM->GetIdText();
	nd->RenderName = GM->GetModelNameFile();
	nd->GM = GM;
	if (nd->GM->GetIdText().empty())
		return std::shared_ptr<Level::Node>();
	
	auto Obj = MainChild->AddNewNode(nd);

	if (clb_OnAddNewNode)
		clb_OnAddNewNode(Obj);

	return Obj;
}

void Level::AddTo(const std::string &ID, const std::shared_ptr<SimpleLogic> &Logic)
{
	auto Obj = MainChild->getNodeByID(ID);
	if (Obj && !Obj->ID.empty())
		Obj->GM->SetLogic(Logic);
}

void Level::AddTo(const std::shared_ptr<Node> &nd, const std::shared_ptr<SimpleLogic> &Logic)
{
	if (nd && !nd->ID.empty())
	{
		nd->GM->SetLogic(Logic);
		nd->SaveInfo->IsAddLogic = true;
	}
}

void Level::RemoveFrom(const std::shared_ptr<Node> &nd) // Remove all the logics
{
	if (nd && !nd->ID.empty())
		nd->GM->RemoveLogic();
}

void Level::Remove(const std::string &ID)
{
	// If Deleted
	if (MainChild->DeleteNode(ID))
	{
		// Then Call CB
		if (clb_OnDeleteNode)
			clb_OnDeleteNode(ID);
	}
#if defined (DS_Engine)
	if (Sound)
		Sound->Remove(ID);
#endif
}

void Level::Destroy()
{
	for (const auto &It: MainChild->GetNodes())
	{
		MainChild->DeleteNode(It->ID);
#if defined (DS_Engine)
		if (Sound)
			Sound->Remove(It->ID);
#endif
	}
	if (doc)
		doc->Clear();

	for (auto &It: Objects)
	{
		It->Destroy();
	}
	Objects.clear();

	Loaded = false;
}

std::string Level::Save(const std::shared_ptr<tinyxml2::XMLDocument> &Doc, const std::shared_ptr<Node> &Node)
{
	XMLNode *scene = !Doc->FirstChildElement(_SCENE_) ?
		Doc->InsertFirstChild(Doc->NewElement(_SCENE_)) :
		Doc->FirstChildElement(_SCENE_), // We're now at <scene>
		*models = nullptr,
		*s_objs = nullptr,
		*model = nullptr,
		*s_obj = nullptr;

	if (Node->SaveInfo->T == GameObject::TYPE::NONE)
		Node->SaveInfo->T = Node->GM->GetType();

	// Save level settings like Pos Cam Or Physics properties
	if (scene && scene->FirstChildElement(_SETTINGS_))
	{
		auto settings = scene->FirstChildElement(_SETTINGS_);
		XMLAttribute *FirstAttr = const_cast<XMLAttribute *>(settings->FirstAttribute());

		if (FirstAttr)
		{
#if defined (DS_Engine)
			for (;;)
			{
				std::string ID_TEXT = FirstAttr->Name(), Data;
				to_lower(ID_TEXT);
				std::vector<float> Result;

				if (ID_TEXT == _ATTR_CAM_POSITION_)
				{
					Result.clear();
					Result.push_back(camera->GetEyePt().x);
					Result.push_back(camera->GetEyePt().y);
					Result.push_back(camera->GetEyePt().z);

					getTextFloat3(Data, ", ", Result);
					FirstAttr->SetAttribute(Data.c_str());
				}
				if (ID_TEXT == _ATTR_CAM_LOOK_)
				{
					Result.clear();
					Result.push_back(camera->GetLookAtPt().x);
					Result.push_back(camera->GetLookAtPt().y);
					Result.push_back(camera->GetLookAtPt().z);

					getTextFloat3(Data, ", ", Result);
					FirstAttr->SetAttribute(Data.c_str());
				}
				FirstAttr = const_cast<XMLAttribute *>(FirstAttr->Next());
				if (!FirstAttr)
					break;
				else
					continue;
			}
#endif
		}
	}
	else
	{
#if defined (DS_Engine)
		XMLElement *tmp = scene->InsertEndChild(Doc->NewElement(_SETTINGS_))->ToElement();
		std::vector<float> Result;
		std::string Data;
		Result.push_back(camera->GetEyePt().x);
		Result.push_back(camera->GetEyePt().y);
		Result.push_back(camera->GetEyePt().z);
		getTextFloat3(Data, ", ", Result);
		tmp->SetAttribute(_ATTR_CAM_POSITION_, Data.c_str());
		
		Data.clear();
		Result.clear();
		Result.push_back(camera->GetLookAtPt().x);
		Result.push_back(camera->GetLookAtPt().y);
		Result.push_back(camera->GetLookAtPt().z);
		getTextFloat3(Data, ", ", Result);
		tmp->SetAttribute(_ATTR_CAM_LOOK_, Data.c_str());
#endif
	}

	if (Node->SaveInfo->T == GameObject::TYPE::Sound_Obj)
	{
		if (scene && (!scene->FirstChildElement(_SOUND_OBJECTS_) ||
			!scene->LastChildElement(_SOUND_OBJECTS_)))
			s_objs = scene->InsertEndChild(Doc->NewElement(_SOUND_OBJECTS_));
		s_objs = scene->FirstChildElement(_SOUND_OBJECTS_);
	}
	if (Node->SaveInfo->T == GameObject::TYPE::Model)
	{
		if (scene && (!scene->FirstChildElement(_MODELS_) || !scene->LastChildElement(_MODELS_)))
			models = scene->InsertEndChild(Doc->NewElement(_MODELS_));
		else
			models = scene->FirstChildElement(_MODELS_);
	}
	auto Needed = (Node->SaveInfo->T == GameObject::TYPE::Sound_Obj
		? s_objs : models)->FirstChildElement();
	if (Needed)
	{
		for (;;)
		{
			std::string Name = Needed->FindAttribute(_ATTR_ID_)->Value(), id = Node->ID;
			to_lower(Name);
			to_lower(id);
			if (Name == id)
			{
				(Node->SaveInfo->T == GameObject::TYPE::Sound_Obj ? s_obj : model) = Needed;
				break;
			}

			if (!Needed->NextSibling())
				break;

			Needed = Needed->NextSiblingElement();
			if (!Needed)
				break;
		}
	}

	if (Node->SaveInfo->T == GameObject::TYPE::Sound_Obj && !s_obj)
	{
		XMLElement *tmp = s_objs->InsertEndChild(Doc->NewElement(_SOUND_OBJECT_))->ToElement();
		tmp->SetAttribute(_ATTR_ID_, Node->ID.c_str());
		tmp->SetAttribute(_ATTR_NAME_, Node->RenderName.c_str());
		tmp->SetAttribute(_ATTR_FILE_NAME_,
			path(Node->GM->GetModelNameFile()).filename().string().c_str());
		tmp->SetAttribute(_ATTR_POSITION_, "0.000, 0.000, 0.000");
		tmp->SetAttribute(_ATTR_SCALE_, "0.000, 0.000, 0.000");
		tmp->SetAttribute(_ATTR_ROTATE_, "0.000, 0.000, 0.000");
		
		Node->SaveInfo->Pos = true;
		Node->SaveInfo->Rot = true;
		Node->SaveInfo->Scale = true;

		s_obj = tmp;
	}

	if (Node->SaveInfo->T == GameObject::TYPE::Model && !model)
	{
		XMLElement *tmp = models->InsertEndChild(Doc->NewElement(_MODEL_))->ToElement();
		tmp->SetAttribute(_ATTR_ID_, Node->ID.c_str());
		tmp->SetAttribute(_ATTR_NAME_, Node->RenderName.c_str());
		tmp->SetAttribute(_ATTR_FILE_NAME_,
			path(Node->GM->GetModelNameFile()).filename().string().c_str());
		tmp->SetAttribute(_ATTR_POSITION_, "0.000, 0.000, 0.000");
		tmp->SetAttribute(_ATTR_SCALE_, "0.000, 0.000, 0.000");
		tmp->SetAttribute(_ATTR_ROTATE_, "0.000, 0.000, 0.000");

		Node->SaveInfo->Pos = true;
		Node->SaveInfo->Rot = true;
		Node->SaveInfo->Scale = true;

		model = tmp;
	}

	if (Node->SaveInfo->IsRemoved || Node->SaveInfo->IsAdded)
	{
		if (Node->SaveInfo->T == GameObject::TYPE::Model && (models && model))
		{
			if (Node->SaveInfo->IsAdded)
			{
				model->ToElement()->SetAttribute(_ATTR_MARK_, _ATTR_MARK_ADDED_);
				Node->SaveInfo->IsAdded = false;
			}
			else
			{
				model->ToElement()->SetAttribute(_ATTR_MARK_, _ATTR_MARK_REMOVED_);
				models->DeleteChild(model);
				model = nullptr;
			}
		}
		else if (Node->SaveInfo->T == GameObject::TYPE::Sound_Obj && (s_objs && s_obj))
		{
			if (Node->SaveInfo->IsAdded)
			{
				s_obj->ToElement()->SetAttribute(_ATTR_MARK_, _ATTR_MARK_ADDED_);
				Node->SaveInfo->IsAdded = false;
			}
			else
			{
				s_obj->ToElement()->SetAttribute(_ATTR_MARK_, _ATTR_MARK_REMOVED_);
				s_objs->DeleteChild(s_obj);
				s_obj = nullptr;
			}
		}
	}

	// If This Node Already Deleted Nothing To Do (Just Skip It)!
	if (!Node->SaveInfo->IsRemoved && model)
	{
		XMLAttribute *FirstAttr = const_cast<XMLAttribute *>(
			(Node->SaveInfo->T == GameObject::TYPE::Sound_Obj
				? s_obj : model)->ToElement()->FirstAttribute());
		for (;;) // Count Of Nodes
		{
			std::vector<float> Pass;
			std::string Result,
				nameNode = FirstAttr->Name();
			to_lower(nameNode);

			if (nameNode == _ATTR_ID_)
				FirstAttr->SetAttribute(Node->ID.c_str());

			if (nameNode == _ATTR_FILE_NAME_ ||
				!(Node->SaveInfo->T == GameObject::TYPE::Sound_Obj ? s_obj : model)
				->ToElement()->FindAttribute(_ATTR_FILE_NAME_))
			{
				if (!(Node->SaveInfo->T == GameObject::TYPE::Sound_Obj ? s_obj : model)
					->ToElement()->FindAttribute(_ATTR_FILE_NAME_))
					(Node->SaveInfo->T == GameObject::TYPE::Sound_Obj ? s_obj : model)
					->ToElement()->SetAttribute(_ATTR_FILE_NAME_,
						path(Node->GM->GetModelNameFile()).filename().string().c_str());
				else
					FirstAttr->SetAttribute(path(Node->GM->GetModelNameFile()).filename()
						.string().c_str());
			}
			if (nameNode == _ATTR_NAME_ || !(Node->SaveInfo->T == GameObject::TYPE::Sound_Obj
				? s_obj : model)
				->ToElement()->FindAttribute(_ATTR_NAME_))
			{
				if (!(Node->SaveInfo->T == GameObject::TYPE::Sound_Obj ? s_obj : model)
					->ToElement()->FindAttribute(_ATTR_NAME_))
					(Node->SaveInfo->T == GameObject::TYPE::Sound_Obj ? s_obj : model)
					->ToElement()->SetAttribute(_ATTR_NAME_, Node->RenderName.c_str());
				else
					FirstAttr->SetAttribute(Node->RenderName.c_str());
			}

			if (Node->SaveInfo->Scale && nameNode == _ATTR_SCALE_)
			{
				Pass.clear();
				Result.clear();
				Pass.push_back(Node->GM->GetScaleCord().x);
				Pass.push_back(Node->GM->GetScaleCord().y);
				Pass.push_back(Node->GM->GetScaleCord().z);

				getTextFloat3(Result, ", ", Pass);
				FirstAttr->SetAttribute(Result.c_str());

				Node->SaveInfo->Scale = false;
			}
			else if (Node->SaveInfo->Rot && nameNode == _ATTR_ROTATE_)
			{
				Pass.clear();
				Result.clear();
				Pass.push_back(Node->GM->GetRotCord().x);
				Pass.push_back(Node->GM->GetRotCord().y);
				Pass.push_back(Node->GM->GetRotCord().z);

				getTextFloat3(Result, ", ", Pass);
				FirstAttr->SetAttribute(Result.c_str());

				Node->SaveInfo->Rot = false;
			}
			else if (Node->SaveInfo->Pos && nameNode == _ATTR_POSITION_)
			{
				Pass.clear();
				Result.clear();
				Pass.push_back(Node->GM->GetPositionCord().x);
				Pass.push_back(Node->GM->GetPositionCord().y);
				Pass.push_back(Node->GM->GetPositionCord().z);

				getTextFloat3(Result, ", ", Pass);
				FirstAttr->SetAttribute(Result.c_str());

				Node->SaveInfo->Pos = false;
			}

			FirstAttr = const_cast<XMLAttribute *>(FirstAttr->Next());
			if (!FirstAttr)
				break;
		}

		if (Node->SaveInfo->IsAddLogic)
		{
			if (Needed)
			{
				XMLElement *_Node = nullptr;

				Needed->DeleteChildren(); // Very HardCoded Here! Be Careful
				_Node = Needed->InsertFirstChild(Doc->NewElement(_LOGIC_))->ToElement();
				for (size_t i = 0; i < Node->GM->GetLogic()->GetPoints().size(); i++)
				{
					auto Point = Node->GM->GetLogic()->GetPoints().at(i);
					XMLElement *New = nullptr;
					std::vector<float> Pass;
					std::string Result;

					New = _Node->InsertFirstChild(Doc->NewElement(_KEY_))->ToElement();
					Pass.push_back(Point->GetPos().x);
					Pass.push_back(Point->GetPos().y);
					Pass.push_back(Point->GetPos().z);
					getTextFloat3(Result, ", ", Pass);
					New->SetAttribute(_ATTR_POSITION_, Result.c_str());

					Pass.clear();
					Result.clear();
					Pass.push_back(Point->GetRotate().x);
					Pass.push_back(Point->GetRotate().y);
					Pass.push_back(Point->GetRotate().z);
					getTextFloat3(Result, ", ", Pass);
					New->SetAttribute(_ATTR_ROTATE_, Result.c_str());

					New->SetAttribute(_ATTR_STATE_, std::to_string((int)Point->GetState()).c_str());
				}
			}
			Node->SaveInfo->IsAddLogic = false;
		}
		
		Node->SaveInfo->IsRemoved = false;
	}

	XMLPrinter Prntr;
	Doc->Print(&Prntr);
	doc = Doc; // Update Our New XML Construction Of File

	return Prntr.CStr();
}

// Need To Use In Dialog For Commit
bool Level::Commit(const std::string &Author, const std::string &Description)
{
	std::string BuffCommit;
	auto Nodes = MainChild->GetNodes();

	for (const auto &It: Nodes)
	{
		if (IsNotSaved() && (MainChild->GetChangedSettings() ||
			(It->IsItChanged || It->SaveInfo->IsRemoved || It->SaveInfo->IsAddLogic)))
			BuffCommit = Save(doc, It);
		if (It->SaveInfo->IsRemoved)
			Remove(It->ID);
	}

	if (!BuffCommit.empty())
	{
		tinyxml2::XMLDocument *ConvertTo = new tinyxml2::XMLDocument();
		if (ConvertTo->Parse(BuffCommit.c_str()) == XML_SUCCESS)
		{
			auto Compressed = nlohmann::json::to_msgpack(XMLtoJSON(ConvertTo->FirstChildElement()));
			BuffCommit = std::string(Compressed.begin(), Compressed.end());
		}
	}

	// If Has Something To Save
	if (!BuffCommit.empty())
	{
		auto now = std::chrono::system_clock::now();
		std::time_t start_time = std::chrono::system_clock::to_time_t(now);
		tm *buf = new tm();
		localtime_s(buf, &start_time);
		char timedisplay[100];
		size_t len = std::strftime(timedisplay, sizeof(timedisplay), "%Y-%m-%d %H:%M:%S", buf);

		std::string Date = std::string(timedisplay, len), Hash = md5_from_buffer(Date + BuffCommit);
		auto CurrentProj = Project->GetCurrentProject();

		Project->DataBase->UpdateValues(CurrentProj,
			{ "Hash_ID"}, { "" });

		ToDo("Add Desc In Commits");
		Project->DataBase->InsertValues(CurrentProj,
			{ "Name Author", "Description", "Date Create", "Data Commit", "Hash Commit", "Hash_ID" },
			{ "root", Description, Date, BuffCommit, Hash, Hash });

		SetNotSaved(false);
		return true;
	}

	return false;
}

HRESULT Level::Init()
{
	return S_OK;
}

std::shared_ptr<Level::Node> Level::Child::AddNewNode(const std::shared_ptr<Node> &ND)
{
	Nodes.push_back(ND);
	return Nodes.back();
}

bool Level::Child::DeleteNode(const std::string &ID)
{
	for (size_t i = 0; i < Nodes.size(); i++)
	{
		if (ID == Nodes.at(i)->ID)
		{
			//Nodes.at(i)->GM->Destroy();
			Nodes.erase(Nodes.begin() + i);

			return true;
		}
	}

	return false;
}

void Level::Child::Update()
{
	for (size_t i = 0; i < Nodes.size(); i++)
	{
		auto it = Nodes.at(i)->GM;
		if (!it->RenderIt || Nodes.at(i)->SaveInfo->IsRemoved)
			continue;

		auto Model = it->GetModel();
		if (it->GetScale())
			Model->setScale(it->GetScaleCord());
		if (it->GetRotation())
			Model->setRotation(it->GetRotCord());

		it->UpdateLogic(
#if defined (DS_Engine)
			Application->getframeTime();
#else
			1
#endif
		);
		if (Model)
		{
			Model->setPosition(it->GetPositionCord());
#if defined (DS_Engine)
			Model->Render(camera->GetViewMatrix(), camera->GetProjMatrix());
#else
			Model->Update();
#endif
		}
	}
}

std::shared_ptr<Level::Node> Level::Child::getNodeByID(const std::string &ID)
{
	std::string LocalCopy = ID;
	to_lower(LocalCopy);
	for (const auto &it: Nodes)
	{
		to_lower(it->ID);
		if (LocalCopy == it->ID)
			return it;
	}

	return make_shared<Node>();
}

void Level::CheckOut(const std::vector<uint8_t> &Comm)
{
	std::shared_ptr<tinyxml2::XMLDocument> OneDoc = make_shared<tinyxml2::XMLDocument>(); 

	OneDoc.reset(JSONtoXML(Decompress(Comm)));
	if (OneDoc->Error())
	{
		Logger_Error_F("Failed With Merge Commit Data! XML Error: %s", OneDoc->ErrorStr());
		return;
	}

	// Clear All Nodes To Fill Up New Data From Commit
	Destroy();
	doc = OneDoc;

	Process();
}
