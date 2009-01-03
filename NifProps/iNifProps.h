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
