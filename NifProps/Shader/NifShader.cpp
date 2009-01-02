/**********************************************************************
 *<
   FILE: NifShader.cpp

   DESCRIPTION:   Appwizard generated plugin

   CREATED BY: tazpn (Theo)

   HISTORY: 

 *>   Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/
#pragma warning( disable:4800 )
#include <max.h>
#include <shaders.h>
#include "MAX_Mem.h"
#include <map>
#include "iparamm.h"
#include "Simpobj.h"
#include "surf_api.h"
#include "notify.h"
#include "macroRec.h"
#include <custcont.h>
#include <shlwapi.h>
#include "..\NifProps\NifProps.h"

#if 0

const Class_ID NIFSHADER_CLASS_ID(0x566e8cc8, 0xb091bd48);
const Interface_ID NIFSHADER_INTERFACE_ID(0x566e8cc9, 0xb091bd48);

extern ClassDesc2* GetNifShaderDesc();
extern ParamMap2UserDlgProc* GetNifShaderDlgProc();

// Paramblock2 name
enum { shader_params, };
// Paramblock2 parameter list

struct TexChannel 
{
   DWORD channelName;
   DWORD maxName;
   DWORD channelType;
};

static const TexChannel texChannelNames[STD2_NMAX_TEXMAPS] = {
   {IDS_CHAN_BASE,      IDS_MAXCHAN_DIFFUSE,       CLR_CHANNEL    },
   {IDS_CHAN_DARK,      IDS_MAXCHAN_SELFILLUMMAP,  CLR_CHANNEL    },
   {IDS_CHAN_DETAIL,    IDS_MAXCHAN_DETAIL,        CLR_CHANNEL    },
   {IDS_CHAN_GLOSS,     IDS_MAXCHAN_GLOSS,         CLR_CHANNEL    },
   {IDS_CHAN_GLOW,      IDS_MAXCHAN_GLOW,          CLR_CHANNEL    },
   {IDS_CHAN_BUMP,      IDS_MAXCHAN_BUMP,          CLR_CHANNEL    },
   {IDS_CHAN_NORMAL,    IDS_MAXCHAN_NORMAL,        CLR_CHANNEL    },
   {IDS_CHAN_UNK2,      IDS_MAXCHAN_DIFFUSE,       CLR_CHANNEL    },
   {IDS_CHAN_DECAL1,    IDS_MAXCHAN_DIFFUSE,       CLR_CHANNEL    },
   {IDS_CHAN_DECAL2,    IDS_MAXCHAN_DIFFUSE,       CLR_CHANNEL    },
   {IDS_CHAN_DECAL3,    IDS_MAXCHAN_DIFFUSE,       CLR_CHANNEL    },
   {IDS_CHAN_ENVMASK,   IDS_MAXCHAN_DIFFUSE,       CLR_CHANNEL    },
   {IDS_CHAN_ENV,       IDS_MAXCHAN_DIFFUSE,       CLR_CHANNEL    },
   {IDS_CHAN_HEIGHT,    IDS_MAXCHAN_DIFFUSE,       CLR_CHANNEL    },
   {IDS_CHAN_REFLECTION,IDS_MAXCHAN_DIFFUSE,       CLR_CHANNEL    },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },
   {IDS_CHAN_EMPTY,     IDS_MAXCHAN_EMPTY,         UNSUPPORTED_CHANNEL },  
};

// map from custom channel to standard map
static const int stdID2Channel[N_ID_CHANNELS] = {
   -1,     //   ambient
    0,     //   diffuse           
   -1,     //   specular
   -1,     //   shininess
    4,     //   shininess strength
    1,     //   self-illumination 
   -1,     //   opacity
   -1,     //   filter color
   -1,     //   bump              
   -1,     //   reflection        
   -1,     //   refraction 
   -1,     //   displacement
};


enum 
{ 
   ns_mat_ambient, ns_mat_diffuse, ns_mat_specular, 
   ns_mat_selfillumclr, ns_mat_selfillum,
   ns_mat_glossiness, ns_mat_speclevel, ns_mat_softenlevel,
   ns_mat_shininess, ns_mat_alpha, ns_mat_dither,
   ns_mat_selfillumon,

   //ns_amb, ns_diff, ns_spec, ns_emit, ns_shininess, ns_alpha, ns_test_ref,
   //ns_srcBlend, ns_destBlend, ns_testmode, ns_alphatest_enable, 
   //ns_applyMode, ns_srcVertexMode, ns_lightingMode,
   //ns_vertexColors_enable, ns_specular_enable, ns_no_sorter, ns_dither,
   //// removing ps2 params would cause loading to fail for legacy files
   //ns_l_base,   ns_l_dark,   ns_l_detail,   ns_l_decal1,   ns_l_decal2,   ns_l_glow, 
   //// allowing ns_k to be 24 causes the .MAX file to be unloadable
   //ns_k_base=100, ns_k_dark,   ns_k_detail,   ns_k_decal1,   ns_k_decal2,
   //ns_k_glow, 
   //ns_lon_base, ns_lon_dark, ns_lon_detail, ns_lon_decal1, ns_lon_decal2, 
   //ns_lon_glow, 
   //ns_kon_base, ns_kon_dark, ns_kon_detail, ns_kon_decal1, ns_kon_decal2,
   //ns_kon_glow, 
   //ns_alphaMode, ns_ps2_dark_rgb, ns_ps2_detail_rgb, ns_bump_magnitude, 
   //ns_luma_offset, ns_luma_scale,
   //ns_base_export, ns_dark_export, ns_detail_export, ns_decal1_export, 
   //ns_decal2_export, ns_gloss_export, ns_glow_export, 
   //ns_shader_name, ns_shader_viewer_technique, ns_shader_export_technique,
   //ns_use_normal_maps, ns_normal_map_technique, ns_l_gloss, ns_lon_gloss,
   //ns_k_gloss, ns_kon_gloss, ns_parallax_offset,
   //ns_num_params
};


struct NifShaderData
{
   // Material 
   Color     ambient;
   Color     diffuse;
   Color     specular;
   Color     emittance;
   float     shininess;
   float     alpha;
   bool      dither;

   Color     reflection;
};

const ULONG SHADER_PARAMS = (STD_PARAM_SELFILLUM | STD_PARAM_SELFILLUM_CLR | STD_PARAM_DIFFUSE_CLR | STD_PARAM_SPECULAR_CLR | STD_PARAM_SELFILLUM_CLR_ON);

class NifShaderDlg;

class NifShader : public Shader 
{
   NifShaderDlg *pDlg;
   IParamBlock2 *pb; 

public:

   //Constructor/Destructor
   NifShader();
   ~NifShader();      


   // Main plugin identity support
   Class_ID ClassID() { return NIFSHADER_CLASS_ID; }
   SClass_ID SuperClassID() { return SHADER_CLASS_ID; }
   TSTR GetName() { return GetString( IDS_SH_NAME ); }
   void GetClassName(TSTR& s) { s = GetName(); }
   void DeleteThis() { delete this; }

   int NumSubs() { return 1; }
   Animatable* SubAnim(int i) { return (i==0)? pb : NULL; }
   TSTR SubAnimName(int i) { return TSTR(GetString( IDS_SH_PARAMETERS)); }
   int SubNumToRefNum(int subNum) { return subNum; }

   int NumRefs() { return 1; }
   RefTargetHandle GetReference(int i) { return (i==0)? pb : NULL; }
   void SetReference(int i, RefTargetHandle rtarg) { if (i==0) pb = (IParamBlock2*)rtarg; }
   void NotifyChanged();
   RefTargetHandle Clone( RemapDir &remap/*=NoRemap()*/ );
   RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget,
      PartID& partID, RefMessage message );

   int NumParamBlocks() { return 1; }
   IParamBlock2* GetParamBlock(int i) { return (i==0)? pb : NULL; }
   IParamBlock2* GetParamBlockByID(BlockID id)  { return (pb->ID()==id)? pb : NULL; }

   //////////////////////////////////////////////////////////////////////////
   // Shader Implementation

   // Find out if we have a key at time value t
   BOOL KeyAtTime(int id,TimeValue t) { return FALSE; }

   // Get the requirements for this material (supersampling, etc)
   ULONG GetRequirements( int subMtlNum ) { return MTLREQ_PHONG;  }

   ShaderParamDlg* CreateParamDialog(HWND hOldRollup, HWND hwMtlEdit, 
      IMtlParams *imp, StdMat2* theMtl, int rollupOpen, int );

   virtual int NParamDlgs(){ return 1; }
   ShaderParamDlg* GetParamDlg(int n=0 ) { return (ShaderParamDlg*)pDlg; }
   void SetParamDlg( ShaderParamDlg* newDlg, int n=0 ) { pDlg = (NifShaderDlg*)pDlg;  }

   // std parameter support
   virtual ULONG SupportStdParams() { return SHADER_PARAMS; }

   // this method only req'd for R2.5 shaders, to convert stdmtl1 paramblks to current
   virtual void ConvertParamBlk( ParamBlockDescID *descOld, int oldCount, IParamBlock *oldPB ){}

   // Shader state methods
   void Update(TimeValue t, Interval& valid);

   // Standard load / save
   //IOResult Save(ISave *isave);
   //IOResult Load(ILoad *iload);

   void GetIllumParams( ShadeContext &sc, IllumParams& ip );
   void Illum(ShadeContext &sc, IllumParams &ip);

   long nTexChannelsSupported();
   TSTR GetTexChannelName( long nTextureChan );
   TSTR GetTexChannelInternalName( long nTextureChan );
   long ChannelType( long nTextureChan );
   long StdIDToChannel( long stdID );

   void Reset();	//reset to default values


   virtual void CopyStdParams( Shader* pFrom );
   // these are the standard shader params
   virtual void SetLockDS(BOOL lock) {}
   virtual BOOL GetLockDS() { return FALSE; }
   virtual void SetLockAD(BOOL lock) {}
   virtual BOOL GetLockAD() { return FALSE; }
   virtual void SetLockADTex(BOOL lock) {}
   virtual BOOL GetLockADTex() { return FALSE; }

   virtual void SetSelfIllum(float v, TimeValue t) {
      pb->SetValue(ns_mat_selfillum, t, v, 0);
   }
   virtual void SetSelfIllumClrOn( BOOL on ) {
      int v = on ? 1 : 0;
      pb->SetValue(ns_mat_selfillumon, 0, v, 0);
   }
   virtual void SetSelfIllumClr(Color c, TimeValue t) {
      pb->SetValue(ns_mat_selfillumclr, t, c, 0);
   }
   virtual void SetAmbientClr(Color c, TimeValue t) {
      pb->SetValue(ns_mat_ambient, t, c, 0);
   }	
   virtual void SetDiffuseClr(Color c, TimeValue t) {
      pb->SetValue(ns_mat_diffuse, t, c, 0);
   }
   virtual void SetSpecularClr(Color c, TimeValue t) {
      pb->SetValue(ns_mat_specular, t, c, 0);
   }
   virtual void SetGlossiness(float v, TimeValue t) {
      pb->SetValue(ns_mat_glossiness, t, v, 0);
   }
   virtual void SetSpecularLevel(float v, TimeValue t) {
      pb->SetValue(ns_mat_speclevel, t, v, 0);
   }
   virtual void SetSoftenLevel(float v, TimeValue t) {
      pb->SetValue(ns_mat_softenlevel, t, v, 0);
   }

   virtual BOOL IsSelfIllumClrOn(int mtlNum, BOOL backFace) {
      return pb->GetInt(ns_mat_selfillumon, 0, 0) ? TRUE : FALSE;
   }
   virtual Color GetAmbientClr(int mtlNum, BOOL backFace) {
      return pb->GetColor(ns_mat_ambient, 0, 0);
   }
   virtual Color GetDiffuseClr(int mtlNum, BOOL backFace) { 
      return pb->GetColor(ns_mat_diffuse, 0, 0);
   }
   virtual Color GetSpecularClr(int mtlNum, BOOL backFace) { 
      return pb->GetColor(ns_mat_specular, 0, 0);
   }
   virtual Color GetSelfIllumClr(int mtlNum, BOOL backFace) { 
      return pb->GetColor(ns_mat_selfillumclr, 0, 0);
   }
   virtual float GetSelfIllum(int mtlNum, BOOL backFace) {
      return pb->GetFloat(ns_mat_selfillum, 0, 0);
   }
   virtual float GetGlossiness(int mtlNum, BOOL backFace) {
      return pb->GetFloat(ns_mat_glossiness, 0, 0);
   }
   virtual float GetSpecularLevel(int mtlNum, BOOL backFace) {
      return pb->GetFloat(ns_mat_speclevel, 0, 0);
   }
   virtual float GetSoftenLevel(int mtlNum, BOOL backFace) {
      return pb->GetFloat(ns_mat_softenlevel, 0, 0);
   }

   virtual BOOL IsSelfIllumClrOn() {
      return pb->GetInt(ns_mat_selfillumon, 0, 0) ? TRUE : FALSE;
   }
   virtual Color GetAmbientClr(TimeValue t) { 
      return pb->GetColor(ns_mat_ambient, 0, 0);
   }
   virtual Color GetDiffuseClr(TimeValue t) { 
      return pb->GetColor(ns_mat_diffuse, 0, 0);
   }		
   virtual Color GetSpecularClr(TimeValue t) { 
      return pb->GetColor(ns_mat_specular, 0, 0);
   }
   virtual float GetGlossiness( TimeValue t) {
      return pb->GetFloat(ns_mat_glossiness, 0, 0);
   }		
   virtual float GetSpecularLevel(TimeValue t) {
      return pb->GetFloat(ns_mat_speclevel, 0, 0);
   }
   virtual float GetSoftenLevel(TimeValue t){
      return pb->GetFloat(ns_mat_softenlevel, 0, 0);
   }
   virtual float GetSelfIllum(TimeValue t) {
      return pb->GetFloat(ns_mat_selfillum, 0, 0);
   }
   virtual Color GetSelfIllumClr(TimeValue t) { 
      return pb->GetColor(ns_mat_selfillumclr, 0, 0);
   }	
   virtual float EvalHiliteCurve(float x){ return 0.0f; }
   virtual float EvalHiliteCurve2(float x, float y, int level = 0 ){ return 0.0f; }
};


