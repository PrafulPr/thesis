#ifndef CRYSTAL_WRAPPING_HH
#define CRYSTAL_WRAPPING_HH

class G4LogicalVolume;
class G4Box;
class G4VPhysicalVolume; //Added after to now define its reflectibility

void BuildCrystalWrapping(
    G4LogicalVolume* worldLogical,
    G4Box* crystalSolid,
    G4VPhysicalVolume* crystalPhysical   //Added after to now define... yknow
);

#endif