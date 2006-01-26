/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

//-----------------------------------------------------------------
//   Implementation of the alignment object class through the abstract
//  class AliAlignObj. From it two derived concrete representation of
//  alignment object class (AliAlignObjAngles, AliAlignObjMatrix) are
//  derived in separate files.
//-----------------------------------------------------------------
/*****************************************************************************
 * AliAlignObjAngles: derived alignment class storing alignment information  *
 *   for a single volume in form of three doubles for the translation        *
 *   and three doubles for the rotation expressed with the euler angles      *
 *   in the xyz-convention (http://mathworld.wolfram.com/EulerAngles.html),  *
 *   also known as roll, pitch, yaw. PLEASE NOTE THE ANGLES SIGNS ARE        *
 *   INVERSE WITH RESPECT TO THIS REFERENCE!!! In this way the representation*
 *   is fully consistent with the TGeo Rotation methods.                     *
 *****************************************************************************/

#include "AliAlignObj.h"
#include "AliTrackPointArray.h"
#include "AliLog.h"
 
ClassImp(AliAlignObj)

Int_t AliAlignObj::fgLayerSize[kLastLayer - kFirstLayer] = {
  80, 160,  // ITS SPD
  84, 176,  // ITS SDD
  748, 950, // ITS SSD
  36, 36,   // TPC
  90, 90, 90, 90, 90, 90,  // TRD
  1,        // TOF ??
  1, 1,     // PHOS ??
  7,        // RICH ??
  1         // MUON ??
};

const char* AliAlignObj::fgLayerName[kLastLayer - kFirstLayer] = {
  "ITS inner pixels layer", "ITS outer pixels layer",
  "ITS inner drifts layer", "ITS outer drifts layer",
  "ITS inner strips layer", "ITS outer strips layer",
  "TPC inner chambers layer", "TPC outer chambers layer",
  "TRD chambers layer 1", "TRD chambers layer 2", "TRD chambers layer 3",
  "TRD chambers layer 4", "TRD chambers layer 5", "TRD chambers layer 6",
  "TOF layer",
  "?","?",
  "RICH layer",
  "?"
};

const char** AliAlignObj::fgVolPath[kLastLayer - kFirstLayer] = {
  0x0,0x0,
  0x0,0x0,
  0x0,0x0,
  0x0,0x0,
  0x0,0x0,0x0,
  0x0,0x0,0x0,
  0x0,
  0x0,0x0,
  0x0,
  0x0
};

//_____________________________________________________________________________
AliAlignObj::AliAlignObj():
  fVolUID(0)
{
  // default constructor
  InitVolPaths();
}

//_____________________________________________________________________________
AliAlignObj::AliAlignObj(const AliAlignObj& theAlignObj) :
  TObject(theAlignObj)
{
  //copy constructor
  fVolPath = theAlignObj.GetVolPath();
  fVolUID = theAlignObj.GetVolUID();
}

//_____________________________________________________________________________
AliAlignObj &AliAlignObj::operator =(const AliAlignObj& theAlignObj)
{
  // assignment operator
  if(this==&theAlignObj) return *this;
  fVolPath = theAlignObj.GetVolPath();
  fVolUID = theAlignObj.GetVolUID();
  return *this;
}

//_____________________________________________________________________________
AliAlignObj::~AliAlignObj()
{
  // dummy destructor
}

//_____________________________________________________________________________
void AliAlignObj::SetVolUID(ELayerID detId, Int_t modId)
{
  // From detector name and module number (according to detector numbering)
  // build fVolUID, unique numerical identity of that volume inside ALICE
  // fVolUID is 16 bits, first 5 reserved for detID (32 possible values),
  // remaining 11 for module ID inside det (2048 possible values).
  //
  fVolUID = LayerToVolUID(detId,modId);
}