class NifShaderClassDesc : public ClassDesc2 {
   public:
   NifShaderClassDesc();
   int            IsPublic()     { return TRUE; }
   void *         Create(BOOL loading = FALSE) { return new NifShader(); }
   const TCHAR *  ClassName()    { return GetString(IDS_NIF_SHADER_NAME); }
   SClass_ID      SuperClassID() { return SHADER_CLASS_ID; }
   Class_ID       ClassID()      { return NIFSHADER_CLASS_ID; }
   const TCHAR*   Category()     { return GetString(IDS_CATEGORY); }

   const TCHAR*   InternalName() { return _T("NifShader"); }   // returns fixed parsable name (scripter-visible name)
   HINSTANCE      HInstance()    { return hInstance; }         // returns owning module handle
};

//--- Parameter map/block descriptors -------------------------------

// Parameter and ParamBlock IDs

class NifShaderPBAccessor : public PBAccessor
{ 
public:
   TSTR iniFileName;
   bool iniFileValid;

   NifShaderPBAccessor() : iniFileValid(false)
   {
      Interface *gi = GetCOREInterface();
      TCHAR iniName[MAX_PATH];

      GetIniFileName(iniName);
      iniFileName = iniName;
      iniFileValid = (_access(iniName, 0) != -1);
   }

