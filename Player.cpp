#include "Player.h"
#include "Engine/Model.h"
#include "Engine/Debug.h"
#include "TestScene.h"
#include "Engine/Input.h"
#include "Ground.h"
#include <cmath>

namespace
{
	//定数
	const float MAX_SPEED = 10.0f;						//最大移動速度
	const float BASE_SPEED = 10.0f;						//アニメ速度1.0の基準速度
	const float ACCELERATION = 0.005f;					//加速度
	const float FRICTION = 0.008f;						//摩擦（減速度）
	const float BRAKE = 0.02f;							//逆入力ブレーキ
	const float TURN_FRAME = 10.0f;						//回転にかかるフレーム数
	const float BLOCK_SIZE = 2.0f;						//1マスのワールドサイズ
	const XMFLOAT3 START_POS = { 15.0f, 0.75f, 0.5f };	//初期位置
	const float JUMP_POWER = 0.2f;						//ジャンプ初速
	const float GRAVITY    = 0.01f;						//重力加速度
	const float AIR_CONTROL = 0.5f;						//空中での入力の重み(地上比)
	
	const float BLOCK_INTERVAL_Y = 1.0f;

	const float PLAYER_HALF_X = 0.5f;
	const float PLAYER_HALF_Y = 0.5f;

	//enum
	enum PLAYER_STATE
	{
		PLAYER_IDLE,
		PLAYER_WALK,
		PLAYER_TURN,		//回転中
		PLAYER_STATE_MAX	//状態の数
	};

	enum PLAYER_DIRECTION
	{
		PLAYER_UP,
		PLAYER_DOWN,
		PLAYER_LEFT,
		PLAYER_RIGHT,
		PLAYER_DIRECTION_MAX //方向の数
	};

	//向きに応じたテーブル
	float P_ANGLE[4] = { 180.0f, 0.0f, 90.0f, 270.0f };	//プレイヤーの向きに応じた角度
	XMVECTOR P_MOVE[4] = { XMVectorSet(0, 0, 1, 0),
						   XMVectorSet(0, 0, -1, 0),
						   XMVectorSet(-1, 0, 0, 0),
						   XMVectorSet(1, 0, 0, 0) };		//プレイヤーの向きに応じた移動ベクトル

	//状態変数
	PLAYER_STATE pstate = PLAYER_STATE::PLAYER_IDLE;	//プレイヤーの状態
	PLAYER_DIRECTION pdirection = PLAYER_DOWN;			//プレイヤーの向き
	float turnStartAngle = 0.0f;						//回転開始時の角度
	float turnEndAngle = 0.0f;							//回転終了時の角度
	PLAYER_DIRECTION turnEndDirection = PLAYER_DOWN;	//回転終了時の向き
	float currentSpeed = 0.0f;							//現在の速度
	float turnFrame = 0.0f;								//回転中のフレーム数
	float jumpVelocity = 0.0f;							//ジャンプ中の垂直速度
	bool  isGrounded   = true;							//地面に接地しているか
	std::vector<std::vector<int>> gmap;					//マップデータ

	//関数
	float AdjustAngle(float angle) {
		if(angle >= 180)
		{
			angle -= 360.0f;
		}
		else if(angle < -180.0f)
		{
			angle += 360.0f;
		}
		return angle;
	}
}


Player::Player(GameObject* parent)
	:GameObject(parent, "Player"), hWalkModel_(-1), hIdleModel_(-1) {
}

void Player::Initialize()
{
	hWalkModel_ = Model::Load("Walking.fbx");
	Model::SetAnimFrame(hWalkModel_, 0, 59, 1.0);
	transform_.position_ = START_POS;
	hIdleModel_ = Model::Load("Idle.fbx");
	Model::SetAnimFrame(hIdleModel_, 0, 117, 1.0);
	SphereCollider* collision = new SphereCollider(XMFLOAT3(0, 0.25, 0), 0.5f);
	AddCollider(collision);

}

void Player::Update()
{
	if (pstate != PLAYER_TURN) pstate = PLAYER_IDLE;

	bool isBraking = HandleInput();

	if (UpdateTurn()) return;

	XMVECTOR pos  = XMLoadFloat3(&transform_.position_);
	XMVECTOR move = XMVectorSet(0, 0, 0, 0);

	if (pstate == PLAYER_WALK)
	{
		float accel = isGrounded ? ACCELERATION : ACCELERATION * AIR_CONTROL;
		currentSpeed += accel;
		if (currentSpeed > MAX_SPEED) currentSpeed = MAX_SPEED;
		move = P_MOVE[pdirection];
		transform_.rotate_.y = P_ANGLE[pdirection];
	}
	else // PLAYER_IDLE
	{
		if (currentSpeed > 0.0f)
		{
			float decel = isGrounded ? (isBraking ? BRAKE : FRICTION) : FRICTION * AIR_CONTROL;
			currentSpeed -= decel;
			if (currentSpeed < 0.0f) currentSpeed = 0.0f;
			move = P_MOVE[pdirection];
		}
	}

	Model::SetAnimSpeed(hWalkModel_, currentSpeed / BASE_SPEED);

	pos = pos + currentSpeed * move;
	XMStoreFloat3(&transform_.position_, pos);

	UpdateJump();

	//ResolveWallCollision(pos, move);

	CheckBrickCollision();
}

