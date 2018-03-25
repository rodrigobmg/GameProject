#include "LeelaBot.h"

#include "StringParse.h"
#include "FileSystem.h"
#include "DataStructure/CycleQueue.h"
#include "Template/StringView.h"

#include "GameGlobal.h"
#include "PropertyKey.h"

#include <mutex>
#include <condition_variable>

namespace Go
{

	template< int N >
	static int StartWith(char const* s1, char const (&s2)[N])
	{
		return ::strncmp(s1, s2, N - 1) == 0;
	}

	template< int N >
	static int StrLen(char const (&s)[N])
	{
		return N - 1;
	}

	static int const gStartSleepMillionSecond = 2000;
#define AUTO_GTP_DEBUG 1

	class CGameOutputThread : public IGameOutputThread
	{
	public:
		int32 mNumNewRead = 0;
		int   mNumUsed = 0;
		char  mBuffer[1024];

		bool  bNeedRead = true;

		Mutex mMutexBuffer;
		ConditionVariable mBufferProcessedCV;

		
#if AUTO_GTP_DEBUG
		std::vector< char > debugString;
#endif


		unsigned run()
		{
			SystemPlatform::Sleep(gStartSleepMillionSecond);
			for( ;;)
			{

				Mutex::Locker locker(mMutexBuffer);
				mBufferProcessedCV.wait(locker, [this] { return bNeedRead; });
				mNumNewRead = 0;

				int numRead;
				char* pData = mBuffer + mNumUsed;

				bool bSuccess = process->readOutputStream(pData, ARRAY_SIZE(mBuffer) - 1 - mNumUsed, numRead);
				if( !bSuccess || ( numRead == 0 && mNumUsed != (ARRAY_SIZE(mBuffer) - 1) ) )
				{
					LogMsg("OutputThread can't read");
					break;
				}

#if AUTO_GTP_DEBUG
				debugString.insert(debugString.end(), pData, pData + numRead);
#endif

				mNumNewRead = numRead;
				mNumUsed += numRead;
				mBuffer[mNumUsed] = 0;

				bNeedRead = false;
			}

			return 0;
		}

		void update()
		{
			if( mMutexBuffer.tryLock() )
			{
				processBuffer();
				mNumNewRead = 0;
				bNeedRead = true;

				mBufferProcessedCV.notifyOne();
				mMutexBuffer.unlock();
			}
		}

		virtual void processBuffer(){}


		static int ParseResult(char const* str, float& resultNum)
		{
			int color;
			if( str[0] == 'B' || str[0] == 'b' )
				color = StoneColor::eBlack;
			else if( str[0] == 'W' || str[0] == 'w' )
				color = StoneColor::eWhite;
			else
				return StoneColor::eEmpty;

			assert(str[1] == '+');

			str = FStringParse::SkipSpace(str + 2);
			if( StartWith(str, "Resign") )
			{
				resultNum = 0;
			}
			else
			{
				resultNum = atof(str);
			}
			return color;
		}

		int parsePlayResult(char const* str, int color)
		{
#define STR_RESIGN "resign"
#define STR_PASS "pass"
			int numRead;
			GameCommand com;
			if( StartWith(str, STR_RESIGN) )
			{
				com.id = GameCommand::eResign;
				addOutputCommand(com);
				return StrLen(STR_RESIGN);
			}
			else if( StartWith(str, STR_PASS) )
			{
				com.id = GameCommand::ePass;
				addOutputCommand(com);
				return StrLen(STR_PASS);
			}
			else if( numRead = Go::ReadCoord(str, com.pos) )
			{
				com.id = GameCommand::ePlay;
				com.playColor = color;
				addOutputCommand(com);
				return numRead;
			}

			LogWarningF(0, "ParsePlayError : %s", str);
			return 0;
		}
	};

	class AutoGTPOutputThread : public CGameOutputThread
	{
	public:
		bool bEngineStart = false;
		int  curStep = 0;


