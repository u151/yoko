#pragma once
#include "Engine/GameObject.h"
#include "Engine/SphereCollider.h"

class Ground;//前方宣言

class Player :
    public GameObject
{
public:
	//コンストラクタ
	//引数：parent  親オブジェクト（SceneManager）
	Player(GameObject* parent);
	//初期化
	void Initialize() override;
	//更新
	void Update() override;
	//描画
	void Draw() override;
	//開放
	void Release() override;
	void SetGround(Ground* ground) { 
		ground_ = ground;
	}
	void OnCollision(GameObject* pTarget) override;
private:
	bool HandleInput();                                          // 入力処理、ブレーキ中ならtrue
	bool UpdateTurn();                                           // 回転処理、回転中ならtrue
	void UpdateJump();                                           // ジャンプ・重力処理
	void ResolveWallCollision(XMVECTOR& pos, const XMVECTOR& move); // 壁当たり判定
	void CheckBrickCollision();
	int hWalkModel_;
	int hIdleModel_;//待機アニメーションのモデルハンドル
	Ground* ground_;//地面オブジェクトのポインタ
};

