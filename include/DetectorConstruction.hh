//BUILDS THE WORLD

#ifndef DETECTOR_CONSTRUCTION_HH
#define DETECTOR_CONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"

class G4GenericMessenger;  //For mac-controls

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();              //For mac-controls
    ~DetectorConstruction() override;    //For mac-controls

    void SetP1(G4double value);          //For mac-controls
    void SetP2(G4double value);          //For mac-controls
    G4VPhysicalVolume* Construct() override;
private:                                 //FOr mac-controls, so that it can only be accessed from inside...
    G4GenericMessenger* fMessenger;      //For mac-controls
};

#endif