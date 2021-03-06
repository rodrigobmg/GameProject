#include "Stage/TestStageHeader.h"

#include "Widget/WidgetUtility.h"
#include "Math/Base.h"
#include "Math/Math2D.h"

namespace BoneIK
{
	using namespace Math;

	typedef Vector2 Vector2;
	struct BoneState
	{
		Vector2 pos;
		float rotation;
	};

	struct Bone
	{
		Vector2 pos;
		float angles[2];
	};
	struct BoneContraint
	{
		bool  bUsed;
		float value[2];
	};

	struct BoneLink
	{
		//int   LinkBone;
		float dist;
		BoneContraint contraint;
	};



	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			Vector2 v1(1, 0);
			Vector2 v2 = Math::GetNormal(Vector2(1,-1));

			Rotation2D rotation = Rotation2D::Make(v1, v2);


			Vector2 v3 = rotation.rotate(v1);

			initBones({ Vector2(0,0) , Vector2(0,10) , Vector2(0,30) , Vector2(0,35) , Vector2(0,50) });
			::Global::GUI().cleanupWidget();
			auto frame = WidgetUtility::CreateDevFrame();
			restart();
			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame)
		{
			Graphics2D& g = Global::GetGraphics2D();

			bool haveIKState = mBoneState.empty() == false;

			RenderUtility::SetPen(g, haveIKState ? EColor::Gray : EColor::Green);
			RenderUtility::SetBrush(g, EColor::Null);

			Vec2i const RectSize(8, 8);
			for( int i = 0; i < mBones.size(); ++i )
			{
				Vec2i rPos = ToScreenPos(mBones[i].pos);
				g.drawRect(rPos - RectSize / 2 , RectSize );
			}



			if( haveIKState )
			{
				RenderUtility::SetPen(g, EColor::Green);
				RenderUtility::SetBrush(g, EColor::Null);

				for( int i = 0; i < mLinks.size(); ++i )
				{
					Vec2i rPos1 = ToScreenPos(mBoneState[i].pos);
					Vec2i rPos2 = ToScreenPos(mBoneState[i + 1].pos);

					g.drawLine(rPos1, rPos2);
				}

				for( int i = 0; i < mBoneState.size(); ++i )
				{
					Vec2i rPos = ToScreenPos(mBoneState[i].pos);
					g.drawRect(rPos - RectSize / 2 , RectSize );
				}
			}
		}

		Vector2 ToScreenPos(Vector2 const& pos)
		{
			return 4 * pos + Vector2(400, 300);
		}
		Vector2 ToWorldPos(Vector2 const& sPos)
		{
			return (sPos - Vector2(400, 300)) / 4;
		}
		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}
		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;

			if( msg.onLeftDown() )
			{
				mLastTargetPos = ToWorldPos(msg.getPos());
				solveIK();
			}
			return true;
		}

		void solveIK()
		{

			mBoneState.resize(mBones.size());
			for( int i = 0; i < mBones.size(); ++i )
			{
				mBoneState[i].pos = mBones[i].pos;
			}

			switch( mUsageMethod )
			{
			case BoneIK::TestStage::FABRIK:
				solveFabrik(mBoneState, mLastTargetPos, 0);
				break;
			case BoneIK::TestStage::CCD:
				solveCCD(mBoneState, mLastTargetPos, 0);
				break;
			default:
				break;
			}
		}

		enum SolveMethod
		{
			FABRIK,
			CCD,
		};
		void changeMethod(SolveMethod method)
		{
			if( mUsageMethod == method )
				return;
			mUsageMethod = method;
			solveIK();
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case Keyboard::eR: restart(); break;
			case Keyboard::eNUM1:
				changeMethod( SolveMethod::FABRIK );
				break;
			case Keyboard::eNUM2:
				changeMethod( SolveMethod::CCD );
				break;

			}
			return false;
		}



		void initBones(std::vector< Vector2 > const& positions)
		{
			mBones.resize(positions.size());
			for( int i = 0; i < positions.size(); ++i )
			{
				mBones[i].pos = positions[i];
			}
			mLinks.resize(mBones.size() - 1);
			for( int i = 0; i < mLinks.size(); ++i )
			{
				auto& link = mLinks[i];
				link.dist = Distance(mBones[i + 1].pos, mBones[i].pos);
				//link.LinkBone = i + 1;
			}
		}



		SolveMethod mUsageMethod = SolveMethod::FABRIK;
		std::vector< Bone > mBones;
		std::vector< BoneLink > mLinks;
		std::vector< BoneState > mBoneState;

		Vector2 mLastTargetPos;

		void solveFabrik(std::vector< BoneState >& inoutState , Vector2 const& targetPos , float targetRotation )
		{
			assert(mBones.size() <= inoutState.size());

			int indexEffector = mBones.size() - 1;

			float maxDistance = 0;
			for( auto const& link : mLinks )
			{
				maxDistance += link.dist;
			}

			Vector2 targetOffset = targetPos - inoutState[0].pos;
			if( targetOffset.length2() > maxDistance * maxDistance )
			{
				for( int i = 0; i < indexEffector; ++i )
				{
					float dist = Distance(targetOffset, inoutState[i].pos);
					inoutState[i+1].pos = Math::LinearLerp(inoutState[i].pos, targetOffset, mLinks[i].dist / dist);
				}
			}
			else
			{
				int numIterator = 0;
				Vector2 startPos = inoutState[0].pos;
				float tol = 1e-2;
				float dist = Distance(inoutState[indexEffector].pos, targetPos);
				while( dist > tol )
				{
					inoutState[indexEffector].pos = targetPos;
					inoutState[indexEffector].rotation = targetRotation;
					for( int i = indexEffector - 1; i >= 0; --i )
					{
						Vector2 dir = Math::GetNormal(inoutState[i + 1].pos - inoutState[i].pos);
						inoutState[i].pos = inoutState[i+1].pos -  mLinks[i].dist * dir;
						float rotation = Math::ATan2(dir.y, dir.x);
					}

					inoutState[0].pos = startPos;
					for( int i = 0; i < indexEffector; ++i )
					{
						Vector2 dir = Math::GetNormal(inoutState[i + 1].pos - inoutState[i].pos);
						inoutState[i+1].pos = inoutState[i].pos + mLinks[i].dist * dir;
					}

					dist = Distance(inoutState[indexEffector].pos, targetPos);

					++numIterator;
				}

				LogMsg("FABRIK IterNum = %d", numIterator);
			} 
		}

		void solveCCD(std::vector< BoneState >& inoutState, Vector2 const& targetPos, float targetRotation)
		{

			assert(mBones.size() <= inoutState.size());

			int indexEffector = mBones.size() - 1;

			float maxDistance = 0;
			for( auto const& link : mLinks )
			{
				maxDistance += link.dist;
			}

			Vector2 targetOffset = targetPos - inoutState[0].pos;
			if( targetOffset.length2() > maxDistance * maxDistance )
			{
				for( int i = 0; i < indexEffector; ++i )
				{
					float dist = Distance(targetOffset, inoutState[i].pos);
					inoutState[i + 1].pos = Math::LinearLerp(inoutState[i].pos, targetOffset, mLinks[i].dist / dist);
				}
			}
			else
			{
				int numIterator = 0;
				Vector2 startPos = inoutState[0].pos;
				float tol = 1e-2;
				float dist = Distance(inoutState[indexEffector].pos, targetPos);
				while( dist > tol )
				{
					for( int i = indexEffector - 1; i >= 0; --i )
					{
						Vector2 dirA = Math::GetNormal(inoutState[indexEffector].pos - inoutState[i].pos);
						Vector2 dirB = Math::GetNormal(targetPos - inoutState[i].pos);
						Rotation2D rotation = Rotation2D::Make(dirA, dirB);
						for( int n = indexEffector ; n != i; --n )
						{
							inoutState[n].pos = inoutState[i].pos + rotation.rotate(inoutState[n].pos - inoutState[i].pos);
						}

					}

					dist = Distance(inoutState[indexEffector].pos, targetPos);
					++numIterator;
				}

				LogMsg("CCD IterNum = %d", numIterator);
			}

		}
	protected:
	};



	REGISTER_STAGE("Bone IK", TestStage, EStageGroup::PhyDev);

}





