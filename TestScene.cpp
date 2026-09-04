#define  _CRT_SECURE_NO_WARNINGS
#include "TestScene.h"
#include "Player.h"
#include "Ground.h"
#include "Engine/Camera.h"
#include "Engine/Text.h"

namespace {
	Ground* pGround;
	const int CAMERA_HEIGHT = 8.0f;
	XMFLOAT3 START_POS = { 15.0f, 0.75, 0.5f };
	const float END_POS_X = 1000.0f;
}

//コンストラクタ
TestScene::TestScene(GameObject * parent)
	: GameObject(parent, "TestScene"), myScore(0)
{
}

//初期化
void TestScene::Initialize()
{	
	//pWp = Instantiate<Weapon>(this);
	pPlayer_ = Instantiate <Player>(this);
	pGround = Instantiate<Ground>(this);
	pPlayer_->SetGround(pGround);

	Camera::SetPosition({ pPlayer_->GetPosition().x, pPlayer_->GetPosition().y + CAMERA_HEIGHT,-22 });
	Camera::SetTarget({ pPlayer_->GetPosition().x, pPlayer_->GetPosition().y + CAMERA_HEIGHT,0 });

	pText_ = new Text;
	pText_->Initialize();//テキストの初期化
}

//更新
void TestScene::Update()
{
	if (pPlayer_->GetPosition().x > START_POS.x && pPlayer_->GetPosition().x < END_POS_X) {
		Camera::SetPosition({ pPlayer_->GetPosition().x, START_POS.y + CAMERA_HEIGHT,-22 });
		Camera::SetTarget({ pPlayer_->GetPosition().x, START_POS.y + CAMERA_HEIGHT,0 });
	}

}

//やること！
// 餌を数えて、残り餌数を表示
// スコアを表示 
// やり方は任せる！
// sprintfでCの文字列を直で作ってもいいよ

//描画
void TestScene::Draw()
{
	std::string scrText;
	char buffer[256];
	sprintf(buffer, "%010d", myScore);
	scrText = "SCORE:" + std::string(buffer);
	pText_->Draw(500, 50, scrText.c_str());
	pText_->Draw(500, 100, scrText.c_str());
}

//開放
void TestScene::Release()
{
	pText_->Release();//テキストの開放
}