bool Player::HandleInput()
{
	bool isBraking = false;
	PLAYER_DIRECTION oldDir = pdirection;

	if (pstate != PLAYER_TURN)
	{
		if (currentSpeed == 0.0f && isGrounded)
		{
			if (Input::IsKey(DIK_LEFT))  { pdirection = PLAYER_LEFT;  pstate = PLAYER_WALK; }
			if (Input::IsKey(DIK_RIGHT)) { pdirection = PLAYER_RIGHT; pstate = PLAYER_WALK; }
		}
		else
		{
			if (Input::IsKey(DIK_LEFT))
			{
				if      (pdirection == PLAYER_LEFT)  pstate = PLAYER_WALK;
				else if (pdirection == PLAYER_RIGHT) isBraking = !isGrounded ? false : true;
			}
			if (Input::IsKey(DIK_RIGHT))
			{
				if      (pdirection == PLAYER_RIGHT) pstate = PLAYER_WALK;
				else if (pdirection == PLAYER_LEFT)  isBraking = !isGrounded ? false : true;
			}
		}
	}

	if (Input::IsKeyDown(DIK_SPACE) && isGrounded) { 
		jumpVelocity = JUMP_POWER; 
		isGrounded = false; 
	}

	if (oldDir != pdirection)
	{
		pstate = PLAYER_TURN;
		turnFrame = 0.0f;
		turnStartAngle = P_ANGLE[oldDir];
		float diff = AdjustAngle(P_ANGLE[pdirection] - P_ANGLE[oldDir]);
		turnEndDirection = pdirection;
		turnEndAngle = turnStartAngle + diff;
	}

	return isBraking;
}

bool Player::UpdateTurn()
{
	if (pstate != PLAYER_TURN) return false;

	turnFrame += 1.0f;
	float t = min(turnFrame / TURN_FRAME, 1.0f);
	transform_.rotate_.y = turnStartAngle + (turnEndAngle - turnStartAngle) * t;

	if (turnFrame >= TURN_FRAME)
	{
		pdirection = turnEndDirection;
		transform_.rotate_.y = P_ANGLE[pdirection];
		pstate = PLAYER_WALK;
	}
	return true;
}

void Player::UpdateJump()
{
	transform_.position_.y += jumpVelocity;
	jumpVelocity -= GRAVITY;

	if (transform_.position_.y <= START_POS.y)
	{
		transform_.position_.y = START_POS.y;
		jumpVelocity = 0.0f;
		isGrounded   = true;
	}
}

//void Player::ResolveWallCollision(XMVECTOR& pos, const XMVECTOR& move)
//{
//	gmap = ground_->GetMapData();
//	int mapWidth  = (int)gmap[0].size();
//	int mapHeight = (int)gmap.size();
//	XMFLOAT3 wpos = transform_.position_;
//	int mapX = (int)((wpos.x + BLOCK_SIZE / 2.0f) / BLOCK_SIZE);
//	int mapZ = 1; // 外壁はすべての行に存在するため固定行で参照
//
//	if (mapX >= 0 && mapX < mapWidth && mapZ >= 0 && mapZ < mapHeight)
//	{
//		if (gmap[mapZ][mapX] == 1 && (pdirection == PLAYER_LEFT || pdirection == PLAYER_RIGHT))
//		{
//			pos = pos - currentSpeed * move;
//			XMStoreFloat3(&transform_.position_, pos);
//			currentSpeed = 0.0f;
//		}
//	}
//}