		std::string lastNetworkName;
		std::string blackNetworkName;
		std::string whiteNetworkName;

		void processBuffer()
		{
			if( mNumUsed )
			{
				int numSaved = parseOutput(mBuffer, mNumUsed);
				if( numSaved && numSaved != mNumUsed )
				{
					::memmove(mBuffer, mBuffer + mNumUsed - numSaved, numSaved);
				}
				mNumUsed = numSaved;
			}
		}


		int parseOutput(char* buffer, int bufferSize)
		{

#define STR_SCORE "Score:"
#define STR_GOT_NEW_JOB "Got new job:"
#define STR_NET_FILENAME "Net fileName:"
#define STR_NET          "net:"
#define STR_1ST_NETWORK  "first network:"
#define STR_2ND_NETWORK  "second network"

			char const* cur = buffer;
			while( *cur != 0 )
			{
				int step;
				char coord[32];
				int numRead;
				char color[32];
				cur = FStringParse::SkipSpace(cur);

				if( bEngineStart )
				{
					GameCommand com;

					if( sscanf(cur, "%d%s%s%n", &step, color, coord, &numRead) == 3 && color[0] == '(' )
					{
						if( step == 1 )
						{
							com.id = GameCommand::eStart;
							addOutputCommand(com);
							curStep = 1;
						}

						if( curStep != step )
						{
							LogMsgF("Warning:Error Step");
						}
						if( StartWith(coord, "pass") )
						{
							com.id = GameCommand::ePass;
							addOutputCommand(com);
						}
						else if( StartWith(coord, "resign") )
						{
							com.id = GameCommand::eResign;
							addOutputCommand(com);
						}
						else if ( Go::ReadCoord(coord, com.pos) )
						{
							com.id = GameCommand::ePlay;
							if( color[1] == 'B' || color[1] == 'b' )
							{
								com.playColor = StoneColor::eBlack;
							}
							else if( color[1] == 'W' || color[1] == 'w' )
							{
								com.playColor = StoneColor::eWhite;
							}
							else
							{
								LogWarningF(0 , "Unknown color : %s" , color);
								com.playColor = StoneColor::eEmpty;
							}
							addOutputCommand(com);
						}
						else
						{
							LogMsgF("Unknown Com = %s", coord);
						}
						++curStep;
						cur += numRead;
					}
					else if( sscanf(cur, "%d(", &step, &numRead) == 1 )
					{
						//LogMsgF("==comand not complete : %s ==", cur);
						return bufferSize - (cur - buffer);
					}
					else
					{
						char const* next = FStringParse::SkipToNextLine(cur);
						int num = next - cur;
						if( num )
						{
							FixString< 512 > str{ cur , num };
							LogMsgF("%s", str.c_str());
							if( StartWith(str , STR_SCORE) )
							{
								char const* strResult = FStringParse::SkipSpace(str.c_str() + StrLen(STR_SCORE) );
								GameCommand com;
								com.id = GameCommand::eEnd;
								com.winner = ParseResult(strResult, com.winNum);

								addOutputCommand(com);
#if AUTO_GTP_DEBUG
								debugString.clear();
#endif
								bEngineStart = false;
							}

							cur = next;
						}
					}
				}
				else
				{
					char const* next = FStringParse::SkipToNextLine(cur);


					int num = next - cur;
					if( num )
					{
						while( num )
						{
							char c = cur[num-1];
							if ( c != ' ' && c != '\r' && c != '\n' )
								break;
							--num;
						}
						FixString< 512 > str{ cur , num };
						LogMsgF(str.c_str());

						if( StartWith(str, STR_NET) )
						{
							lastNetworkName = FStringParse::SkipSpace(str.c_str() + StrLen(STR_NET));
							if ( lastNetworkName.back() == '.' )
								lastNetworkName.pop_back();
							GameCommand com;
							com.setParam(LeelaGameParam::eLastNetWeight, lastNetworkName.c_str());
							addOutputCommand(com);
						}
						else if( StartWith(str, STR_GOT_NEW_JOB) )
						{
							bool bMatchJob = false;

							char const* jobName = FStringParse::SkipSpace(str.c_str() + StrLen(STR_GOT_NEW_JOB));
							if( StartWith(jobName, "match") )
							{
								bMatchJob = true;
							}
							GameCommand com;
							com.setParam(LeelaGameParam::eJobMode, bMatchJob);
							addOutputCommand(com);
						}
						else if( StartWith(str, STR_1ST_NETWORK) )
						{
							blackNetworkName = FStringParse::SkipSpace(str.c_str() + StrLen(STR_1ST_NETWORK));
							if( blackNetworkName.back() == '.' )
								blackNetworkName.pop_back();

							if( blackNetworkName == lastNetworkName )
							{
								GameCommand com;
								com.setParam(LeelaGameParam::eMatchChallengerColor, StoneColor::eWhite);
								addOutputCommand(com);
							}

						}
						else if( StartWith(str, STR_2ND_NETWORK) )
						{
							whiteNetworkName = FStringParse::SkipSpace(str.c_str() + StrLen(STR_2ND_NETWORK));
							if( whiteNetworkName.back() == '.' )
								whiteNetworkName.pop_back();

							if( whiteNetworkName == lastNetworkName )
							{
								GameCommand com;
								com.setParam(LeelaGameParam::eMatchChallengerColor, StoneColor::eBlack);
								addOutputCommand(com);
							}
						}
						else if( StartWith(str, "Engine has started.") )
						{
							bEngineStart = true;
						}
						cur = next;
					}
				}
			}

			return 0;
		}

	};


