#pragma once


#include "lib/CGameInfoCallback.h"
#include "lib/int3.h" // for int3

/*
 * CCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGHeroInstance;
class CGameState;
struct CPath;
class CGObjectInstance;
class CArmedInstance;
struct BattleAction;
class CGTownInstance;
struct lua_State;
class CClient;
class IShipyard;
struct CGPathNode;
struct CGPath;
struct CPathsInfo;
struct CPack;
class IBattleEventsReceiver;
class IGameEventsReceiver;
struct ArtifactLocation;

class IBattleCallback
{
public:
	bool waitTillRealize; //if true, request functions will return after they are realized by server
	bool unlockGsWhenWaiting;//if true after sending each request, gs mutex will be unlocked so the changes can be applied; NOTICE caller must have gs mx locked prior to any call to actiob callback!
	//battle
	virtual int battleMakeAction(BattleAction* action)=0;//for casting spells by hero - DO NOT use it for moving active stack
	virtual bool battleMakeTacticAction(BattleAction * action) =0; // performs tactic phase actions
};

class IGameActionCallback
{
public:
	//hero
	virtual bool moveHero(const CGHeroInstance *h, int3 dst) =0; //dst must be free, neighbouring tile (this function can move hero only by one tile)
	virtual bool dismissHero(const CGHeroInstance * hero)=0; //dismisses given hero; true - successfuly, false - not successfuly
	virtual void dig(const CGObjectInstance *hero)=0;
	virtual void castSpell(const CGHeroInstance *hero, SpellID spellID, const int3 &pos = int3(-1, -1, -1))=0; //cast adventure map spell

	//town
	virtual void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero)=0;
	virtual bool buildBuilding(const CGTownInstance *town, BuildingID buildingID)=0;
	virtual void recruitCreatures(const CGDwelling *obj, const CArmedInstance * dst, CreatureID ID, ui32 amount, si32 level=-1)=0;
	virtual bool upgradeCreature(const CArmedInstance *obj, SlotID stackPos, CreatureID newID=CreatureID::NONE)=0; //if newID==-1 then best possible upgrade will be made
	virtual void swapGarrisonHero(const CGTownInstance *town)=0;

	virtual void trade(const CGObjectInstance *market, EMarketMode::EMarketMode mode, int id1, int id2, int val1, const CGHeroInstance *hero = nullptr)=0; //mode==0: sell val1 units of id1 resource for id2 resiurce

	virtual int selectionMade(int selection, QueryID queryID) =0;
	virtual int swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)=0;//swaps creatures between two possibly different garrisons // TODO: AI-unsafe code - fix it!
	virtual int mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)=0;//joins first stack to the second (creatures must be same type)
	virtual int mergeOrSwapStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2) =0; //first goes to the second
	virtual int splitStack(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2, int val)=0;//split creatures from the first stack
	//virtual bool swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2)=0; //swaps artifacts between two given heroes
	virtual bool swapArtifacts(const ArtifactLocation &l1, const ArtifactLocation &l2)=0;
	virtual bool assembleArtifacts(const CGHeroInstance * hero, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo)=0;
	virtual bool dismissCreature(const CArmedInstance *obj, SlotID stackPos)=0;
	virtual void endTurn()=0;
	virtual void buyArtifact(const CGHeroInstance *hero, ArtifactID aid)=0; //used to buy artifacts in towns (including spell book in the guild and war machines in blacksmith)
	virtual void setFormation(const CGHeroInstance * hero, bool tight)=0;

	virtual void save(const std::string &fname) = 0;
	virtual void sendMessage(const std::string &mess, const CGObjectInstance * currentObject = nullptr) = 0;
	virtual void buildBoat(const IShipyard *obj) = 0;
};

struct CPack;

class CBattleCallback : public IBattleCallback, public CPlayerBattleCallback
{
protected:
	int sendRequest(const CPack *request); //returns requestID (that'll be matched to requestID in PackageApplied)
	CClient *cl;
	//virtual bool hasAccess(int playerId) const;

public:
	CBattleCallback(CGameState *GS, boost::optional<PlayerColor> Player, CClient *C);
	int battleMakeAction(BattleAction* action) override;//for casting spells by hero - DO NOT use it for moving active stack
	bool battleMakeTacticAction(BattleAction * action) override; // performs tactic phase actions

	friend class CCallback;
	friend class CClient;
};

class CCallback : public CPlayerSpecificInfoCallback, public IGameActionCallback, public CBattleCallback
{
public:
	CCallback(CGameState * GS, boost::optional<PlayerColor> Player, CClient *C);
	virtual ~CCallback();

	//client-specific functionalities (pathfinding)
	virtual bool canMoveBetween(const int3 &a, const int3 &b);
	virtual int getMovementCost(const CGHeroInstance * hero, int3 dest);
	virtual int3 getGuardingCreaturePosition(int3 tile);
	virtual const CPathsInfo * getPathsInfo(const CGHeroInstance *h);

	virtual void calculatePaths(const CGHeroInstance *hero, CPathsInfo &out);

	//Set of metrhods that allows adding more interfaces for this player that'll receive game event call-ins.
	void registerGameInterface(shared_ptr<IGameEventsReceiver> gameEvents);
	void registerBattleInterface(shared_ptr<IBattleEventsReceiver> battleEvents);
	void unregisterGameInterface(shared_ptr<IGameEventsReceiver> gameEvents);
	void unregisterBattleInterface(shared_ptr<IBattleEventsReceiver> battleEvents);

	void unregisterAllInterfaces(); //stops delivering information about game events to player interfaces -> can be called ONLY after victory/loss

//commands
	bool moveHero(const CGHeroInstance *h, int3 dst); //dst must be free, neighbouring tile (this function can move hero only by one tile)
	bool teleportHero(const CGHeroInstance *who, const CGTownInstance *where);
	int selectionMade(int selection, QueryID queryID);
	int swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2);
	int mergeOrSwapStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2); //first goes to the second
	int mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2); //first goes to the second
	int splitStack(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2, int val);
	bool dismissHero(const CGHeroInstance * hero);
	//bool swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2);
	bool swapArtifacts(const ArtifactLocation &l1, const ArtifactLocation &l2);
	//bool moveArtifact(const CGHeroInstance * hero, ui16 src, const CStackInstance * stack, ui16 dest); // TODO: unify classes
	//bool moveArtifact(const CStackInstance * stack, ui16 src , const CGHeroInstance * hero, ui16 dest); // TODO: unify classes
	bool assembleArtifacts(const CGHeroInstance * hero, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo);
	bool buildBuilding(const CGTownInstance *town, BuildingID buildingID) override;
	void recruitCreatures(const CGDwelling * obj, const CArmedInstance * dst, CreatureID ID, ui32 amount, si32 level=-1);
	bool dismissCreature(const CArmedInstance *obj, SlotID stackPos);
	bool upgradeCreature(const CArmedInstance *obj, SlotID stackPos, CreatureID newID=CreatureID::NONE) override;
	void endTurn();
	void swapGarrisonHero(const CGTownInstance *town);
	void buyArtifact(const CGHeroInstance *hero, ArtifactID aid) override;
	void trade(const CGObjectInstance *market, EMarketMode::EMarketMode mode, int id1, int id2, int val1, const CGHeroInstance *hero = nullptr);
	void setFormation(const CGHeroInstance * hero, bool tight);
	void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero);
	void save(const std::string &fname);
	void sendMessage(const std::string &mess, const CGObjectInstance * currentObject = nullptr);
	void buildBoat(const IShipyard *obj);
	void dig(const CGObjectInstance *hero);
	void castSpell(const CGHeroInstance *hero, SpellID spellID, const int3 &pos = int3(-1, -1, -1));

//friends
	friend class CClient;
};