   // Generic IniFile reading routine
   template<typename T>
   T GetIniValue(LPCTSTR Section, LPCTSTR Setting, T Default){
      if (!iniFileValid) 
         return Default;
      return ::GetIniValue<T>(Section, Setting, Default, iniFileName);
   }

   // Generic IniFile reading routine
   template<typename T>
   void SetIniValue(LPCTSTR Section, LPCTSTR Setting, T value){
      if (!iniFileValid) 
         return;
      ::SetIniValue<T>(Section, Setting, value, iniFileName);
   }

   void Reset(ReferenceMaker* owner)
   {
      //GetIniValue<TSTR>("Shader", "ShortDescription", v.s);
   }

   virtual void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid) 
   { 
      switch (id)
      {
      //case IMP_PB_SHORTDESC:        v.s = ShortDesc = GetIniValue<TSTR>("Shader", "ShortDescription", v.s); break;
      //case IMP_PB_FLIPUV:           v.i = GetIniValue<BOOL>("Shader", "FlipUVTextures", v.i); break;
      //case IMP_PB_AUTOSMOOTH_ANGLE: v.f = GetIniValue<float>("Shader", "AutoSmoothAngle", v.f); break;
      default: break;
      }
   }

   // set from v
   void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t) 
   { 
      switch (id)
      {
      //case IMP_PB_SHORTDESC:        SetIniValue<TCHAR*>("Shader", "ShortDescription", v.s); break;
      //case IMP_PB_FLIPUV:           SetIniValue<BOOL>("Shader", "FlipUVTextures", v.i); break;
      //case IMP_PB_AUTOSMOOTH_ANGLE: SetIniValue<float>("Shader", "AutoSmoothAngle", v.f); break;
      default: break;
      }
   }
};