	class GTPOutputThread : public CGameOutputThread
	{
		typedef CGameOutputThread BaseClass;
	public:
		int  mColor;
		bool bThinking;
		bool bShowDiagnosticOutput = true;
		//TCycleQueue<GTPCommand> mProcQueue;
		std::vector< GTPCommand > mProcQueue;

		virtual void restart()
		{
			BaseClass::restart();
			bThinking = false;
			mProcQueue.clear();
		}

		virtual void dumpCommandMsgBegin(GTPCommand com){}
		virtual void procDumpCommandMsg(GTPCommand com , char* buffer, int num ){}
		virtual void dumpCommandMsgEnd(GTPCommand com){}

		void processBuffer()
		{
			assert(mNumUsed >= mNumNewRead);

			char* pData = mBuffer;

			char* pDataEnd = mBuffer + mNumUsed;
			char* pLineEnd = pDataEnd - mNumNewRead;

			for( ;; )
			{
				for( ; pLineEnd != pDataEnd; ++pLineEnd )
				{
					if( *pLineEnd == '\r' || *pLineEnd == '\n' || *pLineEnd == '\0' )
						break;
				}
				if( pLineEnd == pDataEnd )
					break;

				*pLineEnd = 0;
				if( pData != pLineEnd )
				{
					if( bShowDiagnosticOutput )
					{
						LogMsgF("GTP: %s ", pData);
					}
					parseLine(pData, pLineEnd - pData);
				}

				++pLineEnd;
				pData = pLineEnd;
			}

			if( pData != pDataEnd )
			{
				mNumUsed = pDataEnd - pData;
				if( pData != mBuffer )
				{
					::memmove(mBuffer, pData, mNumUsed);
				}
				mBuffer[mNumUsed] = 0;
			}
			else
			{
				mNumUsed = 0;
			}
		}

		GTPCommand getHeadRequest()
		{
			if( mProcQueue.empty() )
				return{ GTPCommand::eNone , 0 };
			return mProcQueue.front();
		}

