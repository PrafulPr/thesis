#ifndef COLLIMATOR_HH
#define COLLIMATOR_HH

class G4LogicalVolume;
class G4Box;

void BuildCollimator(
    G4LogicalVolume* worldLogical,
    G4Box* crystalSolid
);

#endif