static NifShaderPBAccessor NifShaderAccessor;

static ParamBlockDesc2 param_blk ( 
   shader_params, _T("shaderParameters"),  0, NULL, P_CLASS_PARAMS | P_AUTO_UI | P_MULTIMAP,
   //rollout
   1,
   shader_params,   IDD_NIFSHADER,  IDS_PARAMS, 0, 0, NULL, 
   // params
   ns_mat_ambient, _T("ambient"),  TYPE_RGBA, P_ANIMATABLE, IDS_MAT_AMBIENT,
      p_default, Color(0.0f, 0.0f, 0.0f), 
      end,

   ns_mat_diffuse, _T("diffuse"),  TYPE_RGBA, P_ANIMATABLE, IDS_MAT_DIFFUSE,
      p_default, Color(0.5f, 0.5f, 0.5f), 
      end,
      
   ns_mat_specular, _T("specular"),  TYPE_RGBA, P_ANIMATABLE, IDS_MAT_SPECULAR,
      p_default, Color(1.0f, 1.0f, 1.0f), 
      end,

   ns_mat_selfillumon, _T("useSelfIllumColor"),  TYPE_BOOL, 0, IDS_MAT_SELFILLUMON,
      p_default, FALSE, 
      end,

   ns_mat_selfillumclr, _T("selfillumclr"),  TYPE_RGBA, P_ANIMATABLE, IDS_MAT_SELFILLUMCLR,
      p_default, Color(0.0f, 0.0f, 0.0f), 
      end,

   ns_mat_selfillum, _T("selfillum"),  TYPE_FLOAT, P_ANIMATABLE, IDS_MAT_SELFILLUM,
      p_default, 0.0f, 
      end,

   ns_mat_glossiness, _T("glossiness"),  TYPE_FLOAT, P_ANIMATABLE, IDS_MAT_GLOSSINESS,
      p_default, 0.0f, 
      p_range,    0.0, 100.0,
      end,

   ns_mat_speclevel, _T("specularLevel"),  TYPE_FLOAT, P_ANIMATABLE, IDS_MAT_SPECLEVEL,
      p_default, 0.0f, 
      p_range,    0.0, 999.0,
      end,

   ns_mat_softenlevel, _T("softenlevel"),  TYPE_FLOAT, P_ANIMATABLE, IDS_MAT_SOFTENLEVEL,
      p_default, 0.0f, 
      end,

   ns_mat_shininess, _T("shininess"),  TYPE_FLOAT, P_ANIMATABLE, IDS_MAT_SHININESS,
      p_default, 10.0f, 
      end,

   ns_mat_alpha, _T("alpha"),  TYPE_FLOAT, P_ANIMATABLE, IDS_MAT_ALPHA,
      p_default, 1.0f, 
      end,

   ns_mat_dither, _T("dither"),  TYPE_INT, P_ANIMATABLE, IDS_MAT_DITHER,
      p_default, 0, 
      end,

   end
   );

