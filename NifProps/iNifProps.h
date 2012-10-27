#pragma once

const Class_ID BSDSMODIFIER_CLASS_ID(0xe9a0a68e, 0xb091bd48);

class IBSDismemberSkinModifierData;

// Selection levels:
#define IMESHSEL_OBJECT 0
#define IMESHSEL_VERTEX 1
#define IMESHSEL_FACE 2
#define IMESHSEL_EDGE 3

enum DismemberBodyPartType {
   BP_TORSO = 0, /*!< Torso */
   BP_HEAD = 1, /*!< Head */
   BP_HEAD2 = 2, /*!< Head 2 */
   BP_LEFTARM = 3, /*!< Left Arm */
   BP_LEFTARM2 = 4, /*!< Left Arm 2 */
   BP_RIGHTARM = 5, /*!< Right Arm */
   BP_RIGHTARM2 = 6, /*!< Right Arm 2 */
   BP_LEFTLEG = 7, /*!< Left Leg */
   BP_LEFTLEG2 = 8, /*!< Left Leg 2 */
   BP_LEFTLEG3 = 9, /*!< Left Leg 3 */
   BP_RIGHTLEG = 10, /*!< Right Leg */
   BP_RIGHTLEG2 = 11, /*!< Right Leg 2 */
   BP_RIGHTLEG3 = 12, /*!< Right Leg 3 */
   BP_BRAIN = 13, /*!< Brain */

BP_SKY_HEAD = 30, /*!< Head */ 
BP_SKY_HAIR = 31, /*!< Hair */ 
BP_SKY_BODY = 32, /*!< Body */ 
BP_SKY_HAND = 33, /*!< Hands */ 
BP_SKY_FOREARMS = 34, /*!< Forearms */ 
BP_SKY_AMULET = 35, /*!< Amulet */ 
BP_SKY_RING = 36, /*!< Ring */ 
BP_SKY_FEET = 37, /*!< FEET */ 
BP_SKY_CALVES = 38, /*!< CALVES */ 
BP_SKY_SHIELD = 39, /*!< SHIELD */ 
BP_SKY_TAIL = 40, /*!< TAIL */ 
BP_SKY_LONG_HAIR = 41, /*!< LONG HAIR */  
BP_SKY_CIRCLET = 42, /*!< CIRCLET */  
BP_SKY_EARS = 43, /*!< EARS */  
BP_SKY_DECAPITATED_HEAD = 50, /*!< DECAPITATED HEAD */  
BP_SKY_DECAPITATE = 51, /*!< DECAPITATE */  
BP_SKY_FX01 = 61, /*!< FX01 */  
BP_SKY_BLOODIED_DRAGON_HEADS = 44, /*!< Used in bloodied dragon heads, so it is free for NPCs */  
 

BP_SKY_BLOODIED_DRAGON_HEADS_1 = 45, /*!< Used in bloodied dragon wings, so it is free for NPCs */
BP_SKY_BLOODIED_DRAGON_HEADS_2 = 47, /*!< Used in bloodied dragon tails, so it is free for NPCs */
BP_SKY_HELMET = 130, /*!< Used in helmetts that conceal the whole head and neck inside */
BP_SKY_OPEN_FACE_HELMET = 131, /*!< Used in open faced helmets\hoods (Also the nightingale hood) */
BP_SKY_DISABLE_HAIR_HEADPART = 141, /*!< Disables Hair Geometry like 131 and 31 */
BP_SKY_CIRCLETS_ACCESOARIES = 142, /*!< Used in circlets */
BP_SKY_DISABLE_EAR_HEADPART = 143, /*!< Disabled Ear geometry to prevent clipping issues? */
BP_SKY_GORE_COVERS_HEAD_NECK = 150, /*!< The gore that covers a decapitated head neck */
BP_SKY_NECK_DECAPITATION_POINT = 230, /*!< Neck, where 130 and this meets is the decapitation point of the neck */

BP_SKY_FACE_MOUTH = 44, /*!< face/mouth */
BP_SKY_NECK = 45, /*!< neck (like a cape, scarf, or shawl, neck-tie etc) */
BP_SKY_CHEST = 46, /*!< chest primary or outergarment */
BP_SKY_BACK = 47, /*!< back (like a backpack/wings etc) */
BP_SKY_MISC_FX = 48, /*!< misc/FX (use for anything that doesnt fit in the list) */
BP_SKY_PELVIS = 49, /*!< pelvis primary or outergarment */
BP_SKY_PELVIS_1 = 52, /*!< pelvis secondary or undergarment */
BP_SKY_LEG = 53, /*!< leg primary or outergarment or right leg */
BP_SKY_LEG_1 = 54, /*!< leg secondary or undergarment or leftt leg */
BP_SKY_FACE = 55, /*!< face alternate or jewelry */
BP_SKY_CHEST_1 = 56, /*!< chest secondary or undergarment */
BP_SKY_SHOULDER = 57, /*!< shoulder */
BP_SKY_ARM_1 = 58, /*!< arm secondary or undergarment or left arm */
BP_SKY_ARM = 59, /*!< arm primary or outergarment or right arm */
BP_SKY_MISC_FX_1 = 60, /*!< misc/FX (use for anything that doesnt fit in the list)*/

