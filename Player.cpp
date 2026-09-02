#include "Player.h"
#include "Engine/Model.h"
#include "Engine/Debug.h"
#include "TestScene.h"
#include "Engine/Input.h"
#include "Ground.h"

namespace
{
	//定数
	const float MAX_SPEED = 0.2f;						//最大移動速度
	const float BASE_SPEED = 0.1f;						//アニメ速度1.0の基準速度
	const float ACCELERATION = 0.005f;					//加速度
	const float FRICTION = 0.008f;						//摩擦（減速度）
	const float BRAKE = 0.02f;							//逆入力ブレーキ
	const float TURN_FRAME = 10.0f;						//回転にかかるフレーム数
	const float BLOCK_SIZE = 2.0f;						//1マスのワールドサイズ
	const XMFLOAT3 START_POS = { 15.0f, 0.75f, 0.5f };	//初期位置
	const float JUMP_POWER = 0.2f;						//ジャンプ初速
	const float GRAVITY    = 0.01f;						//重力加速度
	const float AIR_CONTROL = 0.5f;						//空中での入力の重み(地上比)

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

	ResolveWallCollision(pos, move);
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
	if (isGrounded)
	{
		transform_.position_.y = START_POS.y;
		return;
	}

	transform_.position_.y += jumpVelocity;
	jumpVelocity -= GRAVITY;

	if (transform_.position_.y <= START_POS.y)
	{
		transform_.position_.y = START_POS.y;
		jumpVelocity = 0.0f;
		isGrounded   = true;
	}
}

void Player::ResolveWallCollision(XMVECTOR& pos, const XMVECTOR& move)
{
	gmap = ground_->GetMapData();
	int mapWidth  = (int)gmap[0].size();
	int mapHeight = (int)gmap.size();
	XMFLOAT3 wpos = transform_.position_;
	int mapX = (int)((wpos.x + BLOCK_SIZE / 2.0f) / BLOCK_SIZE);
	int mapZ = 1; // 外壁はすべての行に存在するため固定行で参照

	if (mapX >= 0 && mapX < mapWidth && mapZ >= 0 && mapZ < mapHeight)
	{
		if (gmap[mapZ][mapX] == 1 && (pdirection == PLAYER_LEFT || pdirection == PLAYER_RIGHT))
		{
			pos = pos - currentSpeed * move;
			XMStoreFloat3(&transform_.position_, pos);
			currentSpeed = 0.0f;
		}
	}
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
