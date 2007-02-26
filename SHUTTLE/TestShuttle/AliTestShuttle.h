#ifndef ALI_TEST_SHUTTLE_H
#define ALI_TEST_SHUTTLE_H

/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

//
// test implementation of the AliShuttleInterface, to be used for local tests of preprocessors
//

#include <AliShuttleInterface.h>
#include <TString.h>

class TMap;
class TList;
class AliCDBMetaData;
class AliCDBPath;
class AliCDBPath;

class AliTestShuttle : public AliShuttleInterface
{
  public:
    AliTestShuttle(Int_t run, UInt_t startTime, UInt_t endTime);
    virtual ~AliTestShuttle();

    void AddInputFile(Int_t system, const char* detector, const char* id, const char* source, const char* fileName);
    void SetDCSInput(TMap* dcsAliasMap) { fDcsAliasMap = dcsAliasMap; }
    void AddInputRunParameter(const char* key, const char* value);
    Bool_t AddInputCDBEntry(AliCDBEntry* entry);

    void Process();

    // AliShuttleInterface functions
    virtual UInt_t Store(const AliCDBPath& path, TObject* object, AliCDBMetaData* metaData,
        				Int_t validityStart = 0, Bool_t validityInfinite = kFALSE);
    virtual UInt_t StoreReferenceData(const AliCDBPath& path, TObject* object, AliCDBMetaData* metaData);
    virtual const char* GetFile(Int_t system, const char* detector, const char* id, const char* source);
    virtual TList* GetFileSources(Int_t system, const char* detector, const char* id);
    virtual const char* GetRunParameter(const char* key);
    virtual AliCDBEntry* GetFromOCDB(const AliCDBPath& path);
    virtual void Log(const char* detector, const char* message);

    virtual void RegisterPreprocessor(AliPreprocessor* preprocessor);

    static void SetMainCDB (TString mainCDB) {fgkMainCDB = mainCDB;}
    static void SetLocalCDB (TString localCDB) {fgkLocalCDB = localCDB;}

    static void SetMainRefStorage (TString mainRefStorage) {fgkMainRefStorage = mainRefStorage;}
    static void SetLocalRefStorage (TString localRefStorage) {fgkLocalRefStorage = localRefStorage;}

    static void SetShuttleTempDir (const char* tmpDir);
    static void SetShuttleLogDir (const char* logDir);

  protected:

    Int_t fRun;         // run that is simulated with the AliTestShuttle
    UInt_t fStartTime;  // starttime that is simulated with the AliTestShuttle
    UInt_t fEndTime;    // endtime that is simulated with the AliTestShuttle

    TMap* fInputFiles;      // files for GetFile, GetFileSources
    TMap* fRunParameters;    // run parameters
    TObjArray* fPreprocessors; // list of preprocessors that are to be tested
    TMap* fDcsAliasMap; // DCS data for testing

  private:
    ClassDef(AliTestShuttle, 0);
};

#endif
