#include "Title.h"
#include "Engine/SceneManager.h"
#include "Engine/Input.h"
#include "Engine/Text.h"

Title::Title(GameObject* parent)
	: GameObject(parent, "Title")
{}

void Title::Initialize()
{
	pText_ = new Text;
	pText_->Initialize();
}

void Title::Update()
{
	if (Input::IsKeyDown(DIK_R))
	{
		SceneManager* sceneManager =
			dynamic_cast<SceneManager*>(GetParent());
		sceneManager->ChangeScene(SCENE_ID_TEST);
	}
}

void Title::Draw()
{
	std::string scrText;
	scrText = "Title";
	pText_->Draw(20, 20, scrText.c_str());
}

void Title::Release()
{}