//_____________________________________________________________________________
void AliAlignObj::GetVolUID(ELayerID &layerId, Int_t &modId) const
{
  // From detector name and module number (according to detector numbering)
  // build fVolUID, unique numerical identity of that volume inside ALICE
  // fVolUID is 16 bits, first 5 reserved for detID (32 possible values),
  // remaining 11 for module ID inside det (2048 possible values).
  //
  layerId = VolUIDToLayer(fVolUID,modId);
}

//_____________________________________________________________________________
void AliAlignObj::AnglesToMatrix(const Double_t *angles, Double_t *rot) const
{
  // Calculates the rotation matrix using the 
  // Euler angles in "x y z" notation
  Double_t degrad = TMath::DegToRad();
  Double_t sinpsi = TMath::Sin(degrad*angles[0]);
  Double_t cospsi = TMath::Cos(degrad*angles[0]);
  Double_t sinthe = TMath::Sin(degrad*angles[1]);
  Double_t costhe = TMath::Cos(degrad*angles[1]);
  Double_t sinphi = TMath::Sin(degrad*angles[2]);
  Double_t cosphi = TMath::Cos(degrad*angles[2]);

  rot[0] =  costhe*cosphi;
  rot[1] = -costhe*sinphi;
  rot[2] =  sinthe;
  rot[3] =  sinpsi*sinthe*cosphi + cospsi*sinphi;
  rot[4] = -sinpsi*sinthe*sinphi + cospsi*cosphi;
  rot[5] = -costhe*sinpsi;
  rot[6] = -cospsi*sinthe*cosphi + sinpsi*sinphi;
  rot[7] =  cospsi*sinthe*sinphi + sinpsi*cosphi;
  rot[8] =  costhe*cospsi;
}

//_____________________________________________________________________________
Bool_t AliAlignObj::MatrixToAngles(const Double_t *rot, Double_t *angles) const
{
  // Calculates the Euler angles in "x y z" notation
  // using the rotation matrix
  if(rot[0]<1e-7 || rot[8]<1e-7) return kFALSE;
  Double_t raddeg = TMath::RadToDeg();
  angles[0]=raddeg*TMath::ATan2(-rot[5],rot[8]);
  angles[1]=raddeg*TMath::ASin(rot[2]);
  angles[2]=raddeg*TMath::ATan2(-rot[1],rot[0]);
  return kTRUE;
}

//______________________________________________________________________________
void AliAlignObj::Transform(AliTrackPoint &p) const
{
  // The method transforms the space-point coordinates using the
  // transformation matrix provided by the AliAlignObj
  // The covariance matrix is not affected since we assume
  // that the transformations are sufficiently small

  if (fVolUID != p.GetVolumeID())
    AliWarning(Form("Alignment object ID is not equal to the space-point ID (%d != %d)",fVolUID,p.GetVolumeID())); 

  TGeoHMatrix m;
  GetMatrix(m);
  Double_t *rot = m.GetRotationMatrix();
  Double_t *tr  = m.GetTranslation();

  Float_t xyzin[3],xyzout[3];
  p.GetXYZ(xyzin);
  for (Int_t i = 0; i < 3; i++)
    xyzout[i] = tr[i]+
                xyzin[0]*rot[3*i]+
                xyzin[1]*rot[3*i+1]+
                xyzin[2]*rot[3*i+2];
  p.SetXYZ(xyzout);
  
}

//______________________________________________________________________________
void AliAlignObj::Transform(AliTrackPointArray &array) const
{
  AliTrackPoint p;
  for (Int_t i = 0; i < array.GetNPoints(); i++) {
    array.GetPoint(p,i);
    Transform(p);
    array.AddPoint(i,&p);
  }
}

