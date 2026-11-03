#ifndef EVENT_ACTION_HH
#define EVENT_ACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"

#include <array>
#include <vector>
#include <fstream>

class G4Event;
class G4GenericMessenger;    //For mac-controls

class EventAction : public G4UserEventAction
{
public:
    EventAction();
    ~EventAction() override; //For mac-controls

    void BeginScan();        //For mac-controls
    void SaveAndReset();     //For mac-controls
    void FinishScan();       //For mac-controls

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    //DATA COLLECTION: =======================
    void RecordGenerated() { ++fGenerated; }
    void RecordAbsorbed() { ++fAbsorbed; }
    //========================================

    void RecordPhoton(G4int sensorID, G4double energy);

private:
    void OpenOutput();                         //For mac-controls
    void RunAnalysis(const char* mode);        //For mac-controls

    G4GenericMessenger* fMessenger = nullptr;  //For mac-controls
    bool fScanActive = false;                  //For mac-controls

    std::array<std::vector<G4double>, 100> fPhotonEnergies;
    std::ofstream fOutput;

    //DATA COLLECTION: =======================
    std::ofstream fSummary;
    G4long fGenerated = 0;
    G4long fAbsorbed = 0;
    //========================================
};

#endif