// bug in pb desc? forces us to declare class desc after param block
static NifShaderClassDesc NifShaderDesc;
ClassDesc2* GetNifShaderDesc() { return &NifShaderDesc; }

NifShaderClassDesc::NifShaderClassDesc() {
   param_blk.SetClassDesc(this);
   //MakeAutoParamBlocks(this);
}

//--- NifShader -------------------------------------------------------
NifShader::NifShader()
{
   pb = NULL;
   pDlg = NULL;
}

NifShader::~NifShader() 
{

}

void NifShader::NotifyChanged()
{
   NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); 
}

RefTargetHandle NifShader::Clone( RemapDir &remap/*=NoRemap()*/ )
{
   NifShader* pShader = new NifShader();
   pShader->ReplaceReference(0, remap.CloneRef(pb));
   return pShader;
}

void NifShader::CopyStdParams( Shader* pFrom )
{
   // We don't want to see this parameter copying in macrorecorder
   macroRecorder->Disable(); 

   TimeValue t = 0;

   SetSelfIllum( pFrom->GetSelfIllum(0,0), t );
   SetSelfIllumClrOn( pFrom->IsSelfIllumClrOn(0,0) );
   SetSelfIllumClr( pFrom->GetSelfIllumClr(0,0), t );
   SetAmbientClr( pFrom->GetAmbientClr(0,0), t );
   SetDiffuseClr( pFrom->GetDiffuseClr(0,0), t );
   SetSpecularClr( pFrom->GetSpecularClr(0,0), t );
   SetGlossiness( pFrom->GetGlossiness(0,0), t );
   SetSpecularLevel( pFrom->GetSpecularLevel(0,0), t );
   SetSoftenLevel( pFrom->GetSoftenLevel(0,0), t );

   macroRecorder->Enable();
}

RefResult NifShader::NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message )
{
   switch (message) 
   {
   case REFMSG_CHANGE:
      if (hTarget == pb)
      {
         //if (pDlg)
         //   pDlg->UpdateDialog( pb->LastNotifyParamID() );
         NotifyChanged();
      }
      break;
   }
   return(REF_SUCCEED);
}

long NifShader::nTexChannelsSupported()
{
   return 15;
}

TSTR NifShader::GetTexChannelName( long nTextureChan )
{
   return GetString(texChannelNames[nTextureChan].channelName);
}

TSTR NifShader::GetTexChannelInternalName( long nTextureChan )
{
   return GetString(texChannelNames[nTextureChan].maxName);
}

long NifShader::ChannelType( long nTextureChan )
{
   return texChannelNames[nTextureChan].channelType;
}

long NifShader::StdIDToChannel( long stdID )
{
   return stdID2Channel[stdID];
}

void NifShader::Reset()
{
   NifShaderDesc.MakeAutoParamBlocks(this);   
}

void NifShader::GetIllumParams( ShadeContext &sc, IllumParams& ip )
{
   ip.stdParams = SupportStdParams();
   // ip.shFlags = selfIllumClrOn? SELFILLUM_CLR_ON : 0;
   ip.channels[ID_AM] = GetDiffuseClr(0);
   ip.channels[ID_DI] = GetDiffuseClr(0);
   ip.channels[ID_SP] = GetSpecularClr(0);
   ip.channels[StdIDToChannel(ID_SH)].r = GetGlossiness(0);
   ip.channels[StdIDToChannel(ID_SS)].r = GetSpecularLevel(0);
   if( IsSelfIllumClrOn() )
      ip.channels[ID_SI] = GetSelfIllumClr(0);
   else
      ip.channels[ID_SI].r = ip.channels[ID_SI].g = ip.channels[ID_SI].b = GetSelfIllum(0);

}



inline float PcToFrac(int pc) { return (float)pc/100.0f; }

inline int FracToPc(float f) {
   if (f<0.0) return (int)(100.0f*f - .5f);
   else return (int) (100.0f*f + .5f);
}

// Quadratic
inline float Soften(float r) {
   return r*(2.0f-r);
}

// all the old shaders add the highlights on
void CombineComponentsAdd( IllumParams& ip ) {
   ip.finalC = ip.finalAttenuation * (ip.ambIllumOut + ip.diffIllumOut  + ip.selfIllumOut)
      + ip.specIllumOut + ip.reflIllumOut + ip.transIllumOut; 
}