//_____________________________________________________________________________
void AliAlignObj::Print(Option_t *) const
{
  // Print the contents of the
  // alignment object in angles and
  // matrix representations
  Double_t tr[3];
  GetTranslation(tr);
  Double_t angles[3];
  GetAngles(angles);
  TGeoHMatrix m;
  GetMatrix(m);
  const Double_t *rot = m.GetRotationMatrix();
//   printf("Volume=%s ID=%u\n", GetVolPath(),GetVolUID());
  ELayerID layerId;
  Int_t modId;
  GetVolUID(layerId,modId);
  printf("Volume=%s LayerID=%d ModuleID=%d\n", GetVolPath(),layerId,modId);
  printf("%12.6f%12.6f%12.6f    Tx = %12.6f    Psi   = %12.6f\n", rot[0], rot[1], rot[2], tr[0], angles[0]);
  printf("%12.6f%12.6f%12.6f    Ty = %12.6f    Theta = %12.6f\n", rot[3], rot[4], rot[5], tr[1], angles[1]);
  printf("%12.6f%12.6f%12.6f    Tz = %12.6f    Phi   = %12.6f\n", rot[6], rot[7], rot[8], tr[2], angles[2]);

}

//_____________________________________________________________________________
UShort_t AliAlignObj::LayerToVolUID(ELayerID layerId, Int_t modId)
{
  // From detector (layer) name and module number (according to detector numbering)
  // build fVolUID, unique numerical identity of that volume inside ALICE
  // fVolUID is 16 bits, first 5 reserved for layerID (32 possible values),
  // remaining 11 for module ID inside det (2048 possible values).
  //
  return ((UShort_t(layerId) << 11) | UShort_t(modId));
}

//_____________________________________________________________________________
AliAlignObj::ELayerID AliAlignObj::VolUIDToLayer(UShort_t voluid, Int_t &modId)
{
  // From detector (layer) name and module number (according to detector numbering)
  // build fVolUID, unique numerical identity of that volume inside ALICE
  // fVolUID is 16 bits, first 5 reserved for layerID (32 possible values),
  // remaining 11 for module ID inside det (2048 possible values).
  //
  modId = voluid & 0x7ff;

  return VolUIDToLayer(voluid);
}

//_____________________________________________________________________________
AliAlignObj::ELayerID AliAlignObj::VolUIDToLayer(UShort_t voluid)
{
  // From detector (layer) name and module number (according to detector numbering)
  // build fVolUID, unique numerical identity of that volume inside ALICE
  // fVolUID is 16 bits, first 5 reserved for layerID (32 possible values),
  // remaining 11 for module ID inside det (2048 possible values).
  //
  return ELayerID((voluid >> 11) & 0x1f);
}

