#ifndef SIPM_ARRAY_HH
#define SIPM_ARRAY_HH

#include "globals.hh"

class G4LogicalVolume;
class G4Box;

// Build the coupling layer and all 16 sensors.
void BuildSiPMArray(
    G4LogicalVolume* worldLogical,
    G4Box* crystalSolid,
    G4double* photonEnergy
);

#endif