inline float Abs( float a ) { return (a < 0.0f) ? -a : a; }
inline float LBound( float x, float min = 0.0f ){ return x < min ? min : x; }
inline float UBound( float x, float max = 1.0f ){ return x > max ? max : x; }

void NifShader::Illum( ShadeContext &sc, IllumParams &ip )
{
   LightDesc* l = NULL;
   Color lightCol;

   // Blinn style phong
   BOOL is_shiny= (ip.channels[StdIDToChannel(ID_SS)].r > 0.0f) ? 1:0; 
   double phExp = pow(2.0, ip.channels[StdIDToChannel(ID_SH)].r * 10.0) * 4.0; // expensive.!!  TBD

   for (int i=0; i<sc.nLights; i++) {
      l = sc.Light(i);
      register float NL, diffCoef;
      Point3 L;
      if (l->Illuminate(sc,sc.Normal(),lightCol,L,NL,diffCoef)) {
         if (l->ambientOnly) {
            ip.ambIllumOut += lightCol;
            continue;
         }
         if (NL<=0.0f) 
            continue;

         // diffuse
         if (l->affectDiffuse)
            ip.diffIllumOut += diffCoef * lightCol;

         // specular (Phong2) 
         if (is_shiny&&l->affectSpecular) {
            Point3 H = Normalize( L-sc.V() );
            float c = DotProd(sc.Normal(),H);    
            if (c>0.0f) {
               c = (float)pow((double)c, phExp); // could use table lookup for speed
               ip.specIllumOut += c * ip.channels[StdIDToChannel(ID_SS)].r * lightCol;
            }
         }
      }
   }


   // Apply mono self illumination
   if ( ! IsSelfIllumClrOn() ){
      // lerp between diffuse & white
      // changed back, fixed in getIllumParams, KE 4/27
      float si = 0.3333333f * (ip.channels[ID_SI].r + ip.channels[ID_SI].g + ip.channels[ID_SI].b);
      if ( si > 0.0f ) {
         si = UBound( si );
         ip.selfIllumOut = si * ip.channels[ID_DI];
         ip.diffIllumOut *= (1.0f-si);
         // fade the ambient down on si: 5/27/99 ke
         ip.ambIllumOut *= 1.0f-si;
      }
   } else {
      // colored self illum, 
      ip.selfIllumOut += ip.channels[ID_SI];
   }
   // now we can multiply by the clrs, 
   ip.ambIllumOut *= ip.channels[ID_AM]; 
   ip.diffIllumIntens = Intens(ip.diffIllumOut);
   ip.diffIllumOut *= ip.channels[ID_DI]; 
   ip.specIllumOut *= ip.channels[ID_SP]; 

   ShadeTransmission(sc, ip, ip.channels[ID_RR], ip.refractAmt);
   ShadeReflection( sc, ip, ip.channels[ID_RL] ); 
   CombineComponents( sc, ip ); 
}


void NifShader::Update( TimeValue t, Interval& valid )
{

}

class NifShaderDlg : public ShaderParamDlg {
public:
   NifShaderDlg(bhkRigidBodyModifier* m);
   void DeleteThis() {delete this;}		
};


ShaderParamDlg* NifShader::CreateParamDialog( HWND hOldRollup, HWND hwMtlEdit, IMtlParams *imp, StdMat2* theMtl, int rollupOpen, int )
{
   Interval v;
   Update(imp->GetTime(),v);

   NifShaderDlg *pDlg = new NifShaderDlg(hwMtlEdit, imp);
   pDlg->SetThings( theMtl, this  );
   
   TCHAR* dlgTemplate = MAKEINTRESOURCE(IDD_NIFSHADER);

   // If it already exists, replace it
   if ( !hOldRollup ) 
   {
      pDlg->m_hRollup = imp->AddRollupPage( 
         hInstance,
         dlgTemplate,
         GamebryoShaderDlgProc, 
         GetString(IDS_NETIMMERSE_DESC), 
         (LPARAM)pDlg , 
         rollupOpen);
   }
   else
   {
      pDlg->m_hRollup = imp->ReplaceRollupPage( 
         hOldRollup,
         hInstance,
         dlgTemplate,
         GamebryoShaderDlgProc, 
         GetString(IDS_NETIMMERSE_DESC),
         (LPARAM)pDlg , 
         rollupOpen);
   } 
   return (ShaderParamDlg*)pDlg;   
}


//--- NifShader -------------------------------------------------------
class NifShaderDlgProc : public ParamMap2UserDlgProc {
public:
   NifShaderDlgProc() {}
   INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
   void DeleteThis() { }

   //--- ParamDlgProc --------------------------------
   void TurnSpinner(HWND hWnd,int SpinNum,BOOL ison)
   {   
      ISpinnerControl *spin2 = GetISpinner(GetDlgItem(hWnd,SpinNum));
      if (ison) spin2->Enable();else spin2->Disable();
      ReleaseISpinner(spin2);
   };
   void Update(TimeValue t);