		bool bDumping = false;
		bool parseLine(char* buffer, int num)
		{
			//LogMsgF("%s", buffer);
			char const* cur = buffer;
			if( *cur == '=' )
			{
				cur = FStringParse::SkipSpace(cur + 1);

				GTPCommand com = getHeadRequest();
				switch( com.id )
				{
				case GTPCommand::eGenmove:
					if( parsePlayResult(cur , com.meta) == 0 )
						return false;
					break;
				case GTPCommand::ePlay:
					break;
				case GTPCommand::eHandicap:
					for( ;;)
					{
						if ( *cur == 0 )
							break;

						int numRead = parsePlayResult(cur, StoneColor::eBlack);
						if( numRead == 0 )
							return false;
						cur = FStringParse::SkipSpace(cur + numRead);
					}
					break;
				case GTPCommand::eFinalScore:
					{
						GameCommand gameCom;
						gameCom.id = GameCommand::eEnd;
						gameCom.winner = ParseResult(cur, gameCom.winNum);
						addOutputCommand(gameCom);
					}
					break;
				default:
					break;
				}

				if( com.id != GTPCommand::eNone )
				{
					//mProcQueue.pop_front();
					mProcQueue.erase(mProcQueue.begin());
					bDumping = false;
					dumpCommandMsgEnd(com);
				}
			}
			else if( *cur == '?' )
			{
				GTPCommand com = getHeadRequest();
				cur = FStringParse::SkipSpace(cur + 1);
				//error operator handled

				if( com.id != GTPCommand::eNone )
				{
					//mProcQueue.pop_front(); 
					mProcQueue.erase(mProcQueue.begin());

					bDumping = false;
					dumpCommandMsgEnd(com);
				}
			}
			else
			{
				GTPCommand com = getHeadRequest();
				if( bDumping == false )
				{
					bDumping = true;
					dumpCommandMsgBegin(com);
				}
				procDumpCommandMsg( com , buffer , num );
			}

			return true;
		}


	};


	
	class LeelaOutputThread : public GTPOutputThread
	{
	public:

		struct ThinkInfo
		{
			int   v;
			int   nodeVisited;
			float winRate;
			float evalValue;
			std::vector< int > vSeq;
		};


		std::vector< ThinkInfo > thinkResults;

		virtual void dumpCommandMsgBegin(GTPCommand com) 
		{
			switch( com.id )
			{
			case Go::GTPCommand::eKomi:
				break;
			case Go::GTPCommand::eHandicap:
				break;
			case Go::GTPCommand::ePlay:
				break;
			case Go::GTPCommand::eGenmove:
				thinkResults.clear();
				break;
			case Go::GTPCommand::ePass:
				break;
			case Go::GTPCommand::eUndo:
				break;
			case Go::GTPCommand::eQuit:
				break;
			default:
				break;
			}
		}

		virtual void dumpCommandMsgEnd(GTPCommand com) {}


		static int GetVertex(FixString<128>  const& coord)
		{
			int vertex = -3;

			uint8 pos[2];
			if( coord == "Pass" )
			{
				vertex = -1;
			}
			else if( coord == "Resign" )
			{
				vertex = -2;
			}
			else if( Go::ReadCoord(coord, pos) )
			{
				vertex = LeelaGoSize * pos[1] + pos[0];
			}
			return vertex;
		}

		virtual void procDumpCommandMsg(GTPCommand com, char* buffer, int num)
		{
			switch( com.id )
			{
			case GTPCommand::eGenmove:
				{
					FixString<128>  coord;
					int   nodeVisited;
					float winRate;
					float evalValue;
					int   playout;
					if( sscanf( buffer , "Playouts: %d, Win: %f%% , PV: %s" , &playout , &winRate , coord.data() ) == 3 )
					{
						int vertex = GetVertex(coord);
						if( vertex == -3 )
						{
							LogWarningF(0, "Error Think Str = %s", buffer);
							return;
						}

						GameCommand com;
						com.setParam(LeelaGameParam::eBestMoveVertex, vertex);
						addOutputCommand(com);

					}
					else if( sscanf(buffer, "%s -> %d (V: %f%%) (N: %f%%)", coord.data() , &nodeVisited , &winRate , &evalValue ) == 4 )
					{
						int vertex = GetVertex( coord );
						if ( vertex == -3 )
						{
							LogWarningF(0, "Error Think Str = %s", buffer);
							return;
						}

						ThinkInfo info;
						info.v = vertex;
						info.nodeVisited = nodeVisited;
						info.winRate = winRate;
						info.evalValue = evalValue;
						thinkResults.push_back(info);
					}


				}
				break;
			}
		}


	};