//_____________________________________________________________________________
void AliAlignObj::InitVolPaths()
{
  // Initialize the LUTs which contain
  // the TGeo volume paths for each
  // alignable volume. The LUTs are
  // static, so they are created during
  // the creation of the first intance
  // of AliAlignObj

  if (fgVolPath[0]) return;

  for (Int_t iLayer = 0; iLayer < (kLastLayer - kFirstLayer); iLayer++)
    fgVolPath[iLayer] = new const char *[fgLayerSize[iLayer]];

  /*********************       SPD layer1  ***********************/
  {
    Int_t modnum = 0;
    TString str0 = "ALIC_1/ITSV_1/ITSD_1/IT12_1/I12B_"; //".../I12A_"
    TString str1 = "/I10B_";    //"/I10A_";
    TString str2 = "/I107_";    //"/I103_"
    TString str3 = "/I101_1/ITS1_1";
    TString volpath, volpath1, volpath2;

    for(Int_t c1 = 1; c1<=10; c1++){
      volpath = str0;
      volpath += c1;
      volpath += str1;
      for(Int_t c2 =1; c2<=2; c2++){
	volpath1 = volpath;
	volpath1 += c2;
	volpath1 += str2;
	for(Int_t c3 =1; c3<=4; c3++){
	  volpath2 = volpath1;
	  volpath2 += c3;
	  volpath2 += str3;
	  fgVolPath[kSPD1-kFirstLayer][modnum] = volpath2.Data();
	  modnum++;
	}
      }
    }
  }
  
  /*********************       SPD layer2  ***********************/
  {
    Int_t modnum = 0;
    TString str0 = "ALIC_1/ITSV_1/ITSD_1/IT12_1/I12B_";  //".../I12A_"
    TString str1 = "/I20B_";  //"/I20A"
    TString str2 = "/I1D7_";  //"/I1D3"
    TString str3 = "/I1D1_1/ITS2_1";
    TString volpath, volpath1, volpath2;

    for(Int_t c1 = 1; c1<=10; c1++){
      volpath = str0;
      volpath += c1;
      volpath += str1;
      for(Int_t c2 =1; c2<=4; c2++){
	volpath1 = volpath;
	volpath1 += c2;
	volpath1 += str2;
	for(Int_t c3 =1; c3<=4; c3++){
	  volpath2 = volpath1;
	  volpath2 += c3;
	  volpath2 += str3;
	  fgVolPath[kSPD2-kFirstLayer][modnum] = volpath2.Data();
	  modnum++;
	}
      }
    }
  }

  /*********************       SDD layer1  ***********************/
  {
    Int_t modnum=0;
    TString str0 = "ALIC_1/ITSV_1/ITSD_1/IT34_1/I004_";
    TString str1 = "/I302_";
    TString str2 = "/ITS3_1";
    TString volpath, volpath1;

    for(Int_t c1 = 1; c1<=14; c1++){
      volpath = str0;
      volpath += c1;
      volpath += str1;
      for(Int_t c2 =1; c2<=6; c2++){
	volpath1 = volpath;
	volpath1 += c2;
	volpath1 += str2;
	fgVolPath[kSDD1-kFirstLayer][modnum] = volpath1.Data();
	modnum++;
      }
    }
  }

  /*********************       SDD layer2  ***********************/
  {
    Int_t modnum=0;
    TString str0 = "ALIC_1/ITSV_1/ITSD_1/IT34_1/I005_";
    TString str1 = "/I402_";
    TString str2 = "/ITS4_1";
    TString volpath, volpath1;

    for(Int_t c1 = 1; c1<=22; c1++){
      volpath = str0;
      volpath += c1;
      volpath += str1;
      for(Int_t c2 = 1; c2<=8; c2++){
	volpath1 = volpath;
	volpath1 += c2;
	volpath1 += str2;
	fgVolPath[kSDD2-kFirstLayer][modnum] = volpath1.Data();
	modnum++;
      }
    }
  }

  /*********************       SSD layer1  ***********************/
  {
    Int_t modnum=0;
    TString str0 = "ALIC_1/ITSV_1/ITSD_1/IT56_1/I565_";
    TString str1 = "/I562_";
    TString str2 = "/ITS5_1";
    TString volpath, volpath1;

    for(Int_t c1 = 1; c1<=34; c1++){
      volpath = str0;
      volpath += c1;
      volpath += str1;
      for(Int_t c2 = 1; c2<=22; c2++){
	volpath1 = volpath;
	volpath1 += c2;
	volpath1 += str2;
	fgVolPath[kSSD1-kFirstLayer][modnum] = volpath1.Data();
	modnum++;
      }
    }
  }

  /*********************       SSD layer1  ***********************/
  {
    Int_t modnum=0;
    TString str0 = "ALIC_1/ITSV_1/ITSD_1/IT56_1/I569_";
    TString str1 = "/I566_";
    TString str2 = "/ITS6_1";
    TString volpath, volpath1;

    for(Int_t c1 = 1; c1<=38; c1++){
      volpath = str0;
      volpath += c1;
      volpath += str1;
      for(Int_t c2 = 1; c2<=25; c2++){
	volpath1 = volpath;
	volpath1 += c2;
	volpath1 += str2;
	fgVolPath[kSSD2-kFirstLayer][modnum] = volpath1.Data();
	modnum++;
      }
    }
  }

 
}