   HWND hDlg;
};

extern ParamMap2UserDlgProc* GetNifShaderDlgProc()
{
   static NifShaderDlgProc theNifShaderDlgProc;
   return &theNifShaderDlgProc;
}

INT_PTR NifShaderDlgProc::DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   switch(msg) 
   {
   case WM_INITDIALOG:
      {
         hDlg = hWnd;
         // Append file version to dialog
         TSTR version = GetFileVersion(NULL);
         if (!version.isNull()) {
            char buffer[256];
            GetWindowText(hWnd, buffer, _countof(buffer));
            _tcscat(buffer, TEXT(" "));
            _tcscat(buffer, version);
            SetWindowText(hWnd, buffer);
         }
         CenterWindow(hWnd,GetParent(hWnd));

       //  CheckDlgButton(hWnd, IDC_CHK_FLIP_UV, map->GetParamBlock()->GetPB2Value(IMP_PB_FLIPUV).i);
       //  CheckDlgButton(hWnd, IDC_CHK_AUTOSMOOTH, map->GetParamBlock()->GetPB2Value(IMP_PB_AUTOSMOOTH).i);
       //  CheckDlgButton(hWnd, IDC_CHK_SHOW_TEX, map->GetParamBlock()->GetPB2Value(IMP_PB_RENDERTEX).i);
       //  CheckDlgButton(hWnd, IDC_CHK_COLLISON, map->GetParamBlock()->GetPB2Value(IMP_PB_ENABLE_COLLISION).i);
       //  CheckDlgButton(hWnd, IDC_CHK_WALKMESH, map->GetParamBlock()->GetPB2Value(IMP_PB_ENABLE_WALKMESH).i);
       //  CheckDlgButton(hWnd, IDC_CHK_HOOK, map->GetParamBlock()->GetPB2Value(IMP_PB_ENABLE_HOOKS).i);
       //  CheckDlgButton(hWnd, IDC_CHK_HAIR_HELM, map->GetParamBlock()->GetPB2Value(IMP_PB_ENABLE_HAIRHELM).i);
       //  CheckDlgButton(hWnd, IDC_CHK_BONES, map->GetParamBlock()->GetPB2Value(IMP_PB_ENABLE_BONES).i);
       //CheckDlgButton(hWnd, IDC_CHK_NORMALBUMP, map->GetParamBlock()->GetPB2Value(IMP_PB_NORMALBUMP).i);
       ////CheckDlgButton(hWnd, IDC_CHK_ANIMATION, map->GetParamBlock()->GetPB2Value(IMP_PB_ENABLE_ANIMATION).i);

       //  CheckDlgButton(hWnd, IDC_CHK_LIMIT_WALKMESH, map->GetParamBlock()->GetPB2Value(IMP_PB_TRIM_WALKMESH).i);

       //  EnableWindow(GetDlgItem(hWnd, IDC_ED_WALK_LIMIT), TRUE);
       //  walkLimit = GetISpinner( GetDlgItem(hWnd, IDC_SP_WALK_LIMIT) );
       //  walkLimit->SetLimits(-1000000.0f, 0.0f, TRUE);
       //  walkLimit->SetAutoScale(TRUE);
       //  walkLimit->SetResetValue( -5.0f );
       //  walkLimit->LinkToEdit( GetDlgItem(hWnd, IDC_ED_WALK_LIMIT), EDITTYPE_POS_FLOAT );
       //  walkLimit->SetValue(map->GetParamBlock()->GetPB2Value(IMP_PB_WALKMESH_LIMIT).f, 0);

       //CheckDlgButton(hWnd, IDC_CHK_SCALE_FACTOR, map->GetParamBlock()->GetPB2Value(IMP_PB_SCALE_FACTOR).i);

       //EnableWindow(GetDlgItem(hWnd, IDC_ED_SCALE_FACTOR), TRUE);
       //scaleFactor = GetISpinner( GetDlgItem(hWnd, IDC_SP_SCALE_FACTOR) );
       //scaleFactor->SetLimits(0.0f, 100000.0f, TRUE);
       //scaleFactor->SetAutoScale(TRUE);
       //scaleFactor->SetResetValue( 1.0f );
       //scaleFactor->LinkToEdit( GetDlgItem(hWnd, IDC_ED_SCALE_FACTOR), EDITTYPE_POS_FLOAT );
       //scaleFactor->SetValue(map->GetParamBlock()->GetPB2Value(IMP_PB_SCALE_FACTOR).f, 0);

       //CheckDlgButton(hWnd, IDC_CHK_WELD, map->GetParamBlock()->GetPB2Value(IMP_PB_WELD_LIMIT).i);

       //EnableWindow(GetDlgItem(hWnd, IDC_ED_WELD_LIMIT), TRUE);
       //weldLimit = GetISpinner( GetDlgItem(hWnd, IDC_SP_WELD_LIMIT) );
       //weldLimit->SetAutoScale(FALSE);
       //weldLimit->SetLimits(0.00001f, 1.0f, FALSE);
       //weldLimit->SetScale(0.001f);
       //weldLimit->SetResetValue( 0.001f );
       //weldLimit->LinkToEdit( GetDlgItem(hWnd, IDC_ED_WELD_LIMIT), EDITTYPE_POS_FLOAT );
       //weldLimit->SetValue(map->GetParamBlock()->GetPB2Value(IMP_PB_WELD_LIMIT).f, 0);
       //

         Update(t);
      }
      return TRUE;

   case WM_NCDESTROY:
      //if (walkLimit) 
      //{
      //   ReleaseISpinner(walkLimit);
      //   walkLimit = NULL;
      //}
      //if (scaleFactor)
      //{
      //   ReleaseISpinner(scaleFactor);
      //   scaleFactor = NULL;
      //}
      //if (weldLimit)
      //{
      //   ReleaseISpinner(weldLimit);
      //   weldLimit = NULL;
      //}
      break;

   case WM_COMMAND : 
      {
         if (HIWORD(wParam) == BN_CLICKED)
         {
            switch (LOWORD(wParam))
            {
            case IDOK:
               //map->GetParamBlock()->SetValue(IMP_PB_FLIPUV, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_FLIP_UV));
               //map->GetParamBlock()->SetValue(IMP_PB_AUTOSMOOTH, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_AUTOSMOOTH));
               //map->GetParamBlock()->SetValue(IMP_PB_RENDERTEX, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_SHOW_TEX));
               //map->GetParamBlock()->SetValue(IMP_PB_ENABLE_COLLISION, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_COLLISON));
               //map->GetParamBlock()->SetValue(IMP_PB_ENABLE_WALKMESH, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_WALKMESH));
               //map->GetParamBlock()->SetValue(IMP_PB_ENABLE_HOOKS, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_HOOK));
               //map->GetParamBlock()->SetValue(IMP_PB_ENABLE_HAIRHELM, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_HAIR_HELM));
               //map->GetParamBlock()->SetValue(IMP_PB_ENABLE_BONES, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_BONES));
               //map->GetParamBlock()->SetValue(IMP_PB_TRIM_WALKMESH, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_LIMIT_WALKMESH));
               ////map->GetParamBlock()->SetValue(IMP_PB_ENABLE_ANIMATION, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_ANIMATION));
               //map->GetParamBlock()->SetValue(IMP_PB_ENABLE_SCALE, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_SCALE_FACTOR));
               //map->GetParamBlock()->SetValue(IMP_PB_NORMALBUMP, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_NORMALBUMP));
               //map->GetParamBlock()->SetValue(IMP_PB_ENABLE_WELD, t, (BOOL)IsDlgButtonChecked(hWnd, IDC_CHK_WELD));

               //if (walkLimit) map->GetParamBlock()->SetValue(IMP_PB_WALKMESH_LIMIT, t, walkLimit->GetFVal());
               //if (scaleFactor) map->GetParamBlock()->SetValue(IMP_PB_SCALE_FACTOR, t, scaleFactor->GetFVal());
               //if (weldLimit) map->GetParamBlock()->SetValue(IMP_PB_WELD_LIMIT, t, weldLimit->GetFVal());
               break;
            }

            Update(t);
         }
      }
      break;
   }
   return FALSE;
}