	GTPLikeAppRun::~GTPLikeAppRun()
	{
		if( outputThread )
		{
			delete outputThread;
		}
	}

	void GTPLikeAppRun::stop()
	{
		inputProcessStream("quit\n");
		SystemPlatform::Sleep(1);
		if( outputThread )
		{
			outputThread->kill();
			delete outputThread;
			outputThread = nullptr;
		}
		process.terminate();
	}

	bool GTPLikeAppRun::restart()
	{
		if( outputThread )
			outputThread->restart();
		return inputCommand("clear_board\n", { GTPCommand::eRestart , 0 });
	}

	bool GTPLikeAppRun::playStone(int x, int y, int color)
	{
		FixString<128> com;
		char coord = 'A' + x;
		if( coord >= 'I' )
			++coord;
		com.format("play %c %c%d\n", (color == StoneColor::eBlack ? 'b' : 'w'), coord, y + 1);
		return inputCommand(com, { GTPCommand::ePlay , color });
	}

	bool GTPLikeAppRun::playPass()
	{
		FixString<128> com;
		com.format("play pass\n");
		return inputProcessStream(com);
	}

	bool GTPLikeAppRun::thinkNextMove(int color)
	{
		FixString<128> com;
		com.format("genmove %s\n", ((color == StoneColor::eBlack) ? "b" : "w"));
		return inputCommand(com , { GTPCommand::eGenmove , color });
	}

	bool GTPLikeAppRun::undo()
	{
		return inputCommand("undo\n", { GTPCommand::eUndo , 0 });
	}

	bool GTPLikeAppRun::setupGame(GameSetting const& setting )
	{
		FixString<128> com;
		com.format("komi %.1f\n", setting.komi);
		inputCommand( com , { GTPCommand::eKomi , 0 } );
		if( setting.numHandicap )
		{
			if( setting.bFixedHandicap )
			{
				com.format("fixed_handicap %d\n" , setting.numHandicap);
				inputCommand(com, { GTPCommand::eHandicap , setting.numHandicap });
			}
			else
			{
				for( int i = 0; i < i < setting.numHandicap; ++i )
				{
					com.format("genmove %s\n", "b");
					if( !inputCommand(com, { GTPCommand::eGenmove , StoneColor::eBlack }) )
						return false;
				}
			}
		}
		//com.format()

		return true;
	}

	bool GTPLikeAppRun::showResult()
	{
		if( !inputCommand("final_score\n", { GTPCommand::eFinalScore , 0 }) )
			return false;

		return true;
	}

	bool GTPLikeAppRun::inputCommand(char const* command, GTPCommand com)
	{
		if( !inputProcessStream(command) )
			return false;
		static_cast<GTPOutputThread*>(outputThread)->mProcQueue.push_back(com);
		return true;
	}

	bool GTPLikeAppRun::inputProcessStream(char const* command, int length /*= 0*/)
	{
		int numWrite = 0;
		return process.writeInputStream(command, length ? length : strlen(command), numWrite);
	}

	char const* LeelaAppRun::InstallDir = nullptr;

