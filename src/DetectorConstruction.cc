#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"


//SETTING THE WORLD
G4VPhysicalVolume* DetectorConstruction::Construct()
{
    auto material =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");

    auto solid = new G4Box("World", 50*cm, 50*cm, 50*cm);

    auto logical =
        new G4LogicalVolume(solid, material, "World");

    auto physical = new G4PVPlacement(
        nullptr,
        G4ThreeVector(),
        logical,
        "World",
        nullptr,
        false,
        0,
        true
    );

    return physical;
}