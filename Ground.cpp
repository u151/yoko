#include "Ground.h"
#include "Engine/Model.h"
#include "Engine/CsvReader.h"


namespace
{
	using std::vector;
	const float GROUND_WIDTH = 20.0f;
	const float GROUND_Y = 10.0f;
	const float GROUND_Z = 1.0f;
	const float GROUND_ROTATE_X = -90.0f;
	const float BLOCK_INTERVAL_X = 2.0f;
	const float BLOCK_INTERVAL_Y = 1.0f;
}

Ground::Ground(GameObject* parent)
	:GameObject(parent, "Ground"), hModel_(-1), mapWidth_(-1), mapHeight_(-1)
{
	CsvReader csvData;
	csvData.Load("map.csv"); //CSVファイルを読み込む
	mapWidth_ = csvData.GetWidth(); //列数を取得
	mapHeight_ = csvData.GetHeight()/2; //行数を取得
	// mapData_を初期化 mapHeight_個のvector<int>の配列を作る
	mapData_ = vector<vector<int>>(mapHeight_, vector<int>(mapWidth_, 0));
	for (int x = 0; x < mapWidth_; x++)
	{
		for(int y= 0; y < mapHeight_; y++)
		{
			mapData_[y][x] = csvData.GetValue(x, y); //CSVの値をmapData_に格納
		}
	}
}

void Ground::Initialize()
{
	hModel_ = Model::Load("jimen3.fbx");
	hModelt_ = Model::Load("BrickG.fbx");
	//hEsaModel_ = Model::Load("esa.fbx");
	//hPEsaModel_ = Model::Load("Poweresa.fbx");
}

void Ground::Update()
{
}

void Ground::Draw()
{
	for(int i = 0;i < 3; i++) {
		transform_.position_ = { GROUND_WIDTH / 2.0f + GROUND_WIDTH * i, GROUND_Y, GROUND_Z };
		transform_.rotate_ = { GROUND_ROTATE_X, 0.0f, 0.0f };
		Model::SetTransform(hModel_, transform_);
		Model::Draw(hModel_);
	}

	for (int j = 0;j < mapHeight_;j++) {
		for (int i = 0;i < mapWidth_;i++) {
			if (mapData_[j][i] == 1) {
				Transform tr;
				tr.position_ = { i * BLOCK_INTERVAL_X, (mapHeight_ - 1 - j) * BLOCK_INTERVAL_Y, 0.0f };
				Model::SetTransform(hModelt_, tr);
				Model::Draw(hModelt_);
			}
		}
	}
}

void Ground::Release()
{
}
