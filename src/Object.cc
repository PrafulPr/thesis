#include "Object.hh"

#include "G4Box.hh"
#include "G4UnionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

void BuildObject(G4LogicalVolume* worldLogical)
{
    // Rectangle: 8 × 16 cm, with 5 cm thickness.
    const G4double width = 8*cm;
    const G4double height = 16*cm;
    const G4double thickness = 12*cm;

    const G4double centreZ = -35*cm;

    auto material =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");

    auto solid = new G4Box(
        "Object",
        width/2,
        height/2,
        thickness/2
    );

    auto logical = new G4LogicalVolume(
        solid,
        material,
        "Object"
    );

    new G4PVPlacement(
        nullptr,
        G4ThreeVector(-4*cm, 0, centreZ),
        logical,
        "Object",
        worldLogical,
        false,
        0,
        true
    );

    auto appearance =
        new G4VisAttributes(G4Colour(0.2, 0.8, 0.3));

    appearance->SetForceSolid(true);
    logical->SetVisAttributes(appearance);
}