void Player::CheckBrickCollision()
{
	if (ground_ == nullptr)
	{
		return;
	}

	std::vector<std::vector<int>> mapData = ground_->GetMapData();

	if (mapData.empty() || mapData[0].empty())
	{
		return;
	}

	int mapWidth = static_cast<int>(mapData[0].size());
	int mapHeight = static_cast<int>(mapData.size());

	XMFLOAT3 playerPos = transform_.position_;

	// ==========================================
	// Playerの当たり判定
	// ==========================================
	const float PLAYER_HALF_X = 0.5f;
	const float PLAYER_HALF_Y = 0.5f;

	float playerLeft   = playerPos.x - PLAYER_HALF_X;
	float playerRight  = playerPos.x + PLAYER_HALF_X;
	float playerBottom = playerPos.y - PLAYER_HALF_Y;
	float playerTop    = playerPos.y + PLAYER_HALF_Y;


	// ==========================================
	// Player付近のBrickGだけ調べる
	// ==========================================
	int centerX =
		static_cast<int>(
			(playerPos.x + BLOCK_SIZE / 2.0f) /
			BLOCK_SIZE);

	int centerY =
		static_cast<int>(
			(mapHeight - 1) -
			(playerPos.y / BLOCK_INTERVAL_Y));


	for (int y = centerY - 2; y <= centerY + 2; y++)
	{
		for (int x = centerX - 2; x <= centerX + 2; x++)
		{
			// マップ外
			if (x < 0 || x >= mapWidth ||
				y < 0 || y >= mapHeight)
			{
				continue;
			}

			// ==================================
			// CSVの1 = BrickG
			// ==================================
			if (mapData[y][x] != 1)
			{
				continue;
			}


			// ==================================
			// BrickGの座標
			// ==================================
			float brickX = x * BLOCK_SIZE;

			float brickY =
				(mapHeight - 1 - y) *
				BLOCK_INTERVAL_Y;


			// ==================================
			// BrickGの当たり判定
			// ==================================
			const float BRICK_HALF_X =
				BLOCK_SIZE / 2.0f;

			const float BRICK_HALF_Y =
				BLOCK_INTERVAL_Y / 2.0f;


			float brickLeft =
				brickX - BRICK_HALF_X;

			float brickRight =
				brickX + BRICK_HALF_X;

			float brickBottom =
				brickY - BRICK_HALF_Y;

			float brickTop =
				brickY + BRICK_HALF_Y;


			// ==================================
			// X方向の重なり
			// ==================================
			bool overlapX =
				playerRight > brickLeft &&
				playerLeft < brickRight;


			// ==================================
			// Y方向の重なり
			// ==================================
			bool overlapY =
				playerTop > brickBottom &&
				playerBottom < brickTop;


			// 当たっていない
			if (!overlapX || !overlapY)
			{
				continue;
			}


			// ==================================
			// 各方向のめり込み量
			// ==================================
			float pushLeft =
				playerRight - brickLeft;

			float pushRight =
				brickRight - playerLeft;

			float pushDown =
				playerTop - brickBottom;

			float pushUp =
				brickTop - playerBottom;


			// ==================================
			// 一番浅い方向を探す
			// ==================================
			float minPush = pushLeft;

			int collisionDirection = 0;
			// 0 = 左
			// 1 = 右
			// 2 = 下
			// 3 = 上


			if (pushRight < minPush)
			{
				minPush = pushRight;
				collisionDirection = 1;
			}

			if (pushDown < minPush)
			{
				minPush = pushDown;
				collisionDirection = 2;
			}

			if (pushUp < minPush)
			{
				minPush = pushUp;
				collisionDirection = 3;
			}


			// ==================================
			// 左側から衝突
			// ==================================
			if (collisionDirection == 0)
			{
				playerPos.x =
					brickLeft - PLAYER_HALF_X;

				currentSpeed = 0.0f;
			}


			// ==================================
			// 右側から衝突
			// ==================================
			else if (collisionDirection == 1)
			{
				playerPos.x =
					brickRight + PLAYER_HALF_X;

				currentSpeed = 0.0f;
			}


			// ==================================
			// 下から衝突
			// ==================================
			else if (collisionDirection == 2)
			{
				playerPos.y =
					brickBottom - PLAYER_HALF_Y;

				// 頭をぶつけた
				if (jumpVelocity > 0.0f)
				{
					jumpVelocity = 0.0f;
				}
			}


			// ==================================
			// 上から着地
			// ==================================
			else if (collisionDirection == 3)
			{
				playerPos.y =
					brickTop + PLAYER_HALF_Y;

				jumpVelocity = 0.0f;
				isGrounded = true;
			}


			// ==================================
			// 座標を反映
			// ==================================
			playerLeft   = playerPos.x - PLAYER_HALF_X;
			playerRight  = playerPos.x + PLAYER_HALF_X;
			playerBottom = playerPos.y - PLAYER_HALF_Y;
			playerTop    = playerPos.y + PLAYER_HALF_Y;
		}
	}

	transform_.position_ = playerPos;
}

void Player::Draw()
{
	if (pstate == PLAYER_STATE::PLAYER_IDLE)
	{
		Model::SetTransform(hIdleModel_, transform_);
		Model::Draw(hIdleModel_);
	}
	else if(pstate == PLAYER_STATE::PLAYER_WALK || pstate == PLAYER_STATE::PLAYER_TURN)
	{
		Model::SetTransform(hWalkModel_, transform_);
		Model::Draw(hWalkModel_);
	}

}


void Player::Release()
{
}

void Player::OnCollision(GameObject* pTarget)
{
}
