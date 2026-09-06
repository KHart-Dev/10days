#pragma once
// engine
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Foundation/Math/Vector3.h>

// std
#include <cstdint>
#include <memory>
#include <vector>

class StaticModelObject;

/// <summary>
/// ステージの後ろ(Groundより下)を流れていく背景オブジェクトの管理。
/// このActorのTransformが流れる帯の中心になるので、Groundより下へ置いて使う。
/// </summary>
CALYX_OBJECT(Category = GameObject, DisplayName = "BackGroundObjects", Icon = "Textures/white1x1.png")
class BackGroundObjects : public Actor {
public:
	BackGroundObjects();
	~BackGroundObjects() override = default;

	void Initialize() override;
	void Update(float dt) override;

	void DerivativeGui() override;

private:

	static constexpr int32_t kKindCount = 3;
	static constexpr int32_t kMaxObjectCount = 64;

	// 1枚ぶんの状態
	struct BackGroundMotion {
		std::shared_ptr<StaticModelObject> object;

		int32_t kind = 0;   // 0:background01 1:background02 2:background03
		float travel = 0.0f; // 流れる向きの座標。+側へ進んで端まで行ったら-側へワープ
		float side = 0.0f;   // 帯の幅方向の座標
		float depth = 0.0f;  // 動く面と垂直な向きのずれ。XZ面ならY、XY面ならZ
		float angleRad = 0.0f; // 基準の向きから何ラジアン傾いて流れるか
		float size = 1.0f;
		float speed = 0.0f;
	};

	struct BackGroundParam : CalyxEngine::SerializableObject {
		BackGroundParam() {
			AddField("objectCount", objectCount)
				.Category("Spawn")
				.Tooltip("同時に流す枚数")
				.Range(0.0f, static_cast<float>(kMaxObjectCount));

			AddField("depthRandom", depthRandom)
				.Category("Spawn")
				.Tooltip("動く面と垂直な向きのばらつき。XZ面ならY、XY面ならZへ±で散らす");

			AddField("useXYPlane", useXYPlane)
				.Category("Flow")
				.Tooltip("ONでXY平面(Title用)、OFFでXZ平面(ステージ用)を動く");

			AddField("flowDirection", flowDirection)
				.Category("Flow")
				.Tooltip("流れる向き。動く面から外れる成分は落とされる。"
						 "XZ面は(-1,0,-1)、XY面は(-1,-1,0)で画面の右上から左下へ");

			AddField("travelHalfLength", travelHalfLength)
				.Category("Flow")
				.Tooltip("流れる向きの帯の長さの半分。ここまで進んだら反対端へワープする");

			AddField("spreadHalfWidth", spreadHalfWidth)
				.Category("Flow")
				.Tooltip("帯の幅の半分。湧く位置をこの範囲で散らす");

			AddField("flowAngleRandomDeg", flowAngleRandomDeg)
				.Category("Flow")
				.Tooltip("1枚ごとに流れる向きをこの角度の±でばらす(deg)。0で全部同じ向き")
				.Range(0.0f, 90.0f);

			AddField("speedJitter", speedJitter)
				.Category("Flow")
				.Tooltip("同じ種類でも速さをばらす割合。0.2で±20%")
				.Range(0.0f, 1.0f);

			AddField("speed01", speed01)
				.Category("Kind Speed")
				.Tooltip("background01の速さ。ゆっくりめ");

			AddField("speed02", speed02)
				.Category("Kind Speed")
				.Tooltip("background02の速さ。かなり早く");

			AddField("speed03", speed03)
				.Category("Kind Speed")
				.Tooltip("background03の速さ。02よりはゆっくり");

			AddField("sizeMin", sizeMin)
				.Category("Look")
				.Tooltip("大きさの最小値。plane.objは2x2なので見た目は倍のサイズになる");

			AddField("sizeMax", sizeMax)
				.Category("Look")
				.Tooltip("大きさの最大値");

			AddField("brightness", brightness)
				.Category("Look")
				.Tooltip("明るさ。下げるほど奥に沈む")
				.Range(0.0f, 1.0f);

			AddField("alpha", alpha)
				.Category("Look")
				.Tooltip("不透明度")
				.Range(0.0f, 1.0f);
		}

		// TitleとステージでPlaneもDirectionも別物なので、固定キーにすると
		// 片方の設定がもう片方を上書きする。インスタンスごとに分ける。
		CalyxEngine::ParamPath GetParamPath() const override {
			return {
				CalyxEngine::ParamDomain::Game,
				ownerGuid_.ToString(),
				"Actor/BackGroundObjects"
			};
		}

		Guid ownerGuid_;

		// 湧かせ方
		int32_t objectCount = 14;
		float depthRandom = 4.0f;

		// 流れ方
		bool useXYPlane = false;
		CalyxEngine::Vector3 flowDirection = { -1.0f, 0.0f, -1.0f };
		float travelHalfLength = 90.0f;
		float spreadHalfWidth = 70.0f;
		float flowAngleRandomDeg = 8.0f;
		float speedJitter = 0.2f;

		// 種類ごとの速さ
		float speed01 = 3.0f;
		float speed02 = 14.0f;
		float speed03 = 7.0f;

		// 見た目
		float sizeMin = 3.0f;
		float sizeMax = 7.0f;
		float brightness = 1.0f;
		float alpha = 1.0f;
	};

private:

	void RebuildObjects();
	void ResetMotion(BackGroundMotion& motion, bool randomizeTravel);
	void ApplyStaticParams();
	void UpdateMotions(float dt);

	// 動く面の3軸を取り出す。
	// outFlow : 流れる向き / outSide : 帯の幅方向 / outDepth : 面と垂直な向き
	void GetFlowAxis(
		CalyxEngine::Vector3& outFlow,
		CalyxEngine::Vector3& outSide,
		CalyxEngine::Vector3& outDepth
	) const;

	float SpeedOfKind(int32_t kind) const;

	void DisableGravity();

	static float PickInRange(float a, float b);

private:

	BackGroundParam param_;

	std::vector<BackGroundMotion> objects_;

	// Initialize前にRebuildが走らないようにするフラグ
	bool ready_ = false;
};
