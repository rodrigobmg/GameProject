#ifndef TAblilityProp_h__
#define TAblilityProp_h__

enum ActorState
{
	AS_NORMAL  = 0,
	AS_POISON  = BIT( 0 ), //中毒
	AS_STUPOR  = BIT( 1 ), //麻痺
	AS_WILD    = BIT( 2 ), //狂暴
	AS_DIE     = BIT( 3 ), //死亡

	AS_CATCH_OBJ = BIT( 4 ),
	AS_TOUCH_BAG = BIT( 5 ),
	AS_LADDER    = BIT( 6 ),
};


enum PropValueType
{
	PVT_BOOL  ,
	PVT_INT   ,
	PVT_FLOAT ,
};

struct PropValueInfo
{
	char          id;
	PropValueType type;
	char const*   name;
};


enum PropType
{
	PROP_HP  ,
	PROP_MP  ,
	PROP_MAX_HP ,
	PROP_MAX_MP ,
	PROP_STR ,
	PROP_INT ,
	PROP_END ,
	PROP_DEX ,
	PROP_MAT ,
	PROP_SAT ,
	PROP_DT ,
	PROP_VIEW_DIST,
	PROP_AT_RANGE ,
	PROP_AT_SPEED ,
	PROP_MV_SPEED ,
	PROP_KEXP ,

	PROP_JUMP_HEIGHT ,
	PROP_LEVEL ,

	PROP_TYPE_NUM,
};

enum PropFlag
{
	PF_VOP_ADD = 0  ,
	PF_VOP_MUL = 1  ,
	PF_VOP_SUB = 2  ,
	PF_VOP_DIV = 3  ,

	PF_TOP_ADD   = BIT( 4 ),
	PF_TOP_SUB   = BIT( 5 ),
	PF_NO_REPEAT = BIT( 6 ),

	PF_VALUE_OPERAOTR = 0x0000000f,
};

enum ActorState;
typedef float PropVal;

inline PropFlag getInvOp( PropFlag op )
{
	return  PropFlag( ( op + 2 ) % 4 );  
}

struct PropModifyInfo
{
	PropModifyInfo();

	PropType   prop;
	ActorState state;
	float      val;
	float      time;
	unsigned   flag;


	template<class Archive >
	void serialize( Archive & ar , const unsigned int /* file_version */)
	{
		ar & prop & state & val & time & flag;
	}
};


class SAbilityPropData
{
public:
	SAbilityPropData(){}
	SAbilityPropData( int level , int MaxHP , int MaxMP ,int  STR ,int INT ,int DEX ,int END , int KExp );
	int   level;
	int   MaxHP;
	int   MaxMP;
	///////////////////////////
	int   STR; //力量
	int   INT; //智力
	int   DEX; //敏捷
	int   END; //耐力
	////////////////////////////
	int   KExp;    //可獲得的經驗值
	//////////////////////////
	float viewDist;
	float ATRange; //攻擊範圍(由武器更改)
	float ATSpeed; //攻擊速度
	float MVSpeed; //最大移動速度
	float JumpHeight;
	//////////////////////////
	int   MAT; //主武器攻擊力
	int   SAT; //副武器攻擊力
	int   DT;  //防具總裝甲值
	///////////////////////////
	int   HP;
	int   MP;

	PropVal getPropValue( PropType prop ) const;
	void    setPropVal( PropType prop , PropVal val );
	void    modifyPropVal( PropModifyInfo& info );

	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );

};


#endif // TAblilityProp_h__