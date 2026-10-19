#include "CrystalWrapping.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
// FOR REFLECTIBILITY OF THE WRAPING:
#include "G4OpticalSurface.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4MaterialPropertiesTable.hh"

void BuildCrystalWrapping(
    G4LogicalVolume* worldLogical,
    G4Box* crystalSolid,
    G4VPhysicalVolume* crystalPhysical  //for REFLECTIBILITY...
)
{
    auto foilMaterial =
        G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");

    // Provisional teaching thickness.
    const G4double thickness = 0.02*mm;

    const G4double hx = crystalSolid->GetXHalfLength();
    const G4double hy = crystalSolid->GetYHalfLength();
    const G4double hz = crystalSolid->GetZHalfLength();

    auto appearance =
        new G4VisAttributes(G4Colour(0.7, 0.7, 0.7));
    appearance->SetForceWireframe(true);

//==============================REFLECTION==============================|
    // Simplified mirror-like foil: assumed 90% reflection.           //|
    G4double energy[] = {2.0*eV, 2.25*eV, 2.5*eV};                    //|
    G4double reflectivity[] = {1.00, 1.00, 1.00};                     //|
    G4double efficiency[] = {0.0, 0.0, 0.0};                          //|
                                                                      //|
    auto foilSurface = new G4OpticalSurface("AluminiumSurface");      //|
    foilSurface->SetType(dielectric_metal);                           //|
    foilSurface->SetModel(unified);                                   //|
    foilSurface->SetFinish(polished);                                 //|
                                                                      //|
    auto properties = new G4MaterialPropertiesTable();                //|
    properties->AddProperty("REFLECTIVITY", energy, reflectivity, 3); //|
    properties->AddProperty("EFFICIENCY", energy, efficiency, 3);     //|
                                                                      //|
    foilSurface->SetMaterialPropertiesTable(properties);              //|
//==============================REFLECTION==============================|
    
    // Assumes the crystal is centred at the origin.
    auto placePanel = [&](const G4String& name,
                          G4double halfX,
                          G4double halfY,
                          G4double halfZ,
                          G4ThreeVector position)
    {
        auto solid = new G4Box(name, halfX, halfY, halfZ);

        auto logical =
            new G4LogicalVolume(solid, foilMaterial, name);

        logical->SetVisAttributes(appearance);

        auto panelPhysical = new G4PVPlacement(
            nullptr,
            position,
            logical,
            name,
            worldLogical,
            false,
            0,
            true
        );
            new G4LogicalBorderSurface(
            name + "ReflectiveBoundary",
            crystalPhysical,
            panelPhysical,
            foilSurface
        );

        return panelPhysical;
    };

    placePanel(
        "FoilLeft", thickness/2, hy, hz,
        G4ThreeVector(-hx - thickness/2, 0, 0));

    placePanel(
        "FoilRight", thickness/2, hy, hz,
        G4ThreeVector(hx + thickness/2, 0, 0));

    placePanel(
        "FoilBottom", hx + thickness, thickness/2, hz,
        G4ThreeVector(0, -hy - thickness/2, 0));

    placePanel(
        "FoilTop", hx + thickness, thickness/2, hz,
        G4ThreeVector(0, hy + thickness/2, 0));

    placePanel(
        "FoilFront",
        hx + thickness,
        hy + thickness,
        thickness/2,
        G4ThreeVector(0, 0, -hz - thickness/2));

    // Rear face (+z) remains open for the SiPMs.
}