   BP_SECTIONCAP_HEAD = 101, /*!< Section Cap | Head */
   BP_SECTIONCAP_HEAD2 = 102, /*!< Section Cap | Head 2 */
   BP_SECTIONCAP_LEFTARM = 103, /*!< Section Cap | Left Arm */
   BP_SECTIONCAP_LEFTARM2 = 104, /*!< Section Cap | Left Arm 2 */
   BP_SECTIONCAP_RIGHTARM = 105, /*!< Section Cap | Right Arm */
   BP_SECTIONCAP_RIGHTARM2 = 106, /*!< Section Cap | Right Arm 2 */
   BP_SECTIONCAP_LEFTLEG = 107, /*!< Section Cap | Left Leg */
   BP_SECTIONCAP_LEFTLEG2 = 108, /*!< Section Cap | Left Leg 2 */
   BP_SECTIONCAP_LEFTLEG3 = 109, /*!< Section Cap | Left Leg 3 */
   BP_SECTIONCAP_RIGHTLEG = 110, /*!< Section Cap | Right Leg */
   BP_SECTIONCAP_RIGHTLEG2 = 111, /*!< Section Cap | Right Leg 2 */
   BP_SECTIONCAP_RIGHTLEG3 = 112, /*!< Section Cap | Right Leg 3 */
   BP_SECTIONCAP_BRAIN = 113, /*!< Section Cap | Brain */
   BP_TORSOCAP_HEAD = 201, /*!< Torso Cap | Head */
   BP_TORSOCAP_HEAD2 = 202, /*!< Torso Cap | Head 2 */
   BP_TORSOCAP_LEFTARM = 203, /*!< Torso Cap | Left Arm */
   BP_TORSOCAP_LEFTARM2 = 204, /*!< Torso Cap | Left Arm 2 */
   BP_TORSOCAP_RIGHTARM = 205, /*!< Torso Cap | Right Arm */
   BP_TORSOCAP_RIGHTARM2 = 206, /*!< Torso Cap | Right Arm 2 */
   BP_TORSOCAP_LEFTLEG = 207, /*!< Torso Cap | Left Leg */
   BP_TORSOCAP_LEFTLEG2 = 208, /*!< Torso Cap | Left Leg 2 */
   BP_TORSOCAP_LEFTLEG3 = 209, /*!< Torso Cap | Left Leg 3 */
   BP_TORSOCAP_RIGHTLEG = 210, /*!< Torso Cap | Right Leg */
   BP_TORSOCAP_RIGHTLEG2 = 211, /*!< Torso Cap | Right Leg 2 */
   BP_TORSOCAP_RIGHTLEG3 = 212, /*!< Torso Cap | Right Leg 3 */
   BP_TORSOCAP_BRAIN = 213, /*!< Torso Cap | Brain */
   BP_TORSOSECTION_HEAD = 1000, /*!< Torso Section | Head */
   BP_TORSOSECTION_HEAD2 = 2000, /*!< Torso Section | Head 2 */
   BP_TORSOSECTION_LEFTARM = 3000, /*!< Torso Section | Left Arm */
   BP_TORSOSECTION_LEFTARM2 = 4000, /*!< Torso Section | Left Arm 2 */
   BP_TORSOSECTION_RIGHTARM = 5000, /*!< Torso Section | Right Arm */
   BP_TORSOSECTION_RIGHTARM2 = 6000, /*!< Torso Section | Right Arm 2 */
   BP_TORSOSECTION_LEFTLEG = 7000, /*!< Torso Section | Left Leg */
   BP_TORSOSECTION_LEFTLEG2 = 8000, /*!< Torso Section | Left Leg 2 */
   BP_TORSOSECTION_LEFTLEG3 = 9000, /*!< Torso Section | Left Leg 3 */
   BP_TORSOSECTION_RIGHTLEG = 10000, /*!< Torso Section | Right Leg */
   BP_TORSOSECTION_RIGHTLEG2 = 11000, /*!< Torso Section | Right Leg 2 */
   BP_TORSOSECTION_RIGHTLEG3 = 12000, /*!< Torso Section | Right Leg 3 */
   BP_TORSOSECTION_BRAIN = 13000, /*!< Torso Section | Brain */
};