void NifShaderDlgProc::Update(TimeValue t)
{
   //BOOL enabled;

   //// Handle Walk mesh
   //enabled = IsDlgButtonChecked(hDlg, IDC_CHK_WALKMESH);
   //EnableWindow(GetDlgItem(hDlg, IDC_CHK_LIMIT_WALKMESH), enabled);
   //if (enabled) enabled = IsDlgButtonChecked(hDlg, IDC_CHK_LIMIT_WALKMESH);
   //EnableWindow(GetDlgItem(hDlg, IDC_ED_WALK_LIMIT), enabled);
   //EnableWindow(GetDlgItem(hDlg, IDC_SP_WALK_LIMIT), enabled);

   //enabled = IsDlgButtonChecked(hDlg, IDC_CHK_SCALE_FACTOR);
   //EnableWindow(GetDlgItem(hDlg, IDC_ED_SCALE_FACTOR), enabled);
   //EnableWindow(GetDlgItem(hDlg, IDC_SP_SCALE_FACTOR), enabled);

   //enabled = IsDlgButtonChecked(hDlg, IDC_CHK_BONES);
   //EnableWindow(GetDlgItem(hDlg, IDC_STC_SKELETON), enabled);

   //enabled = IsDlgButtonChecked(hDlg, IDC_CHK_WELD);
   //EnableWindow(GetDlgItem(hDlg, IDC_ED_WELD_LIMIT), enabled);
   //EnableWindow(GetDlgItem(hDlg, IDC_SP_WELD_LIMIT), enabled);

   //EnableWindow(GetDlgItem(hDlg, IDC_CHK_ANIMATION), enabled);
   
}

#endif