	std::string LeelaAppRun::GetLastWeightName()
	{
		FileIterator fileIter;
		FixString<256> path;
		path.format("%s/%s" , InstallDir , LEELA_NET_DIR );
		if( !FileSystem::FindFiles(path, nullptr, fileIter) )
		{
			return "";
		}

		bool haveBest = false;
		DateTime bestDate;
		std::string result;
		for( ; fileIter.haveMore(); fileIter.goNext() )
		{
			if( FileUtility::GetSubName(fileIter.getFileName()) != nullptr )
				continue;

			if( strlen(fileIter.getFileName()) != 64 )
				continue;

			DateTime date = fileIter.getLastModifyDate();

			if( !haveBest || date > bestDate )
			{
				result = fileIter.getFileName();
				bestDate = date;
				haveBest = true;
			}
		}
		return result;
	}

	std::string LeelaAppRun::GetDesiredWeightName()
	{
		char const* name;
		if( ::Global::GameConfig().tryGetStringValue("LeelaLastNetWeight", "Go", name) )
			return name;
		return LeelaAppRun::GetLastWeightName();
	}

	bool LeelaAppRun::buildLearningGame()
	{

		FixString<256> path;
		path.format("%s/%s", InstallDir, "/autogtp.exe");
		bool result = buildProcessT< AutoGTPOutputThread >( path , nullptr );
		return result;
	}

	bool LeelaAppRun::buildPlayGame(LeelaAISetting const& setting)
	{
		if( setting.weightName == nullptr )
			return false;

		FixString<256> path;
		path.format("%s/%s", InstallDir, "/leelaz.exe");

		LogMsgF("Play weight = %s", setting.weightName);

		std::string opitions = setting.toString();
		bool result = buildProcessT< LeelaOutputThread >(path, opitions.c_str());

		return result;
	}

	bool LeelaAppRun::buildAnalysisGame()
	{
		LeelaAISetting setting;
		std::string weightName = LeelaAppRun::GetLastWeightName();
		setting.weightName = weightName.c_str();
		setting.seed = generateRandSeed();
		setting.bNoPonder = false;
		setting.playouts = 0;
		setting.visits = 0;
		setting.randomcnt = 0;
		setting.resignpct = 0;
		return buildPlayGame(setting);
	}

	bool LeelaBot::initilize(void* settingData)
	{
		if( settingData )
		{
			if( !mAI.buildPlayGame(*static_cast<LeelaAISetting*>(settingData)) )
				return false;
		}
		else
		{
			LeelaAISetting setting = LeelaAISetting::GetDefalut();
			std::string weightName = LeelaAppRun::GetLastWeightName();
			setting.weightName = weightName.c_str();
			if( !mAI.buildPlayGame(setting) )
				return false;
		}
		return true;
	}

	char const* AQAppRun::InstallDir = nullptr;

	bool AQAppRun::buildPlayGame()
	{
		FixString<256> path;
		path.format("%s/%s", InstallDir, "/AQ.exe");
		FixString<512> command;
		return buildProcessT< GTPOutputThread >(path, command);
	}

	bool AQBot::initilize(void* settingData)
	{
		if( !mAI.buildPlayGame() )
			return false;

		static_cast<GTPOutputThread*>(mAI.outputThread)->bShowDiagnosticOutput = false;
		return true;
	}

	LeelaAISetting LeelaAISetting::GetDefalut()
	{
		LeelaAISetting setting;
		setting.seed = generateRandSeed();
#if 0
		setting.bNoise = true;
		setting.numThread = 4;
		setting.playouts = 10000;
#else
		setting.bNoise = ::Global::GameConfig().getIntValue("bNoise" , "LeelaZeroSetting", 0 );
		setting.bDumbPass = ::Global::GameConfig().getIntValue("bDumbPass", "LeelaZeroSetting", 0);
		setting.numThread = ::Global::GameConfig().getIntValue("numThread", "LeelaZeroSetting", 7);
		setting.visits = ::Global::GameConfig().getIntValue("visits", "LeelaZeroSetting", 20000);
		setting.playouts = ::Global::GameConfig().getIntValue("playouts", "LeelaZeroSetting", 0);
#endif
		return setting;
	}

}