struct BSDSPartitionData
{
   BSDSPartitionData() : bodyPart(::BP_TORSO), partFlag(1), selLevel(0) {}
   DismemberBodyPartType bodyPart;
   DWORD partFlag;
   DWORD selLevel;
};

class SelectModContextEnumProc: public ModContextEnumProc {
public:
   virtual BOOL proc(ModContext *mc) {
      mcList.Append(1, &mc);
      return TRUE;
   }
   ModContextList mcList;
};

const ULONG I_BSDISMEMBERSKINMODIFIER		= I_USERINTERFACE + 0x0000E271;
const ULONG I_BSDISMEMBERSKINMODIFIERDATA	= I_USERINTERFACE + 0x0000E272;

class IBSDismemberSkinModifier
//   : public MaxHeapOperators 
{
public:
   /*! \remarks This method must be called when the <b>LocalModData</b> of
   the modifier is changed. Developers can use the methods of
   <b>IMeshSelectData</b> to get and set the actual selection for vertex, face
   and edge. When a developers does set any of these selection sets this
   method must be called when done. */
   virtual void LocalDataChanged()=0;

   /*! \remarks Gets all of the mod contexts related to this modifier. */
   virtual Tab<IBSDismemberSkinModifierData*> GetModifierData() = 0;
};


class IBSDismemberSkinModifierData 
//   : public MaxHeapOperators 
{
public:

   /*! \remarks Returns the number of partitions for the modifier. */
   virtual DWORD GetNumPartitions()=0;

   /*! \remarks Adds the specified partition and returns its index. */
   virtual DWORD AddPartition()=0;

   /*! \remarks Rempves the specified partition from the modifier. */
   virtual void RemovePartition(DWORD partition)=0;

   /*! \remarks Returns the current level of selection for the modifier. */
   virtual DWORD GetActivePartition()=0;

   /*! \remarks Sets the currently selected partition level of the modifier. */
   virtual void SetActivePartition(DWORD partition)=0;

   virtual BitArray& GetVertSel(int index)=0;
   virtual BitArray& GetFaceSel(int index)=0;
   virtual BitArray& GetEdgeSel(int index)=0;

   virtual void SetVertSel(int index, BitArray &set, IBSDismemberSkinModifier *imod, TimeValue t)=0;
   virtual void SetFaceSel(int index, BitArray &set, IBSDismemberSkinModifier *imod, TimeValue t)=0;
   virtual void SetEdgeSel(int index, BitArray &set, IBSDismemberSkinModifier *imod, TimeValue t)=0;

   virtual Tab<BSDSPartitionData> & GetPartitionFlags ()=0;

   virtual GenericNamedSelSetList & GetVertSelList ()=0;
   virtual GenericNamedSelSetList & GetEdgeSelList ()=0;
   virtual GenericNamedSelSetList & GetFaceSelList ()=0;
};


extern Modifier *GetOrCreateBSDismemberSkin(INode *node);
extern Modifier *GetBSDismemberSkin(